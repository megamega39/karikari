#include "navbar.h"
#include "i18n.h"

// Segoe Fluent Icons のコードポイント
struct NavButtonDef {
    UINT cmd;
    const wchar_t* icon;
    const wchar_t* tooltip;
};

static const NavButtonDef kNavButtons[] = {
    { IDM_NAV_BACK,      L"\uE72B", L"戻る" },
    { IDM_NAV_FORWARD,   L"\uE72A", L"進む" },
    { IDM_NAV_UP,        L"\uE74A", L"上へ" },
    { IDM_NAV_REFRESH,   L"\uE72C", L"更新" },
    { IDM_NAV_BOOKSHELF, L"\uE82D", L"本棚" },
    { IDM_NAV_HISTORY,   L"\uE81C", L"履歴" },
    { 0, nullptr, nullptr }, // セパレータ
    { IDM_NAV_LIST,      L"\uE8FD", L"リスト表示" },
    { IDM_NAV_GRID,      L"\uE80A", L"グリッド表示" },
    { IDM_NAV_HOVER,     L"\uE7B3", L"ホバープレビュー" },
};

// 右端に配置するボタン
static const NavButtonDef kNavButtonsRight[] = {
    { IDM_NAV_SETTINGS,  L"\uE713", L"設定" },
    { IDM_NAV_HELP,      L"\uE897", L"ヘルプ" },
};

// 32bitビットマップに正しいアルファ付きテキストを描画するヘルパー
// 手法: 白テキストを黒背景に描画 → 輝度をアルファに → 目標色でプリマルチプライ
static void RenderTextToBitmap(HDC hdcMem, HFONT hFont, const wchar_t* text, int textLen,
                               int cx, int cy, void* pBits, BYTE tR, BYTE tG, BYTE tB)
{
    HFONT hOldFont = (HFONT)SelectObject(hdcMem, hFont);
    memset(pBits, 0, cx * cy * 4);
    SetBkMode(hdcMem, TRANSPARENT);
    SetTextColor(hdcMem, RGB(255, 255, 255)); // 白テキスト → 黒背景

    RECT rcText = { 0, 0, cx, cy };
    DrawTextW(hdcMem, text, textLen, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // 輝度をアルファに変換、目標色でプリマルチプライ
    BYTE* pixels = (BYTE*)pBits;
    for (int i = 0; i < cx * cy; i++)
    {
        BYTE alpha = (std::max)({ pixels[i * 4], pixels[i * 4 + 1], pixels[i * 4 + 2] });
        pixels[i * 4 + 0] = (BYTE)(tB * alpha / 255); // B
        pixels[i * 4 + 1] = (BYTE)(tG * alpha / 255); // G
        pixels[i * 4 + 2] = (BYTE)(tR * alpha / 255); // R
        pixels[i * 4 + 3] = alpha;
    }
    SelectObject(hdcMem, hOldFont);
}

// イメージリストをファイルキャッシュから読み込み/保存
#include <shlobj.h>
#include <shlwapi.h>
static std::wstring GetCachePath(const wchar_t* name)
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    PathRemoveFileSpecW(path);
    PathAppendW(path, name);
    return path;
}

HIMAGELIST LoadImageListCache(const wchar_t* cacheName)
{
    std::wstring path = GetCachePath(cacheName);
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return nullptr;

    DWORD size = GetFileSize(hFile, nullptr);
    std::vector<BYTE> data(size);
    DWORD read; ReadFile(hFile, data.data(), size, &read, nullptr);
    CloseHandle(hFile);

    IStream* pStream = SHCreateMemStream(data.data(), size);
    if (!pStream) return nullptr;
    HIMAGELIST hil = ImageList_Read(pStream);
    pStream->Release();
    return hil;
}

void SaveImageListCache(HIMAGELIST hil, const wchar_t* cacheName)
{
    IStream* pStream = SHCreateMemStream(nullptr, 0);
    if (!pStream) return;
    ImageList_Write(hil, pStream);

    STATSTG stat; pStream->Stat(&stat, STATFLAG_NONAME);
    LARGE_INTEGER zero = {}; pStream->Seek(zero, STREAM_SEEK_SET, nullptr);

    std::vector<BYTE> data((size_t)stat.cbSize.QuadPart);
    ULONG read; pStream->Read(data.data(), (ULONG)data.size(), &read);
    pStream->Release();

    std::wstring path = GetCachePath(cacheName);
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD written; WriteFile(hFile, data.data(), (DWORD)data.size(), &written, nullptr);
        CloseHandle(hFile);
    }
}

// アイコンフォントでビットマップを作成してイメージリストに追加
static HIMAGELIST CreateIconImageList(int cx, int cy)
{
    // キャッシュから読み込み試行
    HIMAGELIST cached = LoadImageListCache(L"navbar_icons.cache");
    if (cached && ImageList_GetImageCount(cached) > 0) return cached;
    if (cached) ImageList_Destroy(cached);
    HIMAGELIST hil = ImageList_Create(cx, cy, ILC_COLOR32, 16, 4);

    HFONT hFont = CreateFontW(
        27, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, DEFAULT_PITCH,
        L"Segoe Fluent Icons");
    if (!hFont)
        hFont = CreateFontW(
            27, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY, DEFAULT_PITCH,
            L"Segoe MDL2 Assets");

    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    for (auto& btn : kNavButtons)
    {
        if (!btn.icon) continue;

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = cx;
        bmi.bmiHeader.biHeight = -cy;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        void* pBits = nullptr;
        HBITMAP hbmp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
        HBITMAP hOld = (HBITMAP)SelectObject(hdcMem, hbmp);

        RenderTextToBitmap(hdcMem, hFont, btn.icon, 1, cx, cy, pBits, 50, 50, 50);

        SelectObject(hdcMem, hOld);
        ImageList_Add(hil, hbmp, nullptr);
        DeleteObject(hbmp);
    }

    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
    DeleteObject(hFont);

    // キャッシュに保存（次回起動時はビットマップ生成をスキップ）
    SaveImageListCache(hil, L"navbar_icons.cache");
    return hil;
}

HWND CreateNavBar(HWND parent, HINSTANCE hInst)
{
    HWND hwnd = CreateWindowExW(
        0, TOOLBARCLASSNAMEW, nullptr,
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS
        | CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE,
        0, 0, 0, 0,
        parent, (HMENU)(UINT_PTR)IDC_NAVBAR, hInst, nullptr);

    if (!hwnd) return nullptr;

    SendMessageW(hwnd, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    // ドロップダウン矢印の描画を有効化（ボタン追加前に設定）
    DWORD exStyle = (DWORD)SendMessageW(hwnd, TB_GETEXTENDEDSTYLE, 0, 0);
    SendMessageW(hwnd, TB_SETEXTENDEDSTYLE, 0, exStyle | TBSTYLE_EX_DRAWDDARROWS);

    // イメージリスト作成・設定（ボタンサイズに合わせて32x32）
    HIMAGELIST hil = CreateIconImageList(36, 36);
    SendMessageW(hwnd, TB_SETIMAGELIST, 0, (LPARAM)hil);

    // 無効時用イメージリスト（グレーアウト表示）
    {
        HIMAGELIST hilDisabled = ImageList_Create(36, 36, ILC_COLOR32, 16, 4);
        HFONT hFont = CreateFontW(27, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Segoe Fluent Icons");
        if (!hFont) hFont = CreateFontW(27, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Segoe MDL2 Assets");
        HDC hdcScreen = GetDC(nullptr);
        HDC hdcMem = CreateCompatibleDC(hdcScreen);
        for (auto& btn : kNavButtons)
        {
            if (!btn.icon) continue;
            BITMAPINFO bmi = {};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = 36; bmi.bmiHeader.biHeight = -36;
            bmi.bmiHeader.biPlanes = 1; bmi.bmiHeader.biBitCount = 32;
            void* pBits = nullptr;
            HBITMAP hbmp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
            HBITMAP hOld = (HBITMAP)SelectObject(hdcMem, hbmp);
            RenderTextToBitmap(hdcMem, hFont, btn.icon, 1, 36, 36, pBits, 180, 180, 180);
            SelectObject(hdcMem, hOld);
            ImageList_Add(hilDisabled, hbmp, nullptr);
            DeleteObject(hbmp);
        }
        DeleteDC(hdcMem); ReleaseDC(nullptr, hdcScreen); DeleteObject(hFont);
        SendMessageW(hwnd, TB_SETDISABLEDIMAGELIST, 0, (LPARAM)hilDisabled);
    }

    // ボタン追加
    int imgIndex = 0;
    for (auto& def : kNavButtons)
    {
        TBBUTTON tb = {};
        if (!def.icon)
        {
            tb.fsStyle = BTNS_SEP;
            tb.iBitmap = 12;
        }
        else
        {
            tb.iBitmap = imgIndex++;
            tb.idCommand = def.cmd;
            tb.fsState = TBSTATE_ENABLED;
            tb.fsStyle = BTNS_BUTTON;
            // 戻る/進むボタンにドロップダウン矢印を付ける
            if (def.cmd == IDM_NAV_BACK || def.cmd == IDM_NAV_FORWARD)
                tb.fsStyle |= BTNS_DROPDOWN;
        }
        SendMessageW(hwnd, TB_ADDBUTTONS, 1, (LPARAM)&tb);
    }

    // ツールバーの高さを固定
    SendMessageW(hwnd, TB_SETBUTTONSIZE, 0, MAKELONG(48, 48));
    SendMessageW(hwnd, TB_AUTOSIZE, 0, 0);

    g_app.wnd.hwndNavBar = hwnd;
    return hwnd;
}

static HIMAGELIST CreateRightIconImageList(int cx, int cy)
{
    HIMAGELIST cached = LoadImageListCache(L"navbar_right.cache");
    if (cached && ImageList_GetImageCount(cached) > 0) return cached;
    if (cached) ImageList_Destroy(cached);

    HIMAGELIST hil = ImageList_Create(cx, cy, ILC_COLOR32, 4, 2);
    HFONT hFont = CreateFontW(27, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Segoe Fluent Icons");
    if (!hFont)
        hFont = CreateFontW(27, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Segoe MDL2 Assets");

    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    for (auto& btn : kNavButtonsRight)
    {
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = cx;
        bmi.bmiHeader.biHeight = -cy;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        void* pBits = nullptr;
        HBITMAP hbmp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
        HBITMAP hOld = (HBITMAP)SelectObject(hdcMem, hbmp);
        RenderTextToBitmap(hdcMem, hFont, btn.icon, 1, cx, cy, pBits, 50, 50, 50);
        SelectObject(hdcMem, hOld);
        ImageList_Add(hil, hbmp, nullptr);
        DeleteObject(hbmp);
    }
    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
    DeleteObject(hFont);
    SaveImageListCache(hil, L"navbar_right.cache");
    return hil;
}

HWND CreateNavBarRight(HWND parent, HINSTANCE hInst)
{
    HWND hwnd = CreateWindowExW(
        0, TOOLBARCLASSNAMEW, nullptr,
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS
        | CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE,
        0, 0, 0, 0,
        parent, (HMENU)(UINT_PTR)(IDC_NAVBAR + 1), hInst, nullptr);

    if (!hwnd) return nullptr;

    SendMessageW(hwnd, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    HIMAGELIST hil = CreateRightIconImageList(36, 36);
    SendMessageW(hwnd, TB_SETIMAGELIST, 0, (LPARAM)hil);

    int imgIndex = 0;
    for (auto& def : kNavButtonsRight)
    {
        TBBUTTON tb = {};
        tb.iBitmap = imgIndex++;
        tb.idCommand = def.cmd;
        tb.fsState = TBSTATE_ENABLED;
        tb.fsStyle = BTNS_BUTTON;
        SendMessageW(hwnd, TB_ADDBUTTONS, 1, (LPARAM)&tb);
    }

    SendMessageW(hwnd, TB_SETBUTTONSIZE, 0, MAKELONG(48, 48));
    SendMessageW(hwnd, TB_AUTOSIZE, 0, 0);

    g_app.wnd.hwndNavBarRight = hwnd;
    return hwnd;
}

void NavUpdateButtons()
{
    if (!g_app.wnd.hwndNavBar) return;
    SendMessageW(g_app.wnd.hwndNavBar, TB_ENABLEBUTTON, IDM_NAV_BACK,
        MAKELONG(!g_app.nav.historyBack.empty(), 0));
    SendMessageW(g_app.wnd.hwndNavBar, TB_ENABLEBUTTON, IDM_NAV_FORWARD,
        MAKELONG(!g_app.nav.historyForward.empty(), 0));
}

void UpdateNavbarTooltips()
{
    // 左ツールバーのツールチップ文字列テーブルを更新
    // kNavButtons の tooltip はイニシャルのみ使用。
    // 実際のツールチップは TBN_GETINFOTIP で window.cpp が返すため、
    // ここでは kNavButtons / kNavButtonsRight の tooltip を直接書き換える。
    // ただし const 配列なので、代わりに何もしない（window.cpp 側で I18nGet を使う）。
    // この関数は将来的な拡張ポイントとして残す。
}
