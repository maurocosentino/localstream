#pragma once

namespace localstream {

namespace fs = std::filesystem;

template<typename App>
void WebHandler<App>::setupRoutes()
{
    CROW_ROUTE(app_, "/assets/<path>")
    ([this](const std::string& path){
        std::string file_path = static_dir_ + "/assets/" + path;
        if (!fs::exists(file_path)) return crow::response(404);
        std::string content = wh_readFile(file_path);
        auto res = crow::response(200, content);
        res.set_header("Content-Type",  wh_mimeType(file_path));
        res.set_header("Cache-Control", "public, max-age=31536000");
        return res;
    });

    CROW_ROUTE(app_, "/")
    ([this]{
        std::string content = wh_readFile(static_dir_ + "/index.html");
        if (content.empty())
            return crow::response(404, "Frontend no encontrado.");
        auto res = crow::response(200, content);
        res.set_header("Content-Type", "text/html; charset=utf-8");
        return res;
    });

    CROW_ROUTE(app_, "/api/albums/<int>/cover")
    ([this](int album_id){
        auto cover = db_.getAlbumCover(album_id);
        if (!cover || cover->data.empty()) return crow::response(404);
        crow::response res(200);
        res.set_header("Content-Type",  cover->mime_type);
        res.set_header("Cache-Control", "public, max-age=86400");
        res.body.assign(
            reinterpret_cast<const char*>(cover->data.data()),
            cover->data.size());
        return res;
    });
}

} // namespace localstream