// Module 5: Priority Scheduling, Resource Management & Background Worker
// OS Concept: Producer-Consumer Pattern.
// Request threads are producers that wake the background consumer thread to process
// ICU priority queues and auto-book waitlist vacancies.
#pragma once
#include <condition_variable>
#include <mutex>
#include <thread>

#include "db.hpp"
#include "locks.hpp"

class Scheduler {
public:
    explicit Scheduler(int icu_beds);     // starts the background worker thread
    ~Scheduler();                         // safely terminates worker

    void wake();                          // Producer signal: informs worker of queue updates
    int  icu_beds() const { return total_beds_; }

    void icu_request(Db& db, int patient_id, const std::string& priority);   // enqueue patient
    void icu_discharge(Db& db, int patient_id);                              // release bed semaphore

private:
    void worker_loop();
    void admit_waiting_patients(Db& db);
    void fill_freed_slots(Db& db);

    int total_beds_;
    Semaphore free_beds_;

    std::mutex mutex_;
    std::condition_variable wakeup_;
    bool work_pending_ = true;            // process existing queue items on startup
    bool stopping_ = false;
    std::thread worker_;
};
