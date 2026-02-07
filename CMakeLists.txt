cmake_minimum_required(VERSION 3.20)
project(WallBackend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Crow
find_package(Crow CONFIG REQUIRED)

# libpqxx
find_package(libpqxx CONFIG REQUIRED)

add_executable(WallBackend
        src/main.cpp
        src/application/Application.cpp
        src/application/Application.h
        src/middlewares/BearerAuthMiddleware.cpp
        src/config.h
)

target_link_libraries(WallBackend PRIVATE
        Crow::Crow
        libpqxx::pqxx
)