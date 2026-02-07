#pragma once

#include "crow.h"

class Application
{
public:
    Application(void);
    void Run(void);

private:
    void SetupRoutes(void);    
    crow::SimpleApp app;
};
