#include "settings.h"
#include "utils.h"
#include "i18n.h"
#include "cache.h"
#include "prefetch.h"
#include "nav.h"
#include <shlwapi.h>
#include <commctrl.h>

std::wstring GetSettingsPath()
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    PathRemoveFileSpecW(path);
    PathAppendW(path, L"settings.json");
    return path;
}

// JSON関数は utils.h に共通化済み

// === 設定キャッシュ ===
static AppSettings g_cachedSettings;
static bool g_settingsLoaded = false;

const AppSettings& GetCachedSettings()
{
    if (!g_settingsLoaded)
    {
        LoadSettings(g_cachedSettings);
        g_settingsLoaded = true;
    }
    return g_cachedSettings;
}

void InvalidateSettingsCache()
{
    g_settingsLoaded = false;
}

bool LoadSettings(AppSettings& s)
{
    std::wstring json = ReadFileToWString(GetSettingsPath());
    if (json.empty()) return false;

    int x, y, w, h;
    if (JsonGetInt(json, L"windowX", x) && JsonGetInt(json, L"windowY", y) &&
        JsonGetInt(json, L"windowW", w) && JsonGetInt(json, L"windowH", h))
        s.windowRect = { x, y, x + w, y + h };

    JsonGetBool(json, L"maximized", s.maximized);
    JsonGetInt(json, L"mainSplitPos", s.mainSplitPos);
    JsonGetInt(json, L"sidebarSplitPos", s.sidebarSplitPos);
    JsonGetString(json, L"lastPath", s.lastPath);
    JsonGetInt(json, L"lastFileIndex", s.lastFileIndex);
    JsonGetString(json, L"lastImagePath", s.lastImagePath);
    JsonGetInt(json, L"cacheSizeMB", s.cacheSizeMB);
    JsonGetInt(json, L"prefetchCount", s.prefetchCount);
    JsonGetString(json, L"language", s.language);
    JsonGetBool(json, L"wrapNavigation", s.wrapNavigation);
    JsonGetBool(json, L"spreadFirstSingle", s.spreadFirstSingle);
    JsonGetBool(json, L"recursiveLoad", s.recursiveLoad);
    JsonGetBool(json, L"autoPlayMedia", s.autoPlayMedia);
    JsonGetFloat(json, L"spreadThreshold", s.spreadThreshold);
    JsonGetInt(json, L"thumbnailSize", s.thumbnailSize);
    JsonGetInt(json, L"previewSize", s.previewSize);
    JsonGetInt(json, L"fontSize", s.fontSize);
    JsonGetString(json, L"treeSortMode", s.treeSortMode);
    JsonGetBool(json, L"treeSortDescending", s.treeSortDescending);
    JsonGetInt(json, L"viewMode", s.viewMode);
    JsonGetBool(json, L"isRTL", s.isRTL);
    JsonGetInt(json, L"scaleMode", s.scaleMode);

    // 設定値の範囲クランプ
    s.prefetchCount = std::max(1, std::min(s.prefetchCount, 50));
    s.thumbnailSize = std::max(48, std::min(s.thumbnailSize, 512));
    s.previewSize = std::max(100, std::min(s.previewSize, 1024));
    s.fontSize = std::max(6, std::min(s.fontSize, 24));
    s.cacheSizeMB = std::max(50, std::min(s.cacheSizeMB, 2000));
    s.viewMode = std::max(0, std::min(s.viewMode, 2));
    s.scaleMode = std::max(0, std::min(s.scaleMode, 3));

    return true;
}

bool SaveSettings(const AppSettings& s)
{
    int w = s.windowRect.right - s.windowRect.left;
    int h = s.windowRect.bottom - s.windowRect.top;

    wchar_t threshBuf[32];
    swprintf_s(threshBuf, _countof(threshBuf), L"%.2f", s.spreadThreshold);

    std::wstring json;
    json += L"{\n";
    json += L"  \"windowX\": " + std::to_wstring(s.windowRect.left) + L",\n";
    json += L"  \"windowY\": " + std::to_wstring(s.windowRect.top) + L",\n";
    json += L"  \"windowW\": " + std::to_wstring(w) + L",\n";
    json += L"  \"windowH\": " + std::to_wstring(h) + L",\n";
    json += L"  \"maximized\": " + std::wstring(s.maximized ? L"true" : L"false") + L",\n";
    json += L"  \"mainSplitPos\": " + std::to_wstring(s.mainSplitPos) + L",\n";
    json += L"  \"sidebarSplitPos\": " + std::to_wstring(s.sidebarSplitPos) + L",\n";
    json += L"  \"lastPath\": \"" + EscapeJsonPath(s.lastPath) + L"\",\n";
    json += L"  \"lastFileIndex\": " + std::to_wstring(s.lastFileIndex) + L",\n";
    json += L"  \"lastImagePath\": \"" + EscapeJsonPath(s.lastImagePath) + L"\",\n";
    json += L"  \"cacheSizeMB\": " + std::to_wstring(s.cacheSizeMB) + L",\n";
    json += L"  \"prefetchCount\": " + std::to_wstring(s.prefetchCount) + L",\n";
    json += L"  \"language\": \"" + EscapeJsonPath(s.language) + L"\",\n";
    json += L"  \"wrapNavigation\": " + std::wstring(s.wrapNavigation ? L"true" : L"false") + L",\n";
    json += L"  \"spreadFirstSingle\": " + std::wstring(s.spreadFirstSingle ? L"true" : L"false") + L",\n";
    json += L"  \"recursiveLoad\": " + std::wstring(s.recursiveLoad ? L"true" : L"false") + L",\n";
    json += L"  \"autoPlayMedia\": " + std::wstring(s.autoPlayMedia ? L"true" : L"false") + L",\n";
    json += L"  \"spreadThreshold\": " + std::wstring(threshBuf) + L",\n";
    json += L"  \"thumbnailSize\": " + std::to_wstring(s.thumbnailSize) + L",\n";
    json += L"  \"previewSize\": " + std::to_wstring(s.previewSize) + L",\n";
    json += L"  \"fontSize\": " + std::to_wstring(s.fontSize) + L",\n";
    json += L"  \"treeSortMode\": \"" + s.treeSortMode + L"\",\n";
    json += L"  \"treeSortDescending\": " + std::wstring(s.treeSortDescending ? L"true" : L"false") + L",\n";
    json += L"  \"viewMode\": " + std::to_wstring(s.viewMode) + L",\n";
    json += L"  \"isRTL\": " + std::wstring(s.isRTL ? L"true" : L"false") + L",\n";
    json += L"  \"scaleMode\": " + std::to_wstring(s.scaleMode) + L"\n";
    json += L"}\n";

    bool result = WriteWStringToFile(GetSettingsPath(), json);
    g_settingsLoaded = false; // 次回 GetCachedSettings で再読み込み
    return result;
}

// === キーバインディング ===

static std::vector<KeyBinding> g_keyBindings;

static std::wstring GetKeyBindingsPath()
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    PathRemoveFileSpecW(path);
    PathAppendW(path, L"keybindings.json");
    return path;
}

std::vector<KeyBinding>& GetKeyBindings() { return g_keyBindings; }

void InitDefaultKeyBindings()
{
    g_keyBindings.clear();
    auto Add = [](const wchar_t* action, const wchar_t* label,
                  std::initializer_list<KeyCombo> keys) {
        g_keyBindings.push_back({ action, label, keys });
    };
    auto K = [](UINT vk, bool ctrl = false, bool shift = false, bool alt = false) -> KeyCombo {
        return { vk, ctrl, shift, alt };
    };
    // ナビゲーション
    Add(L"prev_page",      L"前のページ",        { K(VK_LEFT) });
    Add(L"next_page",      L"次のページ",        { K(VK_RIGHT) });
    Add(L"first_page",     L"最初のページ",      { K(VK_HOME) });
    Add(L"last_page",      L"最後のページ",      { K(VK_END) });
    Add(L"prev_archive",   L"前の書庫",          { K(VK_UP) });
    Add(L"next_archive",   L"次の書庫",          { K(VK_DOWN) });
    Add(L"nav_back",       L"戻る",              { K(VK_LEFT, false, false, true) });
    Add(L"nav_forward",    L"進む",              { K(VK_RIGHT, false, false, true) });
    Add(L"nav_up",         L"親フォルダへ",      { K(VK_UP, false, false, true) });
    Add(L"open_item",      L"開く/展開・折畳み", { K(VK_RETURN) });
    // 表示
    Add(L"fit_window",     L"ウィンドウに合わせる", { K('W') });
    Add(L"view_single",    L"単ページ表示",      { K('1') });
    Add(L"view_spread",    L"見開き表示",        { K('2') });
    Add(L"view_auto",      L"自動判定",          { K('3') });
    Add(L"toggle_binding", L"綴じ方向切替",      { K('B') });
    Add(L"fullscreen",     L"全画面切替",        { K(VK_F11) });
    // ズーム
    Add(L"zoom_in",        L"拡大",              { K(VK_OEM_PLUS, true), K(VK_ADD, true) });
    Add(L"zoom_out",       L"縮小",              { K(VK_OEM_MINUS, true), K(VK_SUBTRACT, true) });
    Add(L"zoom_reset",     L"ズームリセット",    { K('0', true), K(VK_NUMPAD0, true) });
    // 編集
    Add(L"copy_image",     L"画像コピー",        { K('C', true) });
    Add(L"rotate_cw",      L"時計回り回転",      { K('R', true) });
    Add(L"rotate_ccw",     L"反時計回り回転",    { K('R', true, true) });
    Add(L"rename",         L"名前変更",          { K(VK_F2) });
    Add(L"delete_file",    L"ファイル削除",      { K(VK_DELETE) });
    // メディア
    Add(L"play_pause",     L"再生/一時停止",     { K(VK_SPACE) });
    // その他
    Add(L"refresh",        L"更新",              { K(VK_F5) });
    Add(L"help",           L"ヘルプ",            { K(VK_F1) });
    Add(L"escape",         L"全画面解除",        { K(VK_ESCAPE) });
}

static std::wstring VkToString(UINT vk)
{
    switch (vk)
    {
    case VK_LEFT:      return L"←";
    case VK_RIGHT:     return L"→";
    case VK_UP:        return L"↑";
    case VK_DOWN:      return L"↓";
    case VK_HOME:      return L"Home";
    case VK_END:       return L"End";
    case VK_RETURN:    return L"Enter";
    case VK_SPACE:     return L"Space";
    case VK_DELETE:    return L"Delete";
    case VK_ESCAPE:    return L"Escape";
    case VK_F1: case VK_F2: case VK_F3: case VK_F4: case VK_F5:
    case VK_F6: case VK_F7: case VK_F8: case VK_F9: case VK_F10:
    case VK_F11: case VK_F12:
        return L"F" + std::to_wstring(vk - VK_F1 + 1);
    case VK_OEM_PLUS:  return L"+";
    case VK_OEM_MINUS: return L"-";
    case VK_ADD:       return L"Num+";
    case VK_SUBTRACT:  return L"Num-";
    case VK_NUMPAD0:   return L"Num0";
    default:
        if (vk >= 'A' && vk <= 'Z') return std::wstring(1, (wchar_t)vk);
        if (vk >= '0' && vk <= '9') return std::wstring(1, (wchar_t)vk);
        return L"?";
    }
}

std::wstring KeyComboToString(const KeyCombo& kc)
{
    std::wstring r;
    if (kc.ctrl) r += L"Ctrl+";
    if (kc.shift) r += L"Shift+";
    if (kc.alt) r += L"Alt+";
    r += VkToString(kc.vk);
    return r;
}

std::wstring KeyBindingToString(const KeyBinding& kb)
{
    std::wstring result;
    for (size_t i = 0; i < kb.keys.size(); i++)
    {
        if (i > 0) result += L", ";
        result += KeyComboToString(kb.keys[i]);
    }
    return result;
}

void LoadKeyBindings()
{
    InitDefaultKeyBindings();

    std::wstring json = ReadFileToWString(GetKeyBindingsPath());
    if (json.empty()) return;

    for (auto& kb : g_keyBindings)
    {
        std::wstring section = L"\"" + kb.action + L"\"";
        auto pos = json.find(section);
        if (pos == std::wstring::npos) continue;

        // [ の後から ] まで読む（配列形式）
        auto arrStart = json.find(L'[', pos);
        auto arrEnd = json.find(L']', arrStart);
        if (arrStart == std::wstring::npos || arrEnd == std::wstring::npos) continue;

        kb.keys.clear();
        // 各 { ... } ブロックを読む
        size_t cur = arrStart;
        while (cur < arrEnd)
        {
            auto braceStart = json.find(L'{', cur);
            if (braceStart == std::wstring::npos || braceStart >= arrEnd) break;
            auto braceEnd = json.find(L'}', braceStart);
            if (braceEnd == std::wstring::npos || braceEnd > arrEnd) break;

            std::wstring block = json.substr(braceStart, braceEnd - braceStart + 1);
            KeyCombo kc;
            int vk = 0;
            if (JsonGetInt(block, L"vk", vk)) kc.vk = (UINT)vk;
            JsonGetBool(block, L"ctrl", kc.ctrl);
            JsonGetBool(block, L"shift", kc.shift);
            JsonGetBool(block, L"alt", kc.alt);
            kb.keys.push_back(kc);

            cur = braceEnd + 1;
        }
    }
}

void SaveKeyBindings()
{
    std::wstring json = L"{\n";
    for (size_t i = 0; i < g_keyBindings.size(); i++)
    {
        auto& kb = g_keyBindings[i];
        json += L"  \"" + kb.action + L"\": [\n";
        for (size_t j = 0; j < kb.keys.size(); j++)
        {
            auto& kc = kb.keys[j];
            json += L"    { \"vk\": " + std::to_wstring(kc.vk);
            json += L", \"ctrl\": " + std::wstring(kc.ctrl ? L"true" : L"false");
            json += L", \"shift\": " + std::wstring(kc.shift ? L"true" : L"false");
            json += L", \"alt\": " + std::wstring(kc.alt ? L"true" : L"false") + L" }";
            json += (j + 1 < kb.keys.size()) ? L",\n" : L"\n";
        }
        json += L"  ]";
        json += (i + 1 < g_keyBindings.size()) ? L",\n" : L"\n";
    }
    json += L"}\n";
    WriteWStringToFile(GetKeyBindingsPath(), json);
}

// === 設定ダイアログ ===

// コントロールID
enum {
    IDC_TAB = 200,
    // 一般タブ
    IDC_LANG_COMBO = 201,
    IDC_WRAP_NAV = 202,
    IDC_SPREAD_FIRST = 203,
    IDC_RECURSIVE = 204,
    IDC_AUTOPLAY = 205,
    IDC_CACHE_EDIT = 206,
    IDC_THRESH_EDIT = 207,
    IDC_THUMB_EDIT = 208,
    IDC_PREVIEW_EDIT = 209,
    IDC_FONT_EDIT = 210,
    // ショートカットタブ
    IDC_KEY_LIST = 220,
    IDC_KEY_ADD = 221,
    IDC_KEY_REMOVE = 222,
    IDC_KEY_RESET = 223,
};

static HFONT g_dlgFont = nullptr;
static HWND g_hTab = nullptr;
static std::vector<HWND> g_generalControls;
static std::vector<HWND> g_shortcutControls;
static HWND g_hKeyList = nullptr;

static HFONT g_dlgFontBold = nullptr;

static HWND MkLabel(HWND hDlg, int x, int y, int w, int h, const wchar_t* text, bool bold = false)
{
    HWND hw = CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y, w, h, hDlg, nullptr, g_app.hInstance, nullptr);
    SendMessageW(hw, WM_SETFONT, (WPARAM)(bold && g_dlgFontBold ? g_dlgFontBold : g_dlgFont), TRUE);
    return hw;
}

static HWND MkEdit(HWND hDlg, int x, int y, int w, int h, int id, DWORD style = ES_NUMBER)
{
    HWND hw = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | style, x, y, w, h, hDlg, (HMENU)(INT_PTR)id, g_app.hInstance, nullptr);
    SendMessageW(hw, WM_SETFONT, (WPARAM)g_dlgFont, TRUE);
    return hw;
}

static HWND MkUpDown(HWND hDlg, HWND hBuddy, int id, int lo, int hi, int val)
{
    HWND hw = CreateWindowExW(0, UPDOWN_CLASSW, L"",
        WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS,
        0, 0, 0, 0, hDlg, (HMENU)(INT_PTR)id, g_app.hInstance, nullptr);
    SendMessageW(hw, UDM_SETBUDDY, (WPARAM)hBuddy, 0);
    SendMessageW(hw, UDM_SETRANGE32, lo, hi);
    SendMessageW(hw, UDM_SETPOS32, 0, val);
    return hw;
}

static HWND MkCombo(HWND hDlg, int x, int y, int w, int h, int id)
{
    HWND hw = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, x, y, w, h, hDlg, (HMENU)(INT_PTR)id, g_app.hInstance, nullptr);
    SendMessageW(hw, WM_SETFONT, (WPARAM)g_dlgFont, TRUE);
    return hw;
}

static HWND MkCheck(HWND hDlg, int x, int y, int w, int h, int id, const wchar_t* text)
{
    HWND hw = CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, x, y, w, h, hDlg, (HMENU)(INT_PTR)id, g_app.hInstance, nullptr);
    SendMessageW(hw, WM_SETFONT, (WPARAM)g_dlgFont, TRUE);
    return hw;
}

static HWND MkButton(HWND hDlg, int x, int y, int w, int h, int id, const wchar_t* text)
{
    HWND hw = CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, x, y, w, h, hDlg, (HMENU)(INT_PTR)id, g_app.hInstance, nullptr);
    SendMessageW(hw, WM_SETFONT, (WPARAM)g_dlgFont, TRUE);
    return hw;
}

static void ShowTabPage(int tabIndex)
{
    for (auto h : g_generalControls) ShowWindow(h, tabIndex == 0 ? SW_SHOW : SW_HIDE);
    for (auto h : g_shortcutControls) ShowWindow(h, tabIndex == 1 ? SW_SHOW : SW_HIDE);
}

static void PopulateKeyList()
{
    if (!g_hKeyList) return;
    SendMessageW(g_hKeyList, LVM_DELETEALLITEMS, 0, 0);
    auto& bindings = GetKeyBindings();
    for (int i = 0; i < (int)bindings.size(); i++)
    {
        LVITEMW lvi = {};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;
        lvi.pszText = (LPWSTR)bindings[i].label.c_str();
        SendMessageW(g_hKeyList, LVM_INSERTITEMW, 0, (LPARAM)&lvi);

        std::wstring keyStr = KeyBindingToString(bindings[i]);
        lvi.iSubItem = 1;
        lvi.pszText = (LPWSTR)keyStr.c_str();
        SendMessageW(g_hKeyList, LVM_SETITEMTEXTW, i, (LPARAM)&lvi);
    }
}

// キー入力待ちダイアログ — キャプチャした KeyCombo を GWLP_USERDATA に格納
static LRESULT CALLBACK KeyCaptureProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN)
    {
        if (wParam == VK_SHIFT || wParam == VK_CONTROL || wParam == VK_MENU)
            return 0;

        KeyCombo* kc = (KeyCombo*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        if (kc)
        {
            kc->vk = (UINT)wParam;
            kc->ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
            kc->shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            kc->alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
        }
        EndDialog(hwnd, IDOK);
        return 0;
    }
    if (msg == WM_COMMAND && LOWORD(wParam) == IDCANCEL)
    {
        EndDialog(hwnd, IDCANCEL);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// キーキャプチャ。成功時 true を返し kc に結果を格納。
static bool ShowKeyCaptureDialog(HWND hParent, KeyCombo& kc)
{
    struct {
        DLGTEMPLATE dt;
        WORD menu, cls, title;
        wchar_t titleText[32];
    } tmpl = {};

    tmpl.dt.style = DS_MODALFRAME | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU;
    tmpl.dt.cx = 180;
    tmpl.dt.cy = 60;
    wcscpy_s(tmpl.titleText, L"キー入力");

    kc = {}; // 初期化
    HWND hDlg = CreateDialogIndirectW(g_app.hInstance, &tmpl.dt, hParent, (DLGPROC)KeyCaptureProc);
    if (!hDlg) return false;

    SetWindowLongPtrW(hDlg, GWLP_USERDATA, (LONG_PTR)&kc);

    HWND hLabel = CreateWindowExW(0, L"STATIC", L"新しいキーを押してください...",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        10, 15, 250, 30, hDlg, nullptr, g_app.hInstance, nullptr);
    SendMessageW(hLabel, WM_SETFONT, (WPARAM)g_dlgFont, TRUE);

    ShowWindow(hDlg, SW_SHOW);
    EnableWindow(hParent, FALSE);

    MSG msg;
    while (IsWindow(hDlg) && GetMessageW(&msg, nullptr, 0, 0))
    {
        if (IsDialogMessageW(hDlg, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    EnableWindow(hParent, TRUE);
    SetForegroundWindow(hParent);
    return kc.vk != 0;
}

static INT_PTR CALLBACK SettingsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        AppSettings s;
        LoadSettings(s);

        // 一般タブの値を設定
        HWND hLang = GetDlgItem(hDlg, IDC_LANG_COMBO);
        SendMessageW(hLang, CB_ADDSTRING, 0, (LPARAM)L"日本語");
        SendMessageW(hLang, CB_ADDSTRING, 0, (LPARAM)L"English");
        SendMessageW(hLang, CB_SETCURSEL, s.language == L"en" ? 1 : 0, 0);

        CheckDlgButton(hDlg, IDC_WRAP_NAV, s.wrapNavigation ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_SPREAD_FIRST, s.spreadFirstSingle ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_RECURSIVE, s.recursiveLoad ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hDlg, IDC_AUTOPLAY, s.autoPlayMedia ? BST_CHECKED : BST_UNCHECKED);

        SetDlgItemInt(hDlg, IDC_CACHE_EDIT, s.cacheSizeMB, FALSE);

        // 閾値はfloat文字列
        wchar_t buf[32];
        swprintf_s(buf, _countof(buf), L"%.2f", s.spreadThreshold);
        SetDlgItemTextW(hDlg, IDC_THRESH_EDIT, buf);

        SetDlgItemInt(hDlg, IDC_THUMB_EDIT, s.thumbnailSize, FALSE);
        SetDlgItemInt(hDlg, IDC_PREVIEW_EDIT, s.previewSize, FALSE);
        SetDlgItemInt(hDlg, IDC_FONT_EDIT, s.fontSize, FALSE);

        // ショートカットリスト
        PopulateKeyList();

        return TRUE;
    }
    case WM_NOTIFY:
    {
        LPNMHDR pnm = (LPNMHDR)lParam;
        if (pnm->idFrom == IDC_TAB && pnm->code == TCN_SELCHANGE)
        {
            int sel = (int)SendMessageW(g_hTab, TCM_GETCURSEL, 0, 0);
            ShowTabPage(sel);
        }
        else if (pnm->idFrom == IDC_KEY_LIST && pnm->code == NM_DBLCLK)
        {
            // ダブルクリックでキー追加
            int sel = (int)SendMessageW(g_hKeyList, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
            if (sel >= 0 && sel < (int)GetKeyBindings().size())
            {
                KeyCombo kc;
                if (ShowKeyCaptureDialog(hDlg, kc))
                {
                    GetKeyBindings()[sel].keys.push_back(kc);
                    PopulateKeyList();
                }
            }
        }
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_KEY_ADD)
        {
            // 選択アクションにキーを追加
            int sel = (int)SendMessageW(g_hKeyList, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
            if (sel >= 0 && sel < (int)GetKeyBindings().size())
            {
                KeyCombo kc;
                if (ShowKeyCaptureDialog(hDlg, kc))
                {
                    GetKeyBindings()[sel].keys.push_back(kc);
                    PopulateKeyList();
                }
            }
        }
        else if (LOWORD(wParam) == IDC_KEY_REMOVE)
        {
            // 選択アクションの最後のキーを削除
            int sel = (int)SendMessageW(g_hKeyList, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
            if (sel >= 0 && sel < (int)GetKeyBindings().size())
            {
                auto& keys = GetKeyBindings()[sel].keys;
                if (!keys.empty()) keys.pop_back();
                PopulateKeyList();
            }
        }
        else if (LOWORD(wParam) == IDC_KEY_RESET)
        {
            InitDefaultKeyBindings();
            PopulateKeyList();
        }
        else if (LOWORD(wParam) == IDOK)
        {
            AppSettings s;
            LoadSettings(s);

            int langIdx = (int)SendMessageW(GetDlgItem(hDlg, IDC_LANG_COMBO), CB_GETCURSEL, 0, 0);
            s.language = (langIdx == 1) ? L"en" : L"ja";
            s.wrapNavigation = (IsDlgButtonChecked(hDlg, IDC_WRAP_NAV) == BST_CHECKED);
            s.spreadFirstSingle = (IsDlgButtonChecked(hDlg, IDC_SPREAD_FIRST) == BST_CHECKED);
            s.recursiveLoad = (IsDlgButtonChecked(hDlg, IDC_RECURSIVE) == BST_CHECKED);
            s.autoPlayMedia = (IsDlgButtonChecked(hDlg, IDC_AUTOPLAY) == BST_CHECKED);

            s.cacheSizeMB = GetDlgItemInt(hDlg, IDC_CACHE_EDIT, nullptr, FALSE);
            if (s.cacheSizeMB < 50) s.cacheSizeMB = 50;
            if (s.cacheSizeMB > 2000) s.cacheSizeMB = 2000;

            wchar_t threshBuf[32];
            GetDlgItemTextW(hDlg, IDC_THRESH_EDIT, threshBuf, 32);
            s.spreadThreshold = (float)_wtof(threshBuf);
            if (s.spreadThreshold < 0.1f) s.spreadThreshold = 0.1f;
            if (s.spreadThreshold > 3.0f) s.spreadThreshold = 3.0f;

            s.thumbnailSize = GetDlgItemInt(hDlg, IDC_THUMB_EDIT, nullptr, FALSE);
            s.previewSize = GetDlgItemInt(hDlg, IDC_PREVIEW_EDIT, nullptr, FALSE);
            s.fontSize = GetDlgItemInt(hDlg, IDC_FONT_EDIT, nullptr, FALSE);
            if (s.fontSize < 6) s.fontSize = 6;
            if (s.fontSize > 24) s.fontSize = 24;

            SaveSettings(s);
            SaveKeyBindings();
            CacheInit((size_t)s.cacheSizeMB * 1024 * 1024);
            I18nSetLang(s.language);
            PrefetchResetSettings();
            NavResetSettings();

            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void ShowSettingsDialog(HWND hwndParent)
{
    // ピクセル指定でウィンドウ直接作成（DLGTEMPLATEのDLU変換問題を回避）
    // DPI対応
    int dpi = 96;
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        auto fn = (UINT(WINAPI*)(HWND))GetProcAddress(hUser32, "GetDpiForWindow");
        if (fn) dpi = fn(hwndParent);
    }
    auto S = [dpi](int v) -> int { return MulDiv(v, dpi, 96); };
    int DW = S(360), DH = S(460);

    // 親ウィンドウ中央に配置
    RECT parentRc;
    GetWindowRect(hwndParent, &parentRc);
    int px = parentRc.left + (parentRc.right - parentRc.left - DW) / 2;
    int py = parentRc.top + (parentRc.bottom - parentRc.top - DH) / 2;

    // ダイアログテンプレート（最小限、サイズは後でSetWindowPosで上書き）
    struct { DLGTEMPLATE dt; WORD menu, cls, title; wchar_t titleText[8]; } tmpl = {};
    tmpl.dt.style = DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU;
    tmpl.dt.cx = 1; tmpl.dt.cy = 1;
    wcscpy_s(tmpl.titleText, L"設定");

    HWND hDlg = CreateDialogIndirectW(g_app.hInstance, &tmpl.dt, hwndParent, SettingsDlgProc);
    if (!hDlg) return;

    // ピクセルサイズに補正（非クライアント領域を加算）
    RECT rc = { 0, 0, DW, DH };
    AdjustWindowRectEx(&rc, GetWindowLong(hDlg, GWL_STYLE), FALSE, GetWindowLong(hDlg, GWL_EXSTYLE));
    SetWindowPos(hDlg, nullptr, px, py, rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER);

    g_dlgFont = CreateFontW(-S(13), 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    g_dlgFontBold = CreateFontW(-S(13), 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");

    g_generalControls.clear();
    g_shortcutControls.clear();

    int W = DW - S(10);
    int editX = S(240);
    int editW = S(80);
    int lh = S(18);   // ラベル高さ
    int ch = S(18);   // チェックボックス高さ
    int eh = S(24);   // エディット高さ
    int pad = S(8);   // インデント

    // タブコントロール
    g_hTab = CreateWindowExW(0, WC_TABCONTROLW, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        S(4), S(4), W, DH - S(44), hDlg, (HMENU)(INT_PTR)IDC_TAB, g_app.hInstance, nullptr);
    SendMessageW(g_hTab, WM_SETFONT, (WPARAM)g_dlgFont, TRUE);

    TCITEMW tci = {};
    tci.mask = TCIF_TEXT;
    tci.pszText = (LPWSTR)L"一般";
    SendMessageW(g_hTab, TCM_INSERTITEMW, 0, (LPARAM)&tci);
    tci.pszText = (LPWSTR)L"ショートカット";
    SendMessageW(g_hTab, TCM_INSERTITEMW, 1, (LPARAM)&tci);

    int tx = S(14), ty = S(32);
    auto GL = [&](HWND h) { g_generalControls.push_back(h); return h; };

    // === 一般タブ ===
    GL(MkLabel(hDlg, tx, ty + 2, S(120), lh, L"Language"));
    GL(MkCombo(hDlg, editX, ty, editW, S(120), IDC_LANG_COMBO));
    ty += S(30);

    GL(MkLabel(hDlg, tx, ty, S(200), lh, L"ナビゲーション", true));
    ty += S(20);
    GL(MkCheck(hDlg, tx + pad, ty, W - S(30), ch, IDC_WRAP_NAV, L"端でループする（最後から最初に戻る）"));
    ty += S(20);
    GL(MkCheck(hDlg, tx + pad, ty, W - S(30), ch, IDC_SPREAD_FIRST, L"見開き時に最初のページを単独表示する"));
    ty += S(24);

    GL(MkLabel(hDlg, tx, ty, S(200), lh, L"ファイル読み込み", true));
    ty += S(20);
    GL(MkCheck(hDlg, tx + pad, ty, W - S(30), ch, IDC_RECURSIVE, L"サブフォルダも含めて画像を表示（再帰表示）"));
    ty += S(24);

    GL(MkLabel(hDlg, tx, ty, S(200), lh, L"メディア", true));
    ty += S(20);
    GL(MkCheck(hDlg, tx + pad, ty, W - S(30), ch, IDC_AUTOPLAY, L"動画・音声を自動再生"));
    ty += S(24);

    GL(MkLabel(hDlg, tx, ty, S(200), lh, L"パフォーマンス", true));
    ty += S(22);
    GL(MkLabel(hDlg, tx + pad, ty + 2, S(200), lh, L"キャッシュメモリ上限 (MB)"));
    { HWND h = MkEdit(hDlg, editX, ty, editW, eh, IDC_CACHE_EDIT); GL(h); GL(MkUpDown(hDlg, h, 0, 50, 2000, 256)); }
    ty += S(28);

    GL(MkLabel(hDlg, tx, ty, S(200), lh, L"表示", true));
    ty += S(22);
    GL(MkLabel(hDlg, tx + pad, ty + 2, S(200), lh, L"見開き判定の閾値"));
    GL(MkEdit(hDlg, editX, ty, editW, eh, IDC_THRESH_EDIT, 0));
    ty += S(26);
    GL(MkLabel(hDlg, tx + pad, ty + 2, S(200), lh, L"サムネイルサイズ (px)"));
    { HWND h = MkEdit(hDlg, editX, ty, editW, eh, IDC_THUMB_EDIT); GL(h); GL(MkUpDown(hDlg, h, 0, 32, 512, 192)); }
    ty += S(26);
    GL(MkLabel(hDlg, tx + pad, ty + 2, S(200), lh, L"プレビューサイズ (px)"));
    { HWND h = MkEdit(hDlg, editX, ty, editW, eh, IDC_PREVIEW_EDIT); GL(h); GL(MkUpDown(hDlg, h, 0, 64, 1024, 320)); }
    ty += S(26);
    GL(MkLabel(hDlg, tx + pad, ty + 2, S(200), lh, L"フォントサイズ"));
    { HWND h = MkEdit(hDlg, editX, ty, editW, eh, IDC_FONT_EDIT); GL(h); GL(MkUpDown(hDlg, h, 0, 6, 24, 9)); }

    // === ショートカットタブ ===
    auto SL = [&](HWND h) { g_shortcutControls.push_back(h); return h; };

    int listH = DH - S(44) - S(32) - S(40);
    g_hKeyList = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
        WS_CHILD | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
        tx, S(30), W - S(20), listH, hDlg, (HMENU)(INT_PTR)IDC_KEY_LIST, g_app.hInstance, nullptr);
    SendMessageW(g_hKeyList, WM_SETFONT, (WPARAM)g_dlgFont, TRUE);
    SendMessageW(g_hKeyList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    SL(g_hKeyList);

    LVCOLUMNW lvc = {};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;
    lvc.cx = (W - S(20)) / 2; lvc.pszText = (LPWSTR)L"アクション";
    SendMessageW(g_hKeyList, LVM_INSERTCOLUMNW, 0, (LPARAM)&lvc);
    lvc.cx = (W - S(20)) / 2; lvc.pszText = (LPWSTR)L"キー";
    SendMessageW(g_hKeyList, LVM_INSERTCOLUMNW, 1, (LPARAM)&lvc);

    int btnY = S(30) + listH + S(4);
    SL(MkButton(hDlg, tx, btnY, S(90), S(26), IDC_KEY_ADD, L"キー追加..."));
    SL(MkButton(hDlg, tx + S(96), btnY, S(90), S(26), IDC_KEY_REMOVE, L"キー削除"));
    SL(MkButton(hDlg, tx + S(192), btnY, S(120), S(26), IDC_KEY_RESET, L"デフォルトに戻す"));

    // === OK/キャンセル ===
    MkButton(hDlg, DW - S(170), DH - S(34), S(75), S(26), IDOK, L"OK");
    MkButton(hDlg, DW - S(88), DH - S(34), S(75), S(26), IDCANCEL, L"キャンセル");

    ShowTabPage(0);
    SendMessageW(hDlg, WM_INITDIALOG, 0, 0);

    ShowWindow(hDlg, SW_SHOW);
    EnableWindow(hwndParent, FALSE);

    MSG msg;
    while (IsWindow(hDlg) && GetMessageW(&msg, nullptr, 0, 0))
    {
        if (IsDialogMessageW(hDlg, &msg)) continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    EnableWindow(hwndParent, TRUE);
    SetForegroundWindow(hwndParent);
}
