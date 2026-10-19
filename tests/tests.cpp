// Tests for Medislot core logic.
// Runs against a real PostgreSQL database specified by TEST_DATABASE_URL.
// Erases the "medislot" schema, so never run against a production database.
#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

#include "auth.hpp"
#include "booking.hpp"
#include "scheduler.hpp"

using namespace std::chrono_literals;

// Self-contained minimal test harness
static int failures = 0;
#define CHECK(condition) \
    do { \
        if (!(condition)) { \
            std::cout << "    FAILED: " #condition "  (line " << __LINE__ << ")\n"; \
            ++failures; \
        } \
    } while (0)

struct TestCase {
    const char* name;
    void (*run)();
};

static std::vector<TestCase>& all_tests() {
    static std::vector<TestCase> list;
    return list;
}

struct Register {
    Register(const char* name, void (*run)()) {
        all_tests().push_back({name, run});
    }
};

#define TEST(name) \
    static void name(); \
    static Register register_##name(#name, name); \
    static void name()

// Helper Utilities
static std::string future_slot(int days, const std::string& time = "10:00") {
    return db().query("SELECT to_char(localtimestamp + $1::int * interval '1 day', 'YYYY-MM-DD') || ' ' || $2 AS s",
                      {std::to_string(days), time})[0]["s"];
}

static BookingRequest booking(int patient, int doctor, int room, const std::string& start) {
    BookingRequest r;
    r.patient_id = patient;
    r.doctor_id  = doctor;
    r.room_id    = room;
    r.start      = start;
    return r;
}

template <class F>
static int status_of(F run) {
    try {
        run();
        return 200;
    } catch (const HttpError& e) {
        return e.status;
    }
}

static bool eventually(const std::function<bool()>& condition) {
    for (int i = 0; i < 100; ++i) {
        if (condition()) return true;
        std::this_thread::sleep_for(50ms);
    }
    return false;
}

static int count(const std::string& sql) {
    return std::stoi(db().query(sql)[0]["n"]);
}

static int new_patient(const std::string& name) {
    return std::stoi(db().query("INSERT INTO patients (name) VALUES ($1) RETURNING id", {name})[0]["id"]);
}

static void reset_data() {
    db().run_script("TRUNCATE appointments, waitlist, icu_stays, audit_log RESTART IDENTITY; DELETE FROM patients WHERE id > 3");
}

// -----------------------------------------------------------------------------
// MODULE 2: Authentication & Authorization Tests
// -----------------------------------------------------------------------------
TEST(login_checks_the_password_inside_postgres) {
    auto alice = check_login(db(), "alice", "patient123");
    CHECK(alice && alice->role == "patient" && alice->patient_id == 1);
    CHECK(!check_login(db(), "alice", "wrong"));
    CHECK(!check_login(db(), "nobody", "patient123"));
    CHECK(db().query("SELECT password_hash FROM users WHERE username = 'alice'")[0]["password_hash"] != "patient123");
}

TEST(each_role_has_only_its_own_permissions) {
    CHECK(allowed("admin", "audit") && !allowed("reception", "audit"));
    CHECK(!allowed("patient", "icu") && allowed("doctor", "icu"));
    CHECK(!allowed("doctor", "book") && allowed("patient", "book"));
    CHECK(!allowed("stranger", "list"));
}

// -----------------------------------------------------------------------------
// MODULES 1/3: Booking Rules & Integrity Constraints
// -----------------------------------------------------------------------------
TEST(a_valid_booking_is_saved_and_visible_in_the_view) {
    reset_data();
    int id = book_appointment(db(), booking(1, 1, 1, future_slot(3)));
    CHECK(id > 0);
    Rows rows = db().query("SELECT patient, doctor, room FROM appointment_details WHERE id = $1", {std::to_string(id)});
    CHECK(rows.size() == 1 && rows[0]["patient"] == "Alice Kumar" && rows[0]["room"] == "Consultation 1");
}

TEST(double_booking_the_same_doctor_patient_or_room_is_refused) {
    reset_data();
    std::string slot = future_slot(3);
    book_appointment(db(), booking(1, 1, 1, slot));
    CHECK(status_of([&] { book_appointment(db(), booking(2, 1, 0, slot)); }) == 409);   // same doctor
    CHECK(status_of([&] { book_appointment(db(), booking(1, 2, 0, slot)); }) == 409);   // same patient
    CHECK(status_of([&] { book_appointment(db(), booking(2, 2, 1, slot)); }) == 409);   // same room
    CHECK(status_of([&] { book_appointment(db(), booking(2, 2, 2, slot)); }) == 200);   // distinct resources: OK
}

TEST(bad_input_is_rejected_with_a_clear_status) {
    reset_data();
    CHECK(status_of([&] { book_appointment(db(), booking(1, 1, 0, future_slot(3, "10:15"))); }) == 400);   // invalid interval
    CHECK(status_of([&] { book_appointment(db(), booking(1, 1, 0, future_slot(3, "07:00"))); }) == 400);   // before hours
    CHECK(status_of([&] { book_appointment(db(), booking(1, 1, 0, "tomorrow at ten")); }) == 400);
    CHECK(status_of([&] { book_appointment(db(), booking(1, 1, 0, "2031-02-31 10:00")); }) == 400);        // invalid date
    CHECK(status_of([&] { book_appointment(db(), booking(1, 1, 0, future_slot(-2))); }) == 400);           // past trigger
    CHECK(status_of([&] { book_appointment(db(), booking(1, 999, 0, future_slot(3))); }) == 404);          // non-existent doctor
    CHECK(count("SELECT count(*) AS n FROM appointments") == 0);                                            // rollback verified
}

TEST(cancelling_frees_the_slot_and_patients_can_only_cancel_their_own) {
    reset_data();
    std::string slot = future_slot(3);
    int id = book_appointment(db(), booking(1, 1, 0, slot));
    CHECK(status_of([&] { cancel_appointment(db(), id, 2); }) == 404);      // Patient 2 cannot cancel Patient 1's appointment
    CHECK(status_of([&] { cancel_appointment(db(), id, 1); }) == 200);      // Patient 1 can cancel own appointment
    CHECK(status_of([&] { cancel_appointment(db(), id, 0); }) == 404);      // Already cancelled
    CHECK(status_of([&] { book_appointment(db(), booking(2, 1, 0, slot)); }) == 200);   // Slot is free again
}

// -----------------------------------------------------------------------------
// MODULES 3/4: Concurrency & Synchronization Tests
// -----------------------------------------------------------------------------
TEST(many_threads_booking_one_slot_give_exactly_one_winner) {
    reset_data();
    std::string slot = future_slot(3);
    std::vector<int> patients = {1, 2, 3};
    for (int i = 0; i < 5; ++i) {
        patients.push_back(new_patient("Racer " + std::to_string(i)));
    }

    std::atomic<int> booked{0};
    std::vector<std::thread> threads;
    for (int patient : patients) {
        threads.emplace_back([&, patient] {
            if (status_of([&] { book_appointment(db(), booking(patient, 1, 0, slot)); }) == 200) {
                ++booked;
            }
        });
    }
    for (std::thread& t : threads) t.join();
    CHECK(booked == 1);
    CHECK(count("SELECT count(*) AS n FROM appointments WHERE status = 'booked'") == 1);
}

TEST(sorted_locking_never_deadlocks_even_with_opposite_requests) {
    ResourceLocks locks;
    auto hammer = [&](std::vector<std::string> names) {
        for (int i = 0; i < 2000; ++i) {
            LockSet held = locks.lock_all(names);
        }
    };
    auto a = std::async(std::launch::async, hammer, std::vector<std::string>{"doctor:1", "room:1"});
    auto b = std::async(std::launch::async, hammer, std::vector<std::string>{"room:1", "doctor:1"});
    CHECK(a.wait_for(10s) == std::future_status::ready);
    CHECK(b.wait_for(10s) == std::future_status::ready);
}

TEST(semaphore_counts_units_and_blocks_when_none_are_left) {
    Semaphore beds(2);
    CHECK(beds.try_acquire() && beds.try_acquire() && !beds.try_acquire());
    std::atomic<bool> got_it{false};
    std::thread waiter([&] {
        beds.acquire();
        got_it = true;
    });
    std::this_thread::sleep_for(100ms);
    CHECK(!got_it);
    beds.release();
    waiter.join();
    CHECK(got_it);
}

// -----------------------------------------------------------------------------
// MODULE 5: Scheduling & Resource Allocation Tests
// -----------------------------------------------------------------------------
TEST(icu_beds_are_limited_and_emergencies_jump_the_queue) {
    reset_data();
    Scheduler scheduler(2);                                         // 2 ICU beds
    int p4 = new_patient("Dan"), p5 = new_patient("Eve");
    auto status = [](int patient) {
        Rows r = db().query("SELECT status FROM icu_stays WHERE patient_id = $1 ORDER BY id DESC LIMIT 1",
                            {std::to_string(patient)});
        return r.empty() ? std::string() : r[0]["status"];
    };

    scheduler.icu_request(db(), 1, "normal");
    scheduler.icu_request(db(), 2, "normal");
    CHECK(eventually([&] { return status(1) == "admitted" && status(2) == "admitted"; }));

    scheduler.icu_request(db(), 3, "normal");                       // enters queue first
    std::this_thread::sleep_for(50ms);
    scheduler.icu_request(db(), p4, "emergency");                   // emergency priority
    scheduler.icu_request(db(), p5, "normal");
    std::this_thread::sleep_for(200ms);
    CHECK(status(3) == "waiting" && status(p4) == "waiting");
    CHECK(status_of([&] { scheduler.icu_request(db(), 3, "normal"); }) == 409);   // cannot queue twice

    scheduler.icu_discharge(db(), 1);                               // bed freed
    CHECK(eventually([&] { return status(p4) == "admitted"; }));    // emergency preempts normal waiting
    CHECK(status(3) == "waiting" && status(p5) == "waiting");

    scheduler.icu_discharge(db(), 2);
    CHECK(eventually([&] { return status(3) == "admitted"; }));     // FCFS for remaining
    CHECK(count("SELECT count(*) AS n FROM icu_stays WHERE status = 'admitted'") == 2);
}

TEST(a_waitlisted_patient_gets_the_slot_when_it_is_cancelled) {
    reset_data();
    Scheduler scheduler(1);
    std::string slot = future_slot(3);
    int id = book_appointment(db(), booking(1, 1, 0, slot));
    db().query("INSERT INTO waitlist (patient_id, doctor_id, start_time) VALUES (2, 1, $1)", {slot});

    cancel_appointment(db(), id, 0);
    scheduler.wake();                                               // trigger consumer worker
    CHECK(eventually([&] {
        return count("SELECT count(*) AS n FROM appointments WHERE patient_id = 2 AND status = 'booked'") == 1;
    }));
    CHECK(count("SELECT count(*) AS n FROM waitlist") == 0);
}

// -----------------------------------------------------------------------------
// Test Runner
// -----------------------------------------------------------------------------
int main() {
    const char* url = std::getenv("TEST_DATABASE_URL");
    if (!url) {
        std::cerr << "Set TEST_DATABASE_URL to run the tests. WARNING: they erase the \"medislot\" schema there.\n";
        return 1;
    }
    set_connection_string(url);
    init_database();

    for (const TestCase& test : all_tests()) {
        int before = failures;
        test.run();
        std::cout << (failures == before ? "[ ok ] " : "[FAIL] ") << test.name << "\n";
    }
    std::cout << "\n" << all_tests().size() << " tests, " << failures << " failed\n";
    return failures == 0 ? 0 : 1;
}
