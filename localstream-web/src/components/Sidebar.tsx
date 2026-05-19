import { signal } from '@preact/signals'
import { api } from '../api'
import type { Artist, Playlist } from '../types'
import { currentArtist, currentView, searchQuery } from '../store'
import { currentPlaylist, openPlaylist } from './ViewPlaylist'

const artists  = signal<Artist[]>([])
const playlists = signal<Playlist[]>([])
const libStats = signal('cargando...')
const showNewPlaylist = signal(false)
const newPlaylistName = signal('')

async function loadData() {
    const [ra, rt] = await Promise.all([
        api.getArtists(),
        api.getAllTracks(1, 0)
    ])
    artists.value  = ra.artists
    libStats.value = `${ra.count} artistas · ${rt.total} tracks`
    await reloadPlaylists()
}

export async function reloadPlaylists() {
    const d = await api.getPlaylists()
    playlists.value = d.playlists
}

async function createPlaylist() {
    const name = newPlaylistName.value.trim()
    if (!name) return
    await api.createPlaylist(name)
    newPlaylistName.value  = ''
    showNewPlaylist.value  = false
    await reloadPlaylists()
}

async function deletePlaylist(e: MouseEvent, id: number) {
    e.stopPropagation()
    await api.deletePlaylist(id)
    if (currentPlaylist.value?.id === id) {
        currentView.value = 'home'
        currentPlaylist.value = null
    }
    await reloadPlaylists()
}

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

loadData()

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

            {/* Playlists */}
            <div class="nav-section" style="padding-bottom:4px">
                <div class="nav-label" style="display:flex;align-items:center;justify-content:space-between;padding-right:8px">
                    <span>Playlists</span>
                    <button
                        class="ctrl-btn"
                        style="padding:2px;color:var(--text-3)"
                        title="Nueva playlist"
                        onClick={() => { showNewPlaylist.value = !showNewPlaylist.value }}
                    >
                        <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" width="14" height="14">
                            <line x1="12" y1="5" x2="12" y2="19"/>
                            <line x1="5" y1="12" x2="19" y2="12"/>
                        </svg>
                    </button>
                </div>
            </div>

            {showNewPlaylist.value && (
                <div style="padding:0 14px 8px">
                    <div class="search-box">
                        <input
                            type="text"
                            placeholder="Nombre de playlist..."
                            value={newPlaylistName.value}
                            onInput={(e) => { newPlaylistName.value = (e.target as HTMLInputElement).value }}
                            onKeyDown={(e) => { if (e.key === 'Enter') createPlaylist() }}
                            autoFocus
                        />
                        <button
                            class="ctrl-btn"
                            style="padding:2px;color:var(--accent)"
                            onClick={createPlaylist}
                        >
                            <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" width="14" height="14">
                                <polyline points="20 6 9 17 4 12"/>
                            </svg>
                        </button>
                    </div>
                </div>
            )}

            <div style="padding:0 10px 8px">
                {playlists.value.length === 0 && (
                    <div style="padding:8px 8px;font-size:12px;color:var(--text-3)">
                        Sin playlists — presioná + para crear
                    </div>
                )}
                {playlists.value.map(playlist => (
                    <div
                        key={playlist.id}
                        class={`artist-item ${currentPlaylist.value?.id === playlist.id && currentView.value === 'playlist' ? 'active' : ''}`}
                        onClick={() => openPlaylist(playlist)}
                    >
                        <div class="artist-avatar" style="border-radius:4px">
                            <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" width="12" height="12">
                                <line x1="8" y1="6" x2="21" y2="6"/>
                                <line x1="8" y1="12" x2="21" y2="12"/>
                                <line x1="8" y1="18" x2="21" y2="18"/>
                                <line x1="3" y1="6" x2="3.01" y2="6"/>
                                <line x1="3" y1="12" x2="3.01" y2="12"/>
                                <line x1="3" y1="18" x2="3.01" y2="18"/>
                            </svg>
                        </div>
                        <div class="artist-name" style="flex:1">{playlist.name}</div>
                        <button
                            class="ctrl-btn"
                            style="opacity:0;padding:2px"
                            title="Eliminar"
                            onClick={(e) => deletePlaylist(e, playlist.id)}
                        >
                            <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" width="12" height="12">
                                <line x1="18" y1="6" x2="6" y2="18"/>
                                <line x1="6" y1="6" x2="18" y2="18"/>
                            </svg>
                        </button>
                    </div>
                ))}
            </div>

            <div class="nav-section" style="padding-bottom:6px">
                <div class="nav-label">Artistas</div>
            </div>

            <div class="artists-list">
                {artists.value.map(artist => (
                    <div
                        key={artist.id}
                        class={`artist-item ${currentArtist.value?.id === artist.id && currentView.value !== 'home' && currentView.value !== 'playlist' && currentView.value !== 'search' ? 'active' : ''}`}
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