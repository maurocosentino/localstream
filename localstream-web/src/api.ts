import type { ArtistsResponse, AlbumsResponse, TracksResponse, SearchResponse, PlaylistsResponse, Playlist } from './types'

const BASE = ''

export const api = {
    getArtists: (): Promise<ArtistsResponse> =>
        fetch(`${BASE}/api/artists`).then(r => r.json()),

    getAlbums: (artistId: number): Promise<AlbumsResponse> =>
        fetch(`${BASE}/api/artists/${artistId}/albums`).then(r => r.json()),

    getTracks: (albumId: number): Promise<TracksResponse> =>
        fetch(`${BASE}/api/albums/${albumId}/tracks`).then(r => r.json()),

    getAllTracks: (limit = 50, offset = 0): Promise<TracksResponse> =>
        fetch(`${BASE}/api/tracks?limit=${limit}&offset=${offset}`).then(r => r.json()),

    search: (query: string): Promise<SearchResponse> =>
        fetch(`${BASE}/api/search?q=${encodeURIComponent(query)}`).then(r => r.json()),

    streamUrl: (trackId: number): string =>
        `${BASE}/stream/${trackId}`,

    coverUrl: (albumId: number): string =>
        `${BASE}/api/albums/${albumId}/cover`,
    getPlaylists: (): Promise<PlaylistsResponse> =>
        fetch(`${BASE}/api/playlists`).then(r => r.json()),

    //playlist
    createPlaylist: (name: string): Promise<Playlist> =>
        fetch(`${BASE}/api/playlists`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ name })
    }).then(r => r.json()),

    deletePlaylist: (id: number): Promise<void> =>
        fetch(`${BASE}/api/playlists/${id}`, { method: 'DELETE' }).then(r => r.json()),

    getPlaylistTracks: (id: number): Promise<TracksResponse> =>
        fetch(`${BASE}/api/playlists/${id}/tracks`).then(r => r.json()),

    addTrackToPlaylist: (playlist_id: number, track_id: number): Promise<void> =>
        fetch(`${BASE}/api/playlists/${playlist_id}/tracks`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ track_id })
        }).then(r => r.json()),

    removeTrackFromPlaylist: (playlist_id: number, track_id: number): Promise<void> =>
        fetch(`${BASE}/api/playlists/${playlist_id}/tracks/${track_id}`, {
            method: 'DELETE'
        }).then(r => r.json()),
}