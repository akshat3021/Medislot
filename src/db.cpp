// Module 1: PostgreSQL Client Implementation using libpq
#include "db.hpp"
#include "audit.hpp"

#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>

Db::Db(const std::string& connection_string) {
    conn_ = PQconnectdb(connection_string.c_str());
    if (PQstatus(conn_) != CONNECTION_OK) {
        std::string message = PQerrorMessage(conn_);
        PQfinish(conn_);
        throw DbError("cannot connect to PostgreSQL: " + message, "");
    }
    // Set isolation schema and time zone per configuration
    run_script("SET search_path = medislot, public, extensions");
    const char* tz = std::getenv("HOSPITAL_TZ");
    run_script(std::string("SET TIME ZONE '") + (tz ? tz : "Asia/Kolkata") + "'");
}

Db::~Db() {
    if (conn_) {
        PQfinish(conn_);
    }
}

static void throw_if_failed(PGresult* res) {
    ExecStatusType status = PQresultStatus(res);
    if (status == PGRES_TUPLES_OK || status == PGRES_COMMAND_OK) return;
    const char* state = PQresultErrorField(res, PG_DIAG_SQLSTATE);
    const char* text  = PQresultErrorField(res, PG_DIAG_MESSAGE_PRIMARY);
    DbError error(text ? text : "database error", state ? state : "");
    PQclear(res);
    throw error;
}

Rows Db::query(const std::string& sql, const std::vector<std::string>& params) {
    std::vector<const char*> values;
    values.reserve(params.size());
    for (const std::string& p : params) {
        values.push_back(p.c_str());
    }

    PGresult* res = PQexecParams(conn_, sql.c_str(), static_cast<int>(values.size()), nullptr,
                                 values.data(), nullptr, nullptr, 0);
    throw_if_failed(res);

    Rows rows;
    int num_rows = PQntuples(res);
    int num_fields = PQnfields(res);
    rows.reserve(num_rows);

    for (int i = 0; i < num_rows; ++i) {
        Row row;
        for (int j = 0; j < num_fields; ++j) {
            const char* col_name = PQfname(res, j);
            row[col_name] = PQgetisnull(res, i, j) ? "" : PQgetvalue(res, i, j);
        }
        rows.push_back(std::move(row));
    }
    PQclear(res);
    return rows;
}

void Db::run_script(const std::string& sql) {
    PGresult* res = PQexec(conn_, sql.c_str());
    throw_if_failed(res);
    PQclear(res);
}

// Thread-local connection pattern: one connection per thread
static std::string g_connection_string;

void set_connection_string(const std::string& connection_string) {
    g_connection_string = connection_string;
}

const std::string& db_connection_string() {
    return g_connection_string;
}

Db& db() {
    thread_local std::unique_ptr<Db> connection;
    if (!connection) {
        connection = std::make_unique<Db>(g_connection_string);
    }
    return *connection;
}

void audit(Db& d, const std::string& user, const std::string& action, const std::string& detail) {
    d.query("INSERT INTO audit_log (username, action, detail) VALUES (NULLIF($1, ''), $2, $3)",
            {user, action, detail});
    // Integrate with teammate's in-memory live tracking
    medislot::AuditLogger::instance().log(user, action, detail);
}

static std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("cannot open " + path + " (run from the project folder)");
    std::stringstream text;
    text << file.rdbuf();
    return text.str();
}

void init_database() {
    db().run_script(read_file("sql/schema.sql"));
    db().run_script(read_file("sql/seed.sql"));
}
