export interface Artist {
    id: number
    name: string
}

export interface Album {
    id: number
    title: string
    artist_id: number
    year: number
}

export interface Track {
    id: number
    title: string
    artist_id: number
    album_id: number
    duration_s: number
    track_number: number
    format: string
}

export interface ArtistsResponse {
    artists: Artist[]
    count: number
}

export interface AlbumsResponse {
    albums: Album[]
    count: number
}

export interface TracksResponse {
    tracks: Track[]
    count: number
    total: number
    limit: number
    offset: number
}

export interface SearchResponse {
    tracks: Track[]
    count: number
    query: string
}