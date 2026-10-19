// Module 6: HTTP Web Server and REST API Implementation
#include "routes.hpp"

#include <fstream>
#include <functional>
#include <iostream>

#include <httplib.h>
#include <nlohmann/json.hpp>

#include "auth.hpp"
#include "booking.hpp"

using json = nlohmann::json;

static SessionStore sessions;

static json to_json(const Rows& rows) {
    json list = json::array();
    for (const Row& row : rows) {
        list.push_back(row);
    }
    return list;
}

static json parse_body(const httplib::Request& req) {
    json body = json::parse(req.body, nullptr, false);
    if (body.is_discarded() || !body.is_object()) {
        throw HttpError(400, "request body must be a JSON object");
    }
    return body;
}

static int int_field(const json& body, const char* key) {
    if (!body.contains(key)) return 0;
    if (body[key].is_number_integer()) return body[key].get<int>();
    if (body[key].is_string()) {
        try {
            return std::stoi(body[key].get<std::string>());
        } catch (...) {}
    }
    return 0;
}

static std::string text_field(const json& body, const char* key, const std::string& fallback = "") {
    return body.contains(key) && body[key].is_string() ? body[key].get<std::string>() : fallback;
}

static std::string token_of(const httplib::Request& req) {
    std::string header = req.get_header_value("Authorization");
    return header.rfind("Bearer ", 0) == 0 ? header.substr(7) : "";
}

// OS/Security Concept: Reference Monitor (Authentication & Authorization Guard)
static Session require_login(const httplib::Request& req, const char* action) {
    std::optional<Session> session = sessions.find(token_of(req));
    if (!session) {
        throw HttpError(401, "please log in");
    }
    if (!allowed(session->role, action)) {
        throw HttpError(403, "your role is not allowed to do this");
    }
    return *session;
}

using Handler = std::function<json(const Session&, const httplib::Request&)>;

static httplib::Server::Handler api(const char* action, Handler handler) {
    return [action, handler](const httplib::Request& req, httplib::Response& res) {
        auto fail = [&](int status, const std::string& message) {
            res.status = status;
            res.set_content(json{{"error", message}}.dump(), "application/json");
        };
        try {
            Session who;
            if (action) {
                who = require_login(req, action);
            }
            res.set_content(handler(who, req).dump(), "application/json");
        } catch (const HttpError& e) {
            fail(e.status, e.what());
        } catch (const std::exception& e) {
            std::cerr << "error: " << e.what() << "\n";
            fail(500, "server error");
        }
    };
}

void run_server(Scheduler& scheduler, int port) {
    httplib::Server server;
    // OS Concept: Thread Pool pattern handling concurrent HTTP connections
    server.new_task_queue = [] { return new httplib::ThreadPool(8); };
    server.set_payload_max_length(64 * 1024);
    server.set_default_headers({{"X-Content-Type-Options", "nosniff"}, {"Cache-Control", "no-store"}});

    // Serve frontend from frontend/dist if built, otherwise fallback to web/
    std::ifstream dist_index("frontend/dist/index.html");
    if (dist_index.good()) {
        server.set_mount_point("/", "frontend/dist");
    } else {
        server.set_mount_point("/", "web");
    }

    // --- Authentication ---
    server.Post("/api/login", api(nullptr, [](const Session&, const httplib::Request& req) {
        json body = parse_body(req);
        std::string username = text_field(body, "username");
        std::optional<Session> session = check_login(db(), username, text_field(body, "password"));
        if (!session) {
            audit(db(), username, "login-failed");
            throw HttpError(401, "wrong username or password");
        }
        audit(db(), username, "login");
        return json{
            {"token", sessions.add(db(), *session)},
            {"username", session->username},
            {"role", session->role},
            {"doctor_id", session->doctor_id},
            {"patient_id", session->patient_id}
        };
    }));

    server.Post("/api/logout", api(nullptr, [](const Session&, const httplib::Request& req) {
        sessions.remove(token_of(req));
        return json{{"ok", true}};
    }));

    // --- Entity Lookups ---
    server.Get("/api/doctors", api("list", [](const Session&, const httplib::Request&) {
        return to_json(db().query("SELECT id, name, specialty FROM doctors ORDER BY name"));
    }));

    server.Get("/api/rooms", api("list", [](const Session&, const httplib::Request&) {
        return to_json(db().query("SELECT id, name, kind FROM rooms ORDER BY name"));
    }));

    server.Get("/api/patients", api("patients", [](const Session&, const httplib::Request&) {
        return to_json(db().query("SELECT id, name, phone FROM patients ORDER BY name"));
    }));

    // --- Appointments ---
    server.Get("/api/appointments", api("list", [](const Session& who, const httplib::Request&) {
        std::string sql = "SELECT id, to_char(start_time, 'YYYY-MM-DD HH24:MI') AS start, status, patient, doctor, "
                          "COALESCE(room, '') AS room FROM appointment_details";
        std::vector<std::string> params;
        if (who.role == "patient") {
            sql += " WHERE patient_id = $1";
            params = {std::to_string(who.patient_id)};
        } else if (who.role == "doctor") {
            sql += " WHERE doctor_id = $1";
            params = {std::to_string(who.doctor_id)};
        }
        sql += " ORDER BY start_time DESC LIMIT 100";
        return to_json(db().query(sql, params));
    }));

    server.Post("/api/appointments", api("book", [](const Session& who, const httplib::Request& req) {
        json body = parse_body(req);
        BookingRequest request;
        request.doctor_id  = int_field(body, "doctor_id");
        request.room_id    = int_field(body, "room_id");
        request.start      = text_field(body, "start");
        request.patient_id = (who.role == "patient") ? who.patient_id : int_field(body, "patient_id");
        if (request.doctor_id == 0 || request.patient_id == 0) {
            throw HttpError(400, "doctor_id and patient_id are required");
        }

        int id = book_appointment(db(), request);
        audit(db(), who.username, "book", "appointment " + std::to_string(id));
        return json{{"id", id}};
    }));

    server.Post(R"(/api/appointments/(\d+)/cancel)", api("cancel", [&scheduler](const Session& who, const httplib::Request& req) {
        int id = std::stoi(req.matches[1]);
        cancel_appointment(db(), id, who.role == "patient" ? who.patient_id : 0);
        audit(db(), who.username, "cancel", "appointment " + std::to_string(id));
        scheduler.wake();
        return json{{"ok", true}};
    }));

    // --- Priority Waitlist ---
    server.Post("/api/waitlist", api("waitlist", [&scheduler](const Session& who, const httplib::Request& req) {
        json body = parse_body(req);
        int patient_id = (who.role == "patient") ? who.patient_id : int_field(body, "patient_id");
        std::string start = text_field(body, "start");
        std::string priority = (who.role == "patient") ? "normal" : text_field(body, "priority", "normal");
        check_slot_format(start);
        if (patient_id == 0 || int_field(body, "doctor_id") == 0) {
            throw HttpError(400, "doctor_id and patient_id are required");
        }
        if (priority != "normal" && priority != "emergency") {
            throw HttpError(400, "priority must be normal or emergency");
        }
        try {
            db().query("INSERT INTO waitlist (patient_id, doctor_id, start_time, priority) VALUES ($1, $2, $3, $4)",
                       {std::to_string(patient_id), std::to_string(int_field(body, "doctor_id")), start, priority});
        } catch (const DbError& e) {
            if (e.sqlstate == "23505") throw HttpError(409, "already on the waitlist for that slot");
            if (e.sqlstate == "23503") throw HttpError(404, "unknown doctor or patient");
            throw;
        }
        audit(db(), who.username, "waitlist", "patient " + std::to_string(patient_id) + " at " + start);
        scheduler.wake();
        return json{{"ok", true}};
    }));

    // --- ICU Stays ---
    server.Get("/api/icu", api("icu", [&scheduler](const Session&, const httplib::Request&) {
        Rows stays = db().query(
            "SELECT s.id, s.patient_id, p.name AS patient, s.priority, s.status FROM icu_stays s "
            "JOIN patients p ON p.id = s.patient_id "
            "WHERE s.status IN ('waiting', 'admitted') "
            "ORDER BY (s.status = 'admitted') DESC, (s.priority = 'emergency') DESC, s.requested_at");
        int admitted = 0;
        for (const Row& stay : stays) {
            if (stay.at("status") == "admitted") {
                ++admitted;
            }
        }
        return json{
            {"beds_total", scheduler.icu_beds()},
            {"beds_free", scheduler.icu_beds() - admitted},
            {"stays", to_json(stays)}
        };
    }));

    server.Post("/api/icu/request", api("icu", [&scheduler](const Session& who, const httplib::Request& req) {
        json body = parse_body(req);
        std::string priority = text_field(body, "priority", "normal");
        if (priority != "normal" && priority != "emergency") {
            throw HttpError(400, "priority must be normal or emergency");
        }
        int patient_id = int_field(body, "patient_id");
        scheduler.icu_request(db(), patient_id, priority);
        audit(db(), who.username, "icu-request", "patient " + std::to_string(patient_id) + " (" + priority + ")");
        return json{{"ok", true}};
    }));

    server.Post("/api/icu/discharge", api("icu", [&scheduler](const Session& who, const httplib::Request& req) {
        int patient_id = int_field(parse_body(req), "patient_id");
        scheduler.icu_discharge(db(), patient_id);
        audit(db(), who.username, "icu-discharge", "patient " + std::to_string(patient_id));
        return json{{"ok", true}};
    }));

    // --- Stats & Audit Log ---
    server.Get("/api/stats", api("stats", [](const Session&, const httplib::Request&) {
        Rows totals = db().query("SELECT count(*) FILTER (WHERE status = 'booked') AS booked, "
                                 "count(*) FILTER (WHERE status = 'cancelled') AS cancelled FROM appointments");
        Rows per_doctor = db().query("SELECT d.name AS doctor, count(a.id) AS booked FROM doctors d "
                                     "LEFT JOIN appointments a ON a.doctor_id = d.id AND a.status = 'booked' "
                                     "GROUP BY d.id, d.name ORDER BY d.name");
        return json{{"totals", totals[0]}, {"per_doctor", to_json(per_doctor)}};
    }));

    server.Get("/api/audit", api("audit", [](const Session&, const httplib::Request&) {
        return to_json(db().query("SELECT to_char(at, 'YYYY-MM-DD HH24:MI:SS') AS at, COALESCE(username, '') AS username, "
                                  "action, COALESCE(detail, '') AS detail FROM audit_log ORDER BY id DESC LIMIT 50"));
    }));

    std::cout << "Medislot is running at http://0.0.0.0:" << port << "   (Ctrl+C to stop)\n";
    server.listen("0.0.0.0", port);
}
