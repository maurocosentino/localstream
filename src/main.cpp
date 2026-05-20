#include "crow.h"
#include "config/AppConfig.hpp"
#include "db/Database.hpp"
#include "scanner/LibraryScanner.hpp"
#include "api/ApiRouter.hpp"
#include "streaming/StreamHandler.hpp"
#include "logger/Logger.hpp"
#include "api/WebHandler.hpp"
#include <filesystem>
#include "api/AuthMiddleware.hpp"

int main()
{
    try {
        auto config = localstream::AppConfig::load("../config.json");

        // Configurar log level desde config — antes de cualquier LOG_*
        auto levelFromString = [](const std::string& s) {
            if (s == "DEBUG") return localstream::LogLevel::DEBUG;
            if (s == "WARN")  return localstream::LogLevel::WARN;
            if (s == "ERROR") return localstream::LogLevel::ERROR;
            return localstream::LogLevel::INFO;
        };
        localstream::Logger::instance().setLevel(levelFromString(config.log_level));

        LOG_INFO("Main", "LocalStream v0.1.0 arrancando...");
        LOG_INFO("Main", "Configuracion cargada. Puerto: " + std::to_string(config.server_port));

        localstream::Database       db(config.db_path);
        localstream::LibraryScanner library_scanner(db, config.media_directories);

        LOG_INFO("Main", "Escaneando biblioteca...");
        library_scanner.scan();

        std::string static_dir = "../localstream-web/dist";
        if (!std::filesystem::exists(static_dir)) {
            LOG_WARN("Main", "Frontend no encontrado en " + static_dir + " — UI deshabilitada");
            static_dir = "";
        }

        crow::App<localstream::AuthMiddleware> app;
        app.get_middleware<localstream::AuthMiddleware>().set_key(config.api_key);

        // crow::SimpleApp app;
        localstream::ApiRouter     router(db, app, library_scanner);
        localstream::StreamHandler streamer(db, app);
        localstream::WebHandler    web(db, app, static_dir);

        LOG_INFO("Main", "Servidor escuchando en puerto " + std::to_string(config.server_port));
         app.loglevel(crow::LogLevel::Warning);
        app.port(config.server_port).multithreaded().run();

    } catch (const std::exception& e) {
        LOG_ERROR("Main", std::string("Error fatal: ") + e.what());
        return 1;
    }

    return 0;
}