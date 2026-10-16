# Medislot - Hospital Appointment System (C++ backend + PostgreSQL / Supabase)

A college DBMS + Operating Systems project. Patients and staff book doctor/room slots, manage ICU beds and a
waitlist. The server is **one C++17 program** (~900 lines); the pages are plain HTML/CSS/JavaScript (no npm).

| Part | Technology |
|---|---|
| Backend | C++17, `cpp-httplib` (web server), `nlohmann/json`, `libpq` (PostgreSQL client) |
| Database | PostgreSQL (your Supabase). Tables live in their own schema `medislot`, so your old tables are untouched |
| Frontend | `web/index.html`, `style.css`, `app.js` (served by the C++ server) |

## 1. What you need
* A C++ compiler (`g++`), `make`, and the PostgreSQL client library (`libpq` + headers).
  * **Ubuntu / WSL:** `sudo apt install g++ make libpq-dev`
  * **macOS:** `brew install libpq` (then `export PATH="/opt/homebrew/opt/libpq/bin:$PATH"`)
  * **Windows (recommended: MSYS2 "UCRT64" terminal):** `pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-postgresql make`
    *(Windows was not tested by the author: if it fails, use WSL, which is the same as Ubuntu.)*

## 2. Connect to Supabase
Supabase dashboard -> **Connect** -> copy the **Session pooler** connection string (works on IPv4) and put your
password in it. It looks like `postgresql://postgres.xxxx:PASSWORD@aws-0-xx.pooler.supabase.com:5432/postgres`.
```bash
export DATABASE_URL='postgresql://postgres.xxxx:PASSWORD@aws-0-xx.pooler.supabase.com:5432/postgres?sslmode=require'
# Windows cmd:  set DATABASE_URL=...        PowerShell:  $env:DATABASE_URL='...'
```
Never put this string in a file you commit to GitHub.

## 3. Build, create the tables, run
```bash
make                 # compiles the server  ->  ./medislot
./medislot --init    # creates schema "medislot" + demo data in Supabase (erases only that schema)
./medislot           # starts the server       ->  open http://localhost:8080
```
Demo logins (change them for anything real):

| User | Password | Role |
|---|---|---|
| admin | admin123 | everything + audit log |
| reception | reception123 | book for anyone, ICU, dashboard |
| drmehta | doctor123 | sees only own appointments, ICU view |
| alice, bob | patient123 | book / cancel only their own |

## 4. Live demos for the presentation
```bash
./medislot --demo-race       # race condition: 8 threads, 1 slot  (without lock vs with mutex vs real database)
./medislot --demo-deadlock   # deadlock with opposite lock order, and the fix (sorted order)
```

## 5. Tests
`make test` runs 11 tests on a real database. They **erase the `medislot` schema**, so they only run if you set
`TEST_DATABASE_URL` on purpose (use a separate Supabase project or a local PostgreSQL, not data you care about).

## 6. Folder structure
```
sql/schema.sql   tables, constraints, indexes, trigger, view      (Module 1)
sql/seed.sql     demo data (passwords hashed by PostgreSQL bcrypt)
sql/roles.sql    optional: GRANT/REVOKE + Row-Level Security demo (Module 2)
src/db.*         tiny PostgreSQL wrapper (Module 1)
src/auth.*       login, sessions, permission table             (Module 2)
src/locks.hpp    Semaphore + ordered ResourceLocks             (Modules 3, 4)
src/booking.*    safe booking with locks + transaction         (Modules 3, 4)
src/scheduler.*  ICU beds (semaphore) + waitlist + worker thread (Module 5)
src/routes.cpp   HTTP API                                       (Module 6)
src/demos.cpp    the two live demos
src/main.cpp     start-up
web/             the pages (Module 1 UI, Module 6 dashboard)
tests/tests.cpp  11 tests
third_party/     httplib.h and json.hpp (single-file libraries, MIT licence)
docs/TEAM_GUIDE.md   who does what + viva questions
```

## 7. API (all JSON, send `Authorization: Bearer <token>` after login)
`POST /api/login` `POST /api/logout` `GET /api/doctors|rooms|patients` `GET|POST /api/appointments`
`POST /api/appointments/{id}/cancel` `POST /api/waitlist` `GET /api/icu` `POST /api/icu/request|discharge`
`GET /api/stats` `GET /api/audit`

## 8. Honest limitations (college level on purpose)
* No HTTPS and sessions are kept in memory (restart = log in again). Use it on localhost / a lab network.
* Passwords are bcrypt-hashed by PostgreSQL; there is no login rate limit or CSRF protection (not needed for a Bearer
  token kept in `sessionStorage`, but this is not hardened for the internet).
* Run **one** server process: the mutexes only coordinate threads of the same process (the database unique indexes
  are the safety net behind them).
* The web page was written but its automated browser test was not completed; click through it once before the demo.
* Windows build is untested.
