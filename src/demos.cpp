// Two short demos that SHOW the operating-system problems and their fixes.
#include "demos.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <thread>

#include "booking.hpp"
#include "deadlock.hpp"

using namespace std::chrono_literals;

static std::mutex print_mutex;
static void say(const std::string& line) {            // so lines from different threads do not mix
    std::lock_guard<std::mutex> lock(print_mutex);
    std::cout << line << "\n";
}

// ---------------------------------------------------------------------------------------------
// RACE CONDITION: 8 threads all try to take the same free slot.
// ---------------------------------------------------------------------------------------------
static int count_winners(bool use_lock) {
    std::atomic<bool> taken{false};
    std::atomic<int> winners{0};
    std::mutex slot_mutex;

    auto try_to_take = [&] {
        std::unique_lock<std::mutex> lock(slot_mutex, std::defer_lock);
        if (use_lock) lock.lock();                    // critical section starts here
        if (!taken) {                                 // CHECK: is the slot free?
            std::this_thread::sleep_for(5ms);         // time passes between check and act...
            taken = true;                             // ACT: take it
            ++winners;
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) threads.emplace_back(try_to_take);
    for (std::thread& t : threads) t.join();
    return winners;
}

// The same race, but through the real booking code and PostgreSQL.
static void race_on_real_database() {
    Db& d = db();
    std::string start = d.query("SELECT to_char(localtimestamp + interval '3 days', 'YYYY-MM-DD') || ' 10:00' AS s")[0]["s"];
    int doctor_id = std::stoi(d.query("SELECT min(id) AS id FROM doctors")[0]["id"]);

    std::vector<int> patients;
    for (int i = 1; i <= 8; ++i)
        patients.push_back(std::stoi(d.query("INSERT INTO patients (name) VALUES ($1) RETURNING id",
                                             {"Demo racer " + std::to_string(i)})[0]["id"]));

    std::atomic<int> booked{0}, rejected{0};
    std::vector<std::thread> threads;
    for (int patient_id : patients)
        threads.emplace_back([&, patient_id] {
            BookingRequest request;
            request.patient_id = patient_id;
            request.doctor_id = doctor_id;
            request.start = start;
            try { book_appointment(db(), request); ++booked; }
            catch (const HttpError&) { ++rejected; }
        });
    for (std::thread& t : threads) t.join();

    say("  8 patients book the same doctor at " + start + " at the same time:");
    say("  booked: " + std::to_string(booked) + "   rejected: " + std::to_string(rejected) + "   (exactly one winner)");

    d.query("DELETE FROM appointments WHERE patient_id IN (SELECT id FROM patients WHERE name LIKE 'Demo racer %')");
    d.query("DELETE FROM patients WHERE name LIKE 'Demo racer %'");
}

int run_race_demo() {
    std::cout << "\n1) Eight threads share one slot WITHOUT a lock (check, then act):\n";
    std::cout << "   winners = " << count_winners(false) << "   <- should be 1, the race condition gives more\n";
    std::cout << "\n2) The same code WITH a mutex around check+act:\n";
    std::cout << "   winners = " << count_winners(true) << "   <- always exactly 1\n";
    std::cout << "\n3) The real booking code (mutex + transaction + PostgreSQL):\n";
    race_on_real_database();
    std::cout << "\n";
    return 0;
}

// ---------------------------------------------------------------------------------------------
// DEADLOCK: two bookings both need the doctor AND the room.
// ---------------------------------------------------------------------------------------------
static void lock_in_given_order(std::timed_mutex& first, std::timed_mutex& second, const std::string& name) {
    std::unique_lock<std::timed_mutex> a(first);
    std::this_thread::sleep_for(100ms);               // now each thread holds one lock...
    std::unique_lock<std::timed_mutex> b(second, std::defer_lock);
    if (!b.try_lock_for(500ms)) {                     // ...and waits for the other one: nobody can move
        say("   " + name + ": waited 0.5 s for the second lock -> DEADLOCK detected, giving up");
        return;
    }
    say("   " + name + ": got both locks");
}

static void lock_in_sorted_order(ResourceLocks& locks, std::vector<std::string> names, const std::string& name) {
    LockSet held = locks.lock_all(names);             // sorted inside, so the order is always the same
    std::this_thread::sleep_for(100ms);
    say("   " + name + ": got both locks");
}

int run_deadlock_demo() {
    std::cout << "\n1) Opposite lock order (A: doctor then room, B: room then doctor):\n";
    {
        // OS Concept: Wait-For Graph (WFG) modeling circular wait
        medislot::DeadlockManager& dm = medislot::DeadlockManager::instance();
        dm.request_resource("booking A", "room:1", "booking B");
        dm.request_resource("booking B", "doctor:1", "booking A");

        medislot::DeadlockCycle cycle;
        if (dm.check_deadlock(cycle)) {
            std::cout << "   [Wait-For Graph] Cycle detected: ";
            for (size_t i = 0; i < cycle.cycle_nodes.size(); ++i) {
                std::cout << cycle.cycle_nodes[i] << (i + 1 < cycle.cycle_nodes.size() ? " -> " : "");
            }
            std::cout << "\n";
        }

        std::timed_mutex doctor, room;
        std::thread a(lock_in_given_order, std::ref(doctor), std::ref(room), "booking A");
        std::thread b(lock_in_given_order, std::ref(room), std::ref(doctor), "booking B");
        a.join();
        b.join();

        std::string victim;
        if (dm.resolve_deadlock(victim)) {
            std::cout << "   [Wait-For Graph] Resolved by terminating wait edge for: " << victim << "\n";
        }
    }
    std::cout << "\n2) Same requests, but ResourceLocks always locks in sorted order:\n";
    {
        ResourceLocks locks;
        std::thread a(lock_in_sorted_order, std::ref(locks), std::vector<std::string>{"doctor:1", "room:1"}, "booking A");
        std::thread b(lock_in_sorted_order, std::ref(locks), std::vector<std::string>{"room:1", "doctor:1"}, "booking B");
        a.join();
        b.join();
    }
    std::cout << "\n";
    return 0;
}
