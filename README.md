<div align="center" >
<p style="font-size: 36px; color: #ffffffcc; font-weight: bold">
The Wall
</p>
<p>
  <a href="https://isocpp.org/">
    <img src="https://img.shields.io/badge/C++-17-00599C?style=for-the-badge&logo=cplusplus" alt="C++">
  </a>
  <a href="https://www.postgresql.org/">
    <img src="https://img.shields.io/badge/PostgreSQL-18-4169E1?style=for-the-badge&logo=postgresql" alt="PostgreSQL">
  </a>
  <a href="https://crowcpp.org/">
    <img src="https://img.shields.io/badge/Crow-Framework-black?style=for-the-badge" alt="Crow">
  </a>
  <a href="LICENSE">
    <img src="https://img.shields.io/badge/License-MIT-green?style=for-the-badge" alt="License">
  </a>
</p>

</div>

---

## Overview

#### //todo: make an overview;

## Getting Started

### Prerequisites
- CMake 3.20+
- PostgreSQL 13+
- vcpkg

### Installation

```bash
git clone https://github.com/Aragam113/TheWall-Backend.git
cd TheWall-Backend
cmake -B build
cmake --build build
```

### Environment

```env
PORT = 18080
HOST = localhost
USER = YOUR_USER
PASSWORD = YOUR_PASSWORD
DB_NAME = the_wall_db
DB_PORT = 5432
```

---

## Database Schema

<div align="center">
    <img src="./git-assets/db_schema.png" alt="Database Schema" width="700"/>
</div>

---

## API Endpoints

| Method | Endpoint        | Auth | Description                   |
|--------|-----------------|------|-------------------------------|
| GET    | '/'             | No   | Hello world                   |
| GET    | '/health'       | No   | Server health check           |
| POST   | '/login'        | No   | Test user login               |
| GET    | '/private/data' | Yes  | Get logined user private data |


## Project Structure

```text
├───.githooks
├───.github
├───git-assets
│   └───db_schema.png
├───include
│   └───laserpants
│       └───dotenv
│           └───dotenv.h
└───src
    ├───main.cpp
    │   
    ├───application
    │   ├───Application.cpp
    │   └───Application.h
    │       
    ├───config
    │   ├───config.cpp
    │   └───config.h
    │       
    ├───database
    │   ├───database.cpp
    │   └───database.h
    │       
    └───middlewares
        └───BearerAuthMiddleware.h


```