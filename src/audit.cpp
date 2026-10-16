// Module 6: Audit Logging & Thread Tracking Implementation
#include "audit.hpp"
#include <ctime>
#include <iomanip>
#include <sstream>

namespace medislot {

std::string AuditLogger::current_timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string AuditLogger::current_thread_id() {
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    return oss.str();
}

AuditLogger& AuditLogger::instance() {
    static AuditLogger logger;
    return logger;
}

void AuditLogger::log(const std::string& username,
                      const std::string& action,
                      const std::string& detail,
                      const std::string& status) {
    log_with_thread(current_thread_id(), username, action, detail, status);
}

void AuditLogger::log_with_thread(const std::string& thread_id,
                                  const std::string& username,
                                  const std::string& action,
                                  const std::string& detail,
                                  const std::string& status) {
    std::lock_guard<std::mutex> lock(mutex_);
    records_.push_back({
        current_timestamp(),
        thread_id,
        username.empty() ? "system" : username,
        action,
        detail,
        status
    });
}

std::vector<AuditRecord> AuditLogger::get_recent(size_t limit) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (records_.empty()) return {};

    size_t count = std::min(limit, records_.size());
    std::vector<AuditRecord> result;
    result.reserve(count);

    // Return newest entries first
    auto it = records_.rbegin();
    for (size_t i = 0; i < count && it != records_.rend(); ++i, ++it) {
        result.push_back(*it);
    }
    return result;
}

void AuditLogger::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    records_.clear();
}

void audit_log(const std::string& username, const std::string& action, const std::string& detail) {
    AuditLogger::instance().log(username, action, detail);
}

} // namespace medislot
