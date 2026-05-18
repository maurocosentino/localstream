#include "api/WebHandler.hpp"

namespace localstream {

WebHandler::WebHandler(Database& db, crow::SimpleApp& app)
    : db_(db), app_(app)
{
    setupRoutes();
}

void WebHandler::setupRoutes()
{
    CROW_ROUTE(app_, "/")
    ([]{
        std::string html = R"html(<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>LocalStream</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
            background: #0f0f0f; color: #e0e0e0;
            height: 100vh;
            display: grid;
            grid-template-rows: auto 1fr auto;
        }
        header {
            padding: 16px 24px;
            background: #1a1a1a;
            border-bottom: 1px solid #2a2a2a;
            display: flex; align-items: center; gap: 12px;
        }
        header h1 { font-size: 18px; font-weight: 600; color: #fff; }
        header span { font-size: 13px; color: #666; }
        #search-input {
            margin-left: auto; padding: 6px 12px;
            border-radius: 6px; border: 1px solid #333;
            background: #252525; color: #e0e0e0;
            font-size: 13px; width: 280px; outline: none;
        }
        .container {
            display: grid;
            grid-template-columns: 220px 260px 1fr;
            overflow: hidden;
        }
        .panel {
            border-right: 1px solid #2a2a2a;
            overflow-y: auto; padding: 8px 0;
        }
        .panel-title {
            font-size: 11px; font-weight: 600; color: #555;
            text-transform: uppercase; letter-spacing: 0.08em;
            padding: 8px 16px 4px;
        }
        .item {
            padding: 8px 16px; font-size: 14px; cursor: pointer;
            border-radius: 4px; margin: 1px 8px;
            white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
            color: #ccc; transition: background 0.1s;
        }
        .item:hover { background: #2a2a2a; color: #fff; }
        .item.active { background: #1db954; color: #fff; }
        .track-item {
            padding: 8px 16px; font-size: 14px; cursor: pointer;
            border-radius: 4px; margin: 1px 8px; color: #ccc;
            transition: background 0.1s;
            display: grid;
            grid-template-columns: 24px 1fr auto;
            align-items: center; gap: 8px;
        }
        .track-item:hover { background: #2a2a2a; color: #fff; }
        .track-item.active { background: #1db954; color: #fff; }
        .track-num { font-size: 12px; color: #555; text-align: right; }
        .track-item.active .track-num { color: #fff; }
        .track-duration { font-size: 12px; color: #555; }
        .track-item.active .track-duration { color: #fff; }
        .empty { padding: 24px 16px; font-size: 13px; color: #444; }
        .player {
            background: #1a1a1a; border-top: 1px solid #2a2a2a;
            padding: 16px 24px;
            display: flex; align-items: center; gap: 20px;
        }
        .player-info { flex: 1; min-width: 0; }
        .player-title {
            font-size: 14px; font-weight: 500; color: #fff;
            white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
        }
        .player-artist { font-size: 12px; color: #888; margin-top: 2px; }
        .player-idle { font-size: 14px; color: #444; }
        audio { flex: 2; height: 36px; accent-color: #1db954; }
    </style>
</head>
<body>

<header>
    <h1>LocalStream</h1>
    <span id="header-info">cargando...</span>
    <input type="text" id="search-input"
           placeholder="Buscar artista, álbum o track..."
           oninput="onSearch(this.value)">
</header>

<div class="container">
    <div class="panel" id="artists-panel">
        <div class="panel-title">Artistas</div>
        <div class="empty">Cargando...</div>
    </div>
    <div class="panel" id="albums-panel">
        <div class="panel-title">Álbumes</div>
        <div class="empty">Seleccioná un artista</div>
    </div>
    <div class="panel" id="tracks-panel">
        <div class="panel-title">Tracks</div>
        <div class="empty">Seleccioná un álbum</div>
    </div>
</div>

<div class="player">
    <div class="player-info">
        <div class="player-idle" id="player-title">Sin reproducción</div>
        <div class="player-artist" id="player-artist"></div>
    </div>
    <audio id="audio-player" controls preload="none"></audio>
</div>

<script>
const player      = document.getElementById('audio-player');
const playerTitle  = document.getElementById('player-title');
const playerArtist = document.getElementById('player-artist');

function formatDuration(s) {
    s = Math.floor(s || 0);
    return Math.floor(s / 60) + ':' + String(s % 60).padStart(2, '0');
}

function playTrack(track, artistName) {
    player.src = '/stream/' + track.id;
    player.play();
    playerTitle.textContent  = track.title;
    playerTitle.className    = 'player-title';
    playerArtist.textContent = artistName;
    document.querySelectorAll('.track-item').forEach(el => {
        el.classList.toggle('active', el.dataset.id == track.id);
    });
}

async function loadArtists() {
    const r       = await fetch('/api/artists');
    const data    = await r.json();
    const r2      = await fetch('/api/tracks?limit=1&offset=0');
    const data2   = await r2.json();
    const panel   = document.getElementById('artists-panel');

    document.getElementById('header-info').textContent =
        data.count + ' artistas · ' + data2.total + ' tracks';

    panel.innerHTML = '<div class="panel-title">Artistas</div>';
    data.artists.forEach(artist => {
        const el = document.createElement('div');
        el.className   = 'item';
        el.textContent = artist.name;
        el.onclick     = () => loadAlbums(artist);
        panel.appendChild(el);
    });
}

async function loadAlbums(artist) {
    document.querySelectorAll('#artists-panel .item').forEach(el =>
        el.classList.toggle('active', el.textContent === artist.name));

    const r      = await fetch('/api/artists/' + artist.id + '/albums');
    const data   = await r.json();
    const panel  = document.getElementById('albums-panel');

    document.getElementById('tracks-panel').innerHTML =
        '<div class="panel-title">Tracks</div><div class="empty">Seleccioná un álbum</div>';

    panel.innerHTML = '<div class="panel-title">Álbumes</div>';
    data.albums.forEach(album => {
        const el = document.createElement('div');
        el.className   = 'item';
        el.textContent = album.year ? album.title + ' (' + album.year + ')' : album.title;
        el.onclick     = () => loadTracks(album, artist.name);
        panel.appendChild(el);
    });
}

async function loadTracks(album, artistName) {
    document.querySelectorAll('#albums-panel .item').forEach(el =>
        el.classList.toggle('active', el.textContent.startsWith(album.title)));

    const r     = await fetch('/api/albums/' + album.id + '/tracks');
    const data  = await r.json();
    const panel = document.getElementById('tracks-panel');

    panel.innerHTML = '<div class="panel-title">Tracks</div>';
    data.tracks.forEach(track => {
        const el      = document.createElement('div');
        el.className  = 'track-item';
        el.dataset.id = track.id;
        el.innerHTML  =
            '<span class="track-num">' + (track.track_number || '—') + '</span>' +
            '<span>' + track.title + '</span>' +
            '<span class="track-duration">' + formatDuration(track.duration_s) + '</span>';
        el.onclick = () => playTrack(track, artistName);
        panel.appendChild(el);
    });
}

// Búsqueda con debounce
let searchTimeout = null;

function onSearch(value) {
    clearTimeout(searchTimeout);
    const v = value.trim();

    if (v.length === 0) {
        document.getElementById('tracks-panel').innerHTML =
            '<div class="panel-title">Tracks</div><div class="empty">Seleccioná un álbum</div>';
        return;
    }
    if (v.length < 2) return;

    searchTimeout = setTimeout(() => search(v), 300);
}

async function search(query) {
    const r    = await fetch('/api/search?q=' + encodeURIComponent(query));
    const data = await r.json();
    const panel = document.getElementById('tracks-panel');

    panel.innerHTML = '<div class="panel-title">' +
        data.count + ' resultados para "' + query + '"</div>';

    if (data.count === 0) {
        panel.innerHTML += '<div class="empty">Sin resultados.</div>';
        return;
    }

    data.tracks.forEach((track, i) => {
        const el      = document.createElement('div');
        el.className  = 'track-item';
        el.dataset.id = track.id;
        el.innerHTML  =
            '<span class="track-num">' + (i + 1) + '</span>' +
            '<span>' + track.title + '</span>' +
            '<span class="track-duration">' + formatDuration(track.duration_s) + '</span>';
        el.onclick = () => playTrack(track, '');
        panel.appendChild(el);
    });
}

loadArtists();
</script>
</body>
</html>)html";

        auto response = crow::response(200, html);
        response.set_header("Content-Type", "text/html; charset=utf-8");
        return response;
    });
}

} // namespace localstream