#pragma once

#include <map>
#include <cctype>
#include <fstream>
#include <vector>

namespace localstream {

template<typename App>
std::string StreamHandler<App>::getMimeType(const std::string& format)
{
    if (format == "mp3")  return "audio/mpeg";
    if (format == "flac") return "audio/flac";
    if (format == "ogg")  return "audio/ogg";
    if (format == "opus") return "audio/ogg; codecs=opus";
    if (format == "aac")  return "audio/aac";
    if (format == "wav")  return "audio/wav";
    return "application/octet-stream";
}

template<typename App>
std::optional<RangeRequest> StreamHandler<App>::parseRangeHeader(
    const std::string& range_header,
    long long file_size)
{
    if (range_header.empty()) return std::nullopt;

    const std::string prefix = "bytes=";
    if (range_header.substr(0, prefix.size()) != prefix) return std::nullopt;

    std::string range    = range_header.substr(prefix.size());
    auto        dash_pos = range.find('-');
    if (dash_pos == std::string::npos) return std::nullopt;

    std::string start_str = range.substr(0, dash_pos);
    std::string end_str   = range.substr(dash_pos + 1);

    RangeRequest result;
    result.total = file_size;
    result.start = std::stoll(start_str);
    result.end   = end_str.empty() ? file_size - 1 : std::stoll(end_str);

    if (result.start < 0 || result.end >= file_size || result.start > result.end)
        return std::nullopt;

    return result;
}

template<typename App>
void StreamHandler<App>::setupRoutes()
{
    CROW_ROUTE(app_, "/stream/<int>")
    ([this](const crow::request& req, crow::response& res, int track_id){

        std::string file_path = db_.getTrackPath(track_id);
        if (file_path.empty()) {
            res.code = 404;
            res.write("Track no encontrado");
            res.end();
            return;
        }

        auto track = db_.getTrackById(track_id);
        if (!track) {
            res.code = 404;
            res.write("Track no encontrado");
            res.end();
            return;
        }

        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            res.code = 500;
            res.write("No se pudo abrir el archivo");
            res.end();
            return;
        }

        long long   file_size = track->file_size;
        std::string mime      = getMimeType(track->format);

        std::string range_header = req.get_header_value("Range");
        auto        range        = parseRangeHeader(range_header, file_size);

        if (!range) {
            res.code = 200;
            res.set_header("Content-Type",   mime);
            res.set_header("Content-Length", std::to_string(file_size));
            res.set_header("Accept-Ranges",  "bytes");

            constexpr std::size_t CHUNK = 65536;
            std::vector<char> buf(CHUNK);
            while (file) {
                file.read(buf.data(), CHUNK);
                std::streamsize n = file.gcount();
                if (n > 0) res.write(std::string(buf.data(), n));
            }
            res.end();
            return;
        }

        long long length = range->end - range->start + 1;
        file.seekg(range->start);

        std::string buffer(length, '\0');
        file.read(buffer.data(), length);

        std::string content_range =
            "bytes " +
            std::to_string(range->start) + "-" +
            std::to_string(range->end)   + "/" +
            std::to_string(range->total);

        res.code = 206;
        res.set_header("Content-Type",   mime);
        res.set_header("Content-Range",  content_range);
        res.set_header("Content-Length", std::to_string(length));
        res.set_header("Accept-Ranges",  "bytes");
        res.write(buffer);
        res.end();
    });
}

} // namespace localstream