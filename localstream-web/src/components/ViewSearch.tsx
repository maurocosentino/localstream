import { signal } from '@preact/signals'
import { api } from '../api'
import type { Track } from '../types'
import { currentView, currentTracks, currentIdx, searchQuery } from '../store'
import { playTrack } from './Player'
import { effect } from '@preact/signals'
import { showContextMenu } from './ContextMenu'

const searchResults = signal<Track[]>([])
const resultCount   = signal(0)

effect(() => {
    const q = searchQuery.value
    if (q.length < 2) return
    api.search(q).then(d => {
        searchResults.value = d.tracks
        resultCount.value   = d.count
        currentTracks.value = d.tracks
    })
})

function fmtTime(s: number) {
    s = Math.floor(s || 0)
    return `${Math.floor(s / 60)}:${String(s % 60).padStart(2, '0')}`
}

export function ViewSearch() {
    const idx = currentIdx.value

    return (
        <div id="view-search" class={currentView.value === 'search' ? 'active' : ''}>
            <div class="search-results-header">
                <div class="search-results-title">
                    Resultados para "{searchQuery.value}"
                </div>
                <div class="search-results-sub">
                    {resultCount.value} canciones encontradas
                </div>
            </div>

            <div class="search-results-list">
                <div class="tracks-table-head">
                    <span>#</span>
                    <span>Título</span>
                    <span>Álbum</span>
                    <span class="th-right">Duración</span>
                </div>
                <div>
                    {searchResults.value.map((track, i) => (
                        <div
                            key={track.id}
                            onContextMenu={(e) => showContextMenu(e, track.id)}
                            onClick={() => playTrack(i)}
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
                                <div class="track-artist-name"></div>
                            </div>
                            <div class="track-album-col"></div>
                            <div class="track-dur">{fmtTime(track.duration_s)}</div>
                        </div>
                    ))}
                </div>
            </div>
        </div>
    )
}