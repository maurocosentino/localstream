#pragma once

#include "crow.h"
#include "db/Database.hpp"
#include "scanner/LibraryScanner.hpp"

namespace localstream {

template<typename App>
class ApiRouter {
public:
    ApiRouter(Database& db, App& app, LibraryScanner& library_scanner)
        : db_(db), app_(app), library_scanner_(library_scanner)
    {
        setupRoutes();
    }

private:
    Database&        db_;
    App&             app_;
    LibraryScanner&  library_scanner_;

    void setupRoutes();

    crow::json::wvalue artistToJson(const Artist& artist);
    crow::json::wvalue albumToJson(const Album& album);
    crow::json::wvalue trackToJson(const Track& track);
    crow::json::wvalue playlistToJson(const Playlist& playlist);
};

} // namespace localstream

#include "api/ApiRouter.ipp"