#include "WallBackend.h"
#include "crow.h"

int main()
{
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([]()
    {
        return "Hello World!";
    });

    CROW_ROUTE(app, "/health")([]()
    {
        return "The wall is here";
    });

    app.port(18080).multithreaded().run();
}