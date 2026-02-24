#pragma once

#include "crow.h"
#include "../middlewares/BearerAuthMiddleware.h"
#include "../database/database.h"
#include <memory>

class Application
{
public:
    Application(void);
    void Run(void);

private:
    void SetupRoutes(void);
    crow::App<BearerAuthMiddleware> app;
    std::shared_ptr<database> db;
};
