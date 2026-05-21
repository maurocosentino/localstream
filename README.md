# LocalStream

A personal media server written in C++ that serves your music library over HTTP to any device on your local network. Compatible with any Subsonic client (DSub, Symfonium, Ultrasonic) and accessible via built-in web UI.

## Features

- Scans music directories recursively — MP3, FLAC, OGG, Opus, WAV, AAC, M4A, WMA, AIFF, APE
- Reads and caches track metadata and album artwork via TagLib
- Persists the library in SQLite — fast startup, no re-scan needed
- Cleans up deleted tracks automatically on every scan
- REST API to browse artists, albums, tracks and playlists
- Subsonic API — connect any Subsonic-compatible mobile app
- Streams audio with HTTP Range Request support — seek works everywhere
- Transcoding via FFmpeg for format/bitrate conversion
- Built-in web UI served directly by the server
- API key authentication
- Async library re-scan without restarting
- Runs as a systemd service — starts automatically with your system

## Requirements

- Ubuntu 24 (or any modern Linux)
- CMake 3.14+
- g++ 11+
- FFmpeg (`ffmpeg`)
- TagLib (`libtag1-dev`)
- SQLite3 (`libsqlite3-dev`)

Crow, SQLiteCpp, Asio, and nlohmann/json are fetched automatically by CMake.

```bash
sudo apt install -y build-essential cmake git ffmpeg libtag1-dev libsqlite3-dev pkg-config
```

## Build

```bash
git clone https://github.com/maurocosentino/localstream.git
cd localstream
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Build the web UI

```bash
cd localstream-web
npm install
npm run build
```

## Configuration

Create your `config.json`:

```json
{
    "server": { "port": 8080 },
    "database": { "path": "/var/lib/localstream/localstream.db" },
    "log_level": "INFO",
    "api_key": "your-secret-key-here",
    "media_directories": [
        "/path/to/your/music"
    ]
}
```

| Field | Description |
|-------|-------------|
| `port` | HTTP port to listen on |
| `database.path` | Path to the SQLite database file |
| `log_level` | `DEBUG`, `INFO`, `WARN`, or `ERROR` |
| `api_key` | Secret key for REST API authentication |
| `media_directories` | List of directories to scan for audio files |

## Run (development)

```bash
cd build
./localstream --config ../config.json --static-dir ../localstream-web/dist
```

## Install as a systemd service (production)

```bash
# Install binary and frontend
sudo cp build/localstream /usr/local/bin/localstream
sudo mkdir -p /etc/localstream /var/lib/localstream/dist
sudo cp config.json /etc/localstream/config.json
sudo cp -r localstream-web/dist/* /var/lib/localstream/dist/
sudo chown -R $USER:$USER /var/lib/localstream

# Create the service file
sudo tee /etc/systemd/system/localstream.service > /dev/null << EOF
[Unit]
Description=LocalStream — Personal Media Server
After=network.target

[Service]
Type=simple
User=$USER
WorkingDirectory=/var/lib/localstream
ExecStart=/usr/local/bin/localstream --config /etc/localstream/config.json --static-dir /var/lib/localstream/dist
Restart=on-failure
RestartSec=5
StandardOutput=journal
StandardError=journal
SyslogIdentifier=localstream

[Install]
WantedBy=multi-user.target
EOF

# Enable and start
sudo systemctl daemon-reload
sudo systemctl enable localstream
sudo systemctl start localstream
```

Check status and logs:

```bash
sudo systemctl status localstream
sudo journalctl -u localstream -f
```

## Connecting from a mobile app

LocalStream implements the Subsonic API. Use any Subsonic-compatible app:

- **DSub** (Android)
- **Symfonium** (Android)
- **Ultrasonic** (Android)
- **Substreamer** (iOS)

Configure the app with:

```
Server URL: http://YOUR_LOCAL_IP:8080
Username:   anything
Password:   your-api-key-here
```

Find your local IP:

```bash
ip addr show | grep "inet " | grep -v 127.0.0.1
```

## Web UI

Open `http://localhost:8080` in any browser on your network. Features:

- Browse by artist and album
- Search tracks, artists, and albums
- Create and manage playlists
- Play all / shuffle
- Album artwork

## REST API

All endpoints except `/health` require the header `X-Api-Key: your-key`.

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/health` | Health check |
| GET | `/api/artists` | List all artists |
| GET | `/api/artists/:id/albums` | Albums by artist |
| GET | `/api/albums/:id/tracks` | Tracks in an album |
| GET | `/api/albums/:id/cover` | Album artwork |
| GET | `/api/tracks?limit=50&offset=0` | All tracks (paginated) |
| GET | `/api/tracks/:id` | Track detail |
| GET | `/api/search?q=query` | Search tracks, artists, albums |
| GET | `/stream/:id` | Stream audio file |
| POST | `/api/scan` | Trigger async library re-scan |
| GET | `/api/playlists` | List playlists |
| POST | `/api/playlists` | Create playlist |
| DELETE | `/api/playlists/:id` | Delete playlist |
| GET | `/api/playlists/:id/tracks` | Tracks in a playlist |
| POST | `/api/playlists/:id/tracks` | Add track to playlist |
| DELETE | `/api/playlists/:id/tracks/:track_id` | Remove track from playlist |

## Subsonic API

Compatible endpoints at `/rest/*` — tested with DSub and Symfonium:

`ping` · `getArtists` · `getArtist` · `getAlbumList` · `getAlbumList2` · `getMusicDirectory` · `getSong` · `search3` · `stream` · `getCoverArt` · `getPlaylists` · `getPlaylist` · `getIndexes` · `getLicense` · `getMusicFolders` · `getStarred2` · `getBookmarks` · `getGenres` · `getOpenSubsonicExtensions`

## Architecture

```
src/
├── config/        AppConfig       — loads config.json and CLI args
├── db/            Database        — SQLite schema, queries, cover cache
├── scanner/       FileScanner     — finds audio files recursively
│                  LibraryScanner  — scan pipeline, cleanup, async
├── metadata/      MetadataReader  — extracts tags and artwork via TagLib
├── streaming/     StreamHandler   — HTTP Range Request streaming
│                  Transcoder      — FFmpeg-based transcoding
├── api/           ApiRouter       — REST API
│                  SubsonicRouter  — Subsonic API
│                  WebHandler      — serves static frontend
│                  AuthMiddleware  — API key authentication
└── logger/        Logger          — thread-safe leveled logger
```

## Stack

- [Crow](https://github.com/CrowCpp/Crow) — HTTP server
- [TagLib](https://taglib.org/) — audio metadata and artwork
- [SQLiteCpp](https://github.com/SRombauts/SQLiteCpp) — SQLite wrapper
- [nlohmann/json](https://github.com/nlohmann/json) — JSON parsing
- [FFmpeg](https://ffmpeg.org/) — audio transcoding
- [Preact](https://preactjs.com/) + [Vite](https://vitejs.dev/) — web UI

## License

MIT