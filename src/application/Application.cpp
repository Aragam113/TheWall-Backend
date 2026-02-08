#include "Application.h"
#include "../config.h"
#include "../database/database.h"

Application::Application()
{
    SetupRoutes();
}

void Application::Run()
{
    app.loglevel(crow::LogLevel::Warning);
    try
    {
        database db("host=localhost port=5432 dbname=postgres user=postgres password=postgresql");
        if (db.connect())
        {
            db.startup_database();
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
