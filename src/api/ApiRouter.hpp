#pragma once

#include "api/AuthMiddleware.hpp"
#include "crow.h"
#include "db/Database.hpp"
#include "scanner/LibraryScanner.hpp"

namespace localstream {

class ApiRouter {
 public:
  ApiRouter(Database& db, crow::App<AuthMiddleware>& app,
            LibraryScanner& library_scanner);

 private:
  Database& db_;
  crow::App<AuthMiddleware>& app_;
  LibraryScanner& library_scanner_;

  void setupRoutes();

  crow::json::wvalue artistToJson(const Artist& artist);
  crow::json::wvalue albumToJson(const Album& album);
  crow::json::wvalue trackToJson(const Track& track);
  crow::json::wvalue playlistToJson(const Playlist& playlist);
};

}  // namespace localstream