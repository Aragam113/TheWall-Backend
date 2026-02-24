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
    bool isConnected() const { return is_connected; }

    void startup_database();

    // Users
    pqxx::result create_user(const std::string& username, const std::string& password);
    pqxx::result get_user_by_id(const std::string& user_id);
    pqxx::result get_user_by_username(const std::string& username);
    pqxx::result update_user(const std::string& user_id, const std::string& username,
                             const std::string& avatar_uri, const std::string& banner_uri);
    bool delete_user(const std::string& user_id);
    bool update_last_login(const std::string& user_id);

    // Posts
    pqxx::result create_post(const std::string& author_id, const std::string& text,
                             const std::string& image_uri);
    pqxx::result get_post_by_id(const std::string& post_id);
    pqxx::result get_posts(int limit, int offset);
    pqxx::result get_user_posts(const std::string& user_id, int limit, int offset);
    bool delete_post(const std::string& post_id);

    // Likes
    bool like_post(const std::string& user_id, const std::string& post_id);
    bool unlike_post(const std::string& user_id, const std::string& post_id);
    int get_post_likes_count(const std::string& post_id);
    bool is_post_liked_by_user(const std::string& user_id, const std::string& post_id);

    // Comments
    pqxx::result create_comment(const std::string& user_id, const std::string& post_id,
                                const std::string& text);
    pqxx::result get_post_comments(const std::string& post_id, int limit, int offset);
    bool delete_comment(const std::string& comment_id);
};
