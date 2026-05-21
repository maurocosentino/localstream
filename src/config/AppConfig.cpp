#include "config/AppConfig.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <filesystem>                  

namespace localstream {

AppConfig AppConfig::load(const std::string& config_path)
{
    std::ifstream file(config_path);
    if (!file.is_open()) {
        throw std::runtime_error(
            "No se pudo abrir el archivo de configuracion: " + config_path
        );
    }

    nlohmann::json json_data;
    try {
        file >> json_data;
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error(
            std::string("Error parseando config.json: ") + e.what()
        );
    }

    AppConfig config;

    config.server_port = json_data.at("server").at("port").get<int>();
    config.db_path     = json_data.at("database").at("path").get<std::string>();
    config.log_level   = json_data.value("log_level", "INFO");
    config.api_key     = json_data.value("api_key", "");

    for (const auto& dir : json_data.at("media_directories")) {
        config.media_directories.push_back(dir.get<std::string>());
    }

    if (!config.db_path.empty() && config.db_path[0] != '/') {
        auto config_dir = std::filesystem::path(config_path).parent_path();
        config.db_path  = (config_dir / config.db_path).string();
    }

    return config;
}

} // namespace localstream