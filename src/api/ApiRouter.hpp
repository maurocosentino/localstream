#pragma once

#include "crow.h"
#include "db/Database.hpp"
#include "scanner/LibraryScanner.hpp"
#include <string>

namespace localstream {

class ApiRouter {
public:
    ApiRouter(Database& db, crow::SimpleApp& app, LibraryScanner& library_scanner);

private:
    Database&        db_;
    crow::SimpleApp& app_;
    LibraryScanner&  library_scanner_;

    void setupRoutes();

    crow::json::wvalue artistToJson(const Artist& artist);
    crow::json::wvalue albumToJson(const Album& album);
    crow::json::wvalue trackToJson(const Track& track);
    crow::json::wvalue playlistToJson(const Playlist& playlist);
};

} // namespace localstream