#include "viewer_toolbar.h"
#include "viewer.h"
#include "navbar.h"
#include <algorithm>

// navbar.cpp のキャッシュ関数を再利用
extern HIMAGELIST LoadImageListCache(const wchar_t* cacheName);
extern void SaveImageListCache(HIMAGELIST hil, const wchar_t* cacheName);

// ビューアーツールバー用イメージリスト作成（アイコン＋テキスト統合）
struct ViewerImgDef {
    const wchar_t* text;
    bool useIconFont;  // true=Segoe Fluent Icons, false=GUIフォント
};

static const ViewerImgDef kViewerImages[] = {
    { L"\uE892", true  },  //  0: First (SkipBack)
    { L"\uE76B", true  },  //  1: Prev (ChevronLeft)
    { L"\uE76C", true  },  //  2: Next (ChevronRight)
    { L"\uE893", true  },  //  3: Last (SkipForward)
    { L"\uE740", true  },  //  4: FitWindow (FullScreen)
    { L"W",      false },  //  5: FitWidth
    { L"H",      false },  //  6: FitHeight
    { L"1:1",    false },  //  7: Original
    { L"\uE8A3", true  },  //  8: ZoomIn (ZoomIn icon)
    { L"\uE71F", true  },  //  9: ZoomOut (ZoomOut icon)
    { L"A",      false },  // 10: Auto
    { L"1",      false },  // 11: Single
    { L"2",      false },  // 12: Spread
    { L"\uE72A", true  },  // 13: Binding LTR (→)
    { L"\uE72B", true  },  // 14: Binding RTL (←)
};

// 白テキスト→黒背景で描画し、輝度をアルファに変換して目標色でプリマルチプライ
static void RenderTextToBitmap32(HDC hdcMem, HFONT hFont, const wchar_t* text, int textLen,
                                  int cx, int cy, void* pBits, BYTE tR, BYTE tG, BYTE tB)
{
    HFONT hOldFont = (HFONT)SelectObject(hdcMem, hFont);
    memset(pBits, 0, cx * cy * 4);
    SetBkMode(hdcMem, TRANSPARENT);
    SetTextColor(hdcMem, RGB(255, 255, 255));

    RECT rcText = { 0, 0, cx, cy };
    DrawTextW(hdcMem, text, textLen, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    BYTE* pixels = (BYTE*)pBits;
    for (int i = 0; i < cx * cy; i++)
    {
        BYTE alpha = (std::max)({ pixels[i * 4], pixels[i * 4 + 1], pixels[i * 4 + 2] });
        pixels[i * 4 + 0] = (BYTE)(tB * alpha / 255);
        pixels[i * 4 + 1] = (BYTE)(tG * alpha / 255);
        pixels[i * 4 + 2] = (BYTE)(tR * alpha / 255);
        pixels[i * 4 + 3] = alpha;
    }
    SelectObject(hdcMem, hOldFont);
}

static HIMAGELIST CreateViewerIconImageList(int cx, int cy)
{
    // キャッシュから読み込み試行（アイコン数が一致する場合のみ使用）
    constexpr int kExpectedCount = _countof(kViewerImages);
    HIMAGELIST cached = LoadImageListCache(L"viewer_icons.cache");
    if (cached && ImageList_GetImageCount(cached) == kExpectedCount) return cached;
    if (cached) ImageList_Destroy(cached);

    constexpr int kCount = _countof(kViewerImages);
    HIMAGELIST hil = ImageList_Create(cx, cy, ILC_COLOR32, kCount, 4);

    HFONT hIconFont = CreateFontW(
        20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, DEFAULT_PITCH,
        L"Segoe Fluent Icons");
    if (!hIconFont)
        hIconFont = CreateFontW(
            20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY, DEFAULT_PITCH,
            L"Segoe MDL2 Assets");

    HFONT hTextFont = CreateFontW(
        14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, DEFAULT_PITCH,
        L"Segoe UI");

    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    for (int i = 0; i < kCount; i++)
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

        HFONT hFont = kViewerImages[i].useIconFont ? hIconFont : hTextFont;
        RenderTextToBitmap32(hdcMem, hFont, kViewerImages[i].text, -1,
                             cx, cy, pBits, 50, 50, 50);

        SelectObject(hdcMem, hOld);
        ImageList_Add(hil, hbmp, nullptr);
        DeleteObject(hbmp);
    }

    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
    DeleteObject(hIconFont);
    DeleteObject(hTextFont);

    SaveImageListCache(hil, L"viewer_icons.cache");
    return hil;
}

// セパレータインデックス
static int g_pageCounterSepIndex = -1;  // 左TB内
static int g_zoomLabelSepIndex = -1;    // 右TB内

struct VBtnDef { int type; UINT cmd; int imgIdx; int sepWidth; };

static HWND CreateToolbarBase(HWND parent, HINSTANCE hInst, UINT id, HIMAGELIST hil)
{
    HWND hwnd = CreateWindowExW(
        0, TOOLBARCLASSNAMEW, nullptr,
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS
        | CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE,
        0, 0, 0, 0,
        parent, (HMENU)(UINT_PTR)id, hInst, nullptr);
    if (!hwnd) return nullptr;
    SendMessageW(hwnd, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessageW(hwnd, TB_SETIMAGELIST, 0, (LPARAM)hil);
    return hwnd;
}

static void AddToolbarButtons(HWND hwnd, const VBtnDef* btns, int count,
                               int* pLabelSepIdx1 = nullptr, int* pLabelSepIdx2 = nullptr)
{
    int labelCount = 0;
    for (int i = 0; i < count; i++)
    {
        TBBUTTON tb = {};
        if (btns[i].type == 1) { tb.fsStyle = BTNS_SEP; tb.iBitmap = btns[i].sepWidth; }
        else if (btns[i].type == 2) {
            tb.fsStyle = BTNS_SEP; tb.iBitmap = btns[i].sepWidth;
            if (labelCount == 0 && pLabelSepIdx1) *pLabelSepIdx1 = i;
            else if (labelCount == 1 && pLabelSepIdx2) *pLabelSepIdx2 = i;
            labelCount++;
        }
        else { tb.iBitmap = btns[i].imgIdx; tb.idCommand = btns[i].cmd; tb.fsState = TBSTATE_ENABLED; tb.fsStyle = BTNS_BUTTON; }
        SendMessageW(hwnd, TB_ADDBUTTONS, 1, (LPARAM)&tb);
    }
    SendMessageW(hwnd, TB_SETBUTTONSIZE, 0, MAKELONG(32, 32));
    SendMessageW(hwnd, TB_AUTOSIZE, 0, 0);
}

void CreateViewerToolbars(HWND parent, HINSTANCE hInst)
{
    HIMAGELIST hil = CreateViewerIconImageList(24, 24);

    // === 左ツールバー（ページナビ） ===
    static const VBtnDef leftBtns[] = {
        { 0, IDM_VIEW_FIRST, 0, 0 },   // |<
        { 0, IDM_VIEW_PREV,  1, 0 },   // <
        { 2, 0,              0, 90 },  // ページカウンター
        { 0, IDM_VIEW_NEXT,  2, 0 },   // >
        { 0, IDM_VIEW_LAST,  3, 0 },   // >|
    };
    g_pageCounterSepIndex = -1;
    HWND hLeft = CreateToolbarBase(parent, hInst, IDC_VIEWER_TOOLBAR, hil);
    AddToolbarButtons(hLeft, leftBtns, _countof(leftBtns), &g_pageCounterSepIndex);

    g_app.wnd.hwndPageCounter = CreateWindowExW(
        0, L"STATIC", L"0 / 0",
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
        0, 0, 90, 28, hLeft, nullptr, hInst, nullptr);
    SendMessageW(g_app.wnd.hwndPageCounter, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

    g_app.wnd.hwndViewerTbLeft = hLeft;

    // === 右ツールバー（表示設定） ===
    static const VBtnDef rightBtns[] = {
        { 0, IDM_VIEW_FIT_WINDOW,  4, 0 },
        { 0, IDM_VIEW_FIT_WIDTH,   5, 0 },
        { 0, IDM_VIEW_FIT_HEIGHT,  6, 0 },
        { 0, IDM_VIEW_ORIGINAL,    7, 0 },
        { 1, 0,                    0, 8 },
        { 0, IDM_VIEW_BINDING,    13, 0 },
        { 1, 0,                    0, 8 },
        { 0, IDM_VIEW_ZOOMIN,      8, 0 },
        { 2, 0,                    0, 60 },  // ズーム率
        { 0, IDM_VIEW_ZOOMOUT,     9, 0 },
        { 1, 0,                    0, 8 },
        { 0, IDM_VIEW_AUTO,       10, 0 },
        { 0, IDM_VIEW_SINGLE,     11, 0 },
        { 0, IDM_VIEW_SPREAD,     12, 0 },
    };
    g_zoomLabelSepIndex = -1;
    HWND hRight = CreateToolbarBase(parent, hInst, IDC_VIEWER_TOOLBAR + 1, hil);
    AddToolbarButtons(hRight, rightBtns, _countof(rightBtns), &g_zoomLabelSepIndex);

    g_app.wnd.hwndZoomLabel = CreateWindowExW(
        0, L"STATIC", L"100%",
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE | SS_NOTIFY,
        0, 0, 60, 28, hRight, (HMENU)(UINT_PTR)IDM_VIEW_ZOOM_RESET, hInst, nullptr);
    SendMessageW(g_app.wnd.hwndZoomLabel, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    // クリックでズームリセット（STATICの親がツールバーなのでサブクラスで転送）
    SetWindowSubclass(g_app.wnd.hwndZoomLabel, [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
        UINT_PTR, DWORD_PTR) -> LRESULT {
        if (msg == WM_LBUTTONUP)
            SendMessageW(g_app.wnd.hwndMain, WM_COMMAND, IDM_VIEW_ZOOM_RESET, 0);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }, 0, 0);

    g_app.wnd.hwndViewerTbRight = hRight;

    // チェック可能ボタンに変更（スケールモード/表示モード/綴じ方向）
    UINT checkCmds[] = {
        IDM_VIEW_FIT_WINDOW, IDM_VIEW_FIT_WIDTH, IDM_VIEW_FIT_HEIGHT, IDM_VIEW_ORIGINAL,
        IDM_VIEW_AUTO, IDM_VIEW_SINGLE, IDM_VIEW_SPREAD
    };
    for (UINT cmd : checkCmds)
    {
        TBBUTTONINFOW tbi = {};
        tbi.cbSize = sizeof(tbi);
        tbi.dwMask = TBIF_STYLE;
        SendMessageW(hRight, TB_GETBUTTONINFOW, cmd, (LPARAM)&tbi);
        tbi.fsStyle |= BTNS_CHECK;
        SendMessageW(hRight, TB_SETBUTTONINFOW, cmd, (LPARAM)&tbi);
    }

    // 初期状態を設定
    UpdateViewerToolbarState();
}

void LayoutViewerToolbarLabels()
{
    // ページカウンター（左TB内）
    if (g_app.wnd.hwndPageCounter && g_app.wnd.hwndViewerTbLeft && g_pageCounterSepIndex >= 0)
    {
        RECT rc;
        SendMessageW(g_app.wnd.hwndViewerTbLeft, TB_GETITEMRECT, g_pageCounterSepIndex, (LPARAM)&rc);
        MoveWindow(g_app.wnd.hwndPageCounter, rc.left, rc.top + 2,
                   rc.right - rc.left, rc.bottom - rc.top - 4, TRUE);
    }

    // ズーム率（右TB内）
    if (g_app.wnd.hwndZoomLabel && g_app.wnd.hwndViewerTbRight && g_zoomLabelSepIndex >= 0)
    {
        RECT rc;
        SendMessageW(g_app.wnd.hwndViewerTbRight, TB_GETITEMRECT, g_zoomLabelSepIndex, (LPARAM)&rc);
        MoveWindow(g_app.wnd.hwndZoomLabel, rc.left, rc.top + 2,
                   rc.right - rc.left, rc.bottom - rc.top - 4, TRUE);
    }
}

void UpdatePageCounter(int current, int total)
{
    if (!g_app.wnd.hwndPageCounter) return;
    wchar_t buf[64];
    swprintf_s(buf, _countof(buf), L"%d / %d", current, total);
    SetWindowTextW(g_app.wnd.hwndPageCounter, buf);
}

void UpdateZoomLabel()
{
    if (!g_app.wnd.hwndZoomLabel) return;
    wchar_t buf[32];
    swprintf_s(buf, _countof(buf), L"%d%%", ViewerGetEffectiveZoomPercent());
    SetWindowTextW(g_app.wnd.hwndZoomLabel, buf);
}

// 右ツールバーのボタン押下状態を更新（スケールモード、表示モード、綴じ方向）
void UpdateViewerToolbarState()
{
    HWND tb = g_app.wnd.hwndViewerTbRight;
    if (!tb) return;

    auto SetChecked = [tb](UINT cmd, bool checked) {
        SendMessageW(tb, TB_CHECKBUTTON, cmd, MAKELONG(checked, 0));
    };

    // スケールモード
    SetChecked(IDM_VIEW_FIT_WINDOW, g_app.viewer.scaleMode == FitWindow);
    SetChecked(IDM_VIEW_FIT_WIDTH,  g_app.viewer.scaleMode == FitWidth);
    SetChecked(IDM_VIEW_FIT_HEIGHT, g_app.viewer.scaleMode == FitHeight);
    SetChecked(IDM_VIEW_ORIGINAL,   g_app.viewer.scaleMode == Original);

    // 表示モード
    SetChecked(IDM_VIEW_AUTO,   g_app.viewer.viewMode == 0);
    SetChecked(IDM_VIEW_SINGLE, g_app.viewer.viewMode == 1);
    SetChecked(IDM_VIEW_SPREAD, g_app.viewer.viewMode == 2);

    // 綴じ方向: アイコンを切り替え（→=LTR, ←=RTL）
    TBBUTTONINFOW tbi = {};
    tbi.cbSize = sizeof(tbi);
    tbi.dwMask = TBIF_IMAGE;
    tbi.iImage = g_app.viewer.isRTL ? 14 : 13;  // 14=←, 13=→
    SendMessageW(tb, TB_SETBUTTONINFOW, IDM_VIEW_BINDING, (LPARAM)&tbi);
}
