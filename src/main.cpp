#include "crow.h"
#include <SQLiteCpp/SQLiteCpp.h>
#include <taglib/fileref.h>
#include <iostream>
#include "config/AppConfig.hpp"

int main()
{
    std::cout << "LocalStream v0.1.0 — arrancando...\n";

    try {
        auto config = localstream::AppConfig::load("../config.json");
        std::cout << "Puerto: "      << config.server_port << "\n";
        std::cout << "DB path: "     << config.db_path     << "\n";
        std::cout << "Directorios: " << config.media_directories.size() << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error de configuracion: " << e.what() << "\n";
        return 1;
    }

    return 0;
}