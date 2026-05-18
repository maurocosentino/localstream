#include "crow.h"
#include <iostream>
#include "config/AppConfig.hpp"
#include "db/Database.hpp"

int main()
{
    std::cout << "LocalStream v0.1.0 — arrancando...\n";

    try {
        auto config = localstream::AppConfig::load("../config.json");
        std::cout << "Puerto: "  << config.server_port << "\n";
        std::cout << "DB path: " << config.db_path     << "\n";

        localstream::Database db(config.db_path);
        std::cout << "Base de datos inicializada correctamente.\n";

        auto artists = db.getArtists();
        std::cout << "Artistas en DB: " << artists.size() << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}