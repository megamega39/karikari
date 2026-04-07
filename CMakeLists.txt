cmake_minimum_required(VERSION 3.20)
project(karikari LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(karikari WIN32
    src/main.cpp
    src/window.cpp
    src/viewer.cpp
    src/decoder.cpp
    src/navbar.cpp
    src/addressbar.cpp
    src/statusbar.cpp
    src/splitter.cpp
    src/filelist.cpp
    src/tree.cpp
    src/nav.cpp
    src/archive.cpp
    src/settings.cpp
    src/cache.cpp
    src/stream_cache.cpp
    src/history.cpp
    src/prefetch.cpp
    src/media.cpp
    src/favorites.cpp
    src/bookshelf.cpp
    src/i18n.cpp
    src/fswatcher.cpp
    src/command.cpp
    src/hover_preview.cpp
    src/context_menu.cpp
    src/viewer_toolbar.cpp
    src/karikari.manifest
)

target_link_libraries(karikari PRIVATE
    d2d1
    d3d11
    dxgi
    dwrite
    windowscodecs
    ole32
    comctl32
    shlwapi
    uxtheme
    shcore
)

# PCH
target_precompile_headers(karikari PRIVATE src/app.h)

# Unicode
target_compile_definitions(karikari PRIVATE UNICODE _UNICODE)

# UTF-8 source
target_compile_options(karikari PRIVATE /utf-8)

# Release 最適化: LTO（リンク時最適化）
if(MSVC)
    target_compile_options(karikari PRIVATE $<$<CONFIG:Release>:/GL>)
    target_link_options(karikari PRIVATE $<$<CONFIG:Release>:/LTCG>)
endif()
