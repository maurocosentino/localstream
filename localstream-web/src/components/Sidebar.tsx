import { signal } from '@preact/signals'
import { api } from '../api'
import type { Artist } from '../types'
import { currentArtist, currentView, searchQuery } from '../store'

const artists = signal<Artist[]>([])
const libStats = signal('cargando...')

// Carga inicial
api.getArtists().then(d => {
    artists.value = d.artists
    api.getAllTracks(1, 0).then(t => {
        libStats.value = `${d.count} artistas · ${t.total} tracks`
    })
})

let searchTimeout: ReturnType<typeof setTimeout> | null = null

function onSearch(value: string) {
    if (searchTimeout) clearTimeout(searchTimeout)
    const v = value.trim()
    searchQuery.value = v
    if (v.length === 0) { currentView.value = 'home'; return }
    if (v.length < 2) return
    searchTimeout = setTimeout(() => { currentView.value = 'search' }, 300)
}

function selectArtist(artist: Artist) {
    currentArtist.value = artist
    currentView.value   = 'albums'
}

export function Sidebar() {
    return (
        <aside id="sidebar">
            <div class="logo">
                <div class="logo-mark">
                    <div class="logo-icon">
                        <svg viewBox="0 0 16 16">
                            <path d="M8 2a6 6 0 100 12A6 6 0 008 2zM6.5 5.5a1.5 1.5 0 113 0 1.5 1.5 0 01-3 0zM8 12a4 4 0 01-3.46-6h6.92A4 4 0 018 12z"/>
                        </svg>
                    </div>
                    <div>
                        <div class="logo-text">LocalStream</div>
                        <div class="logo-sub">{libStats.value}</div>
                    </div>
                </div>
            </div>

            <div class="search-wrap">
                <div class="search-box">
                    <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                        <circle cx="11" cy="11" r="8"/><line x1="21" y1="21" x2="16.65" y2="16.65"/>
                    </svg>
                    <input
                        type="text"
                        placeholder="Buscar..."
                        onInput={(e) => onSearch((e.target as HTMLInputElement).value)}
                    />
                </div>
            </div>

            <div class="nav-section">
                <div class="nav-label">Navegar</div>
                <div
                    class={`nav-item ${currentView.value === 'home' ? 'active' : ''}`}
                    onClick={() => { currentView.value = 'home' }}
                >
                    <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                        <path d="M3 9l9-7 9 7v11a2 2 0 01-2 2H5a2 2 0 01-2-2z"/>
                        <polyline points="9 22 9 12 15 12 15 22"/>
                    </svg>
                    Inicio
                </div>
            </div>

            <div class="nav-section" style="padding-bottom:6px">
                <div class="nav-label">Artistas</div>
            </div>

            <div class="artists-list">
                {artists.value.map(artist => (
                    <div
                        key={artist.id}
                        class={`artist-item ${currentArtist.value?.id === artist.id && currentView.value !== 'home' ? 'active' : ''}`}
                        onClick={() => selectArtist(artist)}
                    >
                        <div class="artist-avatar">
                            {artist.name[0].toUpperCase()}
                        </div>
                        <div class="artist-name">{artist.name}</div>
                    </div>
                ))}
            </div>
        </aside>
    )
}