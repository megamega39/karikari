#include "help.h"
#include "app.h"
#include <commctrl.h>

// === ヘルプエントリ ===
struct HelpEntry {
    const wchar_t* operation;
    const wchar_t* method;
};

// --- ナビゲーション ---
static const HelpEntry g_navEntries[] = {
    { L"戻る / 進む",           L"Alt+← / Alt+→" },
    { L"履歴一覧",             L"戻る/進むボタンの▼をクリック" },
    { L"上の階層",             L"Alt+↑" },
    { L"前/次の書庫に移動",     L"PageUp / PageDown" },
    { L"更新",                 L"F5" },
    { L"アドレスバー",          L"各パーツをクリックで直接移動" },
    { L"アドレス編集",          L"アドレスバーをクリック → 入力 → Enter" },
};

// --- 表示モード ---
static const HelpEntry g_viewEntries[] = {
    { L"全画面",               L"F11" },
    { L"ウィンドウに合わせる",   L"W" },
    { L"単ページ表示",          L"1" },
    { L"見開き表示",            L"2" },
    { L"自動表示",             L"3" },
    { L"綴じ方向切替",          L"B" },
    { L"左綴じ / 右綴じ",       L"L / R" },
    { L"表示モード切替",        L"V" },
    { L"ズームイン / アウト",    L"Ctrl++ / Ctrl+-" },
    { L"ズームリセット",        L"Ctrl+0" },
    { L"時計回り回転",          L"Ctrl+R" },
    { L"反時計回り回転",        L"Ctrl+Shift+R" },
};

// --- お気に入り・本棚 ---
static const HelpEntry g_favEntries[] = {
    { L"お気に入り表示",        L"C" },
    { L"本棚表示",             L"本棚ボタンをクリック" },
    { L"履歴表示",             L"履歴ボタンをクリック" },
    { L"通常モードに戻る",      L"本棚/履歴ボタンを再度クリック" },
};

// --- 動画・音声 ---
static const HelpEntry g_mediaEntries[] = {
    { L"再生 / 一時停止",       L"Space" },
    { L"再生 / 一時停止",       L"動画画面をクリック" },
    { L"全画面切替",            L"動画画面をダブルクリック" },
};

// --- ファイル操作 ---
static const HelpEntry g_fileEntries[] = {
    { L"フォルダ/書庫を開く",    L"Enter" },
    { L"前/次のページ",         L"← / →" },
    { L"最初/最後のページ",      L"Home / End" },
    { L"ファイルリスト移動",     L"↑ / ↓" },
    { L"画像をコピー",          L"Ctrl+C" },
    { L"名前変更",             L"F2" },
    { L"ファイル削除",          L"Delete" },
};

// --- タブ定義 ---
struct HelpTabDef {
    const wchar_t* tabName;
    const HelpEntry* entries;
    int entryCount;
};

static const HelpTabDef g_helpTabs[] = {
    { L"ナビゲーション",    g_navEntries,   _countof(g_navEntries) },
    { L"表示モード",       g_viewEntries,  _countof(g_viewEntries) },
    { L"お気に入り・本棚",  g_favEntries,   _countof(g_favEntries) },
    { L"動画・音声",       g_mediaEntries, _countof(g_mediaEntries) },
    { L"ファイル操作",     g_fileEntries,  _countof(g_fileEntries) },
    { L"このアプリについて", nullptr,        0 },
};

static constexpr int TAB_COUNT = _countof(g_helpTabs);
static constexpr int TAB_ABOUT = TAB_COUNT - 1;

// コントロールID
enum {
    IDC_HELP_TAB   = 1100,
    IDC_HELP_CLOSE = 1110,
};

// グローバル（ダイアログ表示中のみ使用）
static HWND g_helpTabCtrl = nullptr;
static HFONT g_helpFont = nullptr;
static HFONT g_helpFontBold = nullptr;
static std::vector<HWND> g_helpTabControls[TAB_COUNT];

// === ヘルパー ===
static HWND MkLabel(HWND hParent, int x, int y, int w, int h, const wchar_t* text, bool bold = false)
{
    HWND hw = CreateWindowExW(0, L"STATIC", text,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x, y, w, h, hParent, nullptr, g_app.hInstance, nullptr);
    SendMessageW(hw, WM_SETFONT, (WPARAM)(bold ? g_helpFontBold : g_helpFont), TRUE);
    return hw;
}

static HWND MkButton(HWND hParent, int x, int y, int w, int h, UINT id, const wchar_t* text)
{
    HWND hw = CreateWindowExW(0, L"BUTTON", text,
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x, y, w, h, hParent, (HMENU)(INT_PTR)id, g_app.hInstance, nullptr);
    SendMessageW(hw, WM_SETFONT, (WPARAM)g_helpFont, TRUE);
    return hw;
}

static HWND CreateHelpListView(HWND hDlg, int x, int y, int w, int h,
                                const HelpEntry* entries, int count)
{
    HWND hList = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
        WS_CHILD | LVS_REPORT | LVS_SINGLESEL | LVS_NOSORTHEADER,
        x, y, w, h, hDlg, nullptr, g_app.hInstance, nullptr);
    SendMessageW(hList, WM_SETFONT, (WPARAM)g_helpFont, TRUE);
    SendMessageW(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
        LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    LVCOLUMNW lvc = {};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;
    lvc.cx = w * 50 / 100;
    lvc.pszText = (LPWSTR)L"操作";
    SendMessageW(hList, LVM_INSERTCOLUMNW, 0, (LPARAM)&lvc);
    lvc.cx = w * 50 / 100;
    lvc.pszText = (LPWSTR)L"方法";
    SendMessageW(hList, LVM_INSERTCOLUMNW, 1, (LPARAM)&lvc);

    for (int i = 0; i < count; i++)
    {
        LVITEMW lvi = {};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;
        lvi.pszText = (LPWSTR)entries[i].operation;
        SendMessageW(hList, LVM_INSERTITEMW, 0, (LPARAM)&lvi);
        lvi.iSubItem = 1;
        lvi.pszText = (LPWSTR)entries[i].method;
        SendMessageW(hList, LVM_SETITEMTEXTW, i, (LPARAM)&lvi);
    }
    return hList;
}

// === タブ切替 ===
static void ShowHelpTabPage(int tabIndex)
{
    for (int i = 0; i < TAB_COUNT; i++)
        for (auto h : g_helpTabControls[i])
            ShowWindow(h, (i == tabIndex) ? SW_SHOW : SW_HIDE);
}

// === ダイアログプロシージャ ===
static INT_PTR CALLBACK HelpDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_NOTIFY:
    {
        LPNMHDR pnm = (LPNMHDR)lParam;
        if (pnm->idFrom == IDC_HELP_TAB && pnm->code == TCN_SELCHANGE)
        {
            int sel = (int)SendMessageW(g_helpTabCtrl, TCM_GETCURSEL, 0, 0);
            ShowHelpTabPage(sel);
        }
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_HELP_CLOSE || LOWORD(wParam) == IDCANCEL)
        {
            DestroyWindow(hDlg);
            return TRUE;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hDlg);
        return TRUE;
    }
    return FALSE;
}

// === ShowHelpDialog ===
void ShowHelpDialog(HWND hwndParent)
{
    // DPI対応
    int dpi = 96;
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        auto fn = (UINT(WINAPI*)(HWND))GetProcAddress(hUser32, "GetDpiForWindow");
        if (fn) dpi = fn(hwndParent);
    }
    auto S = [dpi](int v) -> int { return MulDiv(v, dpi, 96); };

    int DW = S(520), DH = S(440);

    // 親ウィンドウ中央に配置
    RECT parentRc;
    GetWindowRect(hwndParent, &parentRc);
    int px = parentRc.left + (parentRc.right - parentRc.left - DW) / 2;
    int py = parentRc.top + (parentRc.bottom - parentRc.top - DH) / 2;

    // ダイアログテンプレート
    struct { DLGTEMPLATE dt; WORD menu, cls, title; } tmpl = {};
    tmpl.dt.style = DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU;
    tmpl.dt.cx = 1; tmpl.dt.cy = 1;

    HWND hDlg = CreateDialogIndirectW(g_app.hInstance, &tmpl.dt, hwndParent, HelpDlgProc);
    if (!hDlg) return;
    SetWindowTextW(hDlg, L"ヘルプ - karikari の使い方");

    // ピクセルサイズに補正
    RECT rc = { 0, 0, DW, DH };
    AdjustWindowRectEx(&rc, GetWindowLong(hDlg, GWL_STYLE), FALSE, GetWindowLong(hDlg, GWL_EXSTYLE));
    SetWindowPos(hDlg, nullptr, px, py, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);

    // フォント
    g_helpFont = CreateFontW(-S(13), 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
        0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    g_helpFontBold = CreateFontW(-S(13), 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
        0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");

    // タブコントロールの各タブ用コントロール配列をクリア
    for (int i = 0; i < TAB_COUNT; i++)
        g_helpTabControls[i].clear();

    int W = DW - S(10);

    // タブコントロール
    g_helpTabCtrl = CreateWindowExW(0, WC_TABCONTROLW, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        S(4), S(4), W, DH - S(44), hDlg, (HMENU)(INT_PTR)IDC_HELP_TAB, g_app.hInstance, nullptr);
    SendMessageW(g_helpTabCtrl, WM_SETFONT, (WPARAM)g_helpFont, TRUE);

    // タブ挿入
    TCITEMW tci = {};
    tci.mask = TCIF_TEXT;
    for (int i = 0; i < TAB_COUNT; i++)
    {
        tci.pszText = (LPWSTR)g_helpTabs[i].tabName;
        SendMessageW(g_helpTabCtrl, TCM_INSERTITEMW, i, (LPARAM)&tci);
    }

    // タブ内のコンテンツ領域
    int tx = S(14), ty = S(30);
    int listW = W - S(20);
    int listH = DH - S(44) - S(32) - S(8);

    // タブ0-4: ListView
    for (int i = 0; i < TAB_ABOUT; i++)
    {
        HWND hList = CreateHelpListView(hDlg, tx, ty, listW, listH,
            g_helpTabs[i].entries, g_helpTabs[i].entryCount);
        g_helpTabControls[i].push_back(hList);
    }

    // タブ5: このアプリについて
    {
        int ay = ty + S(20);
        auto AL = [&](HWND h) { g_helpTabControls[TAB_ABOUT].push_back(h); return h; };

        // アプリ名（大きめフォント）
        static HFONT titleFont = CreateFontW(-S(20), 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
            0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        HWND hTitle = CreateWindowExW(0, L"STATIC", L"karikari",
            WS_CHILD | SS_LEFT, tx, ay, listW, S(28), hDlg, nullptr, g_app.hInstance, nullptr);
        SendMessageW(hTitle, WM_SETFONT, (WPARAM)titleFont, TRUE);
        AL(hTitle);
        ay += S(32);

        AL(MkLabel(hDlg, tx, ay, listW, S(16), L"バージョン 1.0.0"));
        ay += S(24);

        AL(MkLabel(hDlg, tx, ay, listW, S(16), L"最速の画像・動画ビューア"));
        ay += S(36);

        AL(MkLabel(hDlg, tx, ay, listW, S(16), L"作者:", true));
        ay += S(20);
        AL(MkLabel(hDlg, tx + S(8), ay, listW, S(16), L"megamega39"));
        ay += S(32);

        AL(MkLabel(hDlg, tx, ay, listW, S(16), L"説明:", true));
        ay += S(20);
        AL(MkLabel(hDlg, tx + S(8), ay, listW, S(32),
            L"C++ + Win32 API + Direct2D で構築した高速画像/動画ビューア。"));
        ay += S(24);
        AL(MkLabel(hDlg, tx + S(8), ay, listW, S(16),
            L"leeyez_kai (.NET版) のネイティブ移植。"));
    }

    // 閉じるボタン（下部中央）
    int btnW = S(80), btnH = S(26);
    MkButton(hDlg, (DW - btnW) / 2, DH - S(34), btnW, btnH, IDC_HELP_CLOSE, L"閉じる");

    // 初期タブ表示
    ShowHelpTabPage(0);

    // モーダル表示
    ShowWindow(hDlg, SW_SHOW);
    EnableWindow(hwndParent, FALSE);

    MSG msg;
    BOOL bRet;
    while (IsWindow(hDlg) && (bRet = GetMessageW(&msg, nullptr, 0, 0)) != 0)
    {
        if (bRet == -1) break; // GetMessageWエラー
        if (IsDialogMessageW(hDlg, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    EnableWindow(hwndParent, TRUE);
    SetForegroundWindow(hwndParent);

    // フォントクリーンアップ
    if (g_helpFont) { DeleteObject(g_helpFont); g_helpFont = nullptr; }
    if (g_helpFontBold) { DeleteObject(g_helpFontBold); g_helpFontBold = nullptr; }
}
