/**
 * @file deadlock.cpp
 * @brief Deadlock Detection and Recovery Implementation
 *
 * Concepts Explained:
 * -------------------
 * 1. Wait-For Graph (WFG):
 *    - A directed graph where each vertex represents an active thread or transaction.
 *    - A directed edge (A -> B) means: "Thread A is blocked waiting for a resource
 *      that is currently held by Thread B."
 *
 * 2. Deadlock Condition (Circular Wait):
 *    - A deadlock exists if and only if there is a directed cycle in the WFG
 *      (e.g., Thread A -> Thread B -> Thread A).
 *
 * 3. 3-Color DFS Cycle Detection:
 *    - UNVISITED (White, 0): The node hasn't been explored yet.
 *    - VISITING  (Gray, 1):  The node is currently in the active recursion call stack.
 *      If we encounter a neighbor with state VISITING, we have found a back-edge,
 *      which means a circular dependency (DEADLOCK) exists!
 *    - VISITED   (Black, 2): The node and all its descendants have been fully processed
 *      with no cycles found along its branches.
 *
 * 4. Recovery Strategy:
 *    - Once a cycle is detected, pick a "victim" thread from the cycle and abort/clear
 *      its wait dependencies so the remaining threads can proceed.
 */

#include "deadlock.hpp"

#include <algorithm>
#include <iostream>

namespace medislot {

// ============================================================================
// 1. DFS Node States (3-Color Graph Traversal)
// ============================================================================
namespace {
    constexpr int NODE_UNVISITED = 0; // Node not visited yet (White)
    constexpr int NODE_VISITING  = 1; // Node currently in active recursion stack (Gray)
    constexpr int NODE_VISITED   = 2; // Node completely explored (Black)
} // anonymous namespace

// ============================================================================
// 2. WaitForGraph Implementation
// ============================================================================

/**
 * @brief Adds a directed edge: waiting_thread -> holding_thread
 * 
 * Meaning: waiting_thread is blocked until holding_thread releases the resource.
 */
void WaitForGraph::add_edge(const std::string& waiting_thread, const std::string& holding_thread) {
    // Ignore invalid edges and self-dependencies
    if (waiting_thread.empty() || holding_thread.empty() || waiting_thread == holding_thread) {
        return;
    }

    // Insert directed edge: waiting_thread -> holding_thread
    adj_[waiting_thread].insert(holding_thread);
}

/**
 * @brief Removes a directed edge: waiting_thread -> holding_thread
 * 
 * Called when waiting_thread is no longer waiting on holding_thread.
 */
void WaitForGraph::remove_edge(const std::string& waiting_thread, const std::string& holding_thread) {
    auto it = adj_.find(waiting_thread);
    if (it != adj_.end()) {
        it->second.erase(holding_thread);

        // Clean up empty map entry to keep memory minimal
        if (it->second.empty()) {
            adj_.erase(it);
        }
    }
}

/**
 * @brief Completely removes a thread node from the graph.
 * 
 * Removes all outgoing wait edges (edges where thread_id is waiting)
 * and all incoming wait edges (other threads waiting on thread_id).
 */
void WaitForGraph::remove_node(const std::string& thread_id) {
    // 1. Remove thread_id's outgoing edges
    adj_.erase(thread_id);

    // 2. Remove thread_id from other threads' outgoing edge sets (incoming edges)
    for (auto& [_, holding_threads] : adj_) {
        holding_threads.erase(thread_id);
    }
}

/**
 * @brief Clears all vertices and edges in the graph.
 */
void WaitForGraph::clear() {
    adj_.clear();
}

/**
 * @brief Recursive Depth-First Search (DFS) helper to detect cycles.
 * 
 * @param node Current node being visited
 * @param visited Map of node -> visitation state (UNVISITED, VISITING, VISITED)
 * @param path Active path of nodes in current DFS branch
 * @param cycle_out Output vector populated with cycle nodes if a cycle is found
 * @return true if a cycle was found, false otherwise
 */
bool WaitForGraph::dfs(const std::string& node,
                       std::unordered_map<std::string, int>& visited,
                       std::vector<std::string>& path,
                       std::vector<std::string>& cycle_out) {
    // Step 1: Mark current node as currently being explored (Gray / VISITING)
    visited[node] = NODE_VISITING;
    path.push_back(node);

    // Step 2: Explore all outgoing edges (threads this node is waiting for)
    auto it = adj_.find(node);
    if (it != adj_.end()) {
        for (const auto& neighbor : it->second) {

            // Case A: Neighbor is in current recursion stack -> Back-edge found! Cycle detected!
            if (visited[neighbor] == NODE_VISITING) {
                // Extract only the loop portion of the path: from 'neighbor' to current 'node'
                auto cycle_start = std::find(path.begin(), path.end(), neighbor);
                cycle_out.assign(cycle_start, path.end());
                cycle_out.push_back(neighbor); // Append neighbor again to show completed loop (e.g. A -> B -> A)
                return true;
            }

            // Case B: Neighbor has not been visited yet -> Recursively explore
            if (visited[neighbor] == NODE_UNVISITED) {
                if (dfs(neighbor, visited, path, cycle_out)) {
                    return true; // Propagate cycle detection up the call stack
                }
            }

            // Case C: Neighbor is NODE_VISITED (Black) -> Already proved cycle-free, skip
        }
    }

    // Step 3: Backtrack - this branch has no cycles
    visited[node] = NODE_VISITED; // Mark fully explored (Black)
    path.pop_back();              // Remove from active DFS path
    return false;
}

/**
 * @brief Checks if a cycle exists in the Wait-For Graph.
 * 
 * Iterates through all unique nodes in the graph and runs DFS from any
 * unvisited node. If any DFS run finds a cycle, returns true and fills cycle_nodes.
 */
bool WaitForGraph::detect_cycle(std::vector<std::string>& cycle_nodes) {
    std::unordered_map<std::string, int> visited;
    std::vector<std::string> all_nodes;

    // Collect and initialize all unique nodes (both waiting and holding threads)
    for (const auto& [waiting_thread, holding_threads] : adj_) {
        if (visited.emplace(waiting_thread, NODE_UNVISITED).second) {
            all_nodes.push_back(waiting_thread);
        }
        for (const auto& holding_thread : holding_threads) {
            if (visited.emplace(holding_thread, NODE_UNVISITED).second) {
                all_nodes.push_back(holding_thread);
            }
        }
    }

    // Explore every connected component
    std::vector<std::string> path;
    for (const auto& node : all_nodes) {
        if (visited[node] == NODE_UNVISITED) {
            if (dfs(node, visited, path, cycle_nodes)) {
                return true; // Deadlock cycle detected!
            }
        }
    }

    return false; // No cycle found - system is deadlock-free
}

// ============================================================================
// 3. DeadlockManager Implementation (Thread-Safe Singleton)
// ============================================================================

/**
 * @brief Thread-safe Meyers' Singleton instance accessor.
 */
DeadlockManager& DeadlockManager::instance() {
    static DeadlockManager mgr;
    return mgr;
}

/**
 * @brief Records that a thread wants a resource that is currently held by another thread.
 * 
 * Note: Wait-For Graphs represent direct thread-to-thread dependencies (A waits for B).
 * The specific resource_id is accepted for interface compatibility but not needed for the WFG.
 */
void DeadlockManager::request_resource(const std::string& thread_id,
                                       const std::string& resource_id,
                                       const std::string& current_holder) {
    (void)resource_id; // Unused in WFG: resource is abstracted into a direct wait edge

    std::lock_guard<std::mutex> lock(mutex_);

    // If another thread holds the resource, record the wait dependency
    if (!current_holder.empty() && current_holder != thread_id) {
        graph_.add_edge(thread_id, current_holder);
    }
}

/**
 * @brief Clears wait dependencies when a thread releases or finishes with a resource.
 */
void DeadlockManager::release_resource(const std::string& thread_id,
                                       const std::string& resource_id) {
    (void)resource_id; // Unused in WFG

    std::lock_guard<std::mutex> lock(mutex_);
    graph_.remove_node(thread_id);
}

/**
 * @brief Removes a thread completely from the wait graph.
 */
void DeadlockManager::clear_thread(const std::string& thread_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    graph_.remove_node(thread_id);
}

/**
 * @brief Checks if any deadlock currently exists.
 * 
 * If a cycle is detected:
 * - Populates cycle_out with the participating threads.
 * - Chooses a victim thread to terminate (the closing node of the cycle).
 * - Records the event in history and increments deadlock count.
 */
bool DeadlockManager::check_deadlock(DeadlockCycle& cycle_out) {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> cycle_nodes;
    if (graph_.detect_cycle(cycle_nodes)) {
        cycle_out.cycle_nodes = cycle_nodes;

        // Victim selection policy: pick the thread that closed the circular wait loop
        cycle_out.victim_thread = cycle_nodes.empty() ? "" : cycle_nodes.back();
        cycle_out.detected_at = std::chrono::system_clock::now();

        ++deadlock_count_;
        history_.push_back(cycle_out);
        return true;
    }

    return false;
}

/**
 * @brief Automatically detects and resolves an active deadlock.
 * 
 * Strategy:
 * 1. Check for a cycle using check_deadlock().
 * 2. If found, pick the selected victim.
 * 3. Break the circular dependency by clearing the victim from the graph.
 */
bool DeadlockManager::resolve_deadlock(std::string& selected_victim) {
    DeadlockCycle cycle;

    // Detect deadlock and select a victim
    if (check_deadlock(cycle)) {
        selected_victim = cycle.victim_thread;

        // Recovery action: break the cycle by aborting the victim's wait dependencies
        clear_thread(selected_victim);
        return true;
    }

    selected_victim.clear();
    return false;
}

/**
 * @brief Returns the total number of detected deadlocks.
 */
size_t DeadlockManager::get_deadlock_count() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return deadlock_count_;
}

} // namespace medislot
