#pragma once

#include "crow.h"
#include "db/Database.hpp"

namespace localstream {

class WebHandler {
public:
    WebHandler(Database& db, crow::SimpleApp& app);

private:
    Database&        db_;
    crow::SimpleApp& app_;

    void setupRoutes();
};

} // namespace localstream