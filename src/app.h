#pragma once

#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>
#include <d2d1_1.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>

using Microsoft::WRL::ComPtr;

// コントロールID
enum ControlID : UINT {
    IDC_NAVBAR = 100,
    IDC_ADDRESSBAR_LABEL,
    IDC_ADDRESSBAR_EDIT,
    IDC_TREEVIEW,
    IDC_LISTVIEW,
    IDC_VIEWER_TOOLBAR,
    IDC_VIEWER,
    IDC_STATUSBAR,
    IDC_MAIN_SPLITTER,
    IDC_SIDEBAR_SPLITTER,
};

// ナビバーコマンド
enum NavCommand : UINT {
    IDM_NAV_BACK = 200,
    IDM_NAV_FORWARD,
    IDM_NAV_UP,
    IDM_NAV_REFRESH,
    IDM_NAV_HOVER,
    IDM_NAV_BOOKSHELF,
    IDM_NAV_HISTORY,
    IDM_NAV_LIST,
    IDM_NAV_GRID,
    IDM_NAV_SETTINGS,
    IDM_NAV_HELP,
    IDM_TREE_SORT = 250,
};

// ビューアーコマンド
enum ViewerCommand : UINT {
    IDM_VIEW_FIRST = 300,
    IDM_VIEW_PREV,
    IDM_VIEW_NEXT,
    IDM_VIEW_LAST,
    IDM_VIEW_FIT_WINDOW,
    IDM_VIEW_FIT_WIDTH,
    IDM_VIEW_FIT_HEIGHT,
    IDM_VIEW_ORIGINAL,
    IDM_VIEW_ZOOMIN,
    IDM_VIEW_ZOOMOUT,
    IDM_VIEW_SINGLE,
    IDM_VIEW_SPREAD,
    IDM_VIEW_AUTO,
    IDM_VIEW_BINDING,
};

// 履歴ドロップダウンメニューID
constexpr UINT IDM_HIST_DROPDOWN_BACK = 2000;
constexpr UINT IDM_HIST_DROPDOWN_FWD = 3000;

// カスタムメッセージ
constexpr UINT WM_SPLITTER_MOVED = WM_APP + 1;
constexpr UINT WM_TOGGLE_FULLSCREEN = WM_APP + 2;
constexpr UINT WM_SHOW_CONTEXT_MENU = WM_APP + 5;
constexpr UINT WM_RESTORE_LAST_PATH = WM_APP + 6;
constexpr UINT WM_ARCHIVE_LOADED = WM_APP + 11;
// WM_DEFERRED_INIT 廃止済み

// スケールモード
enum ScaleMode { FitWindow, FitWidth, FitHeight, Original };

// ファイルアイテム
struct FileItem {
    std::wstring name;
    std::wstring fullPath;
    bool isDirectory;
    ULONGLONG fileSize;
    FILETIME lastWriteTime;
};

// === AppState: 責務ごとに分割 ===

struct WindowHandles {
    HWND hwndMain = nullptr;
    HWND hwndNavBar = nullptr;
    HWND hwndAddressLabel = nullptr;
    HWND hwndAddressEdit = nullptr;
    HWND hwndFolderLabel = nullptr;   // 「フォルダ」ラベル
    HWND hwndTreeSortBtn = nullptr;   // ツリーソートボタン
    HWND hwndTree = nullptr;
    HWND hwndList = nullptr;
    HWND hwndFilterBox = nullptr;
    HWND hwndViewerTbLeft = nullptr;   // ビューアーツールバー左（ページナビ）
    HWND hwndViewerTbRight = nullptr;  // ビューアーツールバー右（表示設定）
    HWND hwndViewer = nullptr;
    HWND hwndStatusBar = nullptr;
    HWND hwndMainSplitter = nullptr;
    HWND hwndSidebarSplitter = nullptr;
    HWND hwndMediaPlayer = nullptr;
    HWND hwndHistoryToolbar = nullptr;  // 履歴ツールバー
    HWND hwndHistoryList = nullptr;     // 履歴ListView
    HWND hwndHistoryFilter = nullptr;   // 履歴フィルター
    HWND hwndPageCounter = nullptr;   // ページカウンター "27 / 1127"
    HWND hwndZoomLabel = nullptr;     // ズーム率 "100%"
};

struct ViewerState {
    ComPtr<ID2D1Factory1> d2dFactory;         // D2D 1.1 Factory
    ComPtr<ID2D1DeviceContext> deviceContext;  // DeviceContext (旧: renderTarget)
    ComPtr<ID3D11Device> d3dDevice;           // D3D11デバイス
    ComPtr<IDXGISwapChain1> swapChain;        // スワップチェーン
    ComPtr<ID2D1Bitmap1> backBuffer;          // バックバッファ
    ComPtr<IWICImagingFactory> wicFactory;
    ComPtr<ID2D1Bitmap> bitmap;
    ComPtr<ID2D1Bitmap> bitmap2;

    // アニメーション（GIF/WebP）
    std::vector<ComPtr<ID2D1Bitmap>> animFrames;
    std::vector<int> animDelays; // ms
    int animCurrentFrame = 0;
    UINT_PTR animTimer = 0;
    bool isAnimating = false;

    float zoom = 1.0f;
    float scrollX = 0.0f;
    float scrollY = 0.0f;
    bool isPanning = false;
    POINT panStart = {};
    float panScrollStartX = 0.0f;
    float panScrollStartY = 0.0f;
    ScaleMode scaleMode = FitWindow;

    int viewMode = 0;       // 0=自動, 1=単独, 2=強制見開き
    int rotation = 0;       // 0, 90, 180, 270
    bool isRTL = false;
    bool isSpreadActive = false;
};

struct NavState {
    std::wstring currentPath;
    std::wstring currentFolder;
    std::wstring currentArchive;
    std::wstring currentArchiveEntry;

    std::vector<FileItem> fileItems;
    std::vector<std::wstring> viewableFiles;
    std::unordered_map<std::wstring, int> viewableFileIndex; // lowercase path → index (O(1)検索)
    std::unordered_map<std::wstring, int> fileItemIndex;   // lowercase path → fileItems index (O(1)検索)
    int currentFileIndex = -1;

    std::vector<std::wstring> historyBack;
    std::vector<std::wstring> historyForward;

    bool inArchiveMode = false;
    bool isMediaMode = false;
    int lastNavDirection = 1;  // プリフェッチ方向（1=前進, -1=後退）
};

struct NavigateOptions {
    bool updateHistory = true;
    bool syncTreeSelection = true;
};

struct AppState {
    HINSTANCE hInstance = nullptr;
    WindowHandles wnd;
    ViewerState viewer;
    NavState nav;

    // レイアウト
    int mainSplitPos = 300;
    int sidebarSplitPos = 300;

    // UI状態
    bool isRevealing = false;
    bool suppressListNotify = false; // LVN_ITEMCHANGED 再入防止
    bool hoverPreviewEnabled = false; // ホバープレビュー有効
    HWND hwndHoverPreview = nullptr;  // ホバープレビューウィンドウ
    bool isFullscreen = false;
    LONG savedStyle = 0;
    LONG savedExStyle = 0;
    RECT savedRect = {};
};

extern AppState g_app;

