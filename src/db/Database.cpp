#include "db/Database.hpp"
#include <stdexcept>

namespace localstream {

Database::Database(const std::string& db_path)
    : db_(db_path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
{
    db_.exec("PRAGMA journal_mode=WAL;");
    db_.exec("PRAGMA foreign_keys=ON;");
    createTables();
}

void Database::createTables()
{
    db_.exec(R"(
        CREATE TABLE IF NOT EXISTS artists (
            id   INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE
        );
    )");

    db_.exec(R"(
        CREATE TABLE IF NOT EXISTS albums (
            id        INTEGER PRIMARY KEY AUTOINCREMENT,
            title     TEXT NOT NULL,
            artist_id INTEGER NOT NULL,
            year      INTEGER,
            FOREIGN KEY (artist_id) REFERENCES artists(id),
            UNIQUE(title, artist_id)
        );
    )");

    db_.exec(R"(
        CREATE TABLE IF NOT EXISTS tracks (
            id           INTEGER PRIMARY KEY AUTOINCREMENT,
            title        TEXT NOT NULL,
            artist_id    INTEGER NOT NULL,
            album_id     INTEGER NOT NULL,
            file_path    TEXT NOT NULL UNIQUE,
            duration_s   INTEGER,
            track_number INTEGER,
            file_size    INTEGER,
            format       TEXT,
            FOREIGN KEY (artist_id) REFERENCES artists(id),
            FOREIGN KEY (album_id)  REFERENCES albums(id)
        );
    )");
}

int Database::insertArtist(const std::string& name)
{
    // Si ya existe, retornamos su id
    SQLite::Statement query(db_,
        "SELECT id FROM artists WHERE name = ?");
    query.bind(1, name);

    if (query.executeStep()) {
        return query.getColumn(0).getInt();
    }

    // Si no existe, lo insertamos
    SQLite::Statement insert(db_,
        "INSERT INTO artists (name) VALUES (?)");
    insert.bind(1, name);
    insert.exec();

    return static_cast<int>(db_.getLastInsertRowid());
}

int Database::insertAlbum(const std::string& title, int artist_id, int year)
{
    SQLite::Statement query(db_,
        "SELECT id FROM albums WHERE title = ? AND artist_id = ?");
    query.bind(1, title);
    query.bind(2, artist_id);

    if (query.executeStep()) {
        return query.getColumn(0).getInt();
    }

    SQLite::Statement insert(db_,
        "INSERT INTO albums (title, artist_id, year) VALUES (?, ?, ?)");
    insert.bind(1, title);
    insert.bind(2, artist_id);
    insert.bind(3, year);
    insert.exec();

    return static_cast<int>(db_.getLastInsertRowid());
}

int Database::insertTrack(const Track& track)
{
    // Si el archivo ya está en la DB, lo saltamos
    SQLite::Statement query(db_,
        "SELECT id FROM tracks WHERE file_path = ?");
    query.bind(1, track.file_path);

    if (query.executeStep()) {
        return query.getColumn(0).getInt();
    }

    SQLite::Statement insert(db_, R"(
        INSERT INTO tracks
            (title, artist_id, album_id, file_path,
             duration_s, track_number, file_size, format)
        VALUES
            (?, ?, ?, ?, ?, ?, ?, ?)
    )");

    insert.bind(1, track.title);
    insert.bind(2, track.artist_id);
    insert.bind(3, track.album_id);
    insert.bind(4, track.file_path);
    insert.bind(5, track.duration_s);
    insert.bind(6, track.track_number);
    insert.bind(7, track.file_size);
    insert.bind(8, track.format);
    insert.exec();

    return static_cast<int>(db_.getLastInsertRowid());
}

std::vector<Artist> Database::getArtists()
{
    std::vector<Artist> artists;

    SQLite::Statement query(db_,
        "SELECT id, name FROM artists ORDER BY name");

    while (query.executeStep()) {
        Artist artist;
        artist.id   = query.getColumn(0).getInt();
        artist.name = query.getColumn(1).getString();
        artists.push_back(artist);
    }

    return artists;
}

std::vector<Album> Database::getAlbums(int artist_id)
{
    std::vector<Album> albums;

    SQLite::Statement query(db_,
        "SELECT id, title, artist_id, year FROM albums WHERE artist_id = ? ORDER BY year");
    query.bind(1, artist_id);

    while (query.executeStep()) {
        Album album;
        album.id        = query.getColumn(0).getInt();
        album.title     = query.getColumn(1).getString();
        album.artist_id = query.getColumn(2).getInt();
        album.year      = query.getColumn(3).getInt();
        albums.push_back(album);
    }

    return albums;
}

std::vector<Track> Database::getTracks(int album_id)
{
    std::vector<Track> tracks;

    SQLite::Statement query(db_, R"(
        SELECT id, title, artist_id, album_id,
               file_path, duration_s, track_number, file_size, format
        FROM tracks
        WHERE album_id = ?
        ORDER BY track_number
    )");
    query.bind(1, album_id);

    while (query.executeStep()) {
        Track track;
        track.id           = query.getColumn(0).getInt();
        track.title        = query.getColumn(1).getString();
        track.artist_id    = query.getColumn(2).getInt();
        track.album_id     = query.getColumn(3).getInt();
        track.file_path    = query.getColumn(4).getString();
        track.duration_s   = query.getColumn(5).getInt();
        track.track_number = query.getColumn(6).getInt();
        track.file_size    = query.getColumn(7).getInt();
        track.format       = query.getColumn(8).getString();
        tracks.push_back(track);
    }

    return tracks;
}

std::string Database::getTrackPath(int track_id)
{
    SQLite::Statement query(db_,
        "SELECT file_path FROM tracks WHERE id = ?");
    query.bind(1, track_id);

    if (query.executeStep()) {
        return query.getColumn(0).getString();
    }

    return "";
}

} // namespace localstream