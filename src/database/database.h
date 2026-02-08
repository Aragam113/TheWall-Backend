#pragma once
#include <memory>
#include <string>
#include <pqxx/pqxx>

class database
{
private: 
    std::unique_ptr<pqxx::connection> connection;
    bool is_connected;
    
public: 
    explicit database(const std::string& connection_string);
    ~database();
    bool connect();
    pqxx::result exec(const std::string& query);
    bool isConnected() const
    {
        return is_connected;
    }
    void startup_database();
};