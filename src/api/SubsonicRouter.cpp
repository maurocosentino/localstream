#include "api/SubsonicRouter.hpp"

#include <openssl/md5.h>

#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>

#include "streaming/Transcoder.hpp"

namespace localstream {

bool SubsonicRouter::validateAuth(const crow::request& req) {
  auto u = req.url_params.get("u");
  auto t = req.url_params.get("t");
  auto s = req.url_params.get("s");
  auto p = req.url_params.get("p");

  if (!u) return false;

  // Autenticación por token MD5: t = MD5(password + salt)
  if (t && s) {
    std::string to_hash = api_key_ + std::string(s);

    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5(reinterpret_cast<const unsigned char*>(to_hash.c_str()), to_hash.size(),
        digest);

    std::ostringstream oss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
      oss << std::hex << std::setw(2) << std::setfill('0')
          << static_cast<int>(digest[i]);

    return oss.str() == std::string(t);
  }

  // Autenticación por password plano: p=password
  if (p) {
    std::string pass(p);
    // Algunos clientes mandan "enc:HEX"
    if (pass.substr(0, 4) == "enc:") {
      std::string hex = pass.substr(4);
      std::string decoded;
      for (size_t i = 0; i < hex.size(); i += 2) {
        decoded += static_cast<char>(std::stoi(hex.substr(i, 2), nullptr, 16));
      }
      return decoded == api_key_;
    }
    return pass == api_key_;
  }

  return false;
}

SubsonicRouter::SubsonicRouter(Database& db, crow::App<AuthMiddleware>& app,
                               const std::string& api_key)
    : db_(db), app_(app), api_key_(api_key) {
  setupRoutes();
}

crow::response SubsonicRouter::okResponse(crow::json::wvalue& data) {
  crow::json::wvalue root;
  root["subsonic-response"]["status"] = "ok";
  root["subsonic-response"]["version"] = "1.16.1";
  root["subsonic-response"]["type"] = "localstream";
  return crow::response(200, root);
}

crow::response SubsonicRouter::errorResponse(int code,
                                             const std::string& message) {
  crow::json::wvalue root;
  root["subsonic-response"]["status"] = "failed";
  root["subsonic-response"]["version"] = "1.16.1";
  root["subsonic-response"]["type"] = "localstream";
  root["subsonic-response"]["error"]["code"] = code;
  root["subsonic-response"]["error"]["message"] = message;
  return crow::response(200, root);
}

crow::json::wvalue SubsonicRouter::artistToSubsonic(const Artist& artist) {
  crow::json::wvalue j;
  j["id"] = std::to_string(artist.id);
  j["name"] = artist.name;
  j["albumCount"] = 0;
  return j;
}

crow::json::wvalue SubsonicRouter::albumToSubsonic(const Album& album) {
  crow::json::wvalue j;
  j["id"] = std::to_string(album.id);
  j["name"] = album.title;
  j["title"] = album.title;
  j["artistId"] = std::to_string(album.artist_id);
  j["year"] = album.year;
  j["coverArt"] = "al-" + std::to_string(album.id);
  return j;
}

crow::json::wvalue SubsonicRouter::trackToSubsonic(const Track& track) {
  crow::json::wvalue j;
  j["id"] = std::to_string(track.id);
  j["title"] = track.title;
  j["artistId"] = std::to_string(track.artist_id);
  j["albumId"] = std::to_string(track.album_id);
  j["duration"] = track.duration_s;
  j["track"] = track.track_number;
  j["contentType"] = track.format == "mp3" ? "audio/mpeg" : "audio/flac";
  j["suffix"] = track.format;
  j["size"] = track.file_size;
  j["coverArt"] = "al-" + std::to_string(track.album_id);
  j["isVideo"] = false;
  j["type"] = "music";

  // Campos nuevos — nombres para la vista de biblioteca
  auto artist = db_.getArtistById(track.artist_id);
  if (artist) j["artist"] = artist->name;

  auto album = db_.getAlbumById(track.album_id);
  if (album) {
    j["album"] = album->title;
    if (album->year > 0) j["year"] = album->year;
  }

  return j;
}

void SubsonicRouter::setupRoutes() {
  // ── ping ──────────────────────────────────────────────────────────────────
  CROW_ROUTE(app_, "/rest/ping")
  ([this](const crow::request&) {
    crow::json::wvalue root;

    root["subsonic-response"]["status"] = "ok";
    root["subsonic-response"]["version"] = "1.16.1";
    root["subsonic-response"]["type"] = "localstream";

    // IMPORTANTE
    root["subsonic-response"]["serverVersion"] = "0.1.0";
    root["subsonic-response"]["openSubsonic"] = true;

    crow::json::wvalue::list extensions;

    crow::json::wvalue ext;
    ext["name"] = "transcodeOffset";
    ext["versions"] = "1";

    extensions.push_back(std::move(ext));

    root["subsonic-response"]["extensions"] = std::move(extensions);

    return crow::response(200, root);
  });

  CROW_ROUTE(app_, "/rest/ping.view")
  ([this](const crow::request&) {
    crow::json::wvalue root;

    root["subsonic-response"]["status"] = "ok";
    root["subsonic-response"]["version"] = "1.16.1";
    root["subsonic-response"]["type"] = "localstream";

    root["subsonic-response"]["serverVersion"] = "0.1.0";
    root["subsonic-response"]["openSubsonic"] = true;

    crow::json::wvalue::list extensions;

    crow::json::wvalue ext;
    ext["name"] = "transcodeOffset";
    ext["versions"] = "1";

    extensions.push_back(std::move(ext));

    root["subsonic-response"]["extensions"] = std::move(extensions);

    return crow::response(200, root);
  });

  // ── getArtists ────────────────────────────────────────────────────────────
  auto getArtists_handler = [this](const crow::request& req) {
    std::cout << "REQUEST: " << req.url << std::endl;

    auto artists = db_.getArtists();

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
      idx["name"] = letter;
      idx["artist"] = std::move(artist_list);
      index_list.push_back(std::move(idx));
    }

    crow::json::wvalue root;
    root["subsonic-response"]["status"] = "ok";
    root["subsonic-response"]["version"] = "1.16.1";
    root["subsonic-response"]["type"] = "localstream";
    root["subsonic-response"]["artists"]["ignoredArticles"] =
        "The An A Die Das Ein";
    root["subsonic-response"]["artists"]["index"] = std::move(index_list);

    return crow::response(200, root);
  };

  CROW_ROUTE(app_, "/rest/getArtists")(getArtists_handler);
  CROW_ROUTE(app_, "/rest/getArtists.view")(getArtists_handler);

  // ── getIndexes ────────────────────────────────────────────────────────────
  auto getIndexes_handler = [this](const crow::request& req) {
    std::cout << "REQUEST: " << req.url << std::endl;

    auto artists = db_.getArtists();

    std::map<std::string, std::vector<crow::json::wvalue>> index_map;

    for (const auto& artist : artists) {
      std::string letter = artist.name.empty()
                               ? "#"
                               : std::string(1, std::toupper(artist.name[0]));

      if (!std::isalpha(letter[0])) letter = "#";

      auto j = artistToSubsonic(artist);

      auto albums = db_.getAlbums(artist.id);
      j["albumCount"] = static_cast<int>(albums.size());

      index_map[letter].push_back(std::move(j));
    }

    crow::json::wvalue::list index_list;

    for (auto& [letter, artist_list] : index_map) {
      crow::json::wvalue idx;
      idx["name"] = letter;
      idx["artist"] = std::move(artist_list);
      index_list.push_back(std::move(idx));
    }

    crow::json::wvalue root;
    root["subsonic-response"]["status"] = "ok";
    root["subsonic-response"]["version"] = "1.16.1";
    root["subsonic-response"]["type"] = "localstream";
    root["subsonic-response"]["indexes"]["ignoredArticles"] =
        "The An A Die Das Ein";
    root["subsonic-response"]["indexes"]["lastModified"] = 0;
    root["subsonic-response"]["indexes"]["index"] = std::move(index_list);

    return crow::response(200, root);
  };

  CROW_ROUTE(app_, "/rest/getIndexes")(getIndexes_handler);
  CROW_ROUTE(app_, "/rest/getIndexes.view")(getIndexes_handler);

  // ── getArtist ─────────────────────────────────────────────────────────────
  auto getArtist_handler = [this](const crow::request& req) {
    std::cout << "REQUEST: " << req.url << std::endl;

    auto id_param = req.url_params.get("id");

    if (!id_param) return errorResponse(10, "Missing parameter: id");

    int artist_id = std::stoi(id_param);

    auto albums = db_.getAlbums(artist_id);
    auto artists = db_.getArtists();

    const Artist* found = nullptr;

    for (const auto& a : artists) {
      if (a.id == artist_id) {
        found = &a;
        break;
      }
    }

    if (!found) return errorResponse(70, "Artist not found");

    crow::json::wvalue::list album_list;

    for (const auto& album : albums)
      album_list.push_back(albumToSubsonic(album));

    crow::json::wvalue root;

    root["subsonic-response"]["status"] = "ok";
    root["subsonic-response"]["version"] = "1.16.1";
    root["subsonic-response"]["type"] = "localstream";
    root["subsonic-response"]["artist"]["id"] = std::to_string(found->id);
    root["subsonic-response"]["artist"]["name"] = found->name;
    root["subsonic-response"]["artist"]["albumCount"] =
        static_cast<int>(albums.size());
    root["subsonic-response"]["artist"]["album"] = std::move(album_list);

    return crow::response(200, root);
  };

  CROW_ROUTE(app_, "/rest/getArtist")(getArtist_handler);
  CROW_ROUTE(app_, "/rest/getArtist.view")(getArtist_handler);

  // ── getAlbum ──────────────────────────────────────────────────────────────
  auto getAlbumList_handler = [this](const crow::request& req) {
    auto type_param = req.url_params.get("type");
    auto size_param = req.url_params.get("size");
    auto offset_param = req.url_params.get("offset");
    std::string type =
        type_param ? std::string(type_param) : "alphabeticalByName";
    int size = size_param ? std::stoi(size_param) : 10;
    int offset = offset_param ? std::stoi(offset_param) : 0;
    if (size > 500) size = 500;
    if (size < 1) size = 1;
    if (offset < 0) offset = 0;

    auto albums = db_.getAlbumList(type, size, offset);

    crow::json::wvalue::list list;
    for (const auto& album : albums) {
      auto j = albumToSubsonic(album);
      auto artist = db_.getArtistById(album.artist_id);
      if (artist) j["artist"] = artist->name;
      list.push_back(std::move(j));
    }

    crow::json::wvalue root;
    root["subsonic-response"]["status"] = "ok";
    root["subsonic-response"]["version"] = "1.16.1";
    root["subsonic-response"]["type"] = "localstream";
    root["subsonic-response"]["albumList"]["album"] = std::move(list);
    return crow::response(200, root);
  };
  CROW_ROUTE(app_, "/rest/getAlbumList")(getAlbumList_handler);
  CROW_ROUTE(app_, "/rest/getAlbumList.view")(getAlbumList_handler);

  // ── getAlbumList2 ─────────────────────────────────────────────────────────
  auto getAlbumList2_handler = [this](const crow::request& req) {
    auto type_param = req.url_params.get("type");
    auto size_param = req.url_params.get("size");
    auto offset_param = req.url_params.get("offset");
    std::string type =
        type_param ? std::string(type_param) : "alphabeticalByName";
    int size = size_param ? std::stoi(size_param) : 10;
    int offset = offset_param ? std::stoi(offset_param) : 0;
    if (size > 500) size = 500;
    if (size < 1) size = 1;
    if (offset < 0) offset = 0;
    auto albums = db_.getAlbumList(type, size, offset);
    crow::json::wvalue::list list;
    for (const auto& album : albums) {
      auto j = albumToSubsonic(album);
      auto artist = db_.getArtistById(album.artist_id);
      if (artist) j["artist"] = artist->name;
      list.push_back(std::move(j));
    }
    crow::json::wvalue root;
    root["subsonic-response"]["status"] = "ok";
    root["subsonic-response"]["version"] = "1.16.1";
    root["subsonic-response"]["type"] = "localstream";
    root["subsonic-response"]["albumList2"]["album"] = std::move(list);
    return crow::response(200, root);
  };
  CROW_ROUTE(app_, "/rest/getAlbumList2")(getAlbumList2_handler);
  CROW_ROUTE(app_, "/rest/getAlbumList2.view")(getAlbumList2_handler);

  // ── getSong ───────────────────────────────────────────────────────────────
  auto getSong_handler = [this](const crow::request& req) {
    auto id_param = req.url_params.get("id");
    if (!id_param) return errorResponse(10, "Missing parameter: id");
    auto track = db_.getTrackById(std::stoi(id_param));
    if (!track) return errorResponse(70, "Song not found");
    crow::json::wvalue root;
    root["subsonic-response"]["status"] = "ok";
    root["subsonic-response"]["version"] = "1.16.1";
    root["subsonic-response"]["type"] = "localstream";
    root["subsonic-response"]["song"] = trackToSubsonic(*track);
    return crow::response(200, root);
  };
  CROW_ROUTE(app_, "/rest/getSong")(getSong_handler);
  CROW_ROUTE(app_, "/rest/getSong.view")(getSong_handler);

  // ── search3 ───────────────────────────────────────────────────────────────
  auto search3_handler = [this](const crow::request& req) {
    auto q = req.url_params.get("query");
    std::string query = "";
    if (q) {
      query = std::string(q);
      if (query == "\"\"") query = "";
      if (query == "(null)") query = "";
    }

    // ── Parámetros de paginación por tipo ────────────────────────────────
    auto songCount_p = req.url_params.get("songCount");
    auto songOffset_p = req.url_params.get("songOffset");
    auto albumCount_p = req.url_params.get("albumCount");
    auto albumOffset_p = req.url_params.get("albumOffset");
    auto artistCount_p = req.url_params.get("artistCount");
    auto artistOffset_p = req.url_params.get("artistOffset");

    int songCount = songCount_p ? std::stoi(songCount_p) : 20;
    int songOffset = songOffset_p ? std::stoi(songOffset_p) : 0;
    int albumCount = albumCount_p ? std::stoi(albumCount_p) : 20;
    int albumOffset = albumOffset_p ? std::stoi(albumOffset_p) : 0;
    int artistCount = artistCount_p ? std::stoi(artistCount_p) : 20;
    int artistOffset = artistOffset_p ? std::stoi(artistOffset_p) : 0;

    // ── Songs ─────────────────────────────────────────────────────────────
    crow::json::wvalue::list song_list;
    if (songCount > 0) {
      std::vector<Track> tracks;
      if (query.empty())
        tracks = db_.getAllTracks(songCount, songOffset);
      else
        tracks = db_.searchTracks(query);

      for (const auto& track : tracks)
        song_list.push_back(trackToSubsonic(track));
    }

    // ── Artists ───────────────────────────────────────────────────────────
    crow::json::wvalue::list artist_list;
    if (artistCount > 0) {
      auto artists = db_.getArtists();
      int count = 0;
      int skipped = 0;
      for (const auto& artist : artists) {
        if (skipped < artistOffset) {
          skipped++;
          continue;
        }
        if (count >= artistCount) break;
        crow::json::wvalue j;
        j["id"] = std::to_string(artist.id);
        j["name"] = artist.name;
        auto albums = db_.getAlbums(artist.id);
        j["albumCount"] = static_cast<int>(albums.size());
        artist_list.push_back(std::move(j));
        count++;
      }
    }

    // ── Albums ────────────────────────────────────────────────────────────
    crow::json::wvalue::list album_list;
    if (albumCount > 0) {
      auto albums =
          db_.getAlbumList("alphabeticalByName", albumCount, albumOffset);
      for (const auto& album : albums) {
        auto j = albumToSubsonic(album);
        auto artist = db_.getArtistById(album.artist_id);
        if (artist) j["artist"] = artist->name;
        album_list.push_back(std::move(j));
      }
    }

    crow::json::wvalue root;
    root["subsonic-response"]["status"] = "ok";
    root["subsonic-response"]["version"] = "1.16.1";
    root["subsonic-response"]["type"] = "localstream";
    root["subsonic-response"]["searchResult3"]["song"] = std::move(song_list);
    root["subsonic-response"]["searchResult3"]["artist"] =
        std::move(artist_list);
    root["subsonic-response"]["searchResult3"]["album"] = std::move(album_list);
    return crow::response(200, root);
  };
  CROW_ROUTE(app_, "/rest/search3")(search3_handler);
  CROW_ROUTE(app_, "/rest/search3.view")(search3_handler);

  // ── getPlaylists ──────────────────────────────────────────────────────────
  auto getPlaylists_handler = [this](const crow::request&) {
    auto playlists = db_.getPlaylists();
    crow::json::wvalue::list list;
    for (const auto& p : playlists) {
      crow::json::wvalue j;
      j["id"] = std::to_string(p.id);
      j["name"] = p.name;
      j["songCount"] = 0;
      j["duration"] = 0;
      j["public"] = false;
      list.push_back(std::move(j));
    }
    crow::json::wvalue root;
    root["subsonic-response"]["status"] = "ok";
    root["subsonic-response"]["version"] = "1.16.1";
    root["subsonic-response"]["type"] = "localstream";
    root["subsonic-response"]["playlists"]["playlist"] = std::move(list);
    return crow::response(200, root);
  };
  CROW_ROUTE(app_, "/rest/getPlaylists")(getPlaylists_handler);
  CROW_ROUTE(app_, "/rest/getPlaylists.view")(getPlaylists_handler);

  // ── getPlaylist ───────────────────────────────────────────────────────────
  auto getPlaylist_handler = [this](const crow::request& req) {
    auto id_param = req.url_params.get("id");
    if (!id_param) return errorResponse(10, "Missing parameter: id");
    int playlist_id = std::stoi(id_param);
    auto tracks = db_.getPlaylistTracks(playlist_id);
    auto playlists = db_.getPlaylists();
    const Playlist* found = nullptr;
    for (const auto& p : playlists) {
      if (p.id == playlist_id) {
        found = &p;
        break;
      }
    }
    if (!found) return errorResponse(70, "Playlist not found");
    int total_duration = 0;
    crow::json::wvalue::list song_list;
    for (const auto& track : tracks) {
      total_duration += track.duration_s;
      song_list.push_back(trackToSubsonic(track));
    }
    crow::json::wvalue root;
    root["subsonic-response"]["status"] = "ok";
    root["subsonic-response"]["version"] = "1.16.1";
    root["subsonic-response"]["type"] = "localstream";
    auto& pl = root["subsonic-response"]["playlist"];
    pl["id"] = std::to_string(found->id);
    pl["name"] = found->name;
    pl["songCount"] = static_cast<int>(tracks.size());
    pl["duration"] = total_duration;
    pl["public"] = false;
    pl["entry"] = std::move(song_list);
    return crow::response(200, root);
  };
  CROW_ROUTE(app_, "/rest/getPlaylist")(getPlaylist_handler);
  CROW_ROUTE(app_, "/rest/getPlaylist.view")(getPlaylist_handler);

  // ── stream ────────────────────────────────────────────────────────────────
  auto stream_handler = [this](const crow::request& req, crow::response& res) {
    auto id_param = req.url_params.get("id");
    if (!id_param) {
      res.code = 400;
      res.end();
      return;
    }
    int track_id = std::stoi(id_param);
    auto track = db_.getTrackById(track_id);
    if (!track) {
      res.code = 404;
      res.end();
      return;
    }
    int max_bitrate = 0;
    auto bitrate_param = req.url_params.get("maxBitRate");
    if (bitrate_param) max_bitrate = std::stoi(bitrate_param);
    std::string dst_format = track->format;
    auto format_param = req.url_params.get("format");
    if (format_param && std::string(format_param) != "raw")
      dst_format = std::string(format_param);
    bool should_transcode =
        Transcoder::needsTranscode(track->format, dst_format, 0, max_bitrate);
    res.set_header("Accept-Ranges", "bytes");
    res.set_header("Cache-Control", "no-cache");
    if (!should_transcode) {
      std::ifstream file(track->file_path, std::ios::binary);
      if (!file.is_open()) {
        res.code = 500;
        res.end();
        return;
      }
      long long file_size = track->file_size;
      res.set_header("Content-Type", Transcoder::mimeType(track->format));
      res.set_header("Content-Length", std::to_string(file_size));
      std::string range_header = req.get_header_value("Range");
      if (!range_header.empty() && range_header.substr(0, 6) == "bytes=") {
        std::string range = range_header.substr(6);
        auto dash = range.find('-');
        long long start = std::stoll(range.substr(0, dash));
        long long end = (dash + 1 < range.size())
                            ? std::stoll(range.substr(dash + 1))
                            : file_size - 1;
        long long length = end - start + 1;
        file.seekg(start);
        std::string buf(length, '\0');
        file.read(buf.data(), length);
        res.code = 206;
        res.set_header("Content-Range", "bytes " + std::to_string(start) + "-" +
                                            std::to_string(end) + "/" +
                                            std::to_string(file_size));
        res.set_header("Content-Length", std::to_string(length));
        res.write(buf);
      } else {
        res.code = 200;
        constexpr std::size_t CHUNK = 65536;
        std::vector<char> buf(CHUNK);
        while (file) {
          file.read(buf.data(), CHUNK);
          std::streamsize n = file.gcount();
          if (n > 0) res.write(std::string(buf.data(), n));
        }
      }
      res.end();
      return;
    }
    TranscodeRequest tc_req;
    tc_req.file_path = track->file_path;
    tc_req.format = dst_format;
    tc_req.bitrate_kbps = max_bitrate;
    res.code = 200;
    res.set_header("Content-Type", Transcoder::mimeType(dst_format));
    Transcoder transcoder;
    bool ok =
        transcoder.transcode(tc_req, [&res](const char* data, std::size_t n) {
          res.write(std::string(data, n));
        });
    if (!ok) {
      res.code = 500;
      res.write("Transcoding failed");
    }
    res.end();
  };
  CROW_ROUTE(app_, "/rest/stream")(stream_handler);
  CROW_ROUTE(app_, "/rest/stream.view")(stream_handler);

  // ── getCoverArt ───────────────────────────────────────────────────────────
  auto getCoverArt_handler = [this](const crow::request& req,
                                    crow::response& res) {
    auto id_param = req.url_params.get("id");
    if (!id_param) {
      res.code = 400;
      res.end();
      return;
    }
    std::string id_str(id_param);
    int album_id = (id_str.substr(0, 3) == "al-") ? std::stoi(id_str.substr(3))
                                                  : std::stoi(id_str);
    auto cover = db_.getAlbumCover(album_id);
    if (!cover || cover->data.empty()) {
      res.code = 404;
      res.end();
      return;
    }
    res.code = 200;
    res.set_header("Content-Type", cover->mime_type);
    res.set_header("Cache-Control", "public, max-age=86400");
    res.body.assign(reinterpret_cast<const char*>(cover->data.data()),
                    cover->data.size());
    res.end();
  };
  CROW_ROUTE(app_, "/rest/getCoverArt")(getCoverArt_handler);
  CROW_ROUTE(app_, "/rest/getCoverArt.view")(getCoverArt_handler);

  CROW_ROUTE(app_, "/rest/getLicense.view")
  ([] {
    crow::json::wvalue root;

    root["subsonic-response"]["status"] = "ok";
    root["subsonic-response"]["version"] = "1.16.1";
    root["subsonic-response"]["type"] = "localstream";

    root["subsonic-response"]["license"]["valid"] = true;
    root["subsonic-response"]["license"]["email"] = "";
    root["subsonic-response"]["license"]["trialExpires"] = "";

    return crow::response(200, root);
  });

  CROW_ROUTE(app_, "/rest/getLicense")
  ([] {
    crow::json::wvalue root;

    root["subsonic-response"]["status"] = "ok";
    root["subsonic-response"]["version"] = "1.16.1";
    root["subsonic-response"]["type"] = "localstream";

    root["subsonic-response"]["license"]["valid"] = true;

    return crow::response(200, root);
  });
  CROW_ROUTE(app_, "/rest/getMusicFolders.view")
  ([] {
    crow::json::wvalue root;

    root["subsonic-response"]["status"] = "ok";
    root["subsonic-response"]["version"] = "1.16.1";
    root["subsonic-response"]["type"] = "localstream";

    crow::json::wvalue folder;
    folder["id"] = 1;
    folder["name"] = "Music";

    crow::json::wvalue::list folders;
    folders.push_back(std::move(folder));

    root["subsonic-response"]["musicFolders"]["musicFolder"] =
        std::move(folders);

    return crow::response(200, root);
  });

  CROW_ROUTE(app_, "/rest/getMusicFolders")
  ([] {
    crow::json::wvalue root;

    root["subsonic-response"]["status"] = "ok";
    root["subsonic-response"]["version"] = "1.16.1";
    root["subsonic-response"]["type"] = "localstream";

    crow::json::wvalue folder;
    folder["id"] = 1;
    folder["name"] = "Music";

    crow::json::wvalue::list folders;
    folders.push_back(std::move(folder));

    root["subsonic-response"]["musicFolders"]["musicFolder"] =
        std::move(folders);

    return crow::response(200, root);
  });

  // ── getStarred2 ───────────────────────────────────────────────────────────
  auto getStarred2_handler = [this](const crow::request&) {
    crow::json::wvalue root;
    root["subsonic-response"]["status"] = "ok";
    root["subsonic-response"]["version"] = "1.16.1";
    root["subsonic-response"]["type"] = "localstream";
    root["subsonic-response"]["starred2"]["song"] = crow::json::wvalue::list{};
    root["subsonic-response"]["starred2"]["album"] = crow::json::wvalue::list{};
    root["subsonic-response"]["starred2"]["artist"] =
        crow::json::wvalue::list{};
    return crow::response(200, root);
  };
  CROW_ROUTE(app_, "/rest/getStarred2")(getStarred2_handler);
  CROW_ROUTE(app_, "/rest/getStarred2.view")(getStarred2_handler);

  // ── getBookmarks ──────────────────────────────────────────────────────────
  auto getBookmarks_handler = [](const crow::request&) {
    crow::json::wvalue root;
    root["subsonic-response"]["status"] = "ok";
    root["subsonic-response"]["version"] = "1.16.1";
    root["subsonic-response"]["type"] = "localstream";
    root["subsonic-response"]["bookmarks"] = crow::json::wvalue::object();
    return crow::response(200, root);
  };
  CROW_ROUTE(app_, "/rest/getBookmarks")(getBookmarks_handler);
  CROW_ROUTE(app_, "/rest/getBookmarks.view")(getBookmarks_handler);

  // ── getGenres ─────────────────────────────────────────────────────────────
  auto getGenres_handler = [](const crow::request&) {
    crow::json::wvalue root;
    root["subsonic-response"]["status"] = "ok";
    root["subsonic-response"]["version"] = "1.16.1";
    root["subsonic-response"]["type"] = "localstream";
    root["subsonic-response"]["genres"] = crow::json::wvalue::object();
    return crow::response(200, root);
  };
  CROW_ROUTE(app_, "/rest/getGenres")(getGenres_handler);
  CROW_ROUTE(app_, "/rest/getGenres.view")(getGenres_handler);

  auto getOpenSubsonicExtensions_handler = [](const crow::request&) {
    crow::json::wvalue root;
    root["subsonic-response"]["status"] = "ok";
    root["subsonic-response"]["version"] = "1.16.1";
    root["subsonic-response"]["type"] = "localstream";
    root["subsonic-response"]["openSubsonicExtensions"] =
        crow::json::wvalue::list{};
    return crow::response(200, root);
  };
  CROW_ROUTE(app_, "/rest/getOpenSubsonicExtensions")
  (getOpenSubsonicExtensions_handler);
  CROW_ROUTE(app_, "/rest/getOpenSubsonicExtensions.view")
  (getOpenSubsonicExtensions_handler);

  // ── getMusicDirectory ─────────────────────────────────────────────────────
  auto getMusicDirectory_handler = [this](const crow::request& req) {
    auto id_param = req.url_params.get("id");
    if (!id_param) return errorResponse(10, "Missing parameter: id");

    int id = std::stoi(id_param);

    // ── Caso 1: el id corresponde a un artista ────────────────────────────
    auto artist = db_.getArtistById(id);
    if (artist) {
      auto albums = db_.getAlbums(artist->id);

      crow::json::wvalue::list children;
      for (const auto& album : albums) {
        crow::json::wvalue child;
        child["id"] = std::to_string(album.id);
        child["parent"] = std::to_string(id);
        child["title"] = album.title;
        child["album"] = album.title;
        child["artist"] = artist->name;
        child["isDir"] = true;
        child["coverArt"] = "al-" + std::to_string(album.id);
        if (album.year > 0) child["year"] = album.year;
        children.push_back(std::move(child));
      }

      crow::json::wvalue root;
      root["subsonic-response"]["status"] = "ok";
      root["subsonic-response"]["version"] = "1.16.1";
      root["subsonic-response"]["type"] = "localstream";
      root["subsonic-response"]["directory"]["id"] = std::to_string(id);
      root["subsonic-response"]["directory"]["name"] = artist->name;
      root["subsonic-response"]["directory"]["child"] = std::move(children);
      return crow::response(200, root);
    }

    // ── Caso 2: el id corresponde a un álbum ──────────────────────────────
    auto album = db_.getAlbumById(id);
    if (!album) return errorResponse(70, "Directory not found");

    auto tracks = db_.getTracks(album->id);
    auto artist_opt = db_.getArtistById(album->artist_id);

    crow::json::wvalue::list children;
    for (const auto& track : tracks) {
      auto child = trackToSubsonic(track);
      child["parent"] = std::to_string(id);
      child["isDir"] = false;
      if (artist_opt) child["artist"] = artist_opt->name;
      children.push_back(std::move(child));
    }

    crow::json::wvalue root;
    root["subsonic-response"]["status"] = "ok";
    root["subsonic-response"]["version"] = "1.16.1";
    root["subsonic-response"]["type"] = "localstream";
    root["subsonic-response"]["directory"]["id"] = std::to_string(id);
    root["subsonic-response"]["directory"]["parent"] =
        std::to_string(album->artist_id);
    root["subsonic-response"]["directory"]["name"] = album->title;
    root["subsonic-response"]["directory"]["child"] = std::move(children);
    return crow::response(200, root);
  };

  CROW_ROUTE(app_, "/rest/getMusicDirectory")(getMusicDirectory_handler);
  CROW_ROUTE(app_, "/rest/getMusicDirectory.view")(getMusicDirectory_handler);
}
}  // namespace localstream