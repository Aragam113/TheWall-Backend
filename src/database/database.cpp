#include "database.h"
#include <crow/logging.h>
#include <queue>
#include <mutex>
#include <algorithm>

database::database(const std::string& connection_string) 
    : conn_str(connection_string), connection(nullptr), is_connected(false)
{
}

database::~database() {
    std::lock_guard<std::mutex> lock(pool_mutex);
    while (!pool.empty()) {
        pool.pop();
    }
    if (connection && connection->is_open()) {
        connection->close();
    }
}

bool database::connect() {
    try {
        if (connection) connection.reset();
        connection = std::make_unique<pqxx::connection>(conn_str);
        if (connection->is_open()) {
            is_connected = true;
            init_pool();
            CROW_LOG_INFO << "Connection pool to database is established (" << pool_size << " connections)";
            return true;
        }
        is_connected = false;
        return false;
    } catch (const pqxx::failure& e) {
        is_connected = false;
        CROW_LOG_ERROR << "Connection to database failed: " << e.what();
        return false;
    }
}

void database::init_pool() {
    std::lock_guard<std::mutex> lock(pool_mutex);
    while (!pool.empty()) {
        pool.pop();
    }
    for (size_t i = 0; i < pool_size; ++i) {
        try {
            auto conn = std::make_unique<pqxx::connection>(conn_str);
            pool.push(std::move(conn));
        } catch (const std::exception& e) {
            CROW_LOG_ERROR << "Failed to create pool connection: " << e.what();
        }
    }
}

std::unique_ptr<pqxx::connection> database::get_connection() {
    std::lock_guard<std::mutex> lock(pool_mutex);
    if (pool.empty()) {
        throw std::runtime_error("Connection pool is empty");
    }
    auto conn = std::move(pool.front());
    pool.pop();
    if (!conn->is_open()) {
        conn = std::make_unique<pqxx::connection>(conn_str);
    }
    return conn;
}

void database::release_connection(std::unique_ptr<pqxx::connection> conn) {
    std::lock_guard<std::mutex> lock(pool_mutex);
    pool.push(std::move(conn));
}

pqxx::result database::exec(const std::string& query) {
    if (!is_connected) {
        throw std::runtime_error("The database is not connected");
    }
    
    auto conn = get_connection();
    try {
        pqxx::work txn(*conn);
        auto res = txn.exec(query);
        txn.commit();
        release_connection(std::move(conn));
        return res;
    } catch (const pqxx::sql_error& e) {
        CROW_LOG_ERROR << "SQL error: " << e.what();
        CROW_LOG_ERROR << "Request: " << e.query();
        release_connection(std::move(conn));
        throw;
    }
}
