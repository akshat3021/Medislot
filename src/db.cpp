#ifndef DB_HPP
#define DB_HPP

#include <string>
#include <memory>
#include <pqxx/pqxx>

class Database {
public:
    // Thread-safe Singleton Instance
    static Database& getInstance();

    // Connection & Setup Methods
    bool connect(const std::string& connection_string);
    std::shared_ptr<pqxx::connection> getConnection();

    // SQL Execution Utilities
    void executeScript(const std::string& filepath);
    void initializeSchemaAndSeed();

private:
    Database() = default;
    ~Database() = default;
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    std::shared_ptr<pqxx::connection> conn_;
};

#endif // DB_HPP
