#include "db.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

Database& Database::getInstance() {
    static Database instance;
    return instance;
}

bool Database::connect(const std::string& connection_string) {
    try {
        conn_ = std::make_shared<pqxx::connection>(connection_string);
        if (conn_->is_open()) {
            std::cout << "[DB] Connected to PostgreSQL database: " << conn_->dbname() << std::endl;
            return true;
        } else {
            std::cerr << "[DB Error] Failed to open database connection." << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "[DB Error] Exception caught during connection: " << e.what() << std::endl;
        return false;
    }
}

std::shared_ptr<pqxx::connection> Database::getConnection() {
    return conn_;
}

void Database::executeScript(const std::string& filepath) {
    if (!conn_ || !conn_->is_open()) {
        std::cerr << "[DB Error] Cannot execute script. Connection is not active." << std::endl;
        return;
    }

    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[DB Error] Unable to open SQL file: " << filepath << std::endl;
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sql = buffer.str();

    try {
        pqxx::work W(*conn_);
        W.exec(sql);
        W.commit();
        std::cout << "[DB] Successfully executed: " << filepath << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[DB Error] Execution failed for " << filepath << ": " << e.what() << std::endl;
    }
}

void Database::initializeSchemaAndSeed() {
    std::cout << "[DB] Initializing Database Infrastructure..." << std::endl;
    executeScript("sql/schema.sql");
    executeScript("sql/roles.sql");
    executeScript("sql/seed.sql");
}
