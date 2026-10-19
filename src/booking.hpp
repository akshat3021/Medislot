// Modules 3 & 4: Safe Appointment Booking with Concurrency Control & Transactions
#pragma once
#include <string>
#include "db.hpp"
#include "locks.hpp"

struct BookingRequest {
    int patient_id = 0;
    int doctor_id  = 0;
    int room_id    = 0;          // 0 = no room needed
    std::string start;           // "YYYY-MM-DD HH:MM" (30-minute slots, 08:00 - 19:30)
};

extern ResourceLocks resource_locks;   // shared resource manager

// Validates that slot timestamp is formatted correctly and within operating hours
void check_slot_format(const std::string& start);

// Books an appointment safely using two-phase locking (2PL) and DB transaction
int book_appointment(Db& db, const BookingRequest& request);

// Cancels an appointment. only_patient_id = 0 allows staff to cancel any appointment.
void cancel_appointment(Db& db, int appointment_id, int only_patient_id);
