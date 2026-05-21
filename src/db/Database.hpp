#pragma once

#include <SQLiteCpp/SQLiteCpp.h>

#include <optional>
#include <string>
#include <vector>

#include "models.hpp"

namespace localstream {

struct CoverData {
  std::vector<unsigned char> data;
  std::string mime_type;  // "image/jpeg", "image/png", etc.
};

class Database {
 public:
  explicit Database(const std::string& db_path);

  // Inserción — retornan el id del registro (nuevo o existente)
  int insertArtist(const std::string& name);
  int insertAlbum(const std::string& title, int artist_id, int year);
  int insertTrack(const Track& track, bool& inserted);

  // Limpieza de biblioteca
  int removeNonExistentTracks();
  int removeEmptyAlbums();
  int removeEmptyArtists();

  // Consultas
  std::vector<Artist> getArtists();
  std::optional<Artist> getArtistById(int artist_id);
  std::vector<Album> getAlbums(int artist_id);
  std::optional<Album> getAlbumById(int album_id);
  std::vector<Album> getAlbumList(const std::string& type, int size,
                                  int offset);
  std::vector<Track> getTracks(int album_id);
  std::vector<Track> getAllTracks(int limit, int offset);
  int getTracksCount();
  std::string getTrackPath(int track_id);
  std::optional<Track> getTrackById(int track_id);
  std::vector<Track> searchTracks(const std::string& query);

  // Portadas
  std::optional<CoverData> getAlbumCover(int album_id);

  // Playlists
  std::vector<Playlist> getPlaylists();
  int createPlaylist(const std::string& name);
  bool deletePlaylist(int playlist_id);
  bool addTrackToPlaylist(int playlist_id, int track_id);
  bool removeTrackFromPlaylist(int playlist_id, int track_id);
  std::vector<Track> getPlaylistTracks(int playlist_id);

 private:
  SQLite::Database db_;

  void createTables();
  void migrateCoverCache();

  // Devuelve el file_path de cualquier track del álbum
  std::string getAnyTrackPath(int album_id);
};

}  // namespace localstream