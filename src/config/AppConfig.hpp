#pragma once

#include <string>
#include <vector>

namespace localstream {

struct AppConfig {
    int         server_port;
    std::string db_path;
    std::string log_level;
    std::vector<std::string> media_directories;

    static AppConfig load(const std::string& config_path);
};

} // namespace localstream