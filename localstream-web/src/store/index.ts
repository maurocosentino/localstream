import { signal, computed } from '@preact/signals'
import type { Artist, Album, Track } from '../types'

// Estado de navegación
export const currentArtist  = signal<Artist | null>(null)
export const currentAlbum   = signal<Album | null>(null)
export const currentView    = signal<'home' | 'albums' | 'tracks' | 'search'>('home')
export const searchQuery    = signal('')

// Estado del player
export const currentTracks  = signal<Track[]>([])
export const currentIdx     = signal(-1)
export const isPlaying      = signal(false)

export const currentTrack = computed(() =>
    currentIdx.value >= 0 ? currentTracks.value[currentIdx.value] : null
)