#include "db/Database.hpp"
#include <stdexcept>
#include <filesystem>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/id3v2tag.h>
#include <taglib/mpegfile.h>
#include <taglib/flacfile.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/mp4file.h>
#include <taglib/mp4tag.h>
#include <taglib/mp4coverart.h>
#include <taglib/vorbisfile.h>
#include <taglib/opusfile.h>

namespace localstream {

Database::Database(const std::string& db_path)
    : db_(db_path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
{
    db_.exec("PRAGMA journal_mode=WAL;");
    db_.exec("PRAGMA foreign_keys=ON;");
    createTables();
    migrateCoverCache(); 
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
            id         INTEGER PRIMARY KEY AUTOINCREMENT,
            title      TEXT NOT NULL,
            artist_id  INTEGER NOT NULL,
            year       INTEGER,
            cover_blob BLOB,
            cover_mime TEXT,
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

    db_.exec(R"(
        CREATE TABLE IF NOT EXISTS playlists (
            id         INTEGER PRIMARY KEY AUTOINCREMENT,
            name       TEXT NOT NULL,
            created_at TEXT DEFAULT (datetime('now'))
        );
    )");

    db_.exec(R"(
        CREATE TABLE IF NOT EXISTS playlist_tracks (
            playlist_id INTEGER NOT NULL,
            track_id    INTEGER NOT NULL,
            position    INTEGER NOT NULL DEFAULT 0,
            PRIMARY KEY (playlist_id, track_id),
            FOREIGN KEY (playlist_id) REFERENCES playlists(id) ON DELETE CASCADE,
            FOREIGN KEY (track_id)    REFERENCES tracks(id)
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

int Database::insertTrack(const Track& track, bool& inserted)
{
    SQLite::Statement query(db_,
        "SELECT id FROM tracks WHERE file_path = ?");
    query.bind(1, track.file_path);

    if (query.executeStep()) {
        inserted = false;
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

    inserted = true;
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

std::optional<Artist> Database::getArtistById(int artist_id)
{
    SQLite::Statement q(db_,
        "SELECT id, name FROM artists WHERE id = ?");
    q.bind(1, artist_id);
    if (!q.executeStep()) return std::nullopt;

    Artist artist;
    artist.id   = q.getColumn(0).getInt();
    artist.name = q.getColumn(1).getString();
    return artist;
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

std::optional<Album> Database::getAlbumById(int album_id)
{
    SQLite::Statement q(db_,
        "SELECT id, title, artist_id, year FROM albums WHERE id = ?");
    q.bind(1, album_id);

    if (!q.executeStep()) return std::nullopt;

    Album album;
    album.id        = q.getColumn(0).getInt();
    album.title     = q.getColumn(1).getString();
    album.artist_id = q.getColumn(2).getInt();
    album.year      = q.getColumn(3).getInt();
    return album;
}

std::vector<Album> Database::getAlbumList(
    const std::string& type, int size, int offset)
{
    std::string order;

    if (type == "alphabeticalByName")
        order = "al.title COLLATE NOCASE ASC";
    else if (type == "alphabeticalByArtist")
        order = "ar.name COLLATE NOCASE ASC, al.title COLLATE NOCASE ASC";
    else if (type == "newest")
        order = "al.id DESC";
    else if (type == "random")
        order = "RANDOM()";
    else
        order = "al.title COLLATE NOCASE ASC"; // fallback

    std::string sql = R"(
        SELECT al.id, al.title, al.artist_id, al.year
        FROM albums al
        JOIN artists ar ON al.artist_id = ar.id
        ORDER BY )" + order + R"(
        LIMIT ? OFFSET ?
    )";

    SQLite::Statement q(db_, sql);
    q.bind(1, size);
    q.bind(2, offset);

    std::vector<Album> albums;
    while (q.executeStep()) {
        Album album;
        album.id        = q.getColumn(0).getInt();
        album.title     = q.getColumn(1).getString();
        album.artist_id = q.getColumn(2).getInt();
        album.year      = q.getColumn(3).getInt();
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

std::optional<Track> Database::getTrackById(int track_id)
{
    SQLite::Statement query(db_, R"(
        SELECT id, title, artist_id, album_id,
               file_path, duration_s, track_number, file_size, format
        FROM tracks WHERE id = ?
    )");
    query.bind(1, track_id);

    if (!query.executeStep()) {
        return std::nullopt;
    }

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

    return track;
}

std::vector<Track> Database::getAllTracks(int limit, int offset)
{
    std::vector<Track> tracks;

    SQLite::Statement query(db_, R"(
        SELECT id, title, artist_id, album_id,
               file_path, duration_s, track_number, file_size, format
        FROM tracks
        ORDER BY artist_id, album_id, track_number
        LIMIT ? OFFSET ?
    )");
    query.bind(1, limit);
    query.bind(2, offset);

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

int Database::getTracksCount()
{
    SQLite::Statement query(db_, "SELECT COUNT(*) FROM tracks");
    query.executeStep();
    return query.getColumn(0).getInt();
}

std::vector<Track> Database::searchTracks(const std::string& query)
{
    std::vector<Track> tracks;

    SQLite::Statement stmt(db_, R"(
        SELECT t.id, t.title, t.artist_id, t.album_id,
               t.file_path, t.duration_s, t.track_number, t.file_size, t.format
        FROM tracks t
        JOIN artists ar ON t.artist_id = ar.id
        JOIN albums  al ON t.album_id  = al.id
        WHERE t.title   LIKE ? COLLATE NOCASE
           OR ar.name   LIKE ? COLLATE NOCASE
           OR al.title  LIKE ? COLLATE NOCASE
        ORDER BY ar.name, al.title, t.track_number
        LIMIT 100
    )");

    std::string pattern = "%" + query + "%";
    stmt.bind(1, pattern);
    stmt.bind(2, pattern);
    stmt.bind(3, pattern);

    while (stmt.executeStep()) {
        Track track;
        track.id           = stmt.getColumn(0).getInt();
        track.title        = stmt.getColumn(1).getString();
        track.artist_id    = stmt.getColumn(2).getInt();
        track.album_id     = stmt.getColumn(3).getInt();
        track.file_path    = stmt.getColumn(4).getString();
        track.duration_s   = stmt.getColumn(5).getInt();
        track.track_number = stmt.getColumn(6).getInt();
        track.file_size    = stmt.getColumn(7).getInt();
        track.format       = stmt.getColumn(8).getString();
        tracks.push_back(track);
    }

    return tracks;
}

// ─────────────────────────────────────────────────────────────────────────────

void Database::migrateCoverCache()
{
    // Columna cover_blob y cover_mime en albums — se agrega solo si no existe
    try {
        db_.exec(R"(
            ALTER TABLE albums ADD COLUMN cover_blob BLOB;
        )");
    } catch (...) {}

    try {
        db_.exec(R"(
            ALTER TABLE albums ADD COLUMN cover_mime TEXT;
        )");
    } catch (...) {}
}

std::string Database::getAnyTrackPath(int album_id)
{
    SQLite::Statement q(db_,
        "SELECT file_path FROM tracks WHERE album_id = ? LIMIT 1");
    q.bind(1, album_id);
    if (q.executeStep())
        return q.getColumn(0).getString();
    return {};
}

// Lee la portada embebida usando TagLib.
// Soporta: MP3 (ID3v2), FLAC, OGG Vorbis, MP4/M4A.
static std::optional<localstream::CoverData> extractCoverFromFile(
    const std::string& path)
{
    using namespace TagLib;

    // ── MP3 / ID3v2 ──────────────────────────────────────────────────────────
    {
        MPEG::File f(path.c_str());
        if (f.isValid() && f.ID3v2Tag()) {
            auto& frames = f.ID3v2Tag()->frameListMap()["APIC"];
            if (!frames.isEmpty()) {
                auto* frame = dynamic_cast<ID3v2::AttachedPictureFrame*>(
                    frames.front());
                if (frame) {
                    const auto& pic = frame->picture();
                    localstream::CoverData cd;
                    cd.data.assign(
                        reinterpret_cast<const unsigned char*>(pic.data()),
                        reinterpret_cast<const unsigned char*>(pic.data()) + pic.size());
                    cd.mime_type = frame->mimeType().toCString();
                    if (cd.mime_type.empty()) cd.mime_type = "image/jpeg";
                    return cd;
                }
            }
        }
    }

    // ── FLAC ─────────────────────────────────────────────────────────────────
    {
        FLAC::File f(path.c_str());
        if (f.isValid()) {
            const auto& pics = f.pictureList();
            if (!pics.isEmpty()) {
                auto* pic = pics.front();
                localstream::CoverData cd;
                const auto& data = pic->data();
                cd.data.assign(
                    reinterpret_cast<const unsigned char*>(data.data()),
                    reinterpret_cast<const unsigned char*>(data.data()) + data.size());
                cd.mime_type = pic->mimeType().toCString(true);
                if (cd.mime_type.empty()) cd.mime_type = "image/jpeg";
                return cd;
            }
        }
    }

    // ── MP4 / M4A ────────────────────────────────────────────────────────────
    {
        MP4::File f(path.c_str());
        if (f.isValid() && f.tag()) {
            auto& items = f.tag()->itemMap();
            if (items.contains("covr")) {
                const auto& covers = items["covr"].toCoverArtList();
                if (!covers.isEmpty()) {
                    const auto& cover = covers.front();
                    localstream::CoverData cd;
                    const auto& data = cover.data();
                    cd.data.assign(
                        reinterpret_cast<const unsigned char*>(data.data()),
                        reinterpret_cast<const unsigned char*>(data.data()) + data.size());
                    cd.mime_type = (cover.format() == MP4::CoverArt::PNG)
                                   ? "image/png" : "image/jpeg";
                    return cd;
                }
            }
        }
    }

        // ── OGG Vorbis ───────────────────────────────────────────────────────────────
    {
        Ogg::Vorbis::File f(path.c_str());
        if (f.isValid() && f.tag()) {
            const auto& pics = f.tag()->pictureList();
            if (!pics.isEmpty()) {
                auto* pic = pics.front();
                localstream::CoverData cd;
                const auto& data = pic->data();
                cd.data.assign(
                    reinterpret_cast<const unsigned char*>(data.data()),
                    reinterpret_cast<const unsigned char*>(data.data()) + data.size());
                cd.mime_type = pic->mimeType().toCString(true);
                if (cd.mime_type.empty()) cd.mime_type = "image/jpeg";
                return cd;
            }
        }
    }

    // ── OGG Opus ─────────────────────────────────────────────────────────────────
    {
        TagLib::Ogg::Opus::File f(path.c_str());
        if (f.isValid() && f.tag()) {
            const auto& pics = f.tag()->pictureList();
            if (!pics.isEmpty()) {
                auto* pic = pics.front();
                localstream::CoverData cd;
                const auto& data = pic->data();
                cd.data.assign(
                    reinterpret_cast<const unsigned char*>(data.data()),
                    reinterpret_cast<const unsigned char*>(data.data()) + data.size());
                cd.mime_type = pic->mimeType().toCString(true);
                if (cd.mime_type.empty()) cd.mime_type = "image/jpeg";
                return cd;
            }
        }
    }

    return std::nullopt;
}

std::optional<localstream::CoverData> Database::getAlbumCover(int album_id)
{
    // 1. Intentar desde caché en la DB
    {
        SQLite::Statement q(db_,
            "SELECT cover_blob, cover_mime FROM albums WHERE id = ?");
        q.bind(1, album_id);
        if (q.executeStep()) {
            auto col_blob = q.getColumn(0);
            auto col_mime = q.getColumn(1);
            if (!col_blob.isNull() && col_blob.getBytes() > 0) {
                CoverData cd;
                const void* ptr  = col_blob.getBlob();
                int          sz   = col_blob.getBytes();
                cd.data.assign(
                    static_cast<const unsigned char*>(ptr),
                    static_cast<const unsigned char*>(ptr) + sz);
                cd.mime_type = col_mime.isNull()
                               ? "image/jpeg"
                               : col_mime.getString();
                return cd;
            }
        }
    }

    // 2. Leer desde el archivo de audio
    const std::string path = getAnyTrackPath(album_id);
    if (path.empty()) return std::nullopt;

    auto cd = extractCoverFromFile(path);
    if (!cd) return std::nullopt;

    // 3. Guardar en caché
    try {
        SQLite::Statement upd(db_,
            "UPDATE albums SET cover_blob = ?, cover_mime = ? WHERE id = ?");
        upd.bind(1, cd->data.data(), static_cast<int>(cd->data.size()));
        upd.bind(2, cd->mime_type);
        upd.bind(3, album_id);
        upd.exec();
    } catch (...) {}

    return cd;
}

std::vector<Playlist> Database::getPlaylists()
{
    std::vector<Playlist> playlists;
    SQLite::Statement query(db_,
        "SELECT id, name, created_at FROM playlists ORDER BY created_at DESC");

    while (query.executeStep()) {
        Playlist p;
        p.id         = query.getColumn(0).getInt();
        p.name       = query.getColumn(1).getString();
        p.created_at = query.getColumn(2).getString();
        playlists.push_back(p);
    }
    return playlists;
}

int Database::createPlaylist(const std::string& name)
{
    SQLite::Statement insert(db_,
        "INSERT INTO playlists (name) VALUES (?)");
    insert.bind(1, name);
    insert.exec();
    return static_cast<int>(db_.getLastInsertRowid());
}

bool Database::deletePlaylist(int playlist_id)
{
    SQLite::Statement stmt(db_,
        "DELETE FROM playlists WHERE id = ?");
    stmt.bind(1, playlist_id);
    stmt.exec();
    return db_.getChanges() > 0;
}

bool Database::addTrackToPlaylist(int playlist_id, int track_id)
{
    // Calculamos la siguiente posición
    SQLite::Statement pos(db_,
        "SELECT COALESCE(MAX(position), -1) + 1 FROM playlist_tracks WHERE playlist_id = ?");
    pos.bind(1, playlist_id);
    pos.executeStep();
    int next_pos = pos.getColumn(0).getInt();

    SQLite::Statement insert(db_, R"(
        INSERT OR IGNORE INTO playlist_tracks (playlist_id, track_id, position)
        VALUES (?, ?, ?)
    )");
    insert.bind(1, playlist_id);
    insert.bind(2, track_id);
    insert.bind(3, next_pos);
    insert.exec();
    return db_.getChanges() > 0;
}

bool Database::removeTrackFromPlaylist(int playlist_id, int track_id)
{
    SQLite::Statement stmt(db_, R"(
        DELETE FROM playlist_tracks
        WHERE playlist_id = ? AND track_id = ?
    )");
    stmt.bind(1, playlist_id);
    stmt.bind(2, track_id);
    stmt.exec();
    return db_.getChanges() > 0;
}

std::vector<Track> Database::getPlaylistTracks(int playlist_id)
{
    std::vector<Track> tracks;

    SQLite::Statement query(db_, R"(
        SELECT t.id, t.title, t.artist_id, t.album_id,
               t.file_path, t.duration_s, t.track_number, t.file_size, t.format
        FROM tracks t
        JOIN playlist_tracks pt ON t.id = pt.track_id
        WHERE pt.playlist_id = ?
        ORDER BY pt.position
    )");
    query.bind(1, playlist_id);

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

int Database::removeNonExistentTracks()
{
    // Traemos todos los file_paths de la DB
    SQLite::Statement query(db_, "SELECT id, file_path FROM tracks");

    std::vector<int> to_delete;
    while (query.executeStep()) {
        int         id   = query.getColumn(0).getInt();
        std::string path = query.getColumn(1).getString();

        if (!std::filesystem::exists(path)) {
            to_delete.push_back(id);
        }
    }

    // Eliminamos los que no existen
    for (int id : to_delete) {
        SQLite::Statement del(db_, "DELETE FROM tracks WHERE id = ?");
        del.bind(1, id);
        del.exec();
    }

    return static_cast<int>(to_delete.size());
}

int Database::removeEmptyAlbums()
{
    SQLite::Statement stmt(db_, R"(
        DELETE FROM albums
        WHERE id NOT IN (SELECT DISTINCT album_id FROM tracks)
    )");
    stmt.exec();
    return db_.getChanges();
}

int Database::removeEmptyArtists()
{
    SQLite::Statement stmt(db_, R"(
        DELETE FROM artists
        WHERE id NOT IN (SELECT DISTINCT artist_id FROM tracks)
    )");
    stmt.exec();
    return db_.getChanges();
}

} // namespace localstream