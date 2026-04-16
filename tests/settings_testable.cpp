// settings.cpp からテスト可能な関数を抽出したテスト用ソース
// 本体の settings.cpp と同じロジックだが、テスト環境で独立動作する

#include "app.h"
#include "settings.h"

static std::vector<KeyBinding> g_keyBindings;

std::vector<KeyBinding>& GetKeyBindings() { return g_keyBindings; }

static std::wstring VkToString(UINT vk)
{
    switch (vk)
    {
    case VK_LEFT:      return L"\u2190";
    case VK_RIGHT:     return L"\u2192";
    case VK_UP:        return L"\u2191";
    case VK_DOWN:      return L"\u2193";
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

std::wstring FindAction(UINT vk, bool ctrl, bool shift, bool alt)
{
    for (auto& kb : GetKeyBindings()) {
        for (auto& kc : kb.keys) {
            if (kc.vk == vk && kc.ctrl == ctrl && kc.shift == shift && kc.alt == alt)
                return kb.action;
        }
    }
    return L"";
}

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
    Add(L"fit_window",     L"ウィンドウに合わせる", { K('W') });
    Add(L"fit_width",      L"幅に合わせる",      {});
    Add(L"fit_height",     L"高さに合わせる",    {});
    Add(L"original_size",  L"等倍表示",          {});
    Add(L"view_single",    L"単ページ表示",      { K('1') });
    Add(L"view_spread",    L"見開き表示",        { K('2') });
    Add(L"view_auto",      L"自動判定",          { K('3') });
    Add(L"toggle_binding", L"綴じ方向切替",      { K('B') });
    Add(L"fullscreen",     L"全画面切替",        { K(VK_F11) });
    Add(L"zoom_in",        L"拡大",              { K(VK_OEM_PLUS, true), K(VK_ADD, true) });
    Add(L"zoom_out",       L"縮小",              { K(VK_OEM_MINUS, true), K(VK_SUBTRACT, true) });
    Add(L"zoom_reset",     L"ズームリセット",    { K('0', true), K(VK_NUMPAD0, true) });
    Add(L"copy_image",     L"画像コピー",        { K('C', true) });
    Add(L"rotate_cw",      L"時計回り回転",      { K('R', true) });
    Add(L"rotate_ccw",     L"反時計回り回転",    { K('R', true, true) });
    Add(L"rename",         L"名前変更",          { K(VK_F2) });
    Add(L"delete_file",    L"ファイル削除",      { K(VK_DELETE) });
    Add(L"play_pause",     L"再生/一時停止",     { K(VK_SPACE) });
    Add(L"refresh",        L"更新",              { K(VK_F5) });
    Add(L"help",           L"ヘルプ",            { K(VK_F1) });
    Add(L"escape",         L"全画面解除",        { K(VK_ESCAPE) });
    Add(L"bookshelf",      L"本棚切替",          { K('C') });
    Add(L"history",        L"履歴切替",          { K('H') });
    Add(L"list_view",      L"リスト表示",        {});
    Add(L"grid_view",      L"グリッド表示",      { K('G') });
    Add(L"hover_preview",  L"ホバープレビュー",  { K('P') });
    Add(L"settings",       L"設定",              {});
    Add(L"view_cycle",     L"表示モード切替",    { K('V') });
    Add(L"bind_ltr",       L"左綴じ",            { K('L') });
    Add(L"bind_rtl",       L"右綴じ",            { K('R') });
}

// テスト未使用だがリンクに必要なスタブ
void LoadKeyBindings() {}
void SaveKeyBindings() {}
bool LoadSettings(AppSettings&) { return false; }
bool SaveSettings(const AppSettings&) { return false; }
std::wstring GetSettingsPath() { return L""; }
void ShowSettingsDialog(HWND) {}
const AppSettings& GetCachedSettings() { static AppSettings s; return s; }
void InvalidateSettingsCache() {}
