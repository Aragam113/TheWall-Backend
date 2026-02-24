#pragma once

constexpr int PORT = 18080;
const std::string HOST = "localhost";
const std::string USER = "postgres";
const std::string PASSWORD = "postgresql";
const std::string DB_NAME = "postgres";
constexpr int DB_PORT = 5432;

const std::string CREATE_TABLES_QUERY = R"(
    DO $$
    BEGIN
        IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'user_role') THEN
            CREATE TYPE user_role AS ENUM ('admin', 'user', 'moderator');
        END IF;

        IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'user_status') THEN
            CREATE TYPE user_status AS ENUM ('active', 'closed', 'banned');
        END IF;
    END
    $$;

    CREATE TABLE IF NOT EXISTS users (
        id                  UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
        username            VARCHAR     NOT NULL UNIQUE,
        password            VARCHAR     NOT NULL,
        last_login          TIMESTAMP,
        profile_avatar_uri  VARCHAR,
        profile_banner_uri  VARCHAR,
        role                user_role   NOT NULL DEFAULT 'user',
        status              user_status NOT NULL DEFAULT 'active'
    );

    CREATE TABLE IF NOT EXISTS posts (
        id          UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
        author_id   UUID        NOT NULL REFERENCES users(id) ON DELETE CASCADE,
        text        TEXT,
        image_uri   VARCHAR,
        created_at  TIMESTAMP   NOT NULL DEFAULT now()
    );

    CREATE TABLE IF NOT EXISTS likes (
        id          UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
        user_id     UUID        NOT NULL REFERENCES users(id) ON DELETE CASCADE,
        post_id     UUID        NOT NULL REFERENCES posts(id) ON DELETE CASCADE,
        UNIQUE(user_id, post_id)
    );

    CREATE TABLE IF NOT EXISTS comments (
        id          UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
        user_id     UUID        NOT NULL REFERENCES users(id) ON DELETE CASCADE,
        post_id     UUID        NOT NULL REFERENCES posts(id) ON DELETE CASCADE,
        text        VARCHAR     NOT NULL
    );
)";
