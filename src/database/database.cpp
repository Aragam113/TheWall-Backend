#include "database.h"
#include <crow/logging.h>
#include <queue>
#include <mutex>
#include <algorithm>

database::database(const std::string& connection_string) : conn_str(connection_string), connection(nullptr), is_connected(false)
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

bool database::initialize()
{
    if (!is_connected) return false;

    try
    {
        auto connection = get_connection();
        pqxx::work txn(*connection);
        
        txn.exec("CREATE EXTENSION IF NOT EXISTS \"uuid-ossp\";");
        
        txn.exec(R"(
            DO $$ BEGIN
                CREATE TYPE user_role AS ENUM ('admin', 'moderator', 'user');
            EXCEPTION
                WHEN duplicate_object THEN NULL;
            END $$;
        )");
        
        txn.exec(R"(
            DO $$ BEGIN
                CREATE TYPE user_status AS ENUM ('active', 'closed', 'banned');
            EXCEPTION
                WHEN duplicate_object THEN NULL;
            END $$;
        )");
        
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS Users (
                id                  UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
                username            VARCHAR(255) NOT NULL,
                password            VARCHAR(255) NOT NULL,
                profileAvatarUri    VARCHAR(255),   
                profileBannerUri    VARCHAR(255),
                role                user_role DEFAULT 'user' NOT NULL,
                status              user_status DEFAULT 'active' NOT NULL,
                lastLogin           TIMESTAMP DEFAULT NOW(),
                createdAt           TIMESTAMP DEFAULT NOW()
            )
        )");
        
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS Posts (
                id                  UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
                authorId            UUID NOT NULL,
                text                TEXT NOT NULL,
                imageUri            VARCHAR(255),
                createdAt           TIMESTAMP DEFAULT NOW(),

                CONSTRAINT fk_author
                    FOREIGN KEY(authorId)
                    REFERENCES Users(id)
                    ON DELETE CASCADE
            )
        )");
        
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS Likes (
                id                UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
                userId            UUID NOT NULL,
                postId            UUID NOT NULL,

                CONSTRAINT fk_user
                    FOREIGN KEY(userId)
                    REFERENCES Users(id)
                    ON DELETE CASCADE,

                CONSTRAINT fk_post
                    FOREIGN KEY(postId)
                    REFERENCES Posts(id)
                    ON DELETE CASCADE
            )
        )");
        
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS Comments (
                id                UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
                userId            UUID NOT NULL,
                postId            UUID NOT NULL,
                text              TEXT NOT NULL,

                CONSTRAINT fk_user
                    FOREIGN KEY(userId)
                    REFERENCES Users(id)
                    ON DELETE CASCADE,

                CONSTRAINT fk_post
                    FOREIGN KEY(postId)
                    REFERENCES Posts(id)
                    ON DELETE CASCADE
            )
        )");

        txn.commit();
        release_connection(std::move(connection));
        
        CROW_LOG_INFO << "Database initialized successfully";
        return true;
    }
    catch (const std::exception& e)
    {
        CROW_LOG_ERROR << "Connection to database failed: " << e.what();
        return false;
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
    std::scoped_lock<std::mutex> lock(pool_mutex);
    while (!pool.empty()) {
        pool.pop();
    }
    for (int i = 0; i < pool_size; ++i) {
        try {
            auto conn = std::make_unique<pqxx::connection>(conn_str);
            pool.push(std::move(conn));
        } catch (const std::exception& e) {
            CROW_LOG_ERROR << "Failed to create pool connection: " << e.what();
        }
    }
}

std::unique_ptr<pqxx::connection> database::get_connection() {
    std::scoped_lock<std::mutex> lock(pool_mutex);
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
    std::scoped_lock<std::mutex> lock(pool_mutex);
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
