#include "Application.h"
#include "config.h"

Application::Application()
{
    SetupRoutes();
}


void Application::Run()
{
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

}
