#pragma once

#include <string>
#include <functional>
#include <cstdio>

namespace localstream {

struct TranscodeRequest {
    std::string file_path;
    std::string format;      // formato destino: "mp3", "opus", "aac"
    int         bitrate_kbps; // 0 = sin límite (pass-through)
};

class Transcoder {
public:
    // Llama a callback con cada chunk de audio transcodeado.
    // Retorna false si ffmpeg no pudo abrir el archivo.
    bool transcode(
        const TranscodeRequest& req,
        std::function<void(const char*, std::size_t)> on_chunk
    );

    // Devuelve el MIME type para el formato destino
    static std::string mimeType(const std::string& format);

    // True si el archivo ya está en el formato pedido y al bitrate pedido
    // — en ese caso conviene servir directo sin transcodificar
    static bool needsTranscode(
        const std::string& src_format,
        const std::string& dst_format,
        int src_bitrate_kbps,
        int max_bitrate_kbps
    );
};

} // namespace localstream