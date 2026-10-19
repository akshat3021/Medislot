// Module 1: Database abstraction layer over libpq (PostgreSQL C API).
#pragma once
#include <libpq-fe.h>
#include <string>
#include <vector>

#include "common.hpp"

// PostgreSQL query error containing SQLSTATE error code (e.g., "23505" = unique violation)
struct DbError : std::runtime_error {
    std::string sqlstate;
    DbError(const std::string& message, const std::string& state)
        : std::runtime_error(message), sqlstate(state) {}
};

class Db {
public:
    explicit Db(const std::string& connection_string);
    ~Db();
    Db(const Db&) = delete;
    Db& operator=(const Db&) = delete;

    // Runs a parameterized SQL statement with $1, $2 placeholders (prevents SQL injection)
    Rows query(const std::string& sql, const std::vector<std::string>& params = {});

    // Runs multi-statement SQL scripts (schema/seed setup)
    void run_script(const std::string& sql);

private:
    PGconn* conn_;
};

// RAII Transaction: BEGIN on creation, COMMIT on commit(), ROLLBACK in destructor if uncommitted
class Transaction {
public:
    explicit Transaction(Db& db) : db_(db) { db_.query("BEGIN"); }
    ~Transaction() {
        if (!done_) {
            try { db_.query("ROLLBACK"); } catch (...) {}
        }
    }
    void commit() { db_.query("COMMIT"); done_ = true; }

private:
    Db& db_;
    bool done_ = false;
};

void set_connection_string(const std::string& connection_string);   // called once at start-up
const std::string& db_connection_string();                         // allows worker threads to connect
Db&  db();                                                          // thread-local connection per thread

// Creates tables and demo data from sql/schema.sql and sql/seed.sql
void init_database();

// Records user actions to the database audit_log table and live audit logger
void audit(Db& d, const std::string& user, const std::string& action, const std::string& detail = "");
