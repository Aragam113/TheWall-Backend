#include "Application.h"
#include "../config.h"

Application::Application()
{
    SetupRoutes();
}

void Application::Run()
{
    app.loglevel(crow::LogLevel::Info);
    try
    {
        std::string conn_str = "host=" + std::string(HOST) +
                       " port=" + std::to_string(DB_PORT) +
                       " dbname=" + std::string(DB_NAME) +
                       " user=" + std::string(USER) +
                       " password=" + std::string(PASSWORD);
        db = std::make_shared<database>(conn_str);
        if (db->connect())
        {
            db->startup_database();
        }
    } catch (const std::exception& e)
    {
        CROW_LOG_ERROR << e.what();
    }
    app.port(PORT).multithreaded().run();
}

void Application::SetupRoutes()
{
    CROW_ROUTE(app, "/")([]()
    {
        return "Hello World!";
    });

    CROW_ROUTE(app, "/health")([]()
    {
        return "The wall is here";
    });

    CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req)
    {
        auto x = crow::json::load(req.body);
        if (!x) crow::response(400, "Bad Request");
        if (x["email"].s() == "my-user@gmail.com" && x["password"].s() == "somepassword123")
        {
            crow::json::wvalue res;

            res["token"] = "example-token-for-test";
            res["status"] = "success";

            return crow::response(res);
        }

        return crow::response(401, "Invalid credentials");
    });

    CROW_ROUTE(app, "/private/data")
    .CROW_MIDDLEWARES(app, BearerAuthMiddleware)
    ([this](const crow::request& req)
    {
        auto& ctx = app.get_context<BearerAuthMiddleware>(req);
        return ctx.email;
    });
}
