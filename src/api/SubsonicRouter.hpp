#pragma once

#include "crow.h"
#include "db/Database.hpp"
#include <string>
#include <sstream>

namespace localstream {

// Parámetros comunes que vienen en cada request Subsonic
struct SubsonicParams {
    std::string client;
    std::string version;
    std::string format;   // "json" o "xml" — solo soportaremos json
    std::string username;
    std::string token;
    std::string salt;
};

template<typename App>
class SubsonicRouter {
public:
    SubsonicRouter(Database& db, App& app, const std::string& api_key)
        : db_(db), app_(app), api_key_(api_key)
    {
        setupRoutes();
    }

private:
    Database&   db_;
    App&        app_;
    std::string api_key_;

    void setupRoutes();

    // Helpers de respuesta
    crow::response okResponse(crow::json::wvalue& data);
    crow::response errorResponse(int code, const std::string& message);

    // Converters
    crow::json::wvalue artistToSubsonic(const Artist& artist);
    crow::json::wvalue albumToSubsonic(const Album& album);
    crow::json::wvalue trackToSubsonic(const Track& track);
};

} // namespace localstream

#include "api/SubsonicRouter.ipp"