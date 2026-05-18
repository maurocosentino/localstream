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
        std::string html = R"html(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>LocalStream</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }

        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
            background: #0f0f0f;
            color: #e0e0e0;
            height: 100vh;
            display: grid;
            grid-template-rows: auto 1fr auto;
        }

        header {
            padding: 16px 24px;
            background: #1a1a1a;
            border-bottom: 1px solid #2a2a2a;
            display: flex;
            align-items: center;
            gap: 12px;
        }

        header h1 {
            font-size: 18px;
            font-weight: 600;
            color: #fff;
        }

        header span {
            font-size: 13px;
            color: #666;
        }

        .container {
            display: grid;
            grid-template-columns: 220px 260px 1fr;
            overflow: hidden;
        }

        .panel {
            border-right: 1px solid #2a2a2a;
            overflow-y: auto;
            padding: 8px 0;
        }

        .panel-title {
            font-size: 11px;
            font-weight: 600;
            color: #555;
            text-transform: uppercase;
            letter-spacing: 0.08em;
            padding: 8px 16px 4px;
        }

        .item {
            padding: 8px 16px;
            font-size: 14px;
            cursor: pointer;
            border-radius: 4px;
            margin: 1px 8px;
            white-space: nowrap;
            overflow: hidden;
            text-overflow: ellipsis;
            color: #ccc;
            transition: background 0.1s;
        }

        .item:hover { background: #2a2a2a; color: #fff; }
        .item.active { background: #1db954; color: #fff; }

        .track-item {
            padding: 8px 16px;
            font-size: 14px;
            cursor: pointer;
            border-radius: 4px;
            margin: 1px 8px;
            color: #ccc;
            transition: background 0.1s;
            display: grid;
            grid-template-columns: 24px 1fr auto;
            align-items: center;
            gap: 8px;
        }

        .track-item:hover { background: #2a2a2a; color: #fff; }
        .track-item.active { background: #1db954; color: #fff; }

        .track-num { font-size: 12px; color: #555; text-align: right; }
        .track-item.active .track-num { color: #fff; }
        .track-duration { font-size: 12px; color: #555; }
        .track-item.active .track-duration { color: #fff; }

        .empty {
            padding: 24px 16px;
            font-size: 13px;
            color: #444;
        }

        /* Player */
        .player {
            background: #1a1a1a;
            border-top: 1px solid #2a2a2a;
            padding: 16px 24px;
            display: flex;
            align-items: center;
            gap: 20px;
        }

        .player-info {
            flex: 1;
            min-width: 0;
        }

        .player-title {
            font-size: 14px;
            font-weight: 500;
            color: #fff;
            white-space: nowrap;
            overflow: hidden;
            text-overflow: ellipsis;
        }

        .player-artist {
            font-size: 12px;
            color: #888;
            margin-top: 2px;
        }

        .player-idle {
            font-size: 14px;
            color: #444;
        }

        audio {
            flex: 2;
            height: 36px;
            accent-color: #1db954;
        }

        audio::-webkit-media-controls-panel {
            background: #2a2a2a;
        }
    </style>
</head>
<body>

<header>
    <h1>LocalStream</h1>
    <span id="library-info">Cargando...</span>
</header>

<div class="container">
    <!-- Panel 1: Artistas -->
    <div class="panel" id="artists-panel">
        <div class="panel-title">Artistas</div>
        <div class="empty">Cargando...</div>
    </div>

    <!-- Panel 2: Albums -->
    <div class="panel" id="albums-panel">
        <div class="panel-title">Álbumes</div>
        <div class="empty">Seleccioná un artista</div>
    </div>

    <!-- Panel 3: Tracks -->
    <div class="panel" id="tracks-panel">
        <div class="panel-title">Tracks</div>
        <div class="empty">Seleccioná un álbum</div>
    </div>
</div>

<!-- Player -->
<div class="player">
    <div class="player-info">
        <div class="player-idle" id="player-title">Sin reproducción</div>
        <div class="player-artist" id="player-artist"></div>
    </div>
    <audio id="audio-player" controls preload="none"></audio>
</div>

<script>
const api = {
    async getArtists() {
        const r = await fetch('/api/artists');
        const d = await r.json();
        return d.artists;
    },
    async getAlbums(artistId) {
        const r = await fetch(`/api/artists/${artistId}/albums`);
        const d = await r.json();
        return d.albums;
    },
    async getTracks(albumId) {
        const r = await fetch(`/api/albums/${albumId}/tracks`);
        const d = await r.json();
        return d.tracks;
    },
    async getStats() {
        const r = await fetch('/api/tracks?limit=1&offset=0');
        const d = await r.json();
        return d.total;
    }
};

const player = document.getElementById('audio-player');
const playerTitle  = document.getElementById('player-title');
const playerArtist = document.getElementById('player-artist');
let currentTrackId = null;

function formatDuration(seconds) {
    const m = Math.floor(seconds / 60);
    const s = seconds % 60;
    return `${m}:${s.toString().padStart(2, '0')}`;
}

function playTrack(track, artistName) {
    currentTrackId = track.id;
    player.src = `/stream/${track.id}`;
    player.play();
    playerTitle.textContent  = track.title;
    playerTitle.className    = 'player-title';
    playerArtist.textContent = artistName;
    document.querySelectorAll('.track-item').forEach(el => {
        el.classList.toggle('active', el.dataset.id == track.id);
    });
}

async function loadArtists() {
    const artists = await api.getArtists();
    const panel   = document.getElementById('artists-panel');
    const total   = await api.getStats();

    document.getElementById('library-info').textContent =
        `${artists.length} artistas · ${total} tracks`;

    panel.innerHTML = '<div class="panel-title">Artistas</div>';

    if (artists.length === 0) {
        panel.innerHTML += '<div class="empty">No hay artistas</div>';
        return;
    }

    artists.forEach(artist => {
        const el = document.createElement('div');
        el.className   = 'item';
        el.textContent = artist.name;
        el.onclick     = () => loadAlbums(artist);
        panel.appendChild(el);
    });
}

async function loadAlbums(artist) {
    document.querySelectorAll('#artists-panel .item').forEach(el => {
        el.classList.toggle('active', el.textContent === artist.name);
    });

    const albums = await api.getAlbums(artist.id);
    const panel  = document.getElementById('albums-panel');

    panel.innerHTML = '<div class="panel-title">Álbumes</div>';

    document.getElementById('tracks-panel').innerHTML =
        '<div class="panel-title">Tracks</div><div class="empty">Seleccioná un álbum</div>';

    if (albums.length === 0) {
        panel.innerHTML += '<div class="empty">Sin álbumes</div>';
        return;
    }

    albums.forEach(album => {
        const el = document.createElement('div');
        el.className   = 'item';
        el.textContent = album.year ? `${album.title} (${album.year})` : album.title;
        el.onclick     = () => loadTracks(album, artist.name);
        panel.appendChild(el);
    });
}

async function loadTracks(album, artistName) {
    document.querySelectorAll('#albums-panel .item').forEach(el => {
        el.classList.toggle('active', el.textContent.startsWith(album.title));
    });

    const tracks = await api.getTracks(album.id);
    const panel  = document.getElementById('tracks-panel');

    panel.innerHTML = '<div class="panel-title">Tracks</div>';

    if (tracks.length === 0) {
        panel.innerHTML += '<div class="empty">Sin tracks</div>';
        return;
    }

    tracks.forEach(track => {
        const el       = document.createElement('div');
        el.className   = 'track-item';
        el.dataset.id  = track.id;

        const num      = document.createElement('span');
        num.className  = 'track-num';
        num.textContent = track.track_number || '—';

        const title    = document.createElement('span');
        title.textContent = track.title;

        const dur      = document.createElement('span');
        dur.className  = 'track-duration';
        dur.textContent = formatDuration(track.duration_s);

        el.appendChild(num);
        el.appendChild(title);
        el.appendChild(dur);
        el.onclick = () => playTrack(track, artistName);

        panel.appendChild(el);
    });
}

loadArtists();
</script>
</body>
</html>
)html";

        auto response = crow::response(200, html);
        response.set_header("Content-Type", "text/html; charset=utf-8");
        return response;
    });
}

} // namespace localstream