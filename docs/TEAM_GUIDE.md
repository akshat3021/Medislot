# Medislot Team Guide & Viva Preparation

## 1. Module Distribution & Demonstration

| # | Module | Key Files | What to Demonstrate to Evaluators |
|---|---|---|---|
| 1 | Database Schema & UI | `sql/schema.sql`, `src/db.*`, `frontend/dist/` | Tables in `medislot` schema, primary/foreign keys, CHECK constraints, partial unique indexes, `no_past_booking` trigger, and `appointment_details` view. |
| 2 | Access Control & Security | `src/auth.*`, `sql/roles.sql` | Role-based permission matrix, bcrypt in-database password verification, session tokens, PostgreSQL `GRANT/REVOKE`, and Row-Level Security (RLS). |
| 3 | Concurrency & Locking | `src/locks.hpp` (`Semaphore`), `src/booking.cpp` | Safe slot booking, critical sections, `./medislot --demo-race` showing unsynchronized race condition vs mutex vs PostgreSQL safety. |
| 4 | Deadlock Handling | `src/locks.hpp` (`ResourceLocks`), `src/deadlock.*`, `src/demos.cpp` | `./medislot --demo-deadlock`: Deadlock detection using Wait-For Graph (WFG) cycle detection, timeout detection via `try_lock_for`, and prevention via global resource sorting. |
| 5 | Priority Scheduling & Waitlist | `src/scheduler.*` | Producer-Consumer architecture with background worker thread, ICU bed management using Counting Semaphore, and automatic waitlist allocation on appointment cancellation. |
| 6 | Dashboard, Audit & API | `src/routes.cpp`, `src/audit.*` | Multithreaded REST API (8 worker threads), aggregation metrics with SQL `GROUP BY`, and tamper-resistant audit trail. |

---

## 2. 10-Minute Presentation & Demo Script

1. **Architecture Overview (1 min):**
   - Explain the six-module architecture: C++17 server interfacing directly with PostgreSQL via `libpq` and serving the web client.
2. **Access Control & Booking Workflow (2 mins):**
   - Log in as `reception` (`reception` / `reception123`) and book an appointment for Alice at a future slot.
   - Open an incognito browser tab, log in as `bob` (`bob` / `patient123`), and attempt to book the exact same doctor and slot.
   - Observe the immediate **409 Conflict** error handled gracefully by the UI.
3. **OS Race Condition Demo (2 mins):**
   - In terminal, execute: `./medislot --demo-race`
   - Point out:
     - Part 1: 8 threads without locking produce multiple winners (data inconsistency).
     - Part 2: Mutex around the critical section guarantees exactly 1 winner.
     - Part 3: Real PostgreSQL transaction + partial unique index rejects 7 conflicting concurrent transactions.
4. **Deadlock Detection & Prevention Demo (2 mins):**
   - In terminal, execute: `./medislot --demo-deadlock`
   - Point out:
     - Part 1: Opposite lock orders create a circular wait deadlock. The `WaitForGraph` detects the cycle `booking A -> booking B -> booking A` and `try_lock_for` times out.
     - Part 2: `ResourceLocks::lock_all` lexicographically sorts the requested resources, preventing deadlocks entirely.
5. **Priority Scheduling & Background Worker Demo (2 mins):**
   - Navigate to the ICU tab: Request 3 normal priority admissions (filling all 3 beds).
   - Submit a 4th request marked **Emergency**.
   - Discharge one patient: Observe the background consumer thread immediately admit the emergency patient ahead of any earlier normal requests.
   - Cancel an appointment that has an active waitlist: Watch the scheduler worker automatically book the slot for the waitlisted patient.
6. **Analytics & Audit Log (1 min):**
   - Log in as `admin` (`admin` / `admin123`) and inspect the Dashboard for real-time bookings per doctor (`GROUP BY`) and audit log entries.

---

## 3. DBMS Viva Q&A

- **Q: What is a Partial Unique Index and why is it used here?**
  - **A:** It is an index with a `WHERE` clause (e.g., `WHERE status = 'booked'`). Unlike standard unique constraints that lock columns for all rows, partial unique indexes allow cancelled appointments to remain in the database for history while immediately freeing up the slot time for new bookings.
- **Q: How are ACID transactions implemented in the C++ backend?**
  - **A:** Using RAII via the `Transaction` class in `src/db.hpp`. The constructor issues `BEGIN`. If business operations succeed, `commit()` executes `COMMIT`. If any exception is thrown, the destructor automatically issues `ROLLBACK`, guaranteeing atomicity.
- **Q: How does the trigger prevent past bookings?**
  - **A:** `trg_no_past_booking` is a `BEFORE INSERT` trigger in PostgreSQL. It compares `NEW.start_time < localtimestamp`. If true, it raises an exception with SQLSTATE code `P0001`, which our backend catches and translates to an HTTP 400 Bad Request.
- **Q: How are passwords secured? Is plain text ever stored?**
  - **A:** No plain text or unsalted hashes are stored. PostgreSQL's `pgcrypto` extension is used with Blowfish crypt (`gen_salt('bf')`). Authentication queries verify credentials directly in SQL: `WHERE username = $1 AND password_hash = crypt($2, password_hash)`.
- **Q: How is SQL injection prevented?**
  - **A:** Every single query uses parameterized prepared statements via `PQexecParams` with `$1, $2, ...` placeholders. User input is transferred as parameters, never concatenated into SQL strings.
- **Q: What database normalization level does Medislot achieve?**
  - **A:** 3rd Normal Form (3NF). Master entities (`doctors`, `patients`, `rooms`) have their own tables with primary keys. No transitive dependencies exist. Relational views (`appointment_details`) join them for presentation.
- **Q: What is Row-Level Security (RLS)?**
  - **A:** Demonstrated in `sql/roles.sql`, RLS enforces security policies at the database row level based on session variables (`app.patient_id`), ensuring a patient can only SELECT their own records regardless of client query logic.

---

## 4. Operating Systems Viva Q&A

- **Q: What is a Race Condition and where does it occur?**
  - **A:** A race condition occurs when concurrent threads access and modify shared resources without proper synchronization, causing the final state to depend on thread scheduling order (e.g., two threads check if a slot is free, both see yes, and both take it).
- **Q: What is a Critical Section and how is it protected?**
  - **A:** A critical section is a sequence of instructions accessing shared state that must be executed by at most one thread at a time. It is protected by `std::mutex` and `std::unique_lock` in `src/booking.cpp`.
- **Q: How does your Counting Semaphore work?**
  - **A:** Implemented in `src/locks.hpp` using a `std::mutex`, `std::condition_variable`, and an integer count. `acquire()` (P) waits until count > 0 and decrements it; `release()` (V) increments count and notifies waiting threads. It bounds ICU capacity.
- **Q: What are the 4 Coffman conditions for Deadlock?**
  - **A:**
    1. Mutual Exclusion
    2. Hold and Wait
    3. No Preemption
    4. Circular Wait
- **Q: How does Medislot achieve Deadlock Prevention?**
  - **A:** By eliminating the Circular Wait condition. In `ResourceLocks::lock_all`, all requested resource identifiers (e.g., `"doctor:1"`, `"patient:2"`, `"room:1"`) are sorted lexicographically before acquiring locks. All threads acquire locks in the exact same global order.
- **Q: How does Medislot perform Deadlock Detection?**
  - **A:** Using two complementary approaches:
    1. **Wait-For Graph (WFG):** Implemented in `src/deadlock.cpp`, maintaining a directed graph where edges represent threads waiting for resources held by others. Depth-First Search (DFS) detects cycles in the graph.
    2. **Timeout:** Using `std::timed_mutex::try_lock_for` to detect deadlocked threads that cannot progress within a deadline.
- **Q: Explain the Producer-Consumer pattern in the Scheduler.**
  - **A:** Request threads handling HTTP API calls act as **Producers** that enqueue work (ICU requests, cancellations) and signal `scheduler.wake()`. A dedicated background worker thread acts as the **Consumer**, waking via `std::condition_variable` to admit prioritized ICU patients and auto-book waitlist candidates.
- **Q: Why does the web server use a Thread Pool?**
  - **A:** `httplib::ThreadPool(8)` pre-allocates 8 worker threads to service incoming HTTP requests concurrently, preventing thread creation overhead and bounding system resource usage.

---

## 5. Honest Limitations & Engineering Trade-offs

- **Process-Bound Mutexes:** Mutexes coordinate threads within a single server process. For horizontal multi-node scaling, PostgreSQL's partial unique indexes serve as the definitive shared concurrency barrier.
- **In-Memory Sessions:** Session tokens are held in an in-memory `std::map`. Server restarts invalidate active sessions, which is appropriate for a college project demonstration without requiring an external cache like Redis.
- **Local Network Scope:** The server uses HTTP without TLS/HTTPS; in production, a reverse proxy (e.g., Nginx or Caddy) would terminate SSL/TLS.
