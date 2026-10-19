// Module 2: Authentication & Authorization Implementation
#include "auth.hpp"

#include <set>

std::optional<Session> check_login(Db& db, const std::string& username, const std::string& password) {
    // DBMS Concept: Password verification in PostgreSQL using crypt(pw, salt)
    Rows rows = db.query(
        "SELECT username, role, COALESCE(doctor_id, 0) AS doctor_id, COALESCE(patient_id, 0) AS patient_id "
        "FROM users WHERE username = $1 AND password_hash = crypt($2, password_hash)",
        {username, password});
    if (rows.empty()) return std::nullopt;

    Session s;
    s.username   = rows[0]["username"];
    s.role       = rows[0]["role"];
    s.doctor_id  = std::stoi(rows[0]["doctor_id"]);
    s.patient_id = std::stoi(rows[0]["patient_id"]);
    return s;
}

// OS/Security Concept: Access Control Matrix (Role -> Allowed Actions)
static const std::map<std::string, std::set<std::string>> kPermissions = {
    {"admin",     {"list", "book", "cancel", "waitlist", "patients", "icu", "stats", "audit"}},
    {"reception", {"list", "book", "cancel", "waitlist", "patients", "icu", "stats"}},
    {"doctor",    {"list", "icu"}},
    {"patient",   {"list", "book", "cancel", "waitlist"}},
};

bool allowed(const std::string& role, const std::string& action) {
    auto found = kPermissions.find(role);
    return found != kPermissions.end() && found->second.count(action) > 0;
}

std::string SessionStore::add(Db& db, const Session& session) {
    std::string token = db.query("SELECT gen_random_uuid()::text AS token")[0]["token"];
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_[token] = session;
    return token;
}

std::optional<Session> SessionStore::find(const std::string& token) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto found = sessions_.find(token);
    if (found == sessions_.end()) return std::nullopt;
    return found->second;
}

void SessionStore::remove(const std::string& token) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(token);
}
