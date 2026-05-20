#pragma once

#include "crow.h"
#include "db/Database.hpp"
#include <string>

namespace localstream {

class WebHandler {
public:
    WebHandler(Database& db, crow::SimpleApp& app, const std::string& static_dir);

private:
    Database&        db_;
    crow::SimpleApp& app_;
    std::string      static_dir_;

    void setupRoutes();
};

} // namespace localstream