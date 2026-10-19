// Module 1: Common data structures and exceptions
#pragma once
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

// Tabular representation of database query results
using Row  = std::map<std::string, std::string>;   // one result row: column name -> text
using Rows = std::vector<Row>;

// Business code throws this; routes.cpp converts it to HTTP JSON responses {"error": "..."}.
struct HttpError : std::runtime_error {
    int status;
    HttpError(int status, const std::string& message) : std::runtime_error(message), status(status) {}
};
