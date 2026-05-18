#include "api/WebHandler.hpp"

namespace localstream {

WebHandler::WebHandler(Database& db, crow::SimpleApp& app)
    : db_(db), app_(app)
{
    setupRoutes();
}

void WebHandler::setupRoutes()
{
    CROW_ROUTE(app_, "/")
    ([]{
        std::string html = R"html(<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>LocalStream</title>
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link href="https://fonts.googleapis.com/css2?family=DM+Sans:wght@300;400;500;600&family=DM+Mono:wght@400;500&display=swap" rel="stylesheet">
    <style>
        :root {
            --bg-base:    #0d0d0e;
            --bg-surface: #141416;
            --bg-card:    #1a1a1e;
            --bg-hover:   #222228;
            --bg-active:  #28282f;
            --accent:     #c8a96e;
            --accent-dim: rgba(200,169,110,0.15);
            --accent-glow:rgba(200,169,110,0.08);
            --border:     rgba(255,255,255,0.06);
            --border-mid: rgba(255,255,255,0.10);
            --text-1:     #f0ede8;
            --text-2:     #9e9a93;
            --text-3:     #565450;
            --radius:     10px;
            --radius-sm:  6px;
            --sidebar-w:  220px;
            --player-h:   90px;
        }
        * { box-sizing: border-box; margin: 0; padding: 0; }

        body {
            font-family: 'DM Sans', sans-serif;
            background: var(--bg-base);
            color: var(--text-1);
            height: 100vh;
            display: grid;
            grid-template-rows: 1fr var(--player-h);
            grid-template-columns: var(--sidebar-w) 1fr;
            overflow: hidden;
        }

        /* ── SIDEBAR ── */
        #sidebar {
            grid-row: 1;
            grid-column: 1;
            background: var(--bg-surface);
            border-right: 1px solid var(--border);
            display: flex;
            flex-direction: column;
            overflow: hidden;
        }

        .logo {
            padding: 24px 20px 20px;
            border-bottom: 1px solid var(--border);
        }
        .logo-mark {
            display: flex; align-items: center; gap: 10px;
        }
        .logo-icon {
            width: 30px; height: 30px;
            background: var(--accent);
            border-radius: 8px;
            display: flex; align-items: center; justify-content: center;
        }
        .logo-icon svg { width: 16px; height: 16px; fill: #0d0d0e; }
        .logo-text {
            font-size: 15px; font-weight: 600;
            letter-spacing: 0.02em; color: var(--text-1);
        }
        .logo-sub {
            font-size: 11px; color: var(--text-3);
            margin-top: 2px; letter-spacing: 0.04em;
        }

        .search-wrap {
            padding: 14px 14px 10px;
        }
        .search-box {
            display: flex; align-items: center; gap: 8px;
            background: var(--bg-card);
            border: 1px solid var(--border);
            border-radius: var(--radius-sm);
            padding: 0 10px; height: 34px;
        }
        .search-box svg { width: 14px; height: 14px; stroke: var(--text-3); flex-shrink: 0; }
        .search-box input {
            background: none; border: none; outline: none;
            font-family: inherit; font-size: 13px; color: var(--text-1);
            width: 100%;
        }
        .search-box input::placeholder { color: var(--text-3); }

        .nav-section {
            padding: 10px 10px 4px;
        }
        .nav-label {
            font-size: 10px; font-weight: 600; color: var(--text-3);
            text-transform: uppercase; letter-spacing: 0.12em;
            padding: 0 8px 6px;
        }

        .nav-item {
            display: flex; align-items: center; gap: 9px;
            padding: 7px 8px; border-radius: var(--radius-sm);
            font-size: 13px; color: var(--text-2);
            cursor: pointer; transition: all 0.15s;
            margin-bottom: 1px;
        }
        .nav-item svg { width: 15px; height: 15px; stroke: currentColor; flex-shrink: 0; }
        .nav-item:hover { background: var(--bg-hover); color: var(--text-1); }
        .nav-item.active { background: var(--accent-dim); color: var(--accent); }

        .artists-list {
            flex: 1; overflow-y: auto; padding: 4px 10px 10px;
        }
        .artists-list::-webkit-scrollbar { width: 3px; }
        .artists-list::-webkit-scrollbar-track { background: transparent; }
        .artists-list::-webkit-scrollbar-thumb { background: var(--border-mid); border-radius: 99px; }

        .artist-item {
            display: flex; align-items: center; gap: 10px;
            padding: 7px 8px; border-radius: var(--radius-sm);
            cursor: pointer; transition: all 0.15s;
            margin-bottom: 1px;
        }
        .artist-avatar {
            width: 30px; height: 30px; border-radius: 50%;
            background: var(--bg-active);
            display: flex; align-items: center; justify-content: center;
            font-size: 12px; font-weight: 600; color: var(--text-2);
            flex-shrink: 0; overflow: hidden;
            border: 1px solid var(--border);
        }
        .artist-avatar img { width: 100%; height: 100%; object-fit: cover; }
        .artist-name {
            font-size: 13px; color: var(--text-2);
            white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
        }
        .artist-item:hover { background: var(--bg-hover); }
        .artist-item:hover .artist-name { color: var(--text-1); }
        .artist-item.active { background: var(--accent-dim); }
        .artist-item.active .artist-name { color: var(--accent); font-weight: 500; }
        .artist-item.active .artist-avatar { border-color: var(--accent); color: var(--accent); }

        /* ── MAIN AREA ── */
        #main {
            grid-row: 1;
            grid-column: 2;
            display: grid;
            grid-template-rows: 1fr;
            overflow: hidden;
        }

        /* State: home / albums / tracks */
        #view-home, #view-albums, #view-tracks {
            display: none;
            flex-direction: column;
            overflow: hidden;
            height: 100%;
        }
        #view-home.active, #view-albums.active, #view-tracks.active {
            display: flex;
        }

        /* ── VIEW HOME ── */
        .home-hero {
            padding: 40px 36px 28px;
        }
        .home-greeting {
            font-size: 28px; font-weight: 600; color: var(--text-1);
            letter-spacing: -0.02em; margin-bottom: 4px;
        }
        .home-sub {
            font-size: 14px; color: var(--text-2);
        }
        .home-sub span { color: var(--accent); font-weight: 500; }

        .section-header {
            display: flex; align-items: center; justify-content: space-between;
            padding: 0 36px 16px;
        }
        .section-title {
            font-size: 16px; font-weight: 600; letter-spacing: -0.01em;
        }
        .section-action {
            font-size: 12px; color: var(--text-2); cursor: pointer;
            transition: color 0.15s;
        }
        .section-action:hover { color: var(--accent); }

        .albums-grid {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(150px, 1fr));
            gap: 16px;
            padding: 0 36px 36px;
            overflow-y: auto;
        }
        .albums-grid::-webkit-scrollbar { width: 3px; }
        .albums-grid::-webkit-scrollbar-thumb { background: var(--border-mid); border-radius: 99px; }

        .album-card {
            cursor: pointer; border-radius: var(--radius);
            overflow: hidden; transition: transform 0.2s;
        }
        .album-card:hover { transform: translateY(-2px); }
        .album-thumb {
            width: 100%; aspect-ratio: 1;
            background: var(--bg-card);
            border-radius: var(--radius);
            overflow: hidden; margin-bottom: 10px;
            border: 1px solid var(--border);
            position: relative;
        }
        .album-thumb img {
            width: 100%; height: 100%; object-fit: cover;
            transition: opacity 0.2s;
        }
        .album-thumb-placeholder {
            width: 100%; height: 100%;
            display: flex; align-items: center; justify-content: center;
        }
        .album-thumb-placeholder svg {
            width: 36px; height: 36px; stroke: var(--text-3);
        }
        .album-play-btn {
            position: absolute; bottom: 8px; right: 8px;
            width: 34px; height: 34px; border-radius: 50%;
            background: var(--accent); border: none; cursor: pointer;
            display: flex; align-items: center; justify-content: center;
            opacity: 0; transform: translateY(4px);
            transition: opacity 0.2s, transform 0.2s;
            box-shadow: 0 4px 12px rgba(0,0,0,0.4);
        }
        .album-play-btn svg { width: 14px; height: 14px; fill: #0d0d0e; margin-left: 2px; }
        .album-card:hover .album-play-btn {
            opacity: 1; transform: translateY(0);
        }
        .album-info-title {
            font-size: 13px; font-weight: 500; color: var(--text-1);
            white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
            margin-bottom: 2px;
        }
        .album-info-artist {
            font-size: 12px; color: var(--text-2);
            white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
        }

        /* ── VIEW ALBUMS ── */
        .view-header {
            padding: 32px 36px 0;
            display: flex; align-items: flex-start; gap: 24px;
        }
        .artist-cover {
            width: 100px; height: 100px; border-radius: 50%;
            background: var(--bg-card); border: 1px solid var(--border);
            overflow: hidden; flex-shrink: 0;
            display: flex; align-items: center; justify-content: center;
        }
        .artist-cover img { width: 100%; height: 100%; object-fit: cover; }
        .artist-cover-placeholder svg { width: 42px; height: 42px; stroke: var(--text-3); }
        .view-header-meta { padding-top: 12px; }
        .view-header-label {
            font-size: 11px; font-weight: 600; color: var(--text-3);
            text-transform: uppercase; letter-spacing: 0.1em; margin-bottom: 6px;
        }
        .view-header-name {
            font-size: 28px; font-weight: 600;
            letter-spacing: -0.02em; margin-bottom: 6px;
        }
        .view-header-sub { font-size: 13px; color: var(--text-2); }

        .albums-list {
            flex: 1; overflow-y: auto; padding: 24px 36px 24px;
        }
        .albums-list::-webkit-scrollbar { width: 3px; }
        .albums-list::-webkit-scrollbar-thumb { background: var(--border-mid); border-radius: 99px; }

        .album-row {
            display: flex; align-items: center; gap: 14px;
            padding: 10px 12px; border-radius: var(--radius-sm);
            cursor: pointer; transition: background 0.15s; margin-bottom: 2px;
        }
        .album-row:hover { background: var(--bg-hover); }
        .album-row.active { background: var(--accent-dim); }
        .album-row-thumb {
            width: 46px; height: 46px; border-radius: 6px;
            background: var(--bg-card); border: 1px solid var(--border);
            overflow: hidden; flex-shrink: 0;
            display: flex; align-items: center; justify-content: center;
        }
        .album-row-thumb img { width: 100%; height: 100%; object-fit: cover; }
        .album-row-thumb svg { width: 20px; height: 20px; stroke: var(--text-3); }
        .album-row-info { flex: 1; min-width: 0; }
        .album-row-title {
            font-size: 14px; font-weight: 500; color: var(--text-1);
            white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
        }
        .album-row.active .album-row-title { color: var(--accent); }
        .album-row-year { font-size: 12px; color: var(--text-2); margin-top: 2px; }
        .album-row-tracks { font-size: 12px; color: var(--text-3); white-space: nowrap; }

        /* ── VIEW TRACKS ── */
        .tracks-header {
            padding: 32px 36px 0;
            display: flex; align-items: flex-start; gap: 24px;
        }
        .album-cover-big {
            width: 130px; height: 130px; border-radius: var(--radius);
            background: var(--bg-card); border: 1px solid var(--border);
            overflow: hidden; flex-shrink: 0;
            display: flex; align-items: center; justify-content: center;
            box-shadow: 0 12px 40px rgba(0,0,0,0.5);
        }
        .album-cover-big img { width: 100%; height: 100%; object-fit: cover; }
        .album-cover-big-placeholder svg { width: 52px; height: 52px; stroke: var(--text-3); }
        .tracks-header-meta { padding-top: 8px; }
        .tracks-header-label {
            font-size: 11px; font-weight: 600; color: var(--text-3);
            text-transform: uppercase; letter-spacing: 0.1em; margin-bottom: 6px;
        }
        .tracks-header-name {
            font-size: 26px; font-weight: 600; letter-spacing: -0.02em; margin-bottom: 4px;
        }
        .tracks-header-artist { font-size: 14px; color: var(--accent); font-weight: 500; margin-bottom: 6px; }
        .tracks-header-sub { font-size: 13px; color: var(--text-2); }

        .tracks-actions {
            padding: 20px 36px 8px;
            display: flex; align-items: center; gap: 12px;
        }
        .btn-play-all {
            display: flex; align-items: center; gap: 7px;
            background: var(--accent); color: #0d0d0e;
            border: none; border-radius: 99px; padding: 9px 20px;
            font-family: inherit; font-size: 13px; font-weight: 600;
            cursor: pointer; transition: opacity 0.15s;
        }
        .btn-play-all:hover { opacity: 0.88; }
        .btn-play-all svg { width: 14px; height: 14px; fill: #0d0d0e; }

        .tracks-list {
            flex: 1; overflow-y: auto; padding: 8px 24px 24px;
        }
        .tracks-list::-webkit-scrollbar { width: 3px; }
        .tracks-list::-webkit-scrollbar-thumb { background: var(--border-mid); border-radius: 99px; }

        .tracks-table-head {
            display: grid;
            grid-template-columns: 36px 1fr 100px 60px;
            padding: 6px 12px;
            border-bottom: 1px solid var(--border);
            margin-bottom: 4px;
        }
        .tracks-table-head span {
            font-size: 11px; color: var(--text-3); font-weight: 500;
            text-transform: uppercase; letter-spacing: 0.08em;
        }
        .tracks-table-head .th-right { text-align: right; }

        .track-row {
            display: grid;
            grid-template-columns: 36px 1fr 100px 60px;
            align-items: center;
            padding: 7px 12px; border-radius: var(--radius-sm);
            cursor: pointer; transition: background 0.12s; margin-bottom: 1px;
        }
        .track-row:hover { background: var(--bg-hover); }
        .track-row.active { background: var(--accent-dim); }

        .track-num {
            font-size: 13px; color: var(--text-3);
            font-family: 'DM Mono', monospace;
            text-align: center;
        }
        .track-row.active .track-num { display: none; }
        .track-playing-icon {
            display: none; justify-content: center;
        }
        .track-row.active .track-playing-icon { display: flex; }
        .track-playing-icon svg { width: 14px; height: 14px; fill: var(--accent); }

        .track-info { min-width: 0; }
        .track-title {
            font-size: 14px; color: var(--text-1);
            white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
        }
        .track-row.active .track-title { color: var(--accent); font-weight: 500; }
        .track-artist-name {
            font-size: 12px; color: var(--text-2); margin-top: 1px;
        }

        .track-album-col {
            font-size: 12px; color: var(--text-2);
            white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
        }
        .track-dur {
            font-size: 13px; color: var(--text-3);
            font-family: 'DM Mono', monospace;
            text-align: right;
        }
        .track-row.active .track-dur { color: var(--accent); }

        /* ── SEARCH RESULTS ── */
        #view-search {
            display: none; flex-direction: column; overflow: hidden; height: 100%;
        }
        #view-search.active { display: flex; }
        .search-results-header {
            padding: 32px 36px 20px;
        }
        .search-results-title {
            font-size: 22px; font-weight: 600; letter-spacing: -0.02em;
        }
        .search-results-sub { font-size: 14px; color: var(--text-2); margin-top: 4px; }
        .search-results-list {
            flex: 1; overflow-y: auto; padding: 0 24px 24px;
        }

        /* ── PLAYER ── */
        #player {
            grid-row: 2;
            grid-column: 1 / -1;
            background: var(--bg-surface);
            border-top: 1px solid var(--border);
            display: grid;
            grid-template-columns: 1fr 2fr 1fr;
            align-items: center;
            padding: 0 24px;
            gap: 20px;
        }

        .player-track {
            display: flex; align-items: center; gap: 12px; min-width: 0;
        }
        .player-thumb {
            width: 50px; height: 50px; border-radius: 6px;
            background: var(--bg-card); border: 1px solid var(--border);
            overflow: hidden; flex-shrink: 0;
            display: flex; align-items: center; justify-content: center;
        }
        .player-thumb img { width: 100%; height: 100%; object-fit: cover; }
        .player-thumb-placeholder svg { width: 20px; height: 20px; stroke: var(--text-3); }
        .player-track-info { min-width: 0; }
        .player-track-title {
            font-size: 14px; font-weight: 500; color: var(--text-1);
            white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
        }
        .player-track-artist { font-size: 12px; color: var(--text-2); margin-top: 2px; }
        .player-idle-text { font-size: 13px; color: var(--text-3); }

        .player-controls { display: flex; flex-direction: column; gap: 10px; align-items: center; }
        .ctrl-buttons { display: flex; align-items: center; gap: 16px; }
        .ctrl-btn {
            background: none; border: none; cursor: pointer;
            color: var(--text-2); padding: 4px;
            display: flex; align-items: center; justify-content: center;
            transition: color 0.15s;
        }
        .ctrl-btn:hover { color: var(--text-1); }
        .ctrl-btn svg { width: 18px; height: 18px; stroke: currentColor; }
        .ctrl-btn-play {
            width: 38px; height: 38px; border-radius: 50%;
            background: var(--text-1); color: var(--bg-base);
            display: flex; align-items: center; justify-content: center;
            border: none; cursor: pointer; transition: transform 0.15s, background 0.15s;
        }
        .ctrl-btn-play:hover { transform: scale(1.06); background: #fff; }
        .ctrl-btn-play svg { width: 16px; height: 16px; fill: var(--bg-base); margin-left: 2px; }
        .ctrl-btn-play.paused svg { margin-left: 2px; }

        .progress-row {
            display: flex; align-items: center; gap: 10px; width: 100%;
        }
        .progress-time {
            font-size: 11px; color: var(--text-3);
            font-family: 'DM Mono', monospace; min-width: 36px;
        }
        .progress-time.right { text-align: right; }
        .progress-bar {
            flex: 1; height: 3px; background: var(--bg-active);
            border-radius: 99px; cursor: pointer; position: relative;
        }
        .progress-fill {
            height: 100%; background: var(--text-2);
            border-radius: 99px; width: 0%;
            transition: background 0.15s;
            pointer-events: none;
        }
        .progress-bar:hover .progress-fill { background: var(--accent); }

        .player-right { display: flex; align-items: center; gap: 10px; justify-content: flex-end; }
        .volume-wrap { display: flex; align-items: center; gap: 8px; }
        .vol-btn { background: none; border: none; cursor: pointer; color: var(--text-2); transition: color 0.15s; }
        .vol-btn:hover { color: var(--text-1); }
        .vol-btn svg { width: 16px; height: 16px; stroke: currentColor; display: block; }
        .volume-slider {
            -webkit-appearance: none; appearance: none;
            width: 80px; height: 3px; border-radius: 99px;
            background: var(--bg-active); outline: none; cursor: pointer;
        }
        .volume-slider::-webkit-slider-thumb {
            -webkit-appearance: none; appearance: none;
            width: 12px; height: 12px; border-radius: 50%;
            background: var(--text-2); cursor: pointer;
            transition: background 0.15s;
        }
        .volume-slider:hover::-webkit-slider-thumb { background: var(--accent); }

        /* Empty states */
        .empty-state {
            display: flex; flex-direction: column; align-items: center;
            justify-content: center; height: 100%; gap: 12px; padding: 40px;
        }
        .empty-state svg { width: 48px; height: 48px; stroke: var(--text-3); opacity: 0.5; }
        .empty-state p { font-size: 14px; color: var(--text-3); text-align: center; }

        /* Breadcrumb */
        .breadcrumb {
            display: flex; align-items: center; gap: 6px;
            padding: 12px 36px 0;
            font-size: 12px; color: var(--text-3);
        }
        .breadcrumb-link { cursor: pointer; transition: color 0.15s; }
        .breadcrumb-link:hover { color: var(--text-2); }
        .breadcrumb svg { width: 12px; height: 12px; stroke: currentColor; }
    </style>
</head>
<body>

<!-- SIDEBAR -->
<aside id="sidebar">
    <div class="logo">
        <div class="logo-mark">
            <div class="logo-icon">
                <svg viewBox="0 0 16 16"><path d="M8 2a6 6 0 100 12A6 6 0 008 2zM6.5 5.5a1.5 1.5 0 113 0 1.5 1.5 0 01-3 0zM8 12a4 4 0 01-3.46-6h6.92A4 4 0 018 12z"/></svg>
            </div>
            <div>
                <div class="logo-text">LocalStream</div>
                <div class="logo-sub" id="lib-stats">cargando...</div>
            </div>
        </div>
    </div>

    <div class="search-wrap">
        <div class="search-box">
            <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                <circle cx="11" cy="11" r="8"/><line x1="21" y1="21" x2="16.65" y2="16.65"/>
            </svg>
            <input type="text" id="search-input" placeholder="Buscar..." oninput="onSearch(this.value)">
        </div>
    </div>

    <div class="nav-section">
        <div class="nav-label">Navegar</div>
        <div class="nav-item active" id="nav-home" onclick="showHome()">
            <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                <path d="M3 9l9-7 9 7v11a2 2 0 01-2 2H5a2 2 0 01-2-2z"/><polyline points="9 22 9 12 15 12 15 22"/>
            </svg>
            Inicio
        </div>
    </div>

    <div class="nav-section" style="padding-bottom:6px;">
        <div class="nav-label">Artistas</div>
    </div>
    <div class="artists-list" id="artists-list">
        <div class="empty-state" style="height:auto;padding:20px 8px;">
            <p>Cargando...</p>
        </div>
    </div>
</aside>

<!-- MAIN -->
<main id="main">

    <!-- HOME -->
    <div id="view-home" class="active">
        <div class="home-hero">
            <div class="home-greeting">Escuchar ahora</div>
            <div class="home-sub">Tu biblioteca local · <span id="home-stats">...</span></div>
        </div>
        <div class="section-header">
            <div class="section-title">Álbumes recientes</div>
        </div>
        <div class="albums-grid" id="home-albums-grid">
        </div>
    </div>

    <!-- ALBUMS OF ARTIST -->
    <div id="view-albums">
        <div class="breadcrumb">
            <span class="breadcrumb-link" onclick="showHome()">Inicio</span>
            <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="9 18 15 12 9 6"/></svg>
            <span id="breadcrumb-artist"></span>
        </div>
        <div class="view-header">
            <div class="artist-cover">
                <div class="artist-cover-placeholder">
                    <svg viewBox="0 0 24 24" fill="none" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
                        <path d="M20 21v-2a4 4 0 00-4-4H8a4 4 0 00-4 4v2"/><circle cx="12" cy="7" r="4"/>
                    </svg>
                </div>
            </div>
            <div class="view-header-meta">
                <div class="view-header-label">Artista</div>
                <div class="view-header-name" id="artist-view-name"></div>
                <div class="view-header-sub" id="artist-view-sub"></div>
            </div>
        </div>
        <div class="albums-list" id="albums-list"></div>
    </div>

    <!-- TRACKS OF ALBUM -->
    <div id="view-tracks">
        <div class="breadcrumb">
            <span class="breadcrumb-link" onclick="showHome()">Inicio</span>
            <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="9 18 15 12 9 6"/></svg>
            <span class="breadcrumb-link" id="breadcrumb-artist2" onclick="backToArtist()"></span>
            <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="9 18 15 12 9 6"/></svg>
            <span id="breadcrumb-album"></span>
        </div>
        <div class="tracks-header">
            <div class="album-cover-big">
                <div class="album-cover-big-placeholder" id="album-cover-big-ph">
                    <svg viewBox="0 0 24 24" fill="none" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
                        <path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/>
                    </svg>
                </div>
                <img id="album-cover-big-img" style="display:none;width:100%;height:100%;object-fit:cover;">
            </div>
            <div class="tracks-header-meta">
                <div class="tracks-header-label">Álbum</div>
                <div class="tracks-header-name" id="album-view-name"></div>
                <div class="tracks-header-artist" id="album-view-artist"></div>
                <div class="tracks-header-sub" id="album-view-sub"></div>
            </div>
        </div>
        <div class="tracks-actions">
            <button class="btn-play-all" onclick="playAll()">
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
            <div id="tracks-list-body"></div>
        </div>
    </div>

    <!-- SEARCH RESULTS -->
    <div id="view-search">
        <div class="search-results-header">
            <div class="search-results-title" id="search-title">Resultados</div>
            <div class="search-results-sub" id="search-sub"></div>
        </div>
        <div class="search-results-list">
            <div class="tracks-table-head">
                <span>#</span>
                <span>Título</span>
                <span>Álbum</span>
                <span class="th-right">Duración</span>
            </div>
            <div id="search-results-body"></div>
        </div>
    </div>

</main>

<!-- PLAYER -->
<div id="player">
    <div class="player-track">
        <div class="player-thumb" id="player-thumb">
            <div class="player-thumb-placeholder">
                <svg viewBox="0 0 24 24" fill="none" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
                    <path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/>
                </svg>
            </div>
        </div>
        <div class="player-track-info">
            <div class="player-idle-text" id="player-title">Sin reproducción</div>
            <div class="player-track-artist" id="player-artist" style="display:none"></div>
        </div>
    </div>

    <div class="player-controls">
        <div class="ctrl-buttons">
            <button class="ctrl-btn" onclick="prevTrack()" title="Anterior">
                <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                    <polygon points="19 20 9 12 19 4 19 20"/><line x1="5" y1="19" x2="5" y2="5"/>
                </svg>
            </button>
            <button class="ctrl-btn-play" id="play-btn" onclick="togglePlay()">
                <svg id="play-icon" viewBox="0 0 16 16"><polygon points="3,2 14,8 3,14"/></svg>
            </button>
            <button class="ctrl-btn" onclick="nextTrack()" title="Siguiente">
                <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                    <polygon points="5 4 15 12 5 20 5 4"/><line x1="19" y1="5" x2="19" y2="19"/>
                </svg>
            </button>
        </div>
        <div class="progress-row">
            <span class="progress-time" id="time-cur">0:00</span>
            <div class="progress-bar" id="progress-bar" onclick="seekTo(event)">
                <div class="progress-fill" id="progress-fill"></div>
            </div>
            <span class="progress-time right" id="time-dur">0:00</span>
        </div>
    </div>

    <div class="player-right">
        <div class="volume-wrap">
            <button class="vol-btn" onclick="toggleMute()">
                <svg id="vol-icon" viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                    <polygon points="11 5 6 9 2 9 2 15 6 15 11 19 11 5"/>
                    <path d="M15.54 8.46a5 5 0 010 7.07"/>
                    <path d="M19.07 4.93a10 10 0 010 14.14"/>
                </svg>
            </button>
            <input type="range" class="volume-slider" id="vol-slider" min="0" max="100" value="80" oninput="setVolume(this.value)">
        </div>
    </div>
</div>

<audio id="audio" preload="none"></audio>

<script>
const audio = document.getElementById('audio');
audio.volume = 0.8;

let currentArtist = null;
let currentAlbum  = null;
let currentTracks = [];
let currentIdx    = -1;
let searchTimeout = null;

// ── FORMAT ──
function fmtTime(s) {
    s = Math.floor(s || 0);
    return Math.floor(s / 60) + ':' + String(s % 60).padStart(2, '0');
}

// ── THUMBNAIL ──
function thumbUrl(albumId) {
    return '/api/albums/' + albumId + '/cover';
}

function mkThumb(albumId, cls, size) {
    const img = document.createElement('img');
    img.src = thumbUrl(albumId);
    img.style.cssText = 'width:100%;height:100%;object-fit:cover;';
    img.onerror = function() { this.style.display = 'none'; };
    return img;
}

// ── VIEWS ──
function setView(id) {
    ['view-home','view-albums','view-tracks','view-search'].forEach(v => {
        document.getElementById(v).classList.toggle('active', v === id);
    });
    document.getElementById('nav-home').classList.toggle('active', id === 'view-home');
}

function showHome() { setView('view-home'); }

function backToArtist() {
    if (currentArtist) loadAlbums(currentArtist);
}

// ── LOAD ARTISTS ──
async function init() {
    const [ra, rt] = await Promise.all([
        fetch('/api/artists').then(r => r.json()),
        fetch('/api/tracks?limit=1&offset=0').then(r => r.json())
    ]);

    document.getElementById('lib-stats').textContent =
        ra.count + ' artistas · ' + rt.total + ' tracks';
    document.getElementById('home-stats').textContent =
        ra.count + ' artistas · ' + rt.total + ' tracks';

    const list = document.getElementById('artists-list');
    list.innerHTML = '';
    ra.artists.forEach(artist => {
        const el = document.createElement('div');
        el.className   = 'artist-item';
        el.dataset.id  = artist.id;

        const initial = (artist.name || '?')[0].toUpperCase();
        el.innerHTML = `
            <div class="artist-avatar">${initial}</div>
            <div class="artist-name">${escHtml(artist.name)}</div>
        `;
        el.onclick = () => loadAlbums(artist);
        list.appendChild(el);
    });

    loadHomeAlbums();
}

async function loadHomeAlbums() {
    const grid = document.getElementById('home-albums-grid');
    grid.innerHTML = '';

    const ra = await fetch('/api/artists').then(r => r.json());
    const allAlbums = [];
    await Promise.all(ra.artists.slice(0, 12).map(async artist => {
        const d = await fetch('/api/artists/' + artist.id + '/albums').then(r => r.json());
        (d.albums || []).forEach(album => allAlbums.push({ album, artist }));
    }));

    allAlbums.sort(() => Math.random() - 0.5);
    allAlbums.slice(0, 24).forEach(({ album, artist }) => {
        const card = document.createElement('div');
        card.className = 'album-card';

        card.innerHTML = `
            <div class="album-thumb">
                <img src="/api/albums/${album.id}/cover" onerror="this.style.display='none';this.nextElementSibling.style.display='flex'">
                <div class="album-thumb-placeholder" style="display:none;position:absolute;inset:0;">
                    <svg viewBox="0 0 24 24" fill="none" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
                        <path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/>
                    </svg>
                </div>
                <button class="album-play-btn" title="Reproducir">
                    <svg viewBox="0 0 16 16"><polygon points="3,2 14,8 3,14"/></svg>
                </button>
            </div>
            <div class="album-info-title">${escHtml(album.title)}</div>
            <div class="album-info-artist">${escHtml(artist.name)}${album.year ? ' · ' + album.year : ''}</div>
        `;

        card.querySelector('.album-thumb').onclick = () => loadTracks(album, artist.name);
        card.querySelector('.album-play-btn').onclick = (e) => {
            e.stopPropagation();
            loadTracksAndPlay(album, artist.name);
        };
        grid.appendChild(card);
    });
}

// ── ALBUMS ──
async function loadAlbums(artist) {
    currentArtist = artist;
    document.querySelectorAll('.artist-item').forEach(el =>
        el.classList.toggle('active', el.dataset.id == artist.id));

    setView('view-albums');
    document.getElementById('breadcrumb-artist').textContent = artist.name;
    document.getElementById('artist-view-name').textContent = artist.name;

    const d = await fetch('/api/artists/' + artist.id + '/albums').then(r => r.json());
    document.getElementById('artist-view-sub').textContent =
        (d.albums || []).length + ' álbumes';

    const list = document.getElementById('albums-list');
    list.innerHTML = '';

    (d.albums || []).forEach(album => {
        const row = document.createElement('div');
        row.className   = 'album-row';
        row.dataset.id  = album.id;

        row.innerHTML = `
            <div class="album-row-thumb">
                <img src="/api/albums/${album.id}/cover"
                     onerror="this.style.display='none';this.nextElementSibling.style.display='flex'">
                <svg style="display:none" viewBox="0 0 24 24" fill="none" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
                    <path d="M9 18V5l12-2v13"/><circle cx="6" cy="18" r="3"/><circle cx="18" cy="16" r="3"/>
                </svg>
            </div>
            <div class="album-row-info">
                <div class="album-row-title">${escHtml(album.title)}</div>
                <div class="album-row-year">${album.year || ''}</div>
            </div>
        `;
        row.onclick = () => loadTracks(album, artist.name);
        list.appendChild(row);
    });
}

// ── TRACKS ──
async function loadTracks(album, artistName) {
    currentAlbum = album;
    document.getElementById('breadcrumb-artist2').textContent = currentArtist ? currentArtist.name : artistName;
    document.getElementById('breadcrumb-album').textContent  = album.title;

    document.getElementById('album-view-name').textContent   = album.title;
    document.getElementById('album-view-artist').textContent = artistName;

    document.querySelectorAll('#albums-list .album-row').forEach(el =>
        el.classList.toggle('active', el.dataset.id == album.id));

    const imgEl = document.getElementById('album-cover-big-img');
    const phEl  = document.getElementById('album-cover-big-ph');
    imgEl.src = '/api/albums/' + album.id + '/cover';
    imgEl.style.display = 'block';
    imgEl.onerror = () => { imgEl.style.display = 'none'; phEl.style.display = 'flex'; };
    phEl.style.display = 'none';

    setView('view-tracks');

    const d = await fetch('/api/albums/' + album.id + '/tracks').then(r => r.json());
    currentTracks = d.tracks || [];
    document.getElementById('album-view-sub').textContent =
        currentTracks.length + ' canciones · ' +
        fmtTime(currentTracks.reduce((s, t) => s + (t.duration_s || 0), 0));

    renderTracks(currentTracks, artistName, 'tracks-list-body', album.title);
}

async function loadTracksAndPlay(album, artistName) {
    await loadTracks(album, artistName);
    if (currentTracks.length > 0) playTrack(0);
}

function renderTracks(tracks, artistName, containerId, albumTitle) {
    const body = document.getElementById(containerId);
    body.innerHTML = '';
    tracks.forEach((track, idx) => {
        const row = document.createElement('div');
        row.className   = 'track-row';
        row.dataset.id  = track.id;
        row.dataset.idx = idx;

        row.innerHTML = `
            <div>
                <div class="track-num">${track.track_number || (idx + 1)}</div>
                <div class="track-playing-icon">
                    <svg viewBox="0 0 16 16"><polygon points="3,2 14,8 3,14"/></svg>
                </div>
            </div>
            <div class="track-info">
                <div class="track-title">${escHtml(track.title)}</div>
                <div class="track-artist-name">${escHtml(artistName || '')}</div>
            </div>
            <div class="track-album-col">${escHtml(albumTitle || '')}</div>
            <div class="track-dur">${fmtTime(track.duration_s)}</div>
        `;

        row.ondblclick = () => {
            currentIdx = idx;
            playTrack(idx);
        };
        row.onclick = () => {
            currentIdx = idx;
            playTrack(idx);
        };
        body.appendChild(row);
    });
}

function playTrack(idx) {
    if (idx < 0 || idx >= currentTracks.length) return;
    currentIdx = idx;
    const track = currentTracks[idx];

    audio.src = '/stream/' + track.id;
    audio.play();

    document.querySelectorAll('.track-row').forEach(el => {
        el.classList.toggle('active', parseInt(el.dataset.idx) === idx);
    });

    updatePlayerUI(track);
}

function updatePlayerUI(track) {
    document.getElementById('player-title').textContent = track.title;
    document.getElementById('player-title').className   = 'player-track-title';
    document.getElementById('player-artist').textContent = currentArtist ? currentArtist.name : '';
    document.getElementById('player-artist').style.display = 'block';

    const thumb = document.getElementById('player-thumb');
    if (currentAlbum) {
        thumb.innerHTML = `<img src="/api/albums/${currentAlbum.id}/cover" style="width:100%;height:100%;object-fit:cover;"
            onerror="this.outerHTML='<div class=\\'player-thumb-placeholder\\'><svg viewBox=\\'0 0 24 24\\' fill=\\'none\\' stroke=\\'currentColor\\' stroke-width=\\'1.5\\'><path d=\\'M9 18V5l12-2v13\\'/><circle cx=\\'6\\' cy=\\'18\\' r=\\'3\\'/><circle cx=\\'18\\' cy=\\'16\\' r=\\'3\\'/></svg></div>'">`;
    }
}

function playAll() { if (currentTracks.length > 0) playTrack(0); }

function prevTrack() { if (currentIdx > 0) playTrack(currentIdx - 1); }
function nextTrack() { if (currentIdx < currentTracks.length - 1) playTrack(currentIdx + 1); }

function togglePlay() {
    if (audio.paused) audio.play(); else audio.pause();
}

audio.addEventListener('play', () => {
    document.getElementById('play-icon').innerHTML = '<rect x="3" y="2" width="4" height="12"/><rect x="9" y="2" width="4" height="12"/>';
});
audio.addEventListener('pause', () => {
    document.getElementById('play-icon').innerHTML = '<polygon points="3,2 14,8 3,14"/>';
});
audio.addEventListener('ended', () => nextTrack());

audio.addEventListener('timeupdate', () => {
    if (!audio.duration) return;
    const pct = (audio.currentTime / audio.duration) * 100;
    document.getElementById('progress-fill').style.width = pct + '%';
    document.getElementById('time-cur').textContent = fmtTime(audio.currentTime);
    document.getElementById('time-dur').textContent = fmtTime(audio.duration);
});

function seekTo(e) {
    if (!audio.duration) return;
    const bar  = document.getElementById('progress-bar');
    const rect = bar.getBoundingClientRect();
    audio.currentTime = ((e.clientX - rect.left) / rect.width) * audio.duration;
}

function setVolume(v) { audio.volume = v / 100; }
function toggleMute() { audio.muted = !audio.muted; }

// ── SEARCH ──
function onSearch(value) {
    clearTimeout(searchTimeout);
    const v = value.trim();
    if (v.length === 0) { showHome(); return; }
    if (v.length < 2)   return;
    searchTimeout = setTimeout(() => search(v), 300);
}

async function search(query) {
    setView('view-search');
    document.getElementById('search-title').textContent = 'Resultados para "' + query + '"';

    const d = await fetch('/api/search?q=' + encodeURIComponent(query)).then(r => r.json());
    document.getElementById('search-sub').textContent = d.count + ' canciones encontradas';

    currentTracks = d.tracks || [];
    currentIdx    = -1;

    renderTracks(currentTracks, '', 'search-results-body', '');
}

function escHtml(s) {
    return String(s || '')
        .replace(/&/g,'&amp;').replace(/</g,'&lt;')
        .replace(/>/g,'&gt;').replace(/"/g,'&quot;');
}

init();
</script>
</body>
</html>)html";

        auto response = crow::response(200, html);
        response.set_header("Content-Type", "text/html; charset=utf-8");
        return response;
    });

CROW_ROUTE(app_, "/api/albums/<int>/cover")
([this](int album_id) {
    auto cover = db_.getAlbumCover(album_id);

    if (!cover || cover->data.empty()) {
        return crow::response(404);
    }

    crow::response res(200);
    res.set_header("Content-Type", cover->mime_type);
    res.set_header("Cache-Control", "public, max-age=86400");  // caché 1 día en browser
    res.body.assign(
        reinterpret_cast<const char*>(cover->data.data()),
        cover->data.size());
    return res;
});
}

} // namespace localstream