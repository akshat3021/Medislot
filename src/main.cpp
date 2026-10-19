// Medislot: Hospital Appointment & Resource Management System
// Entry point handling CLI modes:
//   ./medislot                 Starts HTTP web server on PORT (default: 8080)
//   ./medislot --init          Initializes database schema and seed data
//   ./medislot --demo-race     Demonstrates race conditions & synchronization
//   ./medislot --demo-deadlock Demonstrates deadlock detection & prevention
#include <cstdlib>
#include <iostream>
#include <string>

#include "demos.hpp"
#include "routes.hpp"

int main(int argc, char** argv) {
    std::string mode = argc > 1 ? argv[1] : "";
    if (mode == "--demo-deadlock") {
        return run_deadlock_demo();          // Self-contained demo, does not require DB
    }

    const char* url = std::getenv("DATABASE_URL");
    if (!url) {
        std::cerr << "Please set DATABASE_URL first (see README.md, step 2).\n";
        return 1;
    }
    set_connection_string(url);

    try {
        if (mode == "--init") {
            init_database();
            std::cout << "Database ready (schema \"medislot\" created with demo data).\n";
            return 0;
        }
        if (mode == "--demo-race") {
            return run_race_demo();
        }

        const char* port = std::getenv("PORT");
        Scheduler scheduler(3);              // 3 ICU beds managed by counting semaphore
        run_server(scheduler, port ? std::atoi(port) : 8080);
    } catch (const std::exception& e) {
        std::cerr << "fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
