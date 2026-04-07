#include "i18n.h"
#include <unordered_map>

static std::wstring g_lang = L"ja";
static std::unordered_map<std::wstring, std::wstring> g_strings;

static void LoadJa()
{
    g_strings = {
        // ツリービュー
        {L"tree.desktop",    L"デスクトップ"},
        {L"tree.downloads",  L"ダウンロード"},
        {L"tree.documents",  L"ドキュメント"},
        {L"tree.pictures",   L"ピクチャ"},
        {L"tree.videos",     L"ビデオ"},
        {L"tree.music",      L"ミュージック"},
        {L"tree.localdisk",  L"ローカルディスク"},
        // ファイルリスト
        {L"list.name",       L"名前"},
        {L"list.size",       L"サイズ"},
        {L"list.type",       L"種類"},
        {L"list.date",       L"更新日時"},
        {L"list.filter",     L"Filter"},
        // タイプ名
        {L"type.folder",     L"フォルダ"},
        {L"type.file",       L"ファイル"},
        {L"type.jpeg",       L"JPEG 画像"},
        {L"type.png",        L"PNG 画像"},
        {L"type.gif",        L"GIF 画像"},
        {L"type.bmp",        L"BMP 画像"},
        {L"type.webp",       L"WebP 画像"},
        {L"type.avif",       L"AVIF 画像"},
        {L"type.tiff",       L"TIFF 画像"},
        {L"type.ico",        L"アイコン"},
        {L"type.zip",        L"ZIP 書庫"},
        {L"type.7z",         L"7z 書庫"},
        {L"type.rar",        L"RAR 書庫"},
        // コンテキストメニュー
        {L"ctx.fullscreen",      L"全画面 (F11)"},
        {L"ctx.copyimage",       L"画像をコピー (Ctrl+C)"},
        {L"ctx.copypath",        L"パスをコピー"},
        {L"ctx.copyparentpath",  L"親フォルダパスをコピー"},
        {L"ctx.explorer",        L"エクスプローラーで表示"},
        {L"ctx.addfav",          L"お気に入りに追加"},
        {L"ctx.removefav",       L"お気に入りから削除"},
        // ダイアログ
        {L"dlg.confirm",    L"確認"},
        {L"dlg.delete",     L"を削除しますか？"},
        {L"dlg.settings",   L"設定"},
        // ステータスバー
        {L"status.cantplay",     L"再生できません（mpv-2.dll を配置してください）"},
        {L"status.cantopen",     L"書庫を開けません（7z.dll が見つからないか非対応形式）"},
        // ナビバー
        {L"nav.history",    L"履歴"},
        {L"nav.favorites",  L"お気に入り"},
    };
}

static void LoadEn()
{
    g_strings = {
        {L"tree.desktop",    L"Desktop"},
        {L"tree.downloads",  L"Downloads"},
        {L"tree.documents",  L"Documents"},
        {L"tree.pictures",   L"Pictures"},
        {L"tree.videos",     L"Videos"},
        {L"tree.music",      L"Music"},
        {L"tree.localdisk",  L"Local Disk"},
        {L"list.name",       L"Name"},
        {L"list.size",       L"Size"},
        {L"list.type",       L"Type"},
        {L"list.date",       L"Modified"},
        {L"list.filter",     L"Filter"},
        {L"type.folder",     L"Folder"},
        {L"type.file",       L"File"},
        {L"type.jpeg",       L"JPEG Image"},
        {L"type.png",        L"PNG Image"},
        {L"type.gif",        L"GIF Image"},
        {L"type.bmp",        L"BMP Image"},
        {L"type.webp",       L"WebP Image"},
        {L"type.avif",       L"AVIF Image"},
        {L"type.tiff",       L"TIFF Image"},
        {L"type.ico",        L"Icon"},
        {L"type.zip",        L"ZIP Archive"},
        {L"type.7z",         L"7z Archive"},
        {L"type.rar",        L"RAR Archive"},
        {L"ctx.fullscreen",      L"Fullscreen (F11)"},
        {L"ctx.copyimage",       L"Copy Image (Ctrl+C)"},
        {L"ctx.copypath",        L"Copy Path"},
        {L"ctx.copyparentpath",  L"Copy Parent Path"},
        {L"ctx.explorer",        L"Show in Explorer"},
        {L"ctx.addfav",          L"Add to Favorites"},
        {L"ctx.removefav",       L"Remove from Favorites"},
        {L"dlg.confirm",    L"Confirm"},
        {L"dlg.delete",     L"Delete?"},
        {L"dlg.settings",   L"Settings"},
        {L"status.cantplay",     L"Cannot play (place mpv-2.dll)"},
        {L"status.cantopen",     L"Cannot open archive (7z.dll not found)"},
        {L"nav.history",    L"History"},
        {L"nav.favorites",  L"Favorites"},
    };
}

static const std::wstring kEmpty;

void I18nInit(const std::wstring& lang) { I18nSetLang(lang); }

void I18nSetLang(const std::wstring& lang)
{
    g_lang = lang;
    if (lang == L"en") LoadEn();
    else LoadJa(); // デフォルト日本語
}

const std::wstring& I18nGet(const std::wstring& key)
{
    auto it = g_strings.find(key);
    if (it != g_strings.end()) return it->second;
    return kEmpty;
}

const std::wstring& I18nGetLang() { return g_lang; }
