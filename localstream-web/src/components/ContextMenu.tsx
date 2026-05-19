import { signal } from '@preact/signals'
import { api } from '../api'
import type { Playlist } from '../types'

interface ContextMenuState {
    visible: boolean
    x: number
    y: number
    trackId: number
}

const menuState  = signal<ContextMenuState>({ visible: false, x: 0, y: 0, trackId: -1 })
const playlists  = signal<Playlist[]>([])
const feedback   = signal('')

export function showContextMenu(e: MouseEvent, trackId: number) {
    e.preventDefault()
    e.stopPropagation()

    // Cargar playlists actuales
    api.getPlaylists().then(d => { playlists.value = d.playlists })

    // Posición del menú — evitamos que se salga de la pantalla
    const x = Math.min(e.clientX, window.innerWidth  - 200)
    const y = Math.min(e.clientY, window.innerHeight - 300)

    menuState.value = { visible: true, x, y, trackId }
    feedback.value  = ''
}

function hideMenu() {
    menuState.value = { ...menuState.value, visible: false }
}

async function addToPlaylist(playlistId: number, playlistName: string) {
    await api.addTrackToPlaylist(playlistId, menuState.value.trackId)
    feedback.value = `Agregado a "${playlistName}"`
    setTimeout(() => hideMenu(), 800)
}

// Cerrar al clickear afuera
if (typeof window !== 'undefined') {
    window.addEventListener('click', () => {
        if (menuState.value.visible) hideMenu()
    })
    window.addEventListener('keydown', (e) => {
        if (e.key === 'Escape') hideMenu()
    })
}

export function ContextMenu() {
    const menu = menuState.value
    if (!menu.visible) return null

    return (
        <div
            class="context-menu"
            style={{ left: `${menu.x}px`, top: `${menu.y}px` }}
            onClick={(e) => e.stopPropagation()}
        >
            {feedback.value
                ? <div class="context-menu-feedback">{feedback.value}</div>
                : <>
                    <div class="context-menu-title">Agregar a playlist</div>
                    {playlists.value.length === 0
                        ? <div class="context-menu-empty">Sin playlists — creá una primero</div>
                        : playlists.value.map(p => (
                            <div
                                key={p.id}
                                class="context-menu-item"
                                onClick={() => addToPlaylist(p.id, p.name)}
                            >
                                <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" width="14" height="14">
                                    <line x1="8" y1="6" x2="21" y2="6"/>
                                    <line x1="8" y1="12" x2="21" y2="12"/>
                                    <line x1="8" y1="18" x2="21" y2="18"/>
                                    <line x1="3" y1="6" x2="3.01" y2="6"/>
                                    <line x1="3" y1="12" x2="3.01" y2="12"/>
                                    <line x1="3" y1="18" x2="3.01" y2="18"/>
                                </svg>
                                {p.name}
                            </div>
                        ))
                    }
                  </>
            }
        </div>
    )
}