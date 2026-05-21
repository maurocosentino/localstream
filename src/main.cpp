#include <filesystem>

#include "api/ApiRouter.hpp"
#include "api/AuthMiddleware.hpp"
#include "api/SubsonicRouter.hpp"
#include "api/WebHandler.hpp"
#include "config/AppConfig.hpp"
#include "crow.h"
#include "db/Database.hpp"
#include "logger/Logger.hpp"
#include "scanner/LibraryScanner.hpp"
#include "streaming/StreamHandler.hpp"

int main(int argc, char* argv[]) {
  std::string config_path = "../config.json";
  std::string static_dir = "";

  for (int i = 1; i < argc; i++) {
    if (std::string(argv[i]) == "--config" && i + 1 < argc) {
      config_path = argv[i + 1];
    } else if (std::string(argv[i]) == "--static-dir" && i + 1 < argc) {
      static_dir = argv[i + 1];
    }
  }

  try {
    auto config = localstream::AppConfig::load(config_path);

    auto levelFromString = [](const std::string& s) {
      if (s == "DEBUG") return localstream::LogLevel::DEBUG;
      if (s == "WARN") return localstream::LogLevel::WARN;
      if (s == "ERROR") return localstream::LogLevel::ERROR;
      return localstream::LogLevel::INFO;
    };
    localstream::Logger::instance().setLevel(levelFromString(config.log_level));

    LOG_INFO("Main", "LocalStream v0.1.0 arrancando...");
    LOG_INFO("Main", "Configuracion cargada. Puerto: " +
                         std::to_string(config.server_port));

    localstream::Database db(config.db_path);
    localstream::LibraryScanner library_scanner(db, config.media_directories);

    LOG_INFO("Main", "Escaneando biblioteca...");
    library_scanner.scan();

    // Si no se pasó --static-dir, intentamos el default de desarrollo
    if (static_dir.empty()) {
      static_dir = "../localstream-web/dist";
      if (!std::filesystem::exists(static_dir)) {
        LOG_WARN("Main", "Frontend no encontrado en " + static_dir +
                             " — UI deshabilitada");
        static_dir = "";
      }
    }

    crow::App<localstream::AuthMiddleware> app;
    app.get_middleware<localstream::AuthMiddleware>().set_key(config.api_key);

    localstream::ApiRouter router(db, app, library_scanner);
    localstream::SubsonicRouter subsonic(db, app, config.api_key);
    localstream::StreamHandler streamer(db, app);
    localstream::WebHandler web(db, app, static_dir);

    LOG_INFO("Main", "Servidor escuchando en puerto " +
                         std::to_string(config.server_port));
    app.loglevel(crow::LogLevel::Warning);
    app.port(config.server_port).multithreaded().run();

  } catch (const std::exception& e) {
    LOG_ERROR("Main", std::string("Error fatal: ") + e.what());
    return 1;
  }

  return 0;
}