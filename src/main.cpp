#include "app.h"
#include "decoder.h"
#include "viewer.h"
#include "splitter.h"
#include "archive.h"
#include "history.h"
#include "nav.h"
#include "cache.h"
#include "stream_cache.h"
#include "prefetch.h"
#include "media.h"
#include "favorites.h"
#include "bookshelf.h"
#include "fswatcher.h"
#include "i18n.h"
#include "settings.h"
#include "window.h"
#include <shellscalingapi.h>

AppState g_app;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
    // Per-Monitor DPI Awareness v2（Windows 10 1703+）
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    HRESULT hrCom = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (hrCom == RPC_E_CHANGED_MODE)
        hrCom = CoInitializeEx(nullptr, COINIT_MULTITHREADED); // DPI APIがMTAで初期化済みの場合
    if (FAILED(hrCom))
    {
        MessageBoxW(nullptr, L"COM の初期化に失敗しました", L"エラー", MB_OK);
        return 1;
    }

    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_TREEVIEW_CLASSES | ICC_LISTVIEW_CLASSES
              | ICC_BAR_CLASSES | ICC_COOL_CLASSES;
    InitCommonControlsEx(&icc);

    g_app.hInstance = hInstance;

    if (FAILED(InitD2D()))
    {
        MessageBoxW(nullptr, L"Direct2D の初期化に失敗しました", L"エラー", MB_OK);
        CoUninitialize();
        return 1;
    }

    // 設定を読み込んで起動時に反映
    {
        AppSettings startupSettings;
        LoadSettings(startupSettings);
        I18nInit(startupSettings.language.empty() ? L"ja" : startupSettings.language);
        CacheInit((size_t)startupSettings.cacheSizeMB * 1024 * 1024,
                  std::max(10, startupSettings.cacheSizeMB / 3));
        g_app.showBars = startupSettings.showBars;
    }

    LoadKeyBindings(); // ショートカットキー読み込み（デフォルト初期化含む）
    InitArchive();
    HistoryLoad();
    NavHistoryLoad();
    FavoritesLoad();
    BookshelfLoad();
    // MediaInit は遅延初期化（libmpv-2.dll 112MB の LoadLibrary が重いため）
    // MediaPlay 初回呼び出し時に自動初期化
    RegisterViewerClass(hInstance);
    RegisterSplitterClass(hInstance);
    RegisterMainWindow(hInstance);

    HWND hwnd = CreateMainWindow(hInstance, nCmdShow);
    if (!hwnd)
    {
        CoUninitialize();
        return 1;
    }

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    FsWatcherStop();
    MediaShutdown();
    PrefetchShutdown();
    CleanupTempFiles();
    FavoritesSave();
    HistorySave();
    NavHistorySave();
    CacheClear();
    StreamCacheClear();
    CloseArchive();
    // bitmap/deviceContext は WM_DESTROY で解放済み。残りのリソースをここで解放
    g_app.viewer.wicFactory.Reset();
    g_app.viewer.backBuffer.Reset();
    g_app.viewer.swapChain.Reset();
    g_app.viewer.deviceContext.Reset();
    g_app.viewer.d3dDevice.Reset();
    g_app.viewer.d2dFactory.Reset();
    CoUninitialize();

    return (int)msg.wParam;
}
