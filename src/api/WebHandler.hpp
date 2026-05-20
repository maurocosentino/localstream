#pragma once

#include "crow.h"
#include "db/Database.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace localstream {

// Funciones libres acá — visibles al template sin problemas
inline std::string wh_readFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

inline std::string wh_mimeType(const std::string& path)
{
    auto endsWith = [&](const std::string& ext) {
        return path.size() >= ext.size() &&
               path.compare(path.size() - ext.size(), ext.size(), ext) == 0;
    };
    if (endsWith(".html")) return "text/html; charset=utf-8";
    if (endsWith(".js"))   return "application/javascript";
    if (endsWith(".css"))  return "text/css";
    if (endsWith(".svg"))  return "image/svg+xml";
    if (endsWith(".ico"))  return "image/x-icon";
    if (endsWith(".png"))  return "image/png";
    return "application/octet-stream";
}

template<typename App>
class WebHandler {
public:
    WebHandler(Database& db, App& app, const std::string& static_dir)
        : db_(db), app_(app), static_dir_(static_dir)
    {
        setupRoutes();
    }

private:
    Database&   db_;
    App&        app_;
    std::string static_dir_;

    void setupRoutes();
};

} // namespace localstream

#include "api/WebHandler.ipp"