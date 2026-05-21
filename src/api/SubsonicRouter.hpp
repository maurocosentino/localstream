#pragma once

#include <map>
#include <string>

#include "api/AuthMiddleware.hpp"
#include "crow.h"
#include "db/Database.hpp"

namespace localstream {

class SubsonicRouter {
 public:
  SubsonicRouter(Database& db, crow::App<AuthMiddleware>& app,
                 const std::string& api_key);

 private:
  Database& db_;
  crow::App<AuthMiddleware>& app_;
  std::string api_key_;
  bool validateAuth(const crow::request& req);

  void setupRoutes();

  crow::response okResponse(crow::json::wvalue& data);
  crow::response errorResponse(int code, const std::string& message);

  crow::json::wvalue artistToSubsonic(const Artist& artist);
  crow::json::wvalue albumToSubsonic(const Album& album);
  crow::json::wvalue trackToSubsonic(const Track& track);
};

}  // namespace localstream