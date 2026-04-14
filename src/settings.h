#pragma once
#include "app.h"

struct AppSettings {
    RECT windowRect = { CW_USEDEFAULT, CW_USEDEFAULT, 1280, 860 };
    bool maximized = false;
    int mainSplitPos = 280;
    int sidebarSplitPos = 300;
    std::wstring lastPath;
    int lastFileIndex = -1;
    std::wstring lastImagePath;

    // ユーザー設定
    int cacheSizeMB = 512;
    int prefetchCount = 12;
    std::wstring language = L"ja";
    bool wrapNavigation = true;
    bool spreadFirstSingle = true;    // 見開き時最初のページを単独表示（leeyez_kai準拠）
    bool recursiveLoad = false;       // サブフォルダ再帰読み込み
    bool autoPlayMedia = true;        // 動画自動再生
    bool loopMedia = false;            // 動画ループ再生
    float spreadThreshold = 1.0f;     // 見開き判定閾値
    int thumbnailSize = 192;          // サムネイルサイズ
    int previewSize = 320;            // プレビューサイズ
    int fontSize = 9;                 // フォントサイズ
    std::wstring treeSortMode = L"Name";  // ツリーソート: Name/Date/Size/Type
    bool treeSortDescending = false;      // ツリーソート降順
    int viewMode = 0;                     // 0=自動, 1=単独, 2=見開き
    bool isRTL = true;                    // 綴じ方向（右綴じ＝日本漫画方式）
    int scaleMode = 0;                    // 0=FitWindow, 1=FitWidth, 2=FitHeight, 3=Original
    std::vector<int> columnOrder;         // ファイルリスト列順序（空=デフォルト）
    std::vector<int> columnWidths;        // ファイルリスト列幅（空=デフォルト）
};

// 単一キーコンビネーション
struct KeyCombo {
    UINT vk = 0;
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
};

// キーバインディング（1アクションに複数キー登録可能）
struct KeyBinding {
    std::wstring action;  // アクション名
    std::wstring label;   // 表示名
    std::vector<KeyCombo> keys;  // 登録されたキーコンビネーション（複数可）
};

std::vector<KeyBinding>& GetKeyBindings();
void InitDefaultKeyBindings();
void LoadKeyBindings();
void SaveKeyBindings();
std::wstring KeyComboToString(const KeyCombo& kc);
std::wstring KeyBindingToString(const KeyBinding& kb);
std::wstring FindAction(UINT vk, bool ctrl, bool shift, bool alt);

bool LoadSettings(AppSettings& s);
bool SaveSettings(const AppSettings& s);
std::wstring GetSettingsPath();
void ShowSettingsDialog(HWND hwndParent);

const AppSettings& GetCachedSettings();
void InvalidateSettingsCache();
