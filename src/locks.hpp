// Modules 3 & 4: Concurrency Synchronization & Deadlock Prevention
// OS Concepts:
// 1. Counting Semaphore: Resource-bound synchronization using wait(P) / signal(V).
// 2. Deadlock Prevention by Resource Ordering: All requested resources are sorted
//    lexicographically before acquisition to break the Coffman "circular wait" condition.
#pragma once
#include <algorithm>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// OS Concept: Counting Semaphore (Dijkstra's P and V primitives)
class Semaphore {
public:
    explicit Semaphore(int count) : count_(count) {}

    // P / wait: blocks until resources become available
    void acquire() {
        std::unique_lock<std::mutex> lock(mutex_);
        free_.wait(lock, [&] { return count_ > 0; });
        --count_;
    }

    // Non-blocking try_wait: returns false immediately if no resources available
    bool try_acquire() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (count_ == 0) return false;
        --count_;
        return true;
    }

    // V / signal: releases a resource unit and notifies one waiting thread
    void release() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            ++count_;
        }
        free_.notify_one();
    }

private:
    std::mutex mutex_;
    std::condition_variable free_;
    int count_;
};

// RAII container: automatically unlocks all held resource mutexes upon exiting scope
using LockSet = std::vector<std::unique_lock<std::mutex>>;

// OS Concept: Deadlock Prevention via Global Lock Hierarchy / Total Ordering
class ResourceLocks {
public:
    // Acquires locks for all requested resources in strictly sorted order
    LockSet lock_all(std::vector<std::string> names) {
        std::sort(names.begin(), names.end());
        names.erase(std::unique(names.begin(), names.end()), names.end());

        LockSet held;
        held.reserve(names.size());
        for (const std::string& name : names) {
            held.emplace_back(mutex_for(name));
        }
        return held;
    }

private:
    std::mutex& mutex_for(const std::string& name) {
        std::lock_guard<std::mutex> lock(table_mutex_);
        auto& slot = mutexes_[name];
        if (!slot) {
            slot = std::make_unique<std::mutex>();
        }
        return *slot;
    }

    std::mutex table_mutex_;
    std::map<std::string, std::unique_ptr<std::mutex>> mutexes_;
};
