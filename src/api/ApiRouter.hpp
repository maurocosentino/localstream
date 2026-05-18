#pragma once

#include "crow.h"
#include "db/Database.hpp"

namespace localstream {

class ApiRouter {
public:
    ApiRouter(Database& db, crow::SimpleApp& app);

private:
    Database&       db_;
    crow::SimpleApp& app_;

    void setupRoutes();

    // Helpers de serialización
    crow::json::wvalue artistToJson(const Artist& artist);
    crow::json::wvalue albumToJson(const Album& album);
    crow::json::wvalue trackToJson(const Track& track);
};

} // namespace localstream