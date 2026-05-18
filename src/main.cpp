#include "crow.h"
#include <iostream>
#include "config/AppConfig.hpp"
#include "db/Database.hpp"
#include "scanner/LibraryScanner.hpp"
#include "api/ApiRouter.hpp"
#include "streaming/StreamHandler.hpp"

int main()
{
    std::cout << "LocalStream v0.1.0 — arrancando...\n";

    try {
        auto config = localstream::AppConfig::load("../config.json");

        localstream::Database       db(config.db_path);
        localstream::LibraryScanner library_scanner(db, config.media_directories);

        // Escaneo inicial
        std::cout << "Escaneando biblioteca...\n";
        library_scanner.scan();

        // Arrancar servidor HTTP
        crow::SimpleApp app;
        localstream::ApiRouter     router(db, app, library_scanner);
        localstream::StreamHandler streamer(db, app);

        std::cout << "Servidor escuchando en puerto " << config.server_port << "\n";
        app.port(config.server_port).multithreaded().run();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}