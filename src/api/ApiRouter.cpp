#include "api/ApiRouter.hpp"
#include <iostream>

namespace localstream {

ApiRouter::ApiRouter(Database& db, crow::SimpleApp& app, LibraryScanner& library_scanner)
    : db_(db), app_(app), library_scanner_(library_scanner)
{
    setupRoutes();
}

crow::json::wvalue ApiRouter::artistToJson(const Artist& artist)
{
    crow::json::wvalue json;
    json["id"]   = artist.id;
    json["name"] = artist.name;
    return json;
}

crow::json::wvalue ApiRouter::albumToJson(const Album& album)
{
    crow::json::wvalue json;
    json["id"]        = album.id;
    json["title"]     = album.title;
    json["artist_id"] = album.artist_id;
    json["year"]      = album.year;
    return json;
}

crow::json::wvalue ApiRouter::trackToJson(const Track& track)
{
    crow::json::wvalue json;
    json["id"]           = track.id;
    json["title"]        = track.title;
    json["artist_id"]    = track.artist_id;
    json["album_id"]     = track.album_id;
    json["duration_s"]   = track.duration_s;
    json["track_number"] = track.track_number;
    json["format"]       = track.format;
    return json;
}

void ApiRouter::setupRoutes()
{
    // Health check
    CROW_ROUTE(app_, "/health")
    ([]{
        crow::json::wvalue json;
        json["status"] = "ok";
        return crow::response(200, json);
    });

    // GET /api/artists
    CROW_ROUTE(app_, "/api/artists")
    ([this]{
        auto artists = db_.getArtists();

        crow::json::wvalue json;
        crow::json::wvalue::list list;

        for (const auto& artist : artists) {
            list.push_back(artistToJson(artist));
        }

        json["artists"] = std::move(list);
        json["count"]   = static_cast<int>(artists.size());
        return crow::response(200, json);
    });

    // GET /api/artists/:id/albums
    CROW_ROUTE(app_, "/api/artists/<int>/albums")
    ([this](int artist_id){
        auto albums = db_.getAlbums(artist_id);

        crow::json::wvalue json;
        crow::json::wvalue::list list;

        for (const auto& album : albums) {
            list.push_back(albumToJson(album));
        }

        json["albums"] = std::move(list);
        json["count"]  = static_cast<int>(albums.size());
        return crow::response(200, json);
    });

    // GET /api/albums/:id/tracks
    CROW_ROUTE(app_, "/api/albums/<int>/tracks")
    ([this](int album_id){
        auto tracks = db_.getTracks(album_id);

        crow::json::wvalue json;
        crow::json::wvalue::list list;

        for (const auto& track : tracks) {
            list.push_back(trackToJson(track));
        }

        json["tracks"] = std::move(list);
        json["count"]  = static_cast<int>(tracks.size());
        return crow::response(200, json);
    });

    // GET /api/tracks/:id
    CROW_ROUTE(app_, "/api/tracks/<int>")
    ([this](int track_id){
        auto track = db_.getTrackById(track_id);

        if (!track) {
            crow::json::wvalue json;
            json["error"] = "Track no encontrado";
            return crow::response(404, json);
        }

        return crow::response(200, trackToJson(*track));
    });

    // POST /api/scan
    CROW_ROUTE(app_, "/api/scan").methods(crow::HTTPMethod::Post)
    ([this]{
        int new_tracks = library_scanner_.scan();

        crow::json::wvalue json;
        json["status"]     = "ok";
        json["new_tracks"] = new_tracks;
        return crow::response(200, json);
    });
}

} // namespace localstream