#pragma once
// Module 4: Deadlock Detection & Recovery
// Implements Wait-For Graph (WFG) and Cycle Detection for hospital resource allocations.

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <chrono>

namespace medislot {

struct DeadlockCycle {
    std::vector<std::string> cycle_nodes;
    std::string victim_thread;
    std::chrono::system_clock::time_point detected_at;
};

class WaitForGraph {
public:
    WaitForGraph() = default;

    void add_edge(const std::string& waiting_thread, const std::string& holding_thread);
    void remove_edge(const std::string& waiting_thread, const std::string& holding_thread);
    void remove_node(const std::string& thread_id);
    void clear();

    // DFS-based cycle detection returning nodes in deadlock cycle
    bool detect_cycle(std::vector<std::string>& cycle_nodes);

private:
    std::unordered_map<std::string, std::unordered_set<std::string>> adj_;
    bool dfs(const std::string& node,
             std::unordered_map<std::string, int>& visited,
             std::vector<std::string>& path,
             std::vector<std::string>& cycle_out);
};

class DeadlockManager {
public:
    static DeadlockManager& instance();

    void request_resource(const std::string& thread_id, const std::string& resource_id, const std::string& current_holder);
    void release_resource(const std::string& thread_id, const std::string& resource_id);
    void clear_thread(const std::string& thread_id);

    bool check_deadlock(DeadlockCycle& cycle_out);
    bool resolve_deadlock(std::string& selected_victim);
    size_t get_deadlock_count() const;

private:
    DeadlockManager() = default;
    mutable std::mutex mutex_;
    WaitForGraph graph_;
    size_t deadlock_count_{0};
    std::vector<DeadlockCycle> history_;
};

} // namespace medislot
