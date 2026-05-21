#pragma once

#include <string>

#include "api/AuthMiddleware.hpp"
#include "crow.h"
#include "db/Database.hpp"

namespace localstream {

class WebHandler {
 public:
  WebHandler(Database& db, crow::App<AuthMiddleware>& app,
             const std::string& static_dir);

 private:
  Database& db_;
  crow::App<AuthMiddleware>& app_;
  std::string static_dir_;

  void setupRoutes();
};

}  // namespace localstream