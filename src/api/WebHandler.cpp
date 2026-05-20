#include "api/WebHandler.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace localstream {

namespace fs = std::filesystem;

WebHandler::WebHandler(Database& db, crow::SimpleApp& app, const std::string& static_dir)
    : db_(db), app_(app), static_dir_(static_dir)
{
    setupRoutes();
}

static std::string readFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

static std::string mimeType(const std::string& path)
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

void WebHandler::setupRoutes()
{
    // Servir archivos estáticos del dist/
    CROW_ROUTE(app_, "/assets/<path>")
    ([this](const std::string& path){
        std::string file_path = static_dir_ + "/assets/" + path;

        if (!fs::exists(file_path)) {
            return crow::response(404);
        }

        std::string content = readFile(file_path);
        auto res = crow::response(200, content);
        res.set_header("Content-Type", mimeType(file_path));
        res.set_header("Cache-Control", "public, max-age=31536000"); // 1 año — assets tienen hash
        return res;
    });

    // Todas las rutas no-API retornan index.html (SPA routing)
    CROW_ROUTE(app_, "/")
    ([this]{
        std::string content = readFile(static_dir_ + "/index.html");
        if (content.empty()) {
            return crow::response(404, "Frontend no encontrado. Ejecutá npm run build en localstream-web/");
        }
        auto res = crow::response(200, content);
        res.set_header("Content-Type", "text/html; charset=utf-8");
        return res;
    });

    // Endpoint de cover de álbumes
    CROW_ROUTE(app_, "/api/albums/<int>/cover")
    ([this](int album_id){
        auto cover = db_.getAlbumCover(album_id);
        if (!cover || cover->data.empty()) {
            return crow::response(404);
        }
        crow::response res(200);
        res.set_header("Content-Type", cover->mime_type);
        res.set_header("Cache-Control", "public, max-age=86400");
        res.body.assign(
            reinterpret_cast<const char*>(cover->data.data()),
            cover->data.size());
        return res;
    });
}

} // namespace localstream