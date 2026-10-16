#pragma once
// Module 6: Audit Logging & Thread Tracking
// Thread-safe audit recording for hospital transactions and security monitoring.

#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>

namespace medislot {

struct AuditRecord {
    std::string at;          // ISO Timestamp
    std::string thread_id;   // Calling thread identifier
    std::string username;    // Authenticated user
    std::string action;      // BOOK, CANCEL, ICU_REQUEST, etc.
    std::string detail;      // Additional parameters or diagnostics
    std::string status;      // SUCCESS, FAILED, ROLLBACK
};

class AuditLogger {
public:
    static AuditLogger& instance();

    void log(const std::string& username,
             const std::string& action,
             const std::string& detail,
             const std::string& status = "SUCCESS");

    void log_with_thread(const std::string& thread_id,
                         const std::string& username,
                         const std::string& action,
                         const std::string& detail,
                         const std::string& status = "SUCCESS");

    std::vector<AuditRecord> get_recent(size_t limit = 50) const;
    void clear();

private:
    AuditLogger() = default;
    mutable std::mutex mutex_;
    std::vector<AuditRecord> records_;

    static std::string current_timestamp();
    static std::string current_thread_id();
};

void audit_log(const std::string& username, const std::string& action, const std::string& detail);

} // namespace medislot
