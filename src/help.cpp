#include "help.h"
#include "app.h"
#include "i18n.h"
#include <commctrl.h>

// タブのi18nキー
static const wchar_t* g_tabKeys[] = {
    L"help.tab.basic", L"help.tab.view", L"help.tab.tree",
    L"help.tab.file", L"help.tab.settings", L"help.tab.about"
};
// テキストタブのi18nキー
static const wchar_t* g_helpTextKeys[] = {
    L"help.text.basic", L"help.text.view", L"help.text.tree",
    L"help.text.file", L"help.text.settings"
};
static constexpr int TAB_COUNT = _countof(g_tabKeys);
static constexpr int TAB_ABOUT = TAB_COUNT - 1;
static constexpr int TEXT_TAB_COUNT = _countof(g_helpTextKeys);

// コントロールID
enum {
    IDC_HELP_TAB   = 1100,
    IDC_HELP_CLOSE = 1110,
};

static HWND g_helpTabCtrl = nullptr;
static HFONT g_helpFont = nullptr;
static HFONT g_helpFontBold = nullptr;
static std::vector<HWND> g_helpTabControls[TAB_COUNT];

// ヘルパー
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

// タブ切替
static void ShowHelpTabPage(int tabIndex)
{
    for (int i = 0; i < TAB_COUNT; i++)
        for (auto h : g_helpTabControls[i])
            ShowWindow(h, (i == tabIndex) ? SW_SHOW : SW_HIDE);
}

// ダイアログプロシージャ
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

void ShowHelpDialog(HWND hwndParent)
{
    // DPI
    int dpi = 96;
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        auto fn = (UINT(WINAPI*)(HWND))GetProcAddress(hUser32, "GetDpiForWindow");
        if (fn) dpi = fn(hwndParent);
    }
    auto S = [dpi](int v) -> int { return MulDiv(v, dpi, 96); };
    int DW = S(780), DH = S(660);

    RECT parentRc;
    GetWindowRect(hwndParent, &parentRc);
    int px = parentRc.left + (parentRc.right - parentRc.left - DW) / 2;
    int py = parentRc.top + (parentRc.bottom - parentRc.top - DH) / 2;

    struct { DLGTEMPLATE dt; WORD menu, cls, title; } tmpl = {};
    tmpl.dt.style = DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU;
    tmpl.dt.cx = 1; tmpl.dt.cy = 1;

    HWND hDlg = CreateDialogIndirectW(g_app.hInstance, &tmpl.dt, hwndParent, HelpDlgProc);
    if (!hDlg) return;
    SetWindowTextW(hDlg, I18nGet(L"help.title").c_str());

    RECT rc = { 0, 0, DW, DH };
    AdjustWindowRectEx(&rc, GetWindowLong(hDlg, GWL_STYLE), FALSE, GetWindowLong(hDlg, GWL_EXSTYLE));
    SetWindowPos(hDlg, nullptr, px, py, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);

    g_helpFont = CreateFontW(-S(13), 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
        0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    g_helpFontBold = CreateFontW(-S(13), 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
        0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");

    for (int i = 0; i < TAB_COUNT; i++)
        g_helpTabControls[i].clear();

    int W = DW - S(10);

    // タブコントロール
    g_helpTabCtrl = CreateWindowExW(0, WC_TABCONTROLW, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        S(4), S(4), W, DH - S(44), hDlg, (HMENU)(INT_PTR)IDC_HELP_TAB, g_app.hInstance, nullptr);
    SendMessageW(g_helpTabCtrl, WM_SETFONT, (WPARAM)g_helpFont, TRUE);

    TCITEMW tci = {};
    tci.mask = TCIF_TEXT;
    for (int i = 0; i < TAB_COUNT; i++)
    {
        tci.pszText = (LPWSTR)I18nGet(g_tabKeys[i]).c_str();
        SendMessageW(g_helpTabCtrl, TCM_INSERTITEMW, i, (LPARAM)&tci);
    }

    int tx = S(14), ty = S(30);
    int contentW = W - S(20);
    int contentH = DH - S(44) - S(32) - S(8);

    // テキストタブ: 読み取り専用のマルチラインEditコントロール
    for (int i = 0; i < TEXT_TAB_COUNT; i++)
    {
        HWND hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL,
            tx, ty, contentW, contentH, hDlg, nullptr, g_app.hInstance, nullptr);
        SetWindowTextW(hEdit, I18nGet(g_helpTextKeys[i]).c_str());
        SendMessageW(hEdit, WM_SETFONT, (WPARAM)g_helpFont, TRUE);
        g_helpTabControls[i].push_back(hEdit);
    }

    // 「このアプリについて」タブ
    {
        int ay = ty + S(20);
        auto AL = [&](HWND h) { g_helpTabControls[TAB_ABOUT].push_back(h); return h; };

        static HFONT titleFont = CreateFontW(-S(20), 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
            0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
        HWND hTitle = CreateWindowExW(0, L"STATIC", L"karikari",
            WS_CHILD | SS_LEFT, tx, ay, contentW, S(28), hDlg, nullptr, g_app.hInstance, nullptr);
        SendMessageW(hTitle, WM_SETFONT, (WPARAM)titleFont, TRUE);
        AL(hTitle);
        ay += S(32);

        AL(MkLabel(hDlg, tx, ay, contentW, S(16), I18nGet(L"help.version").c_str()));
        ay += S(24);
        AL(MkLabel(hDlg, tx, ay, contentW, S(16), I18nGet(L"help.subtitle").c_str()));
        ay += S(36);
        AL(MkLabel(hDlg, tx, ay, contentW, S(16), I18nGet(L"help.authorlabel").c_str(), true));
        ay += S(20);
        AL(MkLabel(hDlg, tx + S(8), ay, contentW, S(16), I18nGet(L"help.authorname").c_str()));
        ay += S(32);
        AL(MkLabel(hDlg, tx, ay, contentW, S(16), I18nGet(L"help.desclabel").c_str(), true));
        ay += S(20);
        AL(MkLabel(hDlg, tx + S(8), ay, contentW, S(32),
            I18nGet(L"help.desc1").c_str()));
        ay += S(24);
        AL(MkLabel(hDlg, tx + S(8), ay, contentW, S(16),
            I18nGet(L"help.desc2").c_str()));
    }

    // 閉じるボタン
    int btnW = S(80), btnH = S(26);
    MkButton(hDlg, (DW - btnW) / 2, DH - S(34), btnW, btnH, IDC_HELP_CLOSE, I18nGet(L"help.close").c_str());

    ShowHelpTabPage(0);

    // モーダル
    ShowWindow(hDlg, SW_SHOW);
    EnableWindow(hwndParent, FALSE);

    MSG msg;
    BOOL bRet;
    while (IsWindow(hDlg) && (bRet = GetMessageW(&msg, nullptr, 0, 0)) != 0)
    {
        if (bRet == -1) break;
        if (IsDialogMessageW(hDlg, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    EnableWindow(hwndParent, TRUE);
    SetForegroundWindow(hwndParent);

    if (g_helpFont) { DeleteObject(g_helpFont); g_helpFont = nullptr; }
    if (g_helpFontBold) { DeleteObject(g_helpFontBold); g_helpFontBold = nullptr; }
}
