#include "api/ApiRouter.hpp"

namespace localstream {

ApiRouter::ApiRouter(Database& db, crow::App<AuthMiddleware>& app,
                     LibraryScanner& library_scanner)
    : db_(db), app_(app), library_scanner_(library_scanner) {
  setupRoutes();
}

crow::json::wvalue ApiRouter::artistToJson(const Artist& artist) {
  crow::json::wvalue json;
  json["id"] = artist.id;
  json["name"] = artist.name;
  return json;
}

crow::json::wvalue ApiRouter::albumToJson(const Album& album) {
  crow::json::wvalue json;
  json["id"] = album.id;
  json["title"] = album.title;
  json["artist_id"] = album.artist_id;
  json["year"] = album.year;
  return json;
}

crow::json::wvalue ApiRouter::trackToJson(const Track& track) {
  crow::json::wvalue json;
  json["id"] = track.id;
  json["title"] = track.title;
  json["artist_id"] = track.artist_id;
  json["album_id"] = track.album_id;
  json["duration_s"] = track.duration_s;
  json["track_number"] = track.track_number;
  json["format"] = track.format;
  return json;
}

crow::json::wvalue ApiRouter::playlistToJson(const Playlist& playlist) {
  crow::json::wvalue json;
  json["id"] = playlist.id;
  json["name"] = playlist.name;
  json["created_at"] = playlist.created_at;
  return json;
}

void ApiRouter::setupRoutes() {
  CROW_ROUTE(app_, "/health")
  ([] {
    crow::json::wvalue json;
    json["status"] = "ok";
    return crow::response(200, json);
  });

  CROW_ROUTE(app_, "/api/artists")
  ([this] {
    auto artists = db_.getArtists();
    crow::json::wvalue json;
    crow::json::wvalue::list list;
    for (const auto& artist : artists) list.push_back(artistToJson(artist));
    json["artists"] = std::move(list);
    json["count"] = static_cast<int>(artists.size());
    return crow::response(200, json);
  });

  CROW_ROUTE(app_, "/api/artists/<int>/albums")
  ([this](int artist_id) {
    auto albums = db_.getAlbums(artist_id);
    crow::json::wvalue json;
    crow::json::wvalue::list list;
    for (const auto& album : albums) list.push_back(albumToJson(album));
    json["albums"] = std::move(list);
    json["count"] = static_cast<int>(albums.size());
    return crow::response(200, json);
  });

  CROW_ROUTE(app_, "/api/albums/<int>/tracks")
  ([this](int album_id) {
    auto tracks = db_.getTracks(album_id);
    crow::json::wvalue json;
    crow::json::wvalue::list list;
    for (const auto& track : tracks) list.push_back(trackToJson(track));
    json["tracks"] = std::move(list);
    json["count"] = static_cast<int>(tracks.size());
    return crow::response(200, json);
  });

  CROW_ROUTE(app_, "/api/tracks/<int>")
  ([this](int track_id) {
    auto track = db_.getTrackById(track_id);
    if (!track) {
      crow::json::wvalue json;
      json["error"] = "Track no encontrado";
      return crow::response(404, json);
    }
    return crow::response(200, trackToJson(*track));
  });

  CROW_ROUTE(app_, "/api/scan").methods(crow::HTTPMethod::Post)([this] {
    if (library_scanner_.isScanning()) {
      crow::json::wvalue json;
      json["status"] = "already_scanning";
      return crow::response(409, json);
    }
    library_scanner_.scanAsync();
    crow::json::wvalue json;
    json["status"] = "scanning";
    return crow::response(202, json);
  });

  CROW_ROUTE(app_, "/api/tracks")
  ([this](const crow::request& req) {
    int limit = 50;
    int offset = 0;
    auto limit_param = req.url_params.get("limit");
    auto offset_param = req.url_params.get("offset");
    if (limit_param) limit = std::stoi(limit_param);
    if (offset_param) offset = std::stoi(offset_param);
    if (limit < 1) limit = 1;
    if (limit > 200) limit = 200;
    if (offset < 0) offset = 0;

    auto tracks = db_.getAllTracks(limit, offset);
    int total = db_.getTracksCount();

    crow::json::wvalue json;
    crow::json::wvalue::list list;
    for (const auto& track : tracks) list.push_back(trackToJson(track));
    json["tracks"] = std::move(list);
    json["total"] = total;
    json["limit"] = limit;
    json["offset"] = offset;
    return crow::response(200, json);
  });

  CROW_ROUTE(app_, "/api/search")
  ([this](const crow::request& req) {
    auto q = req.url_params.get("q");
    if (!q || std::string(q).empty()) {
      crow::json::wvalue json;
      json["error"] = "Parámetro 'q' requerido";
      return crow::response(400, json);
    }
    std::string query(q);
    auto tracks = db_.searchTracks(query);
    crow::json::wvalue json;
    crow::json::wvalue::list list;
    for (const auto& track : tracks) list.push_back(trackToJson(track));
    json["tracks"] = std::move(list);
    json["count"] = static_cast<int>(tracks.size());
    json["query"] = query;
    return crow::response(200, json);
  });

  CROW_ROUTE(app_, "/api/playlists")
  ([this] {
    auto playlists = db_.getPlaylists();
    crow::json::wvalue json;
    crow::json::wvalue::list list;
    for (const auto& p : playlists) list.push_back(playlistToJson(p));
    json["playlists"] = std::move(list);
    json["count"] = static_cast<int>(playlists.size());
    return crow::response(200, json);
  });

  CROW_ROUTE(app_, "/api/playlists")
      .methods(crow::HTTPMethod::Post)([this](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body || !body.has("name")) {
          crow::json::wvalue json;
          json["error"] = "Campo 'name' requerido";
          return crow::response(400, json);
        }
        std::string name = body["name"].s();
        if (name.empty()) {
          crow::json::wvalue json;
          json["error"] = "El nombre no puede estar vacío";
          return crow::response(400, json);
        }
        int id = db_.createPlaylist(name);
        crow::json::wvalue json;
        json["id"] = id;
        json["name"] = name;
        return crow::response(201, json);
      });

  CROW_ROUTE(app_, "/api/playlists/<int>")
      .methods(crow::HTTPMethod::Delete)([this](int playlist_id) {
        bool deleted = db_.deletePlaylist(playlist_id);
        crow::json::wvalue json;
        if (deleted) {
          json["status"] = "ok";
          return crow::response(200, json);
        }
        json["error"] = "Playlist no encontrada";
        return crow::response(404, json);
      });

  CROW_ROUTE(app_, "/api/playlists/<int>/tracks")
  ([this](int playlist_id) {
    auto tracks = db_.getPlaylistTracks(playlist_id);
    crow::json::wvalue json;
    crow::json::wvalue::list list;
    for (const auto& track : tracks) list.push_back(trackToJson(track));
    json["tracks"] = std::move(list);
    json["count"] = static_cast<int>(tracks.size());
    return crow::response(200, json);
  });

  CROW_ROUTE(app_, "/api/playlists/<int>/tracks")
      .methods(crow::HTTPMethod::Post)(
          [this](const crow::request& req, int playlist_id) {
            auto body = crow::json::load(req.body);
            if (!body || !body.has("track_id")) {
              crow::json::wvalue json;
              json["error"] = "Campo 'track_id' requerido";
              return crow::response(400, json);
            }
            int track_id = body["track_id"].i();
            bool added = db_.addTrackToPlaylist(playlist_id, track_id);
            crow::json::wvalue json;
            json["status"] = added ? "ok" : "already_exists";
            return crow::response(200, json);
          });

  CROW_ROUTE(app_, "/api/playlists/<int>/tracks/<int>")
      .methods(crow::HTTPMethod::Delete)([this](int playlist_id, int track_id) {
        bool removed = db_.removeTrackFromPlaylist(playlist_id, track_id);
        crow::json::wvalue json;
        json["status"] = removed ? "ok" : "not_found";
        return crow::response(200, json);
      });
}

}  // namespace localstream