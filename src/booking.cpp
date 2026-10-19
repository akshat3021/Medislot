// Modules 3 & 4: Safe Appointment Booking Implementation
#include "booking.hpp"

#include <cstdio>

ResourceLocks resource_locks;

// Format validation: "YYYY-MM-DD HH:MM" on 30-minute intervals between 08:00 and 19:30
void check_slot_format(const std::string& start) {
    int year, month, day, hour, minute;
    char extra;
    int fields = std::sscanf(start.c_str(), "%4d-%2d-%2d %2d:%2d%c", &year, &month, &day, &hour, &minute, &extra);
    if (fields != 5 || start.size() != 16) {
        throw HttpError(400, "start must look like 2026-10-20 10:30");
    }
    if ((minute != 0 && minute != 30) || hour < 8 || hour > 19) {
        throw HttpError(400, "slots are every 30 minutes between 08:00 and 19:30");
    }
}

// Checks slot availability before insertion to avoid unnecessary transaction rollbacks
static void reject_if_taken(Db& db, const std::string& column, int id, const std::string& start, const char* message) {
    Rows rows = db.query("SELECT 1 FROM appointments WHERE " + column + " = $1 AND start_time = $2 AND status = 'booked'",
                         {std::to_string(id), start});
    if (!rows.empty()) {
        throw HttpError(409, message);
    }
}

// Maps DBMS SQLSTATE error codes to HTTP status responses
static HttpError explain(const DbError& e) {
    if (e.sqlstate == "23505") return HttpError(409, "that time was just taken by someone else");
    if (e.sqlstate == "23503") return HttpError(404, "unknown doctor, patient or room");
    if (e.sqlstate == "P0001") return HttpError(400, e.what());            // Trigger error: booking in past
    if (e.sqlstate.rfind("22", 0) == 0) return HttpError(400, "invalid date or time");
    return HttpError(500, "database error");
}

int book_appointment(Db& db, const BookingRequest& r) {
    check_slot_format(r.start);

    // OS Concept: Critical Section & Deadlock Prevention.
    // Lock all required resources in sorted order so no circular wait can occur.
    std::vector<std::string> names = {"doctor:" + std::to_string(r.doctor_id),
                                      "patient:" + std::to_string(r.patient_id)};
    if (r.room_id != 0) {
        names.push_back("room:" + std::to_string(r.room_id));
    }
    LockSet locks = resource_locks.lock_all(names);

    try {
        // DBMS Concept: ACID Transaction (Atomicity, Consistency, Isolation, Durability)
        Transaction tx(db);
        reject_if_taken(db, "doctor_id",  r.doctor_id,  r.start, "the doctor is already booked at that time");
        reject_if_taken(db, "patient_id", r.patient_id, r.start, "the patient already has an appointment at that time");
        if (r.room_id != 0) {
            reject_if_taken(db, "room_id", r.room_id, r.start, "the room is already booked at that time");
        }

        Rows row = db.query("INSERT INTO appointments (patient_id, doctor_id, room_id, start_time) "
                            "VALUES ($1, $2, NULLIF($3, '0')::int, $4) RETURNING id",
                            {std::to_string(r.patient_id), std::to_string(r.doctor_id), std::to_string(r.room_id), r.start});
        tx.commit();
        return std::stoi(row[0]["id"]);
    } catch (const DbError& e) {
        throw explain(e);
    }
}

void cancel_appointment(Db& db, int appointment_id, int only_patient_id) {
    Rows rows = db.query("UPDATE appointments SET status = 'cancelled' "
                         "WHERE id = $1 AND status = 'booked' AND ($2::int = 0 OR patient_id = $2::int) RETURNING id",
                         {std::to_string(appointment_id), std::to_string(only_patient_id)});
    if (rows.empty()) {
        throw HttpError(404, "no such booked appointment");
    }
}
