#include "crow.h"
#include <iostream>
#include "config/AppConfig.hpp"
#include "db/Database.hpp"
#include "scanner/FileScanner.hpp"
#include "metadata/MetadataReader.hpp"

int main()
{
    std::cout << "LocalStream v0.1.0 — arrancando...\n";

    try {
        auto config = localstream::AppConfig::load("../config.json");
        localstream::Database       db(config.db_path);
        localstream::FileScanner    scanner;
        localstream::MetadataReader reader;

        std::cout << "Escaneando biblioteca...\n";

        auto files = scanner.scan(config.media_directories);
        std::cout << "Archivos encontrados: " << files.size() << "\n";

        for (const auto& file_path : files) {
            auto metadata = reader.read(file_path);
            if (!metadata) {
                std::cout << "  [skip] " << file_path << "\n";
                continue;
            }

            int artist_id = db.insertArtist(metadata->artist_name);
            int album_id  = db.insertAlbum(metadata->album_title, artist_id, metadata->year);

            localstream::Track track;
            track.title        = metadata->title;
            track.artist_id    = artist_id;
            track.album_id     = album_id;
            track.file_path    = metadata->file_path;
            track.duration_s   = metadata->duration_s;
            track.track_number = metadata->track_number;
            track.file_size    = metadata->file_size;
            track.format       = metadata->format;

            db.insertTrack(track);
            std::cout << "  [ok] " << metadata->artist_name
                      << " — " << metadata->title << "\n";
        }

        auto artists = db.getArtists();
        std::cout << "\nArtistas en DB: " << artists.size() << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}