#pragma once

#include "crow.h"
#include "../middlewares/BearerAuthMiddleware.h"

class Application
{
public:
    Application(void);
    void Run(void);

private:
    void SetupRoutes(void);    
    crow::App<BearerAuthMiddleware> app;
};
