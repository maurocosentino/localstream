#pragma once

namespace localstream {

// Wrapper estándar de respuesta Subsonic
template<typename App>
crow::response SubsonicRouter<App>::okResponse(crow::json::wvalue& data)
{
    crow::json::wvalue root;
    root["subsonic-response"]["status"]  = "ok";
    root["subsonic-response"]["version"] = "1.16.1";
    root["subsonic-response"]["type"]    = "localstream";

    // Mover el contenido de data dentro de subsonic-response
    // Crow no tiene merge, así que el caller pone los datos directamente
    return crow::response(200, root);
}

template<typename App>
crow::response SubsonicRouter<App>::errorResponse(int code, const std::string& message)
{
    crow::json::wvalue root;
    root["subsonic-response"]["status"]        = "failed";
    root["subsonic-response"]["version"]       = "1.16.1";
    root["subsonic-response"]["type"]          = "localstream";
    root["subsonic-response"]["error"]["code"] = code;
    root["subsonic-response"]["error"]["message"] = message;
    return crow::response(200, root); // Subsonic siempre retorna 200
}

template<typename App>
crow::json::wvalue SubsonicRouter<App>::artistToSubsonic(const Artist& artist)
{
    crow::json::wvalue j;
    j["id"]         = std::to_string(artist.id);
    j["name"]       = artist.name;
    j["albumCount"] = 0; // se llena en getArtists
    return j;
}

template<typename App>
crow::json::wvalue SubsonicRouter<App>::albumToSubsonic(const Album& album)
{
    crow::json::wvalue j;
    j["id"]       = std::to_string(album.id);
    j["name"]     = album.title;
    j["title"]    = album.title;
    j["artistId"] = std::to_string(album.artist_id);
    j["year"]     = album.year;
    j["coverArt"] = "al-" + std::to_string(album.id);
    return j;
}

template<typename App>
crow::json::wvalue SubsonicRouter<App>::trackToSubsonic(const Track& track)
{
    crow::json::wvalue j;
    j["id"]          = std::to_string(track.id);
    j["title"]       = track.title;
    j["artistId"]    = std::to_string(track.artist_id);
    j["albumId"]     = std::to_string(track.album_id);
    j["duration"]    = track.duration_s;
    j["track"]       = track.track_number;
    j["contentType"] = track.format == "mp3" ? "audio/mpeg" : "audio/flac";
    j["suffix"]      = track.format;
    j["size"]        = track.file_size;
    j["coverArt"]    = "al-" + std::to_string(track.album_id);
    j["isVideo"]     = false;
    j["type"]        = "music";
    return j;
}

template<typename App>
void SubsonicRouter<App>::setupRoutes()
{
    // ── ping ─────────────────────────────────────────────────────────────────
    // Todas las apps lo usan para verificar conectividad
    CROW_ROUTE(app_, "/rest/ping")
    ([this](const crow::request&){
        crow::json::wvalue root;
        root["subsonic-response"]["status"]  = "ok";
        root["subsonic-response"]["version"] = "1.16.1";
        root["subsonic-response"]["type"]    = "localstream";
        return crow::response(200, root);
    });

    // ── getArtists ───────────────────────────────────────────────────────────
    // Retorna todos los artistas agrupados por letra inicial
    CROW_ROUTE(app_, "/rest/getArtists")
    ([this](const crow::request&){
        auto artists = db_.getArtists();

        // Agrupar por letra inicial
        std::map<std::string, std::vector<crow::json::wvalue>> index_map;
        for (const auto& artist : artists) {
            std::string letter = artist.name.empty()
                ? "#"
                : std::string(1, std::toupper(artist.name[0]));
            if (!std::isalpha(letter[0])) letter = "#";

            auto albums = db_.getAlbums(artist.id);
            auto j = artistToSubsonic(artist);
            j["albumCount"] = static_cast<int>(albums.size());
            index_map[letter].push_back(std::move(j));
        }

        crow::json::wvalue::list index_list;
        for (auto& [letter, artist_list] : index_map) {
            crow::json::wvalue idx;
            idx["name"]   = letter;
            idx["artist"] = std::move(artist_list);
            index_list.push_back(std::move(idx));
        }

        crow::json::wvalue root;
        root["subsonic-response"]["status"]                    = "ok";
        root["subsonic-response"]["version"]                   = "1.16.1";
        root["subsonic-response"]["type"]                      = "localstream";
        root["subsonic-response"]["artists"]["ignoredArticles"] = "The An A Die Das Ein";
        root["subsonic-response"]["artists"]["index"]          = std::move(index_list);
        return crow::response(200, root);
    });

    // ── getArtist ────────────────────────────────────────────────────────────
    // Retorna un artista con sus álbumes
    CROW_ROUTE(app_, "/rest/getArtist")
    ([this](const crow::request& req){
        auto id_param = req.url_params.get("id");
        if (!id_param) return errorResponse(10, "Missing parameter: id");

        int artist_id = std::stoi(id_param);
        auto albums   = db_.getAlbums(artist_id);
        auto artists  = db_.getArtists();

        // Buscar el artista por id
        const Artist* found = nullptr;
        for (const auto& a : artists) {
            if (a.id == artist_id) { found = &a; break; }
        }
        if (!found) return errorResponse(70, "Artist not found");

        crow::json::wvalue::list album_list;
        for (const auto& album : albums)
            album_list.push_back(albumToSubsonic(album));

        crow::json::wvalue root;
        root["subsonic-response"]["status"]              = "ok";
        root["subsonic-response"]["version"]             = "1.16.1";
        root["subsonic-response"]["type"]                = "localstream";
        root["subsonic-response"]["artist"]["id"]        = std::to_string(found->id);
        root["subsonic-response"]["artist"]["name"]      = found->name;
        root["subsonic-response"]["artist"]["albumCount"] = static_cast<int>(albums.size());
        root["subsonic-response"]["artist"]["album"]     = std::move(album_list);
        return crow::response(200, root);
    });

    // ── getAlbum ─────────────────────────────────────────────────────────────
    // Retorna un álbum con sus tracks
    CROW_ROUTE(app_, "/rest/getAlbum")
    ([this](const crow::request& req){
        auto id_param = req.url_params.get("id");
        if (!id_param) return errorResponse(10, "Missing parameter: id");

        int album_id = std::stoi(id_param);
        auto tracks  = db_.getTracks(album_id);
        auto albums  = db_.getAlbums(0); // necesitamos buscar el album

        // Buscar el album — agregamos getAlbumById a la DB después,
        // por ahora lo reconstruimos desde los tracks
        if (tracks.empty()) return errorResponse(70, "Album not found");

        int artist_id = tracks[0].artist_id;
        auto artist_albums = db_.getAlbums(artist_id);

        const Album* found = nullptr;
        for (const auto& al : artist_albums) {
            if (al.id == album_id) { found = &al; break; }
        }

        crow::json::wvalue::list track_list;
        for (const auto& track : tracks)
            track_list.push_back(trackToSubsonic(track));

        crow::json::wvalue root;
        root["subsonic-response"]["status"]  = "ok";
        root["subsonic-response"]["version"] = "1.16.1";
        root["subsonic-response"]["type"]    = "localstream";

        auto& album_node = root["subsonic-response"]["album"];
        album_node["id"]         = std::to_string(album_id);
        album_node["name"]       = found ? found->title : "Unknown";
        album_node["artistId"]   = std::to_string(artist_id);
        album_node["songCount"]  = static_cast<int>(tracks.size());
        album_node["coverArt"]   = "al-" + std::to_string(album_id);
        album_node["song"]       = std::move(track_list);
        return crow::response(200, root);
    });

    // ── getSong ──────────────────────────────────────────────────────────────
    CROW_ROUTE(app_, "/rest/getSong")
    ([this](const crow::request& req){
        auto id_param = req.url_params.get("id");
        if (!id_param) return errorResponse(10, "Missing parameter: id");

        auto track = db_.getTrackById(std::stoi(id_param));
        if (!track) return errorResponse(70, "Song not found");

        crow::json::wvalue root;
        root["subsonic-response"]["status"]  = "ok";
        root["subsonic-response"]["version"] = "1.16.1";
        root["subsonic-response"]["type"]    = "localstream";
        root["subsonic-response"]["song"]    = trackToSubsonic(*track);
        return crow::response(200, root);
    });

    // ── search3 ──────────────────────────────────────────────────────────────
    // Búsqueda unificada que usan casi todas las apps
    CROW_ROUTE(app_, "/rest/search3")
    ([this](const crow::request& req){
        auto q = req.url_params.get("query");
        if (!q) return errorResponse(10, "Missing parameter: query");

        auto tracks = db_.searchTracks(std::string(q));

        crow::json::wvalue::list song_list;
        for (const auto& track : tracks)
            song_list.push_back(trackToSubsonic(track));

        crow::json::wvalue root;
        root["subsonic-response"]["status"]  = "ok";
        root["subsonic-response"]["version"] = "1.16.1";
        root["subsonic-response"]["type"]    = "localstream";
        root["subsonic-response"]["searchResult3"]["song"]   = std::move(song_list);
        root["subsonic-response"]["searchResult3"]["artist"] = crow::json::wvalue::list{};
        root["subsonic-response"]["searchResult3"]["album"]  = crow::json::wvalue::list{};
        return crow::response(200, root);
    });

    // ── stream ───────────────────────────────────────────────────────────────
    // Las apps piden audio por acá — redirigimos a nuestro /stream/:id
    CROW_ROUTE(app_, "/rest/stream")
    ([this](const crow::request& req, crow::response& res){
        auto id_param = req.url_params.get("id");
        if (!id_param) {
            res.code = 400;
            res.end();
            return;
        }
        // Redirect interno — devolvemos el audio directo
        int track_id = std::stoi(id_param);
        std::string file_path = db_.getTrackPath(track_id);
        if (file_path.empty()) { res.code = 404; res.end(); return; }

        auto track = db_.getTrackById(track_id);
        if (!track)            { res.code = 404; res.end(); return; }

        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open())   { res.code = 500; res.end(); return; }

        auto mimeFor = [](const std::string& fmt) -> std::string {
            if (fmt == "mp3")  return "audio/mpeg";
            if (fmt == "flac") return "audio/flac";
            if (fmt == "ogg")  return "audio/ogg";
            if (fmt == "opus") return "audio/ogg; codecs=opus";
            if (fmt == "aac")  return "audio/aac";
            return "application/octet-stream";
        };

        std::string range_header = req.get_header_value("Range");
        long long   file_size    = track->file_size;

        res.set_header("Content-Type",  mimeFor(track->format));
        res.set_header("Accept-Ranges", "bytes");

        if (!range_header.empty() &&
            range_header.substr(0, 6) == "bytes=")
        {
            std::string range = range_header.substr(6);
            auto dash = range.find('-');
            long long start = std::stoll(range.substr(0, dash));
            long long end   = (dash + 1 < range.size() && range[dash+1] != '\0')
                              ? std::stoll(range.substr(dash + 1))
                              : file_size - 1;
            long long length = end - start + 1;

            file.seekg(start);
            std::string buf(length, '\0');
            file.read(buf.data(), length);

            res.code = 206;
            res.set_header("Content-Range",
                "bytes " + std::to_string(start) + "-" +
                           std::to_string(end)   + "/" +
                           std::to_string(file_size));
            res.set_header("Content-Length", std::to_string(length));
            res.write(buf);
        } else {
            res.code = 200;
            res.set_header("Content-Length", std::to_string(file_size));
            constexpr std::size_t CHUNK = 65536;
            std::vector<char> buf(CHUNK);
            while (file) {
                file.read(buf.data(), CHUNK);
                std::streamsize n = file.gcount();
                if (n > 0) res.write(std::string(buf.data(), n));
            }
        }
        res.end();
    });

    // ── getCoverArt ──────────────────────────────────────────────────────────
    // Las apps piden covers con id "al-123"
    CROW_ROUTE(app_, "/rest/getCoverArt")
    ([this](const crow::request& req, crow::response& res){
        auto id_param = req.url_params.get("id");
        if (!id_param) { res.code = 400; res.end(); return; }

        std::string id_str(id_param);

        // Formato: "al-123" para álbumes
        int album_id = 0;
        if (id_str.substr(0, 3) == "al-")
            album_id = std::stoi(id_str.substr(3));
        else
            album_id = std::stoi(id_str); // fallback numérico

        auto cover = db_.getAlbumCover(album_id);
        if (!cover || cover->data.empty()) { res.code = 404; res.end(); return; }

        res.code = 200;
        res.set_header("Content-Type",  cover->mime_type);
        res.set_header("Cache-Control", "public, max-age=86400");
        res.body.assign(
            reinterpret_cast<const char*>(cover->data.data()),
            cover->data.size());
        res.end();
    });

    // ── getPlaylists ─────────────────────────────────────────────────────────
    CROW_ROUTE(app_, "/rest/getPlaylists")
    ([this](const crow::request&){
        auto playlists = db_.getPlaylists();

        crow::json::wvalue::list list;
        for (const auto& p : playlists) {
            crow::json::wvalue j;
            j["id"]        = std::to_string(p.id);
            j["name"]      = p.name;
            j["songCount"] = 0;
            j["duration"]  = 0;
            j["public"]    = false;
            list.push_back(std::move(j));
        }

        crow::json::wvalue root;
        root["subsonic-response"]["status"]              = "ok";
        root["subsonic-response"]["version"]             = "1.16.1";
        root["subsonic-response"]["type"]                = "localstream";
        root["subsonic-response"]["playlists"]["playlist"] = std::move(list);
        return crow::response(200, root);
    });

    // ── getPlaylist ──────────────────────────────────────────────────────────
    CROW_ROUTE(app_, "/rest/getPlaylist")
    ([this](const crow::request& req){
        auto id_param = req.url_params.get("id");
        if (!id_param) return errorResponse(10, "Missing parameter: id");

        int playlist_id = std::stoi(id_param);
        auto tracks     = db_.getPlaylistTracks(playlist_id);
        auto playlists  = db_.getPlaylists();

        const Playlist* found = nullptr;
        for (const auto& p : playlists) {
            if (p.id == playlist_id) { found = &p; break; }
        }
        if (!found) return errorResponse(70, "Playlist not found");

        int total_duration = 0;
        crow::json::wvalue::list song_list;
        for (const auto& track : tracks) {
            total_duration += track.duration_s;
            song_list.push_back(trackToSubsonic(track));
        }

        crow::json::wvalue root;
        root["subsonic-response"]["status"]  = "ok";
        root["subsonic-response"]["version"] = "1.16.1";
        root["subsonic-response"]["type"]    = "localstream";

        auto& pl = root["subsonic-response"]["playlist"];
        pl["id"]        = std::to_string(found->id);
        pl["name"]      = found->name;
        pl["songCount"] = static_cast<int>(tracks.size());
        pl["duration"]  = total_duration;
        pl["public"]    = false;
        pl["entry"]     = std::move(song_list);
        return crow::response(200, root);
    });
}

} // namespace localstream