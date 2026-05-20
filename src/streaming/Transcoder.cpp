#include "streaming/Transcoder.hpp"
#include "logger/Logger.hpp"
#include <array>
#include <stdexcept>

namespace localstream {

std::string Transcoder::mimeType(const std::string& format)
{
    if (format == "mp3")  return "audio/mpeg";
    if (format == "opus") return "audio/ogg; codecs=opus";
    if (format == "aac")  return "audio/aac";
    if (format == "flac") return "audio/flac";
    if (format == "ogg")  return "audio/ogg";
    return "application/octet-stream";
}

bool Transcoder::needsTranscode(
    const std::string& src_format,
    const std::string& dst_format,
    int src_bitrate_kbps,
    int max_bitrate_kbps)
{
    // Formato diferente → siempre transcodificar
    if (src_format != dst_format) return true;
    // Mismo formato pero el original supera el bitrate máximo pedido
    if (max_bitrate_kbps > 0 && src_bitrate_kbps > max_bitrate_kbps) return true;
    return false;
}

bool Transcoder::transcode(
    const TranscodeRequest& req,
    std::function<void(const char*, std::size_t)> on_chunk)
{
    // Construir el comando ffmpeg
    // -i  : archivo de entrada
    // -vn : sin video
    // -b:a: bitrate de audio
    // -f  : formato de salida
    // pipe:1 : escribir a stdout

    std::string bitrate_arg = (req.bitrate_kbps > 0)
    ? "-b:a " + std::to_string(req.bitrate_kbps) + "k -maxrate " +
      std::to_string(req.bitrate_kbps) + "k -bufsize " +
      std::to_string(req.bitrate_kbps * 2) + "k"
    : "-q:a 2";

    std::string codec;
    std::string fmt;
    if (req.format == "mp3") {
        codec = "libmp3lame";
        fmt   = "mp3";
    } else if (req.format == "opus") {
        codec = "libopus";
        fmt   = "ogg";
    } else if (req.format == "aac") {
        codec = "aac";
        fmt   = "adts";
    } else {
        // Formato desconocido — pass-through como mp3
        codec = "libmp3lame";
        fmt   = "mp3";
    }

    std::string cmd =
        "ffmpeg -hide_banner -loglevel error"
        " -i \"" + req.file_path + "\""
        " -vn"
        " -c:a " + codec +
        " " + bitrate_arg +
        " -f " + fmt +
        " pipe:1 2>/dev/null";

    LOG_DEBUG("Transcoder", "Ejecutando: " + cmd);

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        LOG_ERROR("Transcoder", "popen falló para: " + req.file_path);
        return false;
    }

    constexpr std::size_t CHUNK = 65536; // 64 KB
    std::array<char, CHUNK> buf;
    std::size_t bytes_read = 0;

    while ((bytes_read = std::fread(buf.data(), 1, CHUNK, pipe)) > 0) {
        on_chunk(buf.data(), bytes_read);
    }

    int exit_code = pclose(pipe);
    if (exit_code != 0) {
        LOG_WARN("Transcoder", "ffmpeg terminó con código: " +
                 std::to_string(exit_code));
    }

    return true;
}

} // namespace localstream