// Module 5: Priority Scheduling Implementation
#include "scheduler.hpp"

#include <iostream>

#include "booking.hpp"

Scheduler::Scheduler(int icu_beds) : total_beds_(icu_beds), free_beds_(icu_beds) {
    // OS Concept: Resource initialization. Deduct already-occupied beds from available semaphore count
    int taken = std::stoi(db().query("SELECT count(*) AS n FROM icu_stays WHERE status = 'admitted'")[0]["n"]);
    for (int i = 0; i < taken; ++i) {
        free_beds_.try_acquire();
    }
    worker_ = std::thread(&Scheduler::worker_loop, this);
}

Scheduler::~Scheduler() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }
    wakeup_.notify_one();
    if (worker_.joinable()) {
        worker_.join();
    }
}

void Scheduler::wake() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        work_pending_ = true;
    }
    wakeup_.notify_one();
}

void Scheduler::icu_request(Db& db, int patient_id, const std::string& priority) {
    try {
        db.query("INSERT INTO icu_stays (patient_id, priority) VALUES ($1, $2)",
                 {std::to_string(patient_id), priority});
    } catch (const DbError& e) {
        if (e.sqlstate == "23505") {
            throw HttpError(409, "this patient is already waiting for or using an ICU bed");
        }
        if (e.sqlstate == "23503") {
            throw HttpError(404, "unknown patient");
        }
        throw;
    }
    wake();
}

void Scheduler::icu_discharge(Db& db, int patient_id) {
    Rows rows = db.query("UPDATE icu_stays SET status = 'discharged', discharged_at = localtimestamp "
                         "WHERE patient_id = $1 AND status = 'admitted' RETURNING id",
                         {std::to_string(patient_id)});
    if (rows.empty()) {
        throw HttpError(404, "this patient is not in an ICU bed");
    }
    free_beds_.release();                 // OS Concept: V() signal incrementing free resource counter
    wake();
}

// OS Concept: Background Consumer Loop
void Scheduler::worker_loop() {
    Db db(db_connection_string());        // Dedicated connection for the worker thread
    for (;;) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            wakeup_.wait(lock, [&] { return work_pending_ || stopping_; });
            if (stopping_) return;
            work_pending_ = false;
        }
        try {
            admit_waiting_patients(db);
            fill_freed_slots(db);
        } catch (const std::exception& e) {
            std::cerr << "scheduler error: " << e.what() << "\n";
        }
    }
}

// OS Concept: Priority Scheduling (Emergency preempts Normal queue, then FCFS)
void Scheduler::admit_waiting_patients(Db& db) {
    while (free_beds_.try_acquire()) {                     // P(): acquire an available bed
        Rows admitted = db.query(
            "UPDATE icu_stays SET status = 'admitted', admitted_at = localtimestamp WHERE id = ("
            "  SELECT id FROM icu_stays WHERE status = 'waiting' "
            "  ORDER BY (priority = 'emergency') DESC, requested_at, id LIMIT 1) RETURNING patient_id");
        if (admitted.empty()) {                            // No patient in queue: release acquired bed
            free_beds_.release();
            return;
        }
        audit(db, "scheduler", "icu-admit", "patient " + admitted[0]["patient_id"]);
    }
}

// Automatic Slot Filling: Fulfills waitlist requests when appointments are cancelled
void Scheduler::fill_freed_slots(Db& db) {
    db.query("DELETE FROM waitlist WHERE start_time < localtimestamp");   // purge expired slots
    Rows waiting = db.query(
        "SELECT w.id, w.patient_id, w.doctor_id, to_char(w.start_time, 'YYYY-MM-DD HH24:MI') AS start "
        "FROM waitlist w WHERE NOT EXISTS ("
        "  SELECT 1 FROM appointments a WHERE a.doctor_id = w.doctor_id AND a.start_time = w.start_time AND a.status = 'booked') "
        "ORDER BY (w.priority = 'emergency') DESC, w.created_at, w.id");
    for (const Row& w : waiting) {
        BookingRequest request;
        request.patient_id = std::stoi(w.at("patient_id"));
        request.doctor_id  = std::stoi(w.at("doctor_id"));
        request.start      = w.at("start");
        try {
            int id = book_appointment(db, request);
            db.query("DELETE FROM waitlist WHERE id = $1", {w.at("id")});
            audit(db, "scheduler", "waitlist-booked", "appointment " + std::to_string(id));
        } catch (const HttpError&) {
            // Slot was concurrently claimed or patient conflict: remain on waitlist
        }
    }
}
