// Module 2: Authentication, Authorization & Session Management
#pragma once
#include <map>
#include <mutex>
#include <optional>
#include <string>

#include "db.hpp"

struct Session {
    std::string username;
    std::string role;        // admin | reception | doctor | patient
    int doctor_id  = 0;      // 0 = not a doctor
    int patient_id = 0;      // 0 = not a patient
};

// Checks password hash inside PostgreSQL using pgcrypto crypt(). Empty optional on failure.
std::optional<Session> check_login(Db& db, const std::string& username, const std::string& password);

// Role-based permission check: returns true if role is authorized to perform action
bool allowed(const std::string& role, const std::string& action);

// Thread-safe in-memory session store guarded by std::mutex
class SessionStore {
public:
    std::string add(Db& db, const Session& session);             // returns cryptographically random token
    std::optional<Session> find(const std::string& token);
    void remove(const std::string& token);

private:
    std::mutex mutex_;
    std::map<std::string, Session> sessions_;
};
