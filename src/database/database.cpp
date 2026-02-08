#include "database.h"
#include <crow/logging.h>
#include "../config.h"

database::database (const std::string& connection_string) : connection (nullptr), is_connected (false)
{
    connection = std::make_unique<pqxx::connection>(connection_string);
}

database::~database()
{
    if (connection && is_connected)
    {
        connection->close();
    }
}

bool database::connect()
{
    try
    {
        if (connection && connection->is_open())
        {
            is_connected = true;
            CROW_LOG_INFO << "Connection to database is established";
            return true;
        }
        return false;
    } catch (const pqxx::failure& e) {
        CROW_LOG_ERROR << "Connection to database failed" << e.what();
        return false;
    }
}

pqxx::result database::exec(const std::string& query) {
    if (!is_connected) {
        throw std::runtime_error("Нет активного соединения");
    }
    try {
        pqxx::work txn(*connection);
        auto res = txn.exec(query);
        txn.commit();
        return res;
    } catch (const pqxx::sql_error& e) {
        CROW_LOG_ERROR <<  "SQL error: " << e.what();
        CROW_LOG_ERROR <<  "Request: " << e.query();
        throw;
    }
}

void database::startup_database()
{
    if (!is_connected) { throw std::runtime_error("The database is not connected"); }
    
    try {
        pqxx::work txn(*connection);
        txn.exec(CREATE_TABLES_QUERY);
        txn.commit();
        CROW_LOG_INFO << "Succesfully created tables";
    } catch (const pqxx::sql_error& e) {
        CROW_LOG_ERROR << "SQL error: " << e.what();
        CROW_LOG_ERROR << "Request: " << e.query();
        throw;
    }
}
