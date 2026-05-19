import { signal } from '@preact/signals'
import { api } from '../api'
import type { Artist, Album } from '../types'
import { currentArtist, currentAlbum, currentView, currentTracks } from '../store'
import { playTrack } from './Player'

interface AlbumWithArtist { album: Album; artist: Artist }

const homeAlbums = signal<AlbumWithArtist[]>([])
const homeStats  = signal('...')

async function loadHomeAlbums() {
    const [ra, rt] = await Promise.all([
        api.getArtists(),
        api.getAllTracks(1, 0)
    ])

    homeStats.value = `${ra.count} artistas · ${rt.total} tracks`

    const all: AlbumWithArtist[] = []
    await Promise.all(
        ra.artists.slice(0, 12).map(async artist => {
            const d = await api.getAlbums(artist.id)
            d.albums.forEach(album => all.push({ album, artist }))
        })
    )

    all.sort(() => Math.random() - 0.5)
    homeAlbums.value = all.slice(0, 24)
}

loadHomeAlbums()

async function openAlbum(album: Album, artist: Artist) {
    currentArtist.value = artist
    currentAlbum.value  = album
    currentView.value   = 'tracks'
    const d = await api.getTracks(album.id)
    currentTracks.value = d.tracks
}

async function playAlbum(album: Album, artist: Artist) {
    await openAlbum(album, artist)
    playTrack(0)
}

export function ViewHome() {
    return (
        <div id="view-home" class={currentView.value === 'home' ? 'active' : ''}>
            <div class="home-hero">
                <div class="home-greeting">Escuchar ahora</div>
                <div class="home-sub">
                    Tu biblioteca local · <span>{homeStats.value}</span>
                </div>
            </div>
            <div class="section-header">
                <div class="section-title">Álbumes</div>
            </div>
            <div class="albums-grid">
                {homeAlbums.value.map(({ album, artist }) => (
                    <div key={album.id} class="album-card">
                        <div class="album-thumb" onClick={() => openAlbum(album, artist)}>
                            <img
                                src={api.coverUrl(album.id)}
                                onError={(e) => {
                                    const el = e.target as HTMLElement
                                    el.style.display = 'none'
                                    el.nextElementSibling?.setAttribute('style', 'display:flex;position:absolute;inset:0')
                                }}
                            />
                            <div class="album-thumb-placeholder" style="display:none;position:absolute;inset:0">
                                <svg viewBox="0 0 24 24" fill="none" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
                                    <path d="M9 18V5l12-2v13"/>
                                    <circle cx="6" cy="18" r="3"/>
                                    <circle cx="18" cy="16" r="3"/>
                                </svg>
                            </div>
                            <button
                                class="album-play-btn"
                                onClick={(e) => { e.stopPropagation(); playAlbum(album, artist) }}
                            >
                                <svg viewBox="0 0 16 16"><polygon points="3,2 14,8 3,14"/></svg>
                            </button>
                        </div>
                        <div class="album-info-title">{album.title}</div>
                        <div class="album-info-artist">
                            {artist.name}{album.year ? ` · ${album.year}` : ''}
                        </div>
                    </div>
                ))}
            </div>
        </div>
    )
}