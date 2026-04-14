#include "help.h"
#include "app.h"
#include "i18n.h"
#include <commctrl.h>
#include <winhttp.h>
#include <thread>
#include <string>

#pragma comment(lib, "winhttp.lib")

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
    IDC_HELP_TAB      = 1100,
    IDC_HELP_CLOSE    = 1110,
    IDC_CHECK_UPDATE  = 1120,
};

// 更新確認結果をUIスレッドに通知するメッセージ
static constexpr UINT WM_UPDATE_CHECK_RESULT = WM_USER + 500;
// wParam: 0=error, 1=latest, 2=available
// lParam: UpdateResult* (ヒープ確保、UIスレッドで delete)

struct UpdateResult {
    std::wstring version;
    std::wstring url;
};

static HWND g_helpTabCtrl = nullptr;
static HFONT g_helpFont = nullptr;
static HFONT g_helpFontBold = nullptr;
static HFONT g_helpTitleFont = nullptr;
static std::vector<HWND> g_helpTabControls[TAB_COUNT];
static HWND g_updateBtn = nullptr;
static HWND g_updateLabel = nullptr;
static std::wstring g_downloadUrl; // UIスレッドのみで読み書き

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

// バージョン比較: a > b なら正、a < b なら負、等しければ0
static int CompareVersions(const std::wstring& a, const std::wstring& b)
{
    auto parse = [](const std::wstring& s, int parts[3]) {
        parts[0] = parts[1] = parts[2] = 0;
        int idx = 0;
        const wchar_t* p = s.c_str();
        if (*p == L'v' || *p == L'V') p++;
        for (; *p && idx < 3; ) {
            parts[idx] = _wtoi(p);
            idx++;
            while (*p && *p != L'.') p++;
            if (*p == L'.') p++;
        }
    };
    int pa[3], pb[3];
    parse(a, pa);
    parse(b, pb);
    for (int i = 0; i < 3; i++) {
        if (pa[i] != pb[i]) return (pa[i] > pb[i]) ? 1 : -1;
    }
    return 0;
}

// GitHub APIから最新リリース情報を取得（バックグラウンドスレッド）
static void CheckUpdateThread(HWND hDlg)
{
    std::wstring tagName;
    std::wstring htmlUrl;
    bool ok = false;

    HINTERNET hSession = WinHttpOpen(L"karikari-updater/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession) {
        // タイムアウト設定（10秒）
        DWORD timeout = 10000;
        WinHttpSetOption(hSession, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
        WinHttpSetOption(hSession, WINHTTP_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
        WinHttpSetOption(hSession, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

        HINTERNET hConnect = WinHttpConnect(hSession, L"api.github.com",
            INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (hConnect) {
            HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET",
                L"/repos/megamega39/karikari/releases/latest",
                nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
            if (hRequest) {
                if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                        WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
                    WinHttpReceiveResponse(hRequest, nullptr))
                {
                    std::string body;
                    DWORD bytesRead = 0;
                    char buf[4096];
                    static constexpr size_t kMaxBodySize = 65536; // 64KB上限
                    while (WinHttpReadData(hRequest, buf, sizeof(buf), &bytesRead) && bytesRead > 0) {
                        body.append(buf, bytesRead);
                        if (body.size() > kMaxBodySize) break;
                    }

                    if (body.size() <= kMaxBodySize) {
                        // 簡易JSON解析: "tag_name": "v1.0.1"
                        auto extractField = [&](const std::string& json, const char* field) -> std::wstring {
                            std::string key = std::string("\"") + field + "\"";
                            size_t pos = json.find(key);
                            if (pos == std::string::npos) return L"";
                            pos = json.find('"', pos + key.size() + 1);
                            if (pos == std::string::npos) return L"";
                            size_t end = json.find('"', pos + 1);
                            if (end == std::string::npos) return L"";
                            std::string val = json.substr(pos + 1, end - pos - 1);
                            std::wstring wval(val.begin(), val.end());
                            return wval;
                        };

                        tagName = extractField(body, "tag_name");
                        htmlUrl = extractField(body, "html_url");
                        if (!tagName.empty()) ok = true;
                    }
                }
                WinHttpCloseHandle(hRequest);
            }
            WinHttpCloseHandle(hConnect);
        }
        WinHttpCloseHandle(hSession);
    }

    // 結果をヒープに確保してlParamで渡す（スレッド安全）
    if (!ok) {
        PostMessageW(hDlg, WM_UPDATE_CHECK_RESULT, 0, 0);
    } else {
        int cmp = CompareVersions(tagName, KARIKARI_VERSION);
        if (cmp > 0) {
            auto* result = new UpdateResult{ tagName, htmlUrl };
            PostMessageW(hDlg, WM_UPDATE_CHECK_RESULT, 2, (LPARAM)result);
        } else {
            PostMessageW(hDlg, WM_UPDATE_CHECK_RESULT, 1, 0);
        }
    }
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
        // SysLinkクリック
        if ((pnm->code == NM_CLICK || pnm->code == NM_RETURN) &&
            pnm->hwndFrom == g_updateLabel && !g_downloadUrl.empty())
        {
            // URLスキーム検証（https://github.com/ のみ許可）
            if (g_downloadUrl.rfind(L"https://github.com/", 0) == 0)
                ShellExecuteW(nullptr, L"open", g_downloadUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
        break;
    }
    case WM_CTLCOLORSTATIC:
    {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        return (INT_PTR)GetSysColorBrush(COLOR_BTNFACE);
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_HELP_CLOSE || LOWORD(wParam) == IDCANCEL)
        {
            DestroyWindow(hDlg);
            return TRUE;
        }
        if (LOWORD(wParam) == IDC_CHECK_UPDATE)
        {
            // ボタンを無効化して確認中表示
            EnableWindow(g_updateBtn, FALSE);
            SetWindowTextW(g_updateBtn, I18nGet(L"help.update.checking").c_str());
            if (g_updateLabel) { SetWindowTextW(g_updateLabel, L""); ShowWindow(g_updateLabel, SW_HIDE); }
            g_downloadUrl.clear();
            // バックグラウンドスレッドで確認
            std::thread(CheckUpdateThread, hDlg).detach();
            return TRUE;
        }
        break;
    case WM_UPDATE_CHECK_RESULT:
    {
        // ボタンを再度有効化
        SetWindowTextW(g_updateBtn, I18nGet(L"help.checkupdate").c_str());
        EnableWindow(g_updateBtn, TRUE);

        if (wParam == 0) {
            // エラー
            if (g_updateLabel) {
                SetWindowTextW(g_updateLabel, I18nGet(L"help.update.error").c_str());
                ShowWindow(g_updateLabel, SW_SHOW);
            }
        } else if (wParam == 1) {
            // 最新
            if (g_updateLabel) {
                SetWindowTextW(g_updateLabel, I18nGet(L"help.update.latest").c_str());
                ShowWindow(g_updateLabel, SW_SHOW);
            }
        } else if (wParam == 2 && lParam) {
            // 更新あり: ヒープから結果を受け取る
            auto* result = reinterpret_cast<UpdateResult*>(lParam);
            g_downloadUrl = result->url;
            if (g_updateLabel) {
                wchar_t verBuf[128];
                swprintf_s(verBuf, I18nGet(L"help.update.available").c_str(), result->version.c_str());
                std::wstring linkText = std::wstring(L"<a>") + verBuf + L"</a>";
                SetWindowTextW(g_updateLabel, linkText.c_str());
                ShowWindow(g_updateLabel, SW_SHOW);
            }
            delete result;
        }
        return TRUE;
    }
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

    // フォント作成（毎回DPIに合わせて作成、終了時に解放）
    g_helpFont = CreateFontW(-S(13), 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
        0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    g_helpFontBold = CreateFontW(-S(13), 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
        0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    g_helpTitleFont = CreateFontW(-S(20), 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
        0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");

    for (int i = 0; i < TAB_COUNT; i++)
        g_helpTabControls[i].clear();

    // 静的変数リセット
    g_updateBtn = nullptr;
    g_updateLabel = nullptr;
    g_downloadUrl.clear();

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

        HWND hTitle = CreateWindowExW(0, L"STATIC", L"karikari",
            WS_CHILD | SS_LEFT, tx, ay, contentW, S(28), hDlg, nullptr, g_app.hInstance, nullptr);
        SendMessageW(hTitle, WM_SETFONT, (WPARAM)g_helpTitleFont, TRUE);
        AL(hTitle);
        ay += S(32);

        // バージョン表示（動的生成）
        std::wstring verText = I18nGet(L"help.version");
        {
            size_t lastSpace = verText.rfind(L' ');
            if (lastSpace != std::wstring::npos)
                verText = verText.substr(0, lastSpace + 1) + KARIKARI_VERSION;
        }
        AL(MkLabel(hDlg, tx, ay, contentW, S(16), verText.c_str()));
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
        ay += S(40);

        // 更新確認ボタン
        g_updateBtn = MkButton(hDlg, tx, ay, S(160), S(28), IDC_CHECK_UPDATE,
            I18nGet(L"help.checkupdate").c_str());
        AL(g_updateBtn);

        // 結果表示（SysLink — 更新ありの場合クリックでリリースページを開く）
        g_updateLabel = CreateWindowExW(0, WC_LINK, L"",
            WS_CHILD, tx + S(170), ay + S(4), contentW - S(170), S(20),
            hDlg, nullptr, g_app.hInstance, nullptr);
        SendMessageW(g_updateLabel, WM_SETFONT, (WPARAM)g_helpFont, TRUE);
        AL(g_updateLabel);
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

    // フォント解放
    if (g_helpFont) { DeleteObject(g_helpFont); g_helpFont = nullptr; }
    if (g_helpFontBold) { DeleteObject(g_helpFontBold); g_helpFontBold = nullptr; }
    if (g_helpTitleFont) { DeleteObject(g_helpTitleFont); g_helpTitleFont = nullptr; }
}
