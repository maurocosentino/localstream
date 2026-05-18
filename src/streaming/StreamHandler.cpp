#include "streaming/StreamHandler.hpp"
#include <fstream>
#include <sstream>
#include <optional>

namespace localstream {

StreamHandler::StreamHandler(Database& db, crow::SimpleApp& app)
    : db_(db), app_(app)
{
    setupRoutes();
}

std::string StreamHandler::getMimeType(const std::string& format)
{
    if (format == "mp3")  return "audio/mpeg";
    if (format == "flac") return "audio/flac";
    return "application/octet-stream";
}

std::optional<RangeRequest> StreamHandler::parseRangeHeader(
    const std::string& range_header,
    long long file_size)
{
    // El header tiene formato: "bytes=START-END"
    // END es opcional — "bytes=500000-" significa hasta el final
    if (range_header.empty()) {
        return std::nullopt;
    }

    // Verificamos que empiece con "bytes="
    const std::string prefix = "bytes=";
    if (range_header.substr(0, prefix.size()) != prefix) {
        return std::nullopt;
    }

    std::string range = range_header.substr(prefix.size());

    // Separamos en START y END por el guión
    auto dash_pos = range.find('-');
    if (dash_pos == std::string::npos) {
        return std::nullopt;
    }

    std::string start_str = range.substr(0, dash_pos);
    std::string end_str   = range.substr(dash_pos + 1);

    RangeRequest result;
    result.total = file_size;
    result.start = std::stoll(start_str);

    // Si END está vacío, pedimos hasta el final del archivo
    if (end_str.empty()) {
        result.end = file_size - 1;
    } else {
        result.end = std::stoll(end_str);
    }

    // Validación
    if (result.start < 0 || result.end >= file_size || result.start > result.end) {
        return std::nullopt;
    }

    return result;
}

void StreamHandler::setupRoutes()
{
    CROW_ROUTE(app_, "/stream/<int>")
    ([this](const crow::request& req, crow::response& res, int track_id){

        // 1. Buscar el track en la DB
        std::string file_path = db_.getTrackPath(track_id);
        if (file_path.empty()) {
            res.code = 404;
            res.write("Track no encontrado");
            res.end();
            return;
        }

        // 2. Buscar info del track para el MIME type
        auto track = db_.getTrackById(track_id);
        if (!track) {
            res.code = 404;
            res.write("Track no encontrado");
            res.end();
            return;
        }

        // 3. Abrir el archivo
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            res.code = 500;
            res.write("No se pudo abrir el archivo");
            res.end();
            return;
        }

        long long file_size = track->file_size;
        std::string mime    = getMimeType(track->format);

        // 4. Parsear Range header
        std::string range_header = req.get_header_value("Range");
        auto range = parseRangeHeader(range_header, file_size);

        if (!range) {
            // Sin Range header — servimos el archivo completo
            res.code = 200;
            res.set_header("Content-Type",   mime);
            res.set_header("Content-Length", std::to_string(file_size));
            res.set_header("Accept-Ranges",  "bytes");

            std::ostringstream buffer;
            buffer << file.rdbuf();
            res.write(buffer.str());
            res.end();
            return;
        }

        // 5. Servir el rango pedido
        long long length = range->end - range->start + 1;

        // Hacer seek al byte de inicio
        file.seekg(range->start);

        // Leer exactamente 'length' bytes
        std::string buffer(length, '\0');
        file.read(buffer.data(), length);

        // Construir Content-Range: bytes START-END/TOTAL
        std::string content_range =
            "bytes " +
            std::to_string(range->start) + "-" +
            std::to_string(range->end)   + "/" +
            std::to_string(range->total);

        res.code = 206; // Partial Content
        res.set_header("Content-Type",   mime);
        res.set_header("Content-Range",  content_range);
        res.set_header("Content-Length", std::to_string(length));
        res.set_header("Accept-Ranges",  "bytes");

        res.write(buffer);
        res.end();
    });
}

} // namespace localstream