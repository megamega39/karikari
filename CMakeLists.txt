cmake_minimum_required(VERSION 3.20)
project(karikari LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# VC ランタイムを静的リンク（VCRedist 不要にする）
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

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
    src/help.cpp
    src/hover_preview.cpp
    src/context_menu.cpp
    src/viewer_toolbar.cpp
    src/karikari.manifest
    res/karikari.rc
)

# libwebp (WebPアニメーション対応)
set(WEBP_DIR ${CMAKE_SOURCE_DIR}/third_party/libwebp/libwebp-1.5.0-windows-x64)
target_include_directories(karikari PRIVATE ${WEBP_DIR}/include)
target_link_libraries(karikari PRIVATE
    ${WEBP_DIR}/lib/libwebpdemux.lib
    ${WEBP_DIR}/lib/libwebp.lib
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

# UTF-8 source + 警告レベル + 例外ハンドリング
target_compile_options(karikari PRIVATE /utf-8 /W4 /EHsc)

# Release 最適化: LTO（リンク時最適化）
if(MSVC)
    target_compile_options(karikari PRIVATE $<$<CONFIG:Release>:/GL>)
    target_link_options(karikari PRIVATE $<$<CONFIG:Release>:/LTCG>)
endif()

# ビルド後にDLLをexe横にコピー（クリーンビルドでも自動配置）
add_custom_command(TARGET karikari POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/third_party/libmpv-2.dll"
        "$<TARGET_FILE_DIR:karikari>/libmpv-2.dll"
)

# === テスト ===
option(BUILD_TESTS "Build unit tests" OFF)

if(BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
    )
    set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(karikari_tests
        tests/test_main.cpp
        tests/test_utils.cpp
        tests/test_json.cpp
        tests/test_archive_utils.cpp
        tests/test_filelist.cpp
        tests/test_nav.cpp
        tests/test_settings.cpp
        tests/settings_testable.cpp
    )
    target_include_directories(karikari_tests PRIVATE src/)
    target_compile_definitions(karikari_tests PRIVATE UNICODE _UNICODE)
    target_compile_options(karikari_tests PRIVATE /utf-8 /W4 /EHsc)
    target_link_libraries(karikari_tests PRIVATE
        GTest::gtest_main
        shlwapi
        ole32
        shell32
        comctl32
    )

    include(GoogleTest)
    gtest_discover_tests(karikari_tests)
endif()
