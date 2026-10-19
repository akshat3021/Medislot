// Module 6: HTTP Web Server and REST API
#pragma once
#include "scheduler.hpp"

// Starts the cpp-httplib multithreaded web server
void run_server(Scheduler& scheduler, int port);
