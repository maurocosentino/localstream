import { api } from '../api'
import { currentArtist, currentAlbum, currentView, currentTracks, currentIdx } from '../store'
import { playTrack } from './Player'

function fmtTime(s: number) {
    s = Math.floor(s || 0)
    return `${Math.floor(s / 60)}:${String(s % 60).padStart(2, '0')}`
}

export function ViewTracks() {
    const album   = currentAlbum.value
    const artist  = currentArtist.value
    const tracks  = currentTracks.value
    const idx     = currentIdx.value

    const totalDuration = tracks.reduce((s, t) => s + (t.duration_s || 0), 0)

    return (
        <div id="view-tracks" class={currentView.value === 'tracks' ? 'active' : ''}>
            <div class="breadcrumb">
                <span class="breadcrumb-link" onClick={() => { currentView.value = 'home' }}>Inicio</span>
                <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                    <polyline points="9 18 15 12 9 6"/>
                </svg>
                <span
                    class="breadcrumb-link"
                    onClick={() => { currentView.value = 'albums' }}
                >
                    {artist?.name}
                </span>
                <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                    <polyline points="9 18 15 12 9 6"/>
                </svg>
                <span>{album?.title}</span>
            </div>

            <div class="tracks-header">
                <div class="album-cover-big">
                    {album
                        ? <img
                            src={api.coverUrl(album.id)}
                            onError={(e) => { (e.target as HTMLElement).style.display = 'none' }}
                          />
                        : <div class="album-cover-big-placeholder">
                            <svg viewBox="0 0 24 24" fill="none" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
                                <path d="M9 18V5l12-2v13"/>
                                <circle cx="6" cy="18" r="3"/>
                                <circle cx="18" cy="16" r="3"/>
                            </svg>
                          </div>
                    }
                </div>
                <div class="tracks-header-meta">
                    <div class="tracks-header-label">Álbum</div>
                    <div class="tracks-header-name">{album?.title}</div>
                    <div class="tracks-header-artist">{artist?.name}</div>
                    <div class="tracks-header-sub">
                        {tracks.length} canciones · {fmtTime(totalDuration)}
                    </div>
                </div>
            </div>

            <div class="tracks-actions">
                <button class="btn-play-all" onClick={() => playTrack(0)}>
                    <svg viewBox="0 0 16 16"><polygon points="3,2 14,8 3,14"/></svg>
                    Reproducir todo
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
                            onClick={() => playTrack(i)}
                        >
                            <div>
                                <div class="track-num">{track.track_number || i + 1}</div>
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
                                <div class="track-artist-name">{artist?.name}</div>
                            </div>
                            <div class="track-album-col">{album?.title}</div>
                            <div class="track-dur">{fmtTime(track.duration_s)}</div>
                        </div>
                    ))}
                </div>
            </div>
        </div>
    )
}