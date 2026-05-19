import { signal } from '@preact/signals'
import { api } from '../api'
import type { Playlist, Track } from '../types'
import { currentView, currentTracks, currentIdx } from '../store'
import { playTrack } from './Player'

export const currentPlaylist = signal<Playlist | null>(null)
const playlistTracks = signal<Track[]>([])

export async function openPlaylist(playlist: Playlist) {
    currentPlaylist.value = playlist
    currentView.value     = 'playlist'
    currentIdx.value      = -1   
    const d = await api.getPlaylistTracks(playlist.id)
    playlistTracks.value  = d.tracks
    currentTracks.value   = d.tracks
}

export async function removeFromPlaylist(trackId: number) {
    if (!currentPlaylist.value) return
    await api.removeTrackFromPlaylist(currentPlaylist.value.id, trackId)
    const d = await api.getPlaylistTracks(currentPlaylist.value.id)
    playlistTracks.value = d.tracks
    currentTracks.value  = d.tracks
}

function fmtTime(s: number) {
    s = Math.floor(s || 0)
    return `${Math.floor(s / 60)}:${String(s % 60).padStart(2, '0')}`
}

export function ViewPlaylist() {
    const playlist = currentPlaylist.value
    const tracks   = playlistTracks.value
    const idx      = currentIdx.value

    if (!playlist) return null

    const totalDuration = tracks.reduce((s, t) => s + (t.duration_s || 0), 0)

    return (
        <div id="view-playlist" class={currentView.value === 'playlist' ? 'active' : ''}>
            <div class="breadcrumb">
                <span class="breadcrumb-link" onClick={() => { currentView.value = 'home' }}>Inicio</span>
                <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                    <polyline points="9 18 15 12 9 6"/>
                </svg>
                <span>{playlist.name}</span>
            </div>

            <div class="tracks-header">
                <div class="album-cover-big">
                    <div class="album-cover-big-placeholder">
                        <svg viewBox="0 0 24 24" fill="none" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
                            <line x1="8" y1="6" x2="21" y2="6"/>
                            <line x1="8" y1="12" x2="21" y2="12"/>
                            <line x1="8" y1="18" x2="21" y2="18"/>
                            <line x1="3" y1="6" x2="3.01" y2="6"/>
                            <line x1="3" y1="12" x2="3.01" y2="12"/>
                            <line x1="3" y1="18" x2="3.01" y2="18"/>
                        </svg>
                    </div>
                </div>
                <div class="tracks-header-meta">
                    <div class="tracks-header-label">Playlist</div>
                    <div class="tracks-header-name">{playlist.name}</div>
                    <div class="tracks-header-sub">
                        {tracks.length} canciones · {fmtTime(totalDuration)}
                    </div>
                </div>
            </div>

            <div class="tracks-actions">
                <button class="btn-play-all" onClick={() => { currentTracks.value = tracks; playTrack(0) }}>
                    <svg viewBox="0 0 16 16"><polygon points="3,2 14,8 3,14"/></svg>
                    Reproducir todo
                </button>
                <button class="btn-shuffle" onClick={() => {
                    currentTracks.value = [...tracks].sort(() => Math.random() - 0.5)
                    playTrack(0)
                }}>
                    <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                        <polyline points="16 3 21 3 21 8"/>
                        <line x1="4" y1="20" x2="21" y2="3"/>
                        <polyline points="21 16 21 21 16 21"/>
                        <line x1="15" y1="15" x2="21" y2="21"/>
                    </svg>
                    Aleatorio
                </button>
            </div>

            <div class="tracks-list">
                <div class="tracks-table-head">
                    <span>#</span>
                    <span>Título</span>
                    <span>Álbum</span>
                    <span class="th-right">Duración</span>
                </div>
                <div>
                    {tracks.map((track, i) => (
                        <div
                            key={track.id}
                            class={`track-row ${idx === i ? 'active' : ''}`}
                            onClick={() => { currentTracks.value = tracks; playTrack(i) }}
                        >
                            <div>
                                <div class="track-num">{i + 1}</div>
                                {idx === i && (
                                    <div class="track-playing-icon">
                                        <svg viewBox="0 0 16 16" fill="currentColor">
                                            <polygon points="3,2 14,8 3,14"/>
                                        </svg>
                                    </div>
                                )}
                            </div>
                            <div class="track-info">
                                <div class="track-title">{track.title}</div>
                            </div>
                            <div class="track-album-col"></div>
                            <div style="display:flex;align-items:center;gap:8px;justify-content:flex-end">
                                <span class="track-dur">{fmtTime(track.duration_s)}</span>
                                <button
                                    class="ctrl-btn"
                                    style="opacity:0.4;padding:2px"
                                    title="Quitar de playlist"
                                    onClick={(e) => { e.stopPropagation(); removeFromPlaylist(track.id) }}
                                >
                                    <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" width="14" height="14">
                                        <line x1="18" y1="6" x2="6" y2="18"/>
                                        <line x1="6" y1="6" x2="18" y2="18"/>
                                    </svg>
                                </button>
                            </div>
                        </div>
                    ))}
                    {tracks.length === 0 && (
                        <div class="empty-state">
                            <p>Esta playlist está vacía — agregá tracks con click derecho</p>
                        </div>
                    )}
                </div>
            </div>
        </div>
    )
}