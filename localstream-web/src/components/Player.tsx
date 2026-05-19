import { api } from '../api'
import { currentTrack, currentTracks, currentIdx, isPlaying, currentAlbum } from '../store'
import { effect } from '@preact/signals'

export const audio = new Audio()
audio.volume = 0.8

// Sincronizar audio con el store
effect(() => {
    const track = currentTrack.value
    if (!track) return
    audio.src = api.streamUrl(track.id)
    audio.play()
    isPlaying.value = true
})

export function playTrack(idx: number) {
    if (idx < 0 || idx >= currentTracks.value.length) return
    currentIdx.value = idx
}

export function prevTrack() { playTrack(currentIdx.value - 1) }
export function nextTrack() { playTrack(currentIdx.value + 1) }

audio.addEventListener('ended', () => nextTrack())
audio.addEventListener('play',  () => { isPlaying.value = true })
audio.addEventListener('pause', () => { isPlaying.value = false })

function fmtTime(s: number) {
    s = Math.floor(s || 0)
    return `${Math.floor(s / 60)}:${String(s % 60).padStart(2, '0')}`
}

import { signal } from '@preact/signals'
const timeCur  = signal('0:00')
const timeDur  = signal('0:00')
const progress = signal(0)

audio.addEventListener('timeupdate', () => {
    if (!audio.duration) return
    progress.value = (audio.currentTime / audio.duration) * 100
    timeCur.value  = fmtTime(audio.currentTime)
    timeDur.value  = fmtTime(audio.duration)
})

function seekTo(e: MouseEvent) {
    if (!audio.duration) return
    const bar  = e.currentTarget as HTMLElement
    const rect = bar.getBoundingClientRect()
    audio.currentTime = ((e.clientX - rect.left) / rect.width) * audio.duration
}

export function Player() {
    const track  = currentTrack.value
    const album  = currentAlbum.value
    const playing = isPlaying.value

    return (
        <div id="player">
            <div class="player-track">
                <div class="player-thumb">
                    {album
                        ? <img src={api.coverUrl(album.id)} style="width:100%;height:100%;object-fit:cover"
                               onError={(e) => { (e.target as HTMLElement).style.display = 'none' }} />
                        : <div class="player-thumb-placeholder">
                            <svg viewBox="0 0 24 24" fill="none" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
                                <path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/>
                            </svg>
                          </div>
                    }
                </div>
                <div class="player-track-info">
                    {track
                        ? <>
                            <div class="player-track-title">{track.title}</div>
                            <div class="player-track-artist">{currentAlbum.value?.title ?? ''}</div>
                          </>
                        : <div class="player-idle-text">Sin reproducción</div>
                    }
                </div>
            </div>

            <div class="player-controls">
                <div class="ctrl-buttons">
                    <button class="ctrl-btn" onClick={prevTrack}>
                        <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                            <polygon points="19 20 9 12 19 4 19 20"/><line x1="5" y1="19" x2="5" y2="5"/>
                        </svg>
                    </button>
                    <button class="ctrl-btn-play" onClick={() => playing ? audio.pause() : audio.play()}>
                        {playing
                            ? <svg viewBox="0 0 16 16" fill="currentColor">
                                <rect x="3" y="2" width="4" height="12"/><rect x="9" y="2" width="4" height="12"/>
                              </svg>
                            : <svg viewBox="0 0 16 16" fill="currentColor" style="margin-left:2px">
                                <polygon points="3,2 14,8 3,14"/>
                              </svg>
                        }
                    </button>
                    <button class="ctrl-btn" onClick={nextTrack}>
                        <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                            <polygon points="5 4 15 12 5 20 5 4"/><line x1="19" y1="5" x2="19" y2="19"/>
                        </svg>
                    </button>
                </div>
                <div class="progress-row">
                    <span class="progress-time">{timeCur.value}</span>
                    <div class="progress-bar" onClick={seekTo}>
                        <div class="progress-fill" style={{ width: `${progress.value}%` }}></div>
                    </div>
                    <span class="progress-time right">{timeDur.value}</span>
                </div>
            </div>

            <div class="player-right">
                <div class="volume-wrap">
                    <button class="vol-btn" onClick={() => { audio.muted = !audio.muted }}>
                        <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                            <polygon points="11 5 6 9 2 9 2 15 6 15 11 19 11 5"/>
                            <path d="M15.54 8.46a5 5 0 010 7.07"/>
                            <path d="M19.07 4.93a10 10 0 010 14.14"/>
                        </svg>
                    </button>
                    <input type="range" class="volume-slider" min="0" max="100" value="80"
                           onInput={(e) => { audio.volume = parseInt((e.target as HTMLInputElement).value) / 100 }} />
                </div>
            </div>
        </div>
    )
}