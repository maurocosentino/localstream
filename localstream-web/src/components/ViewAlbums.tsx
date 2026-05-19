import { signal } from '@preact/signals'
import { api } from '../api'
import type { Album } from '../types'
import { currentArtist, currentAlbum, currentView, currentTracks, currentIdx } from '../store'
import { playTrack } from './Player'

const albums = signal<Album[]>([])

// Recargar álbumes cuando cambia el artista
import { effect } from '@preact/signals'
effect(() => {
    const artist = currentArtist.value
    if (!artist) return
    api.getAlbums(artist.id).then(d => { albums.value = d.albums })
})

async function openTracks(album: Album) {
    currentAlbum.value  = album
    currentView.value   = 'tracks'
    currentIdx.value    = -1          
    const d = await api.getTracks(album.id)
    currentTracks.value = d.tracks
}

async function playAlbum(album: Album) {
    await openTracks(album)
    playTrack(0)
}

export function ViewAlbums() {
    const artist = currentArtist.value

    return (
        <div id="view-albums" class={currentView.value === 'albums' ? 'active' : ''}>
            <div class="breadcrumb">
                <span class="breadcrumb-link" onClick={() => { currentView.value = 'home' }}>Inicio</span>
                <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                    <polyline points="9 18 15 12 9 6"/>
                </svg>
                <span>{artist?.name}</span>
            </div>

            <div class="view-header">
                <div class="artist-cover">
                    <div class="artist-cover-placeholder">
                        <svg viewBox="0 0 24 24" fill="none" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
                            <path d="M20 21v-2a4 4 0 00-4-4H8a4 4 0 00-4 4v2"/>
                            <circle cx="12" cy="7" r="4"/>
                        </svg>
                    </div>
                </div>
                <div class="view-header-meta">
                    <div class="view-header-label">Artista</div>
                    <div class="view-header-name">{artist?.name}</div>
                    <div class="view-header-sub">{albums.value.length} álbumes</div>
                </div>
            </div>

            <div class="albums-list">
                {albums.value.map(album => (
                    <div
                        key={album.id}
                        class={`album-row ${currentAlbum.value?.id === album.id ? 'active' : ''}`}
                        onClick={() => openTracks(album)}
                    >
                        <div class="album-row-thumb">
                            <img
                                src={api.coverUrl(album.id)}
                                onError={(e) => {
                                    const el = e.target as HTMLElement
                                    el.style.display = 'none'
                                    el.nextElementSibling?.setAttribute('style', 'display:flex')
                                }}
                            />
                            <svg style="display:none" viewBox="0 0 24 24" fill="none" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
                                <path d="M9 18V5l12-2v13"/>
                                <circle cx="6" cy="18" r="3"/>
                                <circle cx="18" cy="16" r="3"/>
                            </svg>
                        </div>
                        <div class="album-row-info">
                            <div class="album-row-title">{album.title}</div>
                            <div class="album-row-year">{album.year || ''}</div>
                        </div>
                        <button
                            class="ctrl-btn"
                            onClick={(e) => { e.stopPropagation(); playAlbum(album) }}
                            style="opacity:0.6"
                        >
                            <svg viewBox="0 0 16 16" fill="currentColor" width="14" height="14">
                                <polygon points="3,2 14,8 3,14"/>
                            </svg>
                        </button>
                    </div>
                ))}
            </div>
        </div>
    )
}