#include "database.h"
#include <crow/logging.h>
#include "../config.h"
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
    std::scoped_lock<std::mutex> lock(pool_mutex);
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

void database::startup_database() {
    if (!is_connected) {
        throw std::runtime_error("The database is not connected");
    }

    auto conn = get_connection();
    try {
        pqxx::work txn(*conn);
        txn.exec(CREATE_TABLES_QUERY);
        txn.commit();
        release_connection(std::move(conn));
        CROW_LOG_INFO << "Successfully created tables";
    } catch (const pqxx::sql_error& e) {
        CROW_LOG_ERROR << "SQL error: " << e.what();
        CROW_LOG_ERROR << "Request: " << e.query();
        release_connection(std::move(conn));
        throw;
    }
}

// ============== Users ==============

pqxx::result database::create_user(const std::string& username, const std::string& password) {
    auto conn = get_connection();
    try {
        pqxx::work txn(*conn);
        auto res = txn.exec_params(
            "INSERT INTO users (username, password) VALUES ($1, $2) RETURNING *",
            username, password);
        txn.commit();
        release_connection(std::move(conn));
        return res;
    } catch (const pqxx::sql_error& e) {
        CROW_LOG_ERROR << "SQL error: " << e.what();
        release_connection(std::move(conn));
        throw;
    }
}

pqxx::result database::get_user_by_id(const std::string& user_id) {
    auto conn = get_connection();
    try {
        pqxx::work txn(*conn);
        auto res = txn.exec_params(
            "SELECT * FROM users WHERE id = $1::uuid", user_id);
        txn.commit();
        release_connection(std::move(conn));
        return res;
    } catch (const pqxx::sql_error& e) {
        CROW_LOG_ERROR << "SQL error: " << e.what();
        release_connection(std::move(conn));
        throw;
    }
}

pqxx::result database::get_user_by_username(const std::string& username) {
    auto conn = get_connection();
    try {
        pqxx::work txn(*conn);
        auto res = txn.exec_params(
            "SELECT * FROM users WHERE username = $1", username);
        txn.commit();
        release_connection(std::move(conn));
        return res;
    } catch (const pqxx::sql_error& e) {
        CROW_LOG_ERROR << "SQL error: " << e.what();
        release_connection(std::move(conn));
        throw;
    }
}

pqxx::result database::update_user(const std::string& user_id, const std::string& username,
                                   const std::string& avatar_uri, const std::string& banner_uri) {
    auto conn = get_connection();
    try {
        pqxx::work txn(*conn);
        auto res = txn.exec_params(
            "UPDATE users SET username=$1, profile_avatar_uri=$2, profile_banner_uri=$3 "
            "WHERE id=$4::uuid RETURNING *",
            username, avatar_uri, banner_uri, user_id);
        txn.commit();
        release_connection(std::move(conn));
        return res;
    } catch (const pqxx::sql_error& e) {
        CROW_LOG_ERROR << "SQL error: " << e.what();
        release_connection(std::move(conn));
        throw;
    }
}

bool database::delete_user(const std::string& user_id) {
    auto conn = get_connection();
    try {
        pqxx::work txn(*conn);
        auto res = txn.exec_params(
            "DELETE FROM users WHERE id = $1::uuid", user_id);
        txn.commit();
        release_connection(std::move(conn));
        return res.affected_rows() > 0;
    } catch (const pqxx::sql_error& e) {
        CROW_LOG_ERROR << "SQL error: " << e.what();
        release_connection(std::move(conn));
        throw;
    }
}

bool database::update_last_login(const std::string& user_id) {
    auto conn = get_connection();
    try {
        pqxx::work txn(*conn);
        auto res = txn.exec_params(
            "UPDATE users SET last_login = now() WHERE id = $1::uuid", user_id);
        txn.commit();
        release_connection(std::move(conn));
        return res.affected_rows() > 0;
    } catch (const pqxx::sql_error& e) {
        CROW_LOG_ERROR << "SQL error: " << e.what();
        release_connection(std::move(conn));
        throw;
    }
}

// ============== Posts ==============

pqxx::result database::create_post(const std::string& author_id, const std::string& text,
                                   const std::string& image_uri) {
    auto conn = get_connection();
    try {
        pqxx::work txn(*conn);
        auto count_res = txn.exec_params(
            "SELECT COUNT(*) FROM posts WHERE author_id=$1::uuid "
            "AND created_at > now() - INTERVAL '7 days'",
            author_id);
        int count = count_res[0][0].as<int>();
        if (count >= 7) {
            release_connection(std::move(conn));
            throw std::runtime_error("Post limit reached: maximum 7 posts per week");
        }
        auto res = txn.exec_params(
            "INSERT INTO posts (author_id, text, image_uri) "
            "VALUES ($1::uuid, $2, $3) RETURNING *",
            author_id, text, image_uri);
        txn.commit();
        release_connection(std::move(conn));
        return res;
    } catch (const pqxx::sql_error& e) {
        CROW_LOG_ERROR << "SQL error: " << e.what();
        release_connection(std::move(conn));
        throw;
    }
}

pqxx::result database::get_post_by_id(const std::string& post_id) {
    auto conn = get_connection();
    try {
        pqxx::work txn(*conn);
        auto res = txn.exec_params(
            "SELECT p.*, u.username as author_username "
            "FROM posts p JOIN users u ON p.author_id=u.id "
            "WHERE p.id=$1::uuid",
            post_id);
        txn.commit();
        release_connection(std::move(conn));
        return res;
    } catch (const pqxx::sql_error& e) {
        CROW_LOG_ERROR << "SQL error: " << e.what();
        release_connection(std::move(conn));
        throw;
    }
}

pqxx::result database::get_posts(int limit, int offset) {
    auto conn = get_connection();
    try {
        pqxx::work txn(*conn);
        auto res = txn.exec_params(
            "SELECT p.*, u.username as author_username "
            "FROM posts p JOIN users u ON p.author_id=u.id "
            "ORDER BY RANDOM() LIMIT $1 OFFSET $2",
            limit, offset);
        txn.commit();
        release_connection(std::move(conn));
        return res;
    } catch (const pqxx::sql_error& e) {
        CROW_LOG_ERROR << "SQL error: " << e.what();
        release_connection(std::move(conn));
        throw;
    }
}

pqxx::result database::get_user_posts(const std::string& user_id, int limit, int offset) {
    auto conn = get_connection();
    try {
        pqxx::work txn(*conn);
        auto res = txn.exec_params(
            "SELECT p.*, u.username as author_username "
            "FROM posts p JOIN users u ON p.author_id=u.id "
            "WHERE p.author_id=$1::uuid "
            "ORDER BY p.created_at DESC LIMIT $2 OFFSET $3",
            user_id, limit, offset);
        txn.commit();
        release_connection(std::move(conn));
        return res;
    } catch (const pqxx::sql_error& e) {
        CROW_LOG_ERROR << "SQL error: " << e.what();
        release_connection(std::move(conn));
        throw;
    }
}

bool database::delete_post(const std::string& post_id) {
    auto conn = get_connection();
    try {
        pqxx::work txn(*conn);
        auto res = txn.exec_params(
            "DELETE FROM posts WHERE id=$1::uuid", post_id);
        txn.commit();
        release_connection(std::move(conn));
        return res.affected_rows() > 0;
    } catch (const pqxx::sql_error& e) {
        CROW_LOG_ERROR << "SQL error: " << e.what();
        release_connection(std::move(conn));
        throw;
    }
}

// ============== Likes ==============

bool database::like_post(const std::string& user_id, const std::string& post_id) {
    auto conn = get_connection();
    try {
        pqxx::work txn(*conn);
        auto res = txn.exec_params(
            "INSERT INTO likes (user_id, post_id) VALUES ($1::uuid, $2::uuid) "
            "ON CONFLICT DO NOTHING",
            user_id, post_id);
        txn.commit();
        release_connection(std::move(conn));
        return res.affected_rows() > 0;
    } catch (const pqxx::sql_error& e) {
        CROW_LOG_ERROR << "SQL error: " << e.what();
        release_connection(std::move(conn));
        throw;
    }
}

bool database::unlike_post(const std::string& user_id, const std::string& post_id) {
    auto conn = get_connection();
    try {
        pqxx::work txn(*conn);
        auto res = txn.exec_params(
            "DELETE FROM likes WHERE user_id=$1::uuid AND post_id=$2::uuid",
            user_id, post_id);
        txn.commit();
        release_connection(std::move(conn));
        return res.affected_rows() > 0;
    } catch (const pqxx::sql_error& e) {
        CROW_LOG_ERROR << "SQL error: " << e.what();
        release_connection(std::move(conn));
        throw;
    }
}

int database::get_post_likes_count(const std::string& post_id) {
    auto conn = get_connection();
    try {
        pqxx::work txn(*conn);
        auto res = txn.exec_params(
            "SELECT COUNT(*) FROM likes WHERE post_id=$1::uuid", post_id);
        txn.commit();
        release_connection(std::move(conn));
        return res[0][0].as<int>();
    } catch (const pqxx::sql_error& e) {
        CROW_LOG_ERROR << "SQL error: " << e.what();
        release_connection(std::move(conn));
        throw;
    }
}

bool database::is_post_liked_by_user(const std::string& user_id, const std::string& post_id) {
    auto conn = get_connection();
    try {
        pqxx::work txn(*conn);
        auto res = txn.exec_params(
            "SELECT EXISTS(SELECT 1 FROM likes WHERE user_id=$1::uuid AND post_id=$2::uuid)",
            user_id, post_id);
        txn.commit();
        release_connection(std::move(conn));
        return res[0][0].as<bool>();
    } catch (const pqxx::sql_error& e) {
        CROW_LOG_ERROR << "SQL error: " << e.what();
        release_connection(std::move(conn));
        throw;
    }
}

// ============== Comments ==============

pqxx::result database::create_comment(const std::string& user_id, const std::string& post_id,
                                      const std::string& text) {
    auto conn = get_connection();
    try {
        pqxx::work txn(*conn);
        auto res = txn.exec_params(
            "INSERT INTO comments (user_id, post_id, text) "
            "VALUES ($1::uuid, $2::uuid, $3) RETURNING *",
            user_id, post_id, text);
        txn.commit();
        release_connection(std::move(conn));
        return res;
    } catch (const pqxx::sql_error& e) {
        CROW_LOG_ERROR << "SQL error: " << e.what();
        release_connection(std::move(conn));
        throw;
    }
}

pqxx::result database::get_post_comments(const std::string& post_id, int limit, int offset) {
    auto conn = get_connection();
    try {
        pqxx::work txn(*conn);
        auto res = txn.exec_params(
            "SELECT c.*, u.username FROM comments c "
            "JOIN users u ON c.user_id=u.id "
            "WHERE c.post_id=$1::uuid ORDER BY c.id LIMIT $2 OFFSET $3",
            post_id, limit, offset);
        txn.commit();
        release_connection(std::move(conn));
        return res;
    } catch (const pqxx::sql_error& e) {
        CROW_LOG_ERROR << "SQL error: " << e.what();
        release_connection(std::move(conn));
        throw;
    }
}

bool database::delete_comment(const std::string& comment_id) {
    auto conn = get_connection();
    try {
        pqxx::work txn(*conn);
        auto res = txn.exec_params(
            "DELETE FROM comments WHERE id=$1::uuid", comment_id);
        txn.commit();
        release_connection(std::move(conn));
        return res.affected_rows() > 0;
    } catch (const pqxx::sql_error& e) {
        CROW_LOG_ERROR << "SQL error: " << e.what();
        release_connection(std::move(conn));
        throw;
    }
}
