#include "scanner/LibraryScanner.hpp"
#include <iostream>

namespace localstream {

LibraryScanner::LibraryScanner(Database& db, const std::vector<std::string>& directories)
    : db_(db), directories_(directories), is_scanning_(false)
{
}

LibraryScanner::~LibraryScanner()
{
    // Si el thread de scan está corriendo, esperamos que termine
    if (scan_thread_.joinable()) {
        scan_thread_.join();
    }
}

bool LibraryScanner::isScanning() const
{
    return is_scanning_.load();
}

int LibraryScanner::scan()
{
    return runScan();
}

bool LibraryScanner::scanAsync()
{
    // Si ya hay un scan en curso, rechazamos
    bool expected = false;
    if (!is_scanning_.compare_exchange_strong(expected, true)) {
        return false;
    }

    // Si hay un thread anterior terminado, hacemos join antes de crear uno nuevo
    if (scan_thread_.joinable()) {
        scan_thread_.join();
    }

    // Lanzamos el scan en un thread separado
    scan_thread_ = std::thread([this]{
        runScan();
        is_scanning_.store(false);
    });

    return true;
}

int LibraryScanner::runScan()
{
    int new_tracks = 0;

    auto files = scanner_.scan(directories_);
    std::cout << "Archivos encontrados: " << files.size() << "\n";

    for (const auto& file_path : files) {
        // Lectura de metadata — sin lock, no toca la DB
        auto metadata = reader_.read(file_path);
        if (!metadata) {
            continue;
        }

        // Lock granular — solo durante las escrituras a DB
        {
            std::lock_guard<std::mutex> lock(db_mutex_);

            int artist_id = db_.insertArtist(metadata->artist_name);
            int album_id  = db_.insertAlbum(metadata->album_title, artist_id, metadata->year);

            Track track;
            track.title        = metadata->title;
            track.artist_id    = artist_id;
            track.album_id     = album_id;
            track.file_path    = metadata->file_path;
            track.duration_s   = metadata->duration_s;
            track.track_number = metadata->track_number;
            track.file_size    = metadata->file_size;
            track.format       = metadata->format;

            bool inserted = false;
            db_.insertTrack(track, inserted);
            if (inserted) {
                new_tracks++;
                std::cout << "  [ok] " << metadata->artist_name
                          << " — " << metadata->title << "\n";
            }
        } // lock liberado acá — lecturas pueden entrar entre archivos

    }

    std::cout << "Scan completado. Tracks nuevos: " << new_tracks << "\n";
    return new_tracks;
}

} // namespace localstream