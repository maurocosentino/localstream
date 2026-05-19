import { Sidebar }    from './components/Sidebar'
import { Player }     from './components/Player'
import { ViewHome }   from './components/ViewHome'
import { ViewAlbums } from './components/ViewAlbums'
import { ViewTracks } from './components/ViewTracks'
import { ViewSearch } from './components/ViewSearch'

export function App() {
    return (
        <>
            <Sidebar />
            <main id="main">
                <ViewHome />
                <ViewAlbums />
                <ViewTracks />
                <ViewSearch />
            </main>
            <Player />
        </>
    )
}