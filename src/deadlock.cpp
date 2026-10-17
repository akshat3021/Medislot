// Module 4: Deadlock Detection & Recovery Implementation
#include "deadlock.hpp"

#include <algorithm>
#include <iostream>

namespace medislot {

namespace {
    // DFS traversal states for cycle detection
    constexpr int UNVISITED = 0;
    constexpr int VISITING  = 1;
    constexpr int VISITED   = 2;
}

void WaitForGraph::add_edge(const std::string& waiting_thread, const std::string& holding_thread) {
    if (waiting_thread.empty() || holding_thread.empty() || waiting_thread == holding_thread) {
        return;
    }
    adj_[waiting_thread].insert(holding_thread);
}

void WaitForGraph::remove_edge(const std::string& waiting_thread, const std::string& holding_thread) {
    auto it = adj_.find(waiting_thread);
    if (it != adj_.end()) {
        it->second.erase(holding_thread);
        if (it->second.empty()) {
            adj_.erase(it);
        }
    }
}

void WaitForGraph::remove_node(const std::string& thread_id) {
    adj_.erase(thread_id);
    for (auto& [_, holding_threads] : adj_) {
        holding_threads.erase(thread_id);
    }
}

void WaitForGraph::clear() {
    adj_.clear();
}

bool WaitForGraph::dfs(const std::string& node,
                       std::unordered_map<std::string, int>& visited,
                       std::vector<std::string>& path,
                       std::vector<std::string>& cycle_out) {
    visited[node] = VISITING;
    path.push_back(node);

    auto it = adj_.find(node);
    if (it != adj_.end()) {
        for (const auto& neighbor : it->second) {
            // Encountered a node in the active call stack -> cycle found
            if (visited[neighbor] == VISITING) {
                auto start_it = std::find(path.begin(), path.end(), neighbor);
                cycle_out.assign(start_it, path.end());
                cycle_out.push_back(neighbor);
                return true;
            }

            if (visited[neighbor] == UNVISITED) {
                if (dfs(neighbor, visited, path, cycle_out)) {
                    return true;
                }
            }
        }
    }

    visited[node] = VISITED;
    path.pop_back();
    return false;
}

bool WaitForGraph::detect_cycle(std::vector<std::string>& cycle_nodes) {
    std::unordered_map<std::string, int> visited;
    std::vector<std::string> all_nodes;

    for (const auto& [waiting, holding_threads] : adj_) {
        if (visited.emplace(waiting, UNVISITED).second) {
            all_nodes.push_back(waiting);
        }
        for (const auto& holding : holding_threads) {
            if (visited.emplace(holding, UNVISITED).second) {
                all_nodes.push_back(holding);
            }
        }
    }

    std::vector<std::string> path;
    for (const auto& node : all_nodes) {
        if (visited[node] == UNVISITED) {
            if (dfs(node, visited, path, cycle_nodes)) {
                return true;
            }
        }
    }

    return false;
}

// DeadlockManager
DeadlockManager& DeadlockManager::instance() {
    static DeadlockManager mgr;
    return mgr;
}

void DeadlockManager::request_resource(const std::string& thread_id,
                                       const std::string& resource_id,
                                       const std::string& current_holder) {
    (void)resource_id;
    std::lock_guard<std::mutex> lock(mutex_);
    if (!current_holder.empty() && current_holder != thread_id) {
        graph_.add_edge(thread_id, current_holder);
    }
}

void DeadlockManager::release_resource(const std::string& thread_id,
                                       const std::string& resource_id) {
    (void)resource_id;
    std::lock_guard<std::mutex> lock(mutex_);
    graph_.remove_node(thread_id);
}

void DeadlockManager::clear_thread(const std::string& thread_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    graph_.remove_node(thread_id);
}

bool DeadlockManager::check_deadlock(DeadlockCycle& cycle_out) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> cycle_nodes;
    if (graph_.detect_cycle(cycle_nodes)) {
        cycle_out.cycle_nodes = cycle_nodes;
        cycle_out.victim_thread = cycle_nodes.empty() ? "" : cycle_nodes.back();
        cycle_out.detected_at = std::chrono::system_clock::now();

        ++deadlock_count_;
        history_.push_back(cycle_out);
        return true;
    }

    return false;
}

bool DeadlockManager::resolve_deadlock(std::string& selected_victim) {
    DeadlockCycle cycle;
    if (check_deadlock(cycle)) {
        selected_victim = cycle.victim_thread;
        clear_thread(selected_victim); // Break cycle by dropping victim's wait dependencies
        return true;
    }

    selected_victim.clear();
    return false;
}

size_t DeadlockManager::get_deadlock_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return deadlock_count_;
}

} // namespace medislot
