#include "scanner/LibraryScanner.hpp"
#include <iostream>

namespace localstream {

LibraryScanner::LibraryScanner(Database& db, const std::vector<std::string>& directories)
    : db_(db), directories_(directories)
{
}

int LibraryScanner::scan()
{
    int new_tracks = 0;

    auto files = scanner_.scan(directories_);
    std::cout << "Archivos encontrados: " << files.size() << "\n";

    for (const auto& file_path : files) {
        auto metadata = reader_.read(file_path);
        if (!metadata) {
            std::cout << "  [skip] " << file_path << "\n";
            continue;
        }

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
        int id = db_.insertTrack(track, inserted);
        if (inserted) {
            new_tracks++;
            std::cout << "  [ok] " << metadata->artist_name
                      << " — " << metadata->title << "\n";
}
    }

    return new_tracks;
}

} // namespace localstream