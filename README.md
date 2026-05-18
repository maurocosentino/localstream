# LocalStream

A personal media server written in C++ that serves your music library over HTTP to any device on your local network.

## What it does

- Scans your music directories and indexes MP3, FLAC, OGG, WAV, AAC, M4A, and other common audio formats
- Reads track metadata (title, artist, album, year, duration) via TagLib
- Persists the library index in SQLite — no re-scan on every start
- Exposes a REST API to browse artists, albums, and tracks
- Streams audio files with HTTP Range Request support — seek works in VLC and any standard player
- Triggers library re-scans via API without restarting the server

## Requirements

- Ubuntu 24 (or any modern Linux)
- CMake 3.14+
- g++ 11+
- TagLib (`libtag1-dev`)
- SQLite3 (`libsqlite3-dev`)

Crow, SQLiteCpp, Asio, and nlohmann/json are fetched automatically by CMake at configure time.

## Build

```bash
git clone https://github.com/maurocosentino/localstream.git
cd localstream
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Configuration

Edit `config.json` before running:

```json
{
    "database": {
        "path": "localstream.db"
    },
    "server": {
        "port": 8080
    },
    "media_directories": [
        "/path/to/your/music"
    ]
}
```

## Run

```bash
cd build
./localstream
```

## API

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/health` | Server health check |
| GET | `/api/artists` | List all artists |
| GET | `/api/artists/:id/albums` | Albums by artist |
| GET | `/api/albums/:id/tracks` | Tracks in an album |
| GET | `/api/tracks` | All tracks (paginated) |
| GET | `/api/tracks/:id` | Track detail |
| GET | `/stream/:id` | Stream audio file |
| POST | `/api/scan` | Trigger library re-scan |

### Pagination

```
GET /api/tracks?limit=50&offset=0
```

Response includes `total`, `limit`, and `offset` for clients to paginate.

## Playing in VLC

**Media → Open Network Stream** (`Ctrl+N`):

```
http://localhost:8080/stream/4
```

From another device on the same WiFi, replace `localhost` with your machine's local IP:

```
http://192.168.1.100:8080/stream/4
```

To find your local IP:

```bash
ip addr show | grep "inet " | grep -v 127.0.0.1
```

## Architecture

```
src/
├── config/        AppConfig     — reads config.json
├── db/            Database      — SQLite schema and queries
├── scanner/       FileScanner   — finds audio files recursively
│                  LibraryScanner — orchestrates the scan pipeline
├── metadata/      MetadataReader — extracts tags via TagLib
├── api/           ApiRouter     — REST endpoints via Crow
└── streaming/     StreamHandler — HTTP Range Request streaming
```

Each layer has a single responsibility and only knows about the layer directly below it.

## Stack

- [Crow](https://github.com/CrowCpp/Crow) — HTTP server
- [TagLib](https://taglib.org/) — audio metadata
- [SQLiteCpp](https://github.com/SRombauts/SQLiteCpp) — SQLite wrapper
- [nlohmann/json](https://github.com/nlohmann/json) — JSON parsing

## License

MIT