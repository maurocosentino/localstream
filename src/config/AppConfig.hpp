#pragma once

#include <string>
#include <vector>

namespace localstream {

struct AppConfig {
    int         server_port;
    std::string db_path;
    std::vector<std::string> media_directories;

    // Factory method — lee y parsea config.json
    // Lanza std::runtime_error si el archivo no existe o es inválido
    static AppConfig load(const std::string& config_path);
};

} // namespace localstream