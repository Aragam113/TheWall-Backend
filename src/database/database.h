#pragma once
#include <memory>
#include <string>
#include <pqxx/pqxx>
#include <queue>
#include <mutex>

class database
{
private:
    std::string conn_str;
    std::unique_ptr<pqxx::connection> connection;
    bool is_connected;
    std::queue<std::unique_ptr<pqxx::connection>> pool;
    std::mutex pool_mutex;
    size_t pool_size = 10;
    
    void init_pool();
    std::unique_ptr<pqxx::connection> get_connection();
    void release_connection(std::unique_ptr<pqxx::connection> conn);

    
public: 
    explicit database(const std::string& connection_string);
    ~database();
    bool connect();
    pqxx::result exec(const std::string& query);
    bool isConnected() const
    {
        return is_connected;
    }
    
};