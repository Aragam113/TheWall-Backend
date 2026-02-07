#include "crow.h"

struct BearerAuthMiddleware {
    struct context {
        std::string email;
    };

    void beforeHandle (crow::request& req, crow::response& res, context& ctx) {
        auto authHeader = req.get_header_value("Authorization");

        if (authHeader.empty() || authHeader.substr(0, 7) != "Bearer ") {
            res.code = 401;
            res.body = "Missing or invalid token format";
            res.end();
            return;
        }

        std::string token = authHeader.substr(7);

        if(token == "example-token-for-test")
        {
            res.code = 200;
            ctx.email = "my-user@gmail.com";
        }else
        {
            res.code = 401;
            res.body = "Invalid token";
            res.end();        
        }
    }

    void afterHandle (crow::request& req, crow::response& res, context& ctx) {

    }
};