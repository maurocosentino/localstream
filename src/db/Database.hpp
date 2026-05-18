#pragma once

#include <string>
#include <vector>
#include <optional>
#include <SQLiteCpp/SQLiteCpp.h>
#include "models.hpp"

namespace localstream {

class Database {
public:
    explicit Database(const std::string& db_path);

    // Inserción — retornan el id del registro (nuevo o existente)
    int insertArtist(const std::string& name);
    int insertAlbum(const std::string& title, int artist_id, int year);
    int insertTrack(const Track& track, bool& inserted);
    // Consultas
    std::vector<Artist> getArtists();
    std::vector<Album>  getAlbums(int artist_id);
    std::vector<Track>  getTracks(int album_id);
    std::vector<Track> getAllTracks(int limit, int offset);
    int                getTracksCount();   
    std::string         getTrackPath(int track_id);
    std::optional<Track> getTrackById(int track_id);

private:
    SQLite::Database db_;

    void createTables();
};

} // namespace localstream