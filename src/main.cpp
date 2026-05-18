#include "crow.h"
#include <iostream>
#include "config/AppConfig.hpp"
#include "db/Database.hpp"
#include "scanner/LibraryScanner.hpp"
#include "api/ApiRouter.hpp"
#include "streaming/StreamHandler.hpp"
#include "logger/Logger.hpp"

int main()
{
    // Nivel INFO por default — cambiá a DEBUG para ver todos los tracks
    localstream::Logger::instance().setLevel(localstream::LogLevel::INFO);

    LOG_INFO("Main", "LocalStream v0.1.0 arrancando...");

    try {
        auto config = localstream::AppConfig::load("../config.json");
        LOG_INFO("Main", "Configuracion cargada. Puerto: " + std::to_string(config.server_port));

        localstream::Database       db(config.db_path);
        localstream::LibraryScanner library_scanner(db, config.media_directories);

        LOG_INFO("Main", "Escaneando biblioteca...");
        library_scanner.scan();

        crow::SimpleApp app;
        localstream::ApiRouter     router(db, app, library_scanner);
        localstream::StreamHandler streamer(db, app);

        LOG_INFO("Main", "Servidor escuchando en puerto " + std::to_string(config.server_port));
        app.loglevel(crow::LogLevel::Warning);
        app.port(config.server_port).multithreaded().run();

    } catch (const std::exception& e) {
        LOG_ERROR("Main", std::string("Error fatal: ") + e.what());
        return 1;
    }

    return 0;
}