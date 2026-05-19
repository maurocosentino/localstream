import type { ArtistsResponse, AlbumsResponse, TracksResponse, SearchResponse } from './types'

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
}