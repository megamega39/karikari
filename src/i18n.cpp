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
        {L"ctx.openassoc",       L"関連付けで開く"},
        {L"ctx.explorer",        L"エクスプローラーで表示"},
        {L"ctx.addfav",          L"お気に入りに追加"},
        {L"ctx.removefav",       L"お気に入りから削除"},
        {L"ctx.rename",          L"名前の変更"},
        {L"ctx.delete",          L"削除"},
        {L"ctx.properties",      L"プロパティ"},
        // ダイアログ
        {L"dlg.confirm",    L"確認"},
        {L"dlg.delete",     L"を削除しますか？"},
        {L"dlg.settings",   L"設定"},
        // ステータスバー
        {L"status.cantplay",     L"再生できません（mpv-2.dll を配置してください）"},
        {L"status.cantopen",     L"書庫を開けません（7z.dll が見つからないか非対応形式）"},
        {L"status.loading",      L"読み込み中..."},
        // ナビバー
        {L"nav.history",    L"履歴"},
        {L"nav.favorites",  L"お気に入り"},
        // UI・ナビバー
        {L"ui.folder",      L"フォルダ"},
        {L"ui.address",     L" アドレス(A)"},
        {L"ui.bookshelf",   L"本棚"},
        {L"ui.history",     L"履歴"},
        {L"ui.settings",    L"設定"},
        {L"ui.help",        L"ヘルプ"},
        {L"nav.back",       L"戻る"},
        {L"nav.forward",    L"進む"},
        {L"nav.up",         L"上へ"},
        {L"nav.refresh",    L"更新"},
        {L"nav.list",       L"リスト表示"},
        {L"nav.grid",       L"グリッド表示"},
        {L"nav.hover",      L"ホバープレビュー"},
        {L"kb.fit_window",  L"ウィンドウに合わせる"},
        {L"kb.fit_width",   L"幅に合わせる"},
        {L"kb.fit_height",  L"高さに合わせる"},
        {L"kb.original",    L"等倍"},
        {L"kb.zoom_in",     L"拡大"},
        {L"kb.zoom_out",    L"縮小"},
        {L"kb.view_auto",   L"自動判定"},
        {L"kb.view_single", L"単ページ"},
        {L"kb.view_spread", L"見開き"},
        {L"kb.binding",     L"綴じ方向"},
        {L"kb.rotate",      L"回転"},
        // コンテキストメニュー（本棚）
        {L"ctx.addshelf",       L"本棚に追加"},
        {L"ctx.removeshelf",    L"本棚から解除"},
        {L"ctx.newshelf",       L"新しい本棚を作成..."},
        {L"ctx.shelfascat",     L"フォルダを本棚として追加"},
        {L"ctx.shelfname",      L"新しい本棚を作成"},
        {L"ctx.shelfprompt",    L"本棚名を入力してください:"},
        {L"ctx.deleteshelf",    L"この本棚を削除"},
        // ソートメニュー
        {L"sort.asc",           L"昇順"},
        {L"sort.desc",          L"降順"},
        // ツールバー・パネル
        {L"ui.clearall",        L"全削除"},
        {L"ui.renamethis",      L"名前を変更"},
        {L"ui.filter",          L"フィルター..."},
        // ドライブ種別
        {L"drive.network",      L"ネットワークドライブ"},
        {L"drive.removable",    L"リムーバブルディスク"},
        {L"drive.cdrom",        L"CD/DVDドライブ"},
        {L"drive.local",        L"ローカルディスク"},
        // メディアツールチップ
        {L"media.loop",         L"ループ再生"},
        {L"media.autoplay",     L"自動再生"},
        // 確認ダイアログ
        {L"dlg.bookshelfclear", L"本棚の登録をすべて削除しますか？"},
        {L"dlg.historyclear",   L"履歴を全て削除しますか？"},
        // グループ名（履歴）
        {L"group.today",        L"今日"},
        {L"group.yesterday",    L"昨日"},
        {L"group.thisweek",     L"今週"},
        {L"group.lastweek",     L"先週"},
        {L"group.older",        L"それ以前"},
        // 設定ダイアログ
        {L"settings.tab.general",   L"一般"},
        {L"settings.tab.shortcuts", L"ショートカット"},
        {L"settings.nav",           L"ナビゲーション"},
        {L"settings.wrap",          L"端でループする（最後から最初に戻る）"},
        {L"settings.spreadfirst",   L"見開き時に最初のページを単独表示する"},
        {L"settings.fileload",      L"ファイル読み込み"},
        {L"settings.recursive",     L"サブフォルダも含めて画像を表示（再帰表示）"},
        {L"settings.media",         L"メディア"},
        {L"settings.autoplay",      L"動画・音声を自動再生"},
        {L"settings.perf",          L"パフォーマンス"},
        {L"settings.cache",         L"キャッシュメモリ上限 (MB)"},
        {L"settings.display",       L"表示"},
        {L"settings.threshold",     L"見開き判定の閾値"},
        {L"settings.thumbsize",     L"グリッドサムネイルサイズ (px)"},
        {L"settings.previewsize",   L"ホバープレビューサイズ (px)"},
        {L"settings.fontsize",      L"フォントサイズ"},
        {L"settings.action",        L"アクション"},
        {L"settings.key",           L"キー"},
        {L"settings.addkey",        L"キー追加..."},
        {L"settings.removekey",     L"キー削除"},
        {L"settings.resetkeys",     L"デフォルトに戻す"},
        {L"settings.resetall",      L"デフォルトに戻す"},
        {L"settings.ok",            L"OK"},
        {L"settings.cancel",        L"キャンセル"},
        {L"settings.resetconfirm",  L"すべての設定をデフォルトに戻しますか？"},
        {L"settings.keycapture",    L"キー入力"},
        {L"settings.keyprompt",     L"新しいキーを押してください..."},
        // ヘルプダイアログ
        {L"help.title",         L"ヘルプ - karikari の使い方"},
        {L"help.tab.basic",     L"基本操作"},
        {L"help.tab.view",      L"表示モード"},
        {L"help.tab.tree",      L"ツリービュー"},
        {L"help.tab.file",      L"ファイル操作"},
        {L"help.tab.settings",  L"設定"},
        {L"help.tab.about",     L"このアプリについて"},
        {L"help.version",       L"バージョン 1.0.0"},
        {L"help.subtitle",      L"最速の画像・動画ビューア"},
        {L"help.authorlabel",   L"作者:"},
        {L"help.authorname",    L"megamega39"},
        {L"help.desclabel",     L"説明:"},
        {L"help.desc1",         L"C++ + Win32 API + Direct2D で構築した高速画像/動画ビューア。"},
        {L"help.desc2",         L"leeyez_kai (.NET版) のネイティブ移植。"},
        {L"help.close",         L"閉じる"},
        {L"help.text.basic",
            L"■ 画像を見る\r\n"
            L"  ・左側のツリーやファイルリストからフォルダや書庫を選んで開けます\r\n"
            L"  ・← → キーやマウスホイールでページをめくれます\r\n"
            L"  ・Home / End で最初や最後のページに飛べます\r\n"
            L"\r\n"
            L"■ 書庫ファイルを開く\r\n"
            L"  ・ZIP、7z、RAR、CBZ、CBR、CB7 の書庫に対応しています\r\n"
            L"  ・書庫の中のフォルダもそのまま見られます\r\n"
            L"  ・↑ ↓ キーで同じフォルダにある前後の書庫に移動できます\r\n"
            L"\r\n"
            L"■ 動画・音声を再生する\r\n"
            L"  ・動画や音声ファイルもそのまま再生できます\r\n"
            L"  ・Space キーで再生と一時停止を切り替えられます\r\n"
            L"  ・画面下のバーでシーク、ループ、音量調整ができます\r\n"
            L"  ・再生バーの「A」ボタンで自動再生のオン/オフを切り替えられます\r\n"},
        {L"help.text.view",
            L"■ 見開き表示\r\n"
            L"  ・2ページを左右に並べて表示できます\r\n"
            L"  ・自動モードでは、縦長の画像は見開き、横長の画像は単独で表示します\r\n"
            L"  ・1キーで単独、2キーで見開き、3キーで自動に切り替えられます\r\n"
            L"  ・設定で表紙（1ページ目）を常に単独表示にすることもできます\r\n"
            L"  ・アニメーションGIF/WebPも見開きで両方同時に動きます\r\n"
            L"\r\n"
            L"■ ズーム（拡大・縮小）\r\n"
            L"  ・Ctrl を押しながらマウスホイールでなめらかに拡大・縮小できます\r\n"
            L"  ・ツールバーのボタンで「ウィンドウに合わせる」「幅に合わせる」等を選べます\r\n"
            L"  ・Ctrl+0 で元のサイズに戻せます\r\n"
            L"\r\n"
            L"■ 回転\r\n"
            L"  ・ツールバーの回転ボタン、または Ctrl+R で画像を回転できます\r\n"
            L"\r\n"
            L"■ 全画面表示\r\n"
            L"  ・F11 キーで全画面に切り替えられます\r\n"
            L"  ・Esc キーで元に戻ります\r\n"},
        {L"help.text.tree",
            L"■ 3つのモード\r\n"
            L"  ・通常モード: お気に入り、よく使うフォルダ、ドライブが表示されます\r\n"
            L"  ・本棚モード: 自分で登録した書庫やフォルダだけを表示します\r\n"
            L"    カテゴリを作って整理できます\r\n"
            L"  ・履歴モード: 過去に開いたフォルダや書庫が新しい順に並びます\r\n"
            L"\r\n"
            L"■ お気に入り\r\n"
            L"  ・よく使うフォルダや書庫を右クリックから「お気に入りに追加」できます\r\n"
            L"  ・お気に入りはツリーの一番上に表示されます\r\n"
            L"\r\n"
            L"■ 本棚\r\n"
            L"  ・本棚ボタンで本棚モードに切り替えられます\r\n"
            L"  ・右クリック→「本棚に追加」でカテゴリに登録できます\r\n"
            L"  ・カテゴリは右クリックで名前変更や削除ができます\r\n"
            L"  ・フォルダを右クリック→「フォルダを本棚として追加」で\r\n"
            L"    中の書庫をまとめて登録できます\r\n"
            L"\r\n"
            L"■ 並び替え\r\n"
            L"  ・ソートボタンで名前、日時、サイズ、種類の順に並び替えられます\r\n"
            L"  ・現在の並び順はボタンに表示されています\r\n"},
        {L"help.text.file",
            L"■ ファイル一覧の表示方法\r\n"
            L"  ・上部のボタンでリスト表示とグリッド（サムネイル）表示を切り替えられます\r\n"
            L"  ・リスト表示では列の見出しをドラッグして並び順を変えられます\r\n"
            L"  ・列の幅や並び順は次回起動時にも保持されます\r\n"
            L"\r\n"
            L"■ 名前を変更する\r\n"
            L"  ・ファイルやフォルダを選んで F2 キーを押すと、その場で名前を編集できます\r\n"
            L"  ・右クリックメニューの「名前の変更」からも同様にできます\r\n"
            L"  ・Enter で確定、Esc でキャンセルです\r\n"
            L"\r\n"
            L"■ ファイルを削除する\r\n"
            L"  ・Delete キーまたは右クリックメニューから削除できます\r\n"
            L"  ・削除したファイルはごみ箱に移動されるので、元に戻すことができます\r\n"
            L"\r\n"
            L"■ ファイルを絞り込む\r\n"
            L"  ・ファイルリストの下にある入力欄に文字を入れると、\r\n"
            L"    名前に一致するファイルだけが表示されます\r\n"
            L"\r\n"
            L"■ 右クリックメニュー\r\n"
            L"  ・ファイルを右クリックすると、さまざまな操作ができます\r\n"
            L"  ・関連付けで開く: 画像編集ソフトなど、別のアプリで開けます\r\n"
            L"  ・エクスプローラーで表示: ファイルの場所をエクスプローラーで開きます\r\n"
            L"  ・他にもパスのコピー、お気に入り/本棚への追加、\r\n"
            L"    名前変更、削除、プロパティが使えます\r\n"},
        {L"help.text.settings",
            L"■ 言語を変える\r\n"
            L"  ・12の言語に対応しています\r\n"
            L"  ・設定画面で変更するとすぐに切り替わります\r\n"
            L"\r\n"
            L"■ キーボードショートカットを変える\r\n"
            L"  ・設定画面の「ショートカット」タブですべての操作のキーを変更できます\r\n"
            L"  ・ひとつの操作に複数のキーを登録することもできます\r\n"
            L"\r\n"
            L"■ 見た目を変える\r\n"
            L"  ・文字の大きさ、サムネイルの大きさ、ホバープレビューの大きさを\r\n"
            L"    設定画面で変更できます\r\n"
            L"  ・変更はすぐに反映されます\r\n"
            L"\r\n"
            L"■ そのほかの設定\r\n"
            L"  ・キャッシュサイズ: 表示した画像を記憶しておくメモリの量を調整できます\r\n"
            L"  ・端でループ: 最後のページから次へ進むと最初に戻ります\r\n"
            L"  ・見開き表紙単独: 見開き表示で1ページ目だけ単独表示にできます\r\n"
            L"  ・サブフォルダ表示: フォルダの中のフォルダの画像もまとめて表示できます\r\n"
            L"  ・動画自動再生: 動画ファイルを選んだとき自動的に再生を始めます\r\n"},
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
        {L"ctx.openassoc",       L"Open with Default App"},
        {L"ctx.explorer",        L"Show in Explorer"},
        {L"ctx.addfav",          L"Add to Favorites"},
        {L"ctx.removefav",       L"Remove from Favorites"},
        {L"ctx.rename",          L"Rename"},
        {L"ctx.delete",          L"Delete"},
        {L"ctx.properties",      L"Properties"},
        {L"dlg.confirm",    L"Confirm"},
        {L"dlg.delete",     L"Delete?"},
        {L"dlg.settings",   L"Settings"},
        {L"status.cantplay",     L"Cannot play (place mpv-2.dll)"},
        {L"status.cantopen",     L"Cannot open archive (7z.dll not found)"},
        {L"status.loading",      L"Loading..."},
        {L"nav.history",    L"History"},
        {L"nav.favorites",  L"Favorites"},
        {L"ui.folder",      L"Folder"},
        {L"ui.address",     L" Address(A)"},
        {L"ui.bookshelf",   L"Bookshelf"},
        {L"ui.history",     L"History"},
        {L"ui.settings",    L"Settings"},
        {L"ui.help",        L"Help"},
        {L"nav.back",       L"Back"},
        {L"nav.forward",    L"Forward"},
        {L"nav.up",         L"Up"},
        {L"nav.refresh",    L"Refresh"},
        {L"nav.list",       L"List View"},
        {L"nav.grid",       L"Grid View"},
        {L"nav.hover",      L"Hover Preview"},
        {L"kb.fit_window",  L"Fit Window"},
        {L"kb.fit_width",   L"Fit Width"},
        {L"kb.fit_height",  L"Fit Height"},
        {L"kb.original",    L"Original Size"},
        {L"kb.zoom_in",     L"Zoom In"},
        {L"kb.zoom_out",    L"Zoom Out"},
        {L"kb.view_auto",   L"Auto"},
        {L"kb.view_single", L"Single Page"},
        {L"kb.view_spread", L"Spread"},
        {L"kb.binding",     L"Binding Direction"},
        {L"kb.rotate",      L"Rotate"},
        {L"ctx.addshelf",       L"Add to Bookshelf"},
        {L"ctx.removeshelf",    L"Remove from Bookshelf"},
        {L"ctx.newshelf",       L"Create New Shelf..."},
        {L"ctx.shelfascat",     L"Add Folder as Shelf"},
        {L"ctx.shelfname",      L"Create New Shelf"},
        {L"ctx.shelfprompt",    L"Enter shelf name:"},
        {L"ctx.deleteshelf",    L"Delete this shelf"},
        {L"sort.asc",           L"Ascending"},
        {L"sort.desc",          L"Descending"},
        {L"ui.clearall",        L"Clear All"},
        {L"ui.renamethis",      L"Rename"},
        {L"ui.filter",          L"Filter..."},
        {L"drive.network",      L"Network Drive"},
        {L"drive.removable",    L"Removable Disk"},
        {L"drive.cdrom",        L"CD/DVD Drive"},
        {L"drive.local",        L"Local Disk"},
        {L"media.loop",         L"Loop"},
        {L"media.autoplay",     L"Auto Play"},
        {L"dlg.bookshelfclear", L"Delete all bookshelf entries?"},
        {L"dlg.historyclear",   L"Clear all history?"},
        {L"group.today",        L"Today"},
        {L"group.yesterday",    L"Yesterday"},
        {L"group.thisweek",     L"This Week"},
        {L"group.lastweek",     L"Last Week"},
        {L"group.older",        L"Older"},
        // Settings dialog
        {L"settings.tab.general",   L"General"},
        {L"settings.tab.shortcuts", L"Shortcuts"},
        {L"settings.nav",           L"Navigation"},
        {L"settings.wrap",          L"Loop at edges (wrap from last to first)"},
        {L"settings.spreadfirst",   L"Show first page as single in spread mode"},
        {L"settings.fileload",      L"File Loading"},
        {L"settings.recursive",     L"Show images from subfolders (recursive)"},
        {L"settings.media",         L"Media"},
        {L"settings.autoplay",      L"Auto-play video and audio"},
        {L"settings.perf",          L"Performance"},
        {L"settings.cache",         L"Cache memory limit (MB)"},
        {L"settings.display",       L"Display"},
        {L"settings.threshold",     L"Spread detection threshold"},
        {L"settings.thumbsize",     L"Grid thumbnail size (px)"},
        {L"settings.previewsize",   L"Hover preview size (px)"},
        {L"settings.fontsize",      L"Font size"},
        {L"settings.action",        L"Action"},
        {L"settings.key",           L"Key"},
        {L"settings.addkey",        L"Add Key..."},
        {L"settings.removekey",     L"Remove Key"},
        {L"settings.resetkeys",     L"Reset to Default"},
        {L"settings.resetall",      L"Reset to Default"},
        {L"settings.ok",            L"OK"},
        {L"settings.cancel",        L"Cancel"},
        {L"settings.resetconfirm",  L"Reset all settings to default?"},
        {L"settings.keycapture",    L"Key Input"},
        {L"settings.keyprompt",     L"Press a new key..."},
        // Help dialog
        {L"help.title",         L"Help - How to use karikari"},
        {L"help.tab.basic",     L"Basics"},
        {L"help.tab.view",      L"View Mode"},
        {L"help.tab.tree",      L"Tree View"},
        {L"help.tab.file",      L"File Operations"},
        {L"help.tab.settings",  L"Settings"},
        {L"help.tab.about",     L"About"},
        {L"help.version",       L"Version 1.0.0"},
        {L"help.subtitle",      L"The fastest image and video viewer"},
        {L"help.authorlabel",   L"Author:"},
        {L"help.authorname",    L"megamega39"},
        {L"help.desclabel",     L"Description:"},
        {L"help.desc1",         L"A high-performance image/video viewer built with C++ + Win32 API + Direct2D."},
        {L"help.desc2",         L"Native port of leeyez_kai (.NET version)."},
        {L"help.close",         L"Close"},
        {L"help.text.basic",
            L"## Viewing Images\r\n"
            L"  - Select a folder or archive from the tree or file list on the left to open it\r\n"
            L"  - Use the Left/Right arrow keys or mouse wheel to turn pages\r\n"
            L"  - Press Home/End to jump to the first or last page\r\n"
            L"\r\n"
            L"## Opening Archives\r\n"
            L"  - Supports ZIP, 7z, RAR, CBZ, CBR, and CB7 archives\r\n"
            L"  - You can browse folders inside archives directly\r\n"
            L"  - Use the Up/Down arrow keys to move to the previous or next archive in the same folder\r\n"
            L"\r\n"
            L"## Playing Video & Audio\r\n"
            L"  - Video and audio files can be played directly\r\n"
            L"  - Press Space to toggle play/pause\r\n"
            L"  - Use the bar at the bottom for seeking, looping, and volume control\r\n"
            L"  - Press the \"A\" button on the playback bar to toggle auto-play on/off\r\n"},
        {L"help.text.view",
            L"## Spread (Two-Page) View\r\n"
            L"  - Display two pages side by side\r\n"
            L"  - In Auto mode, portrait images are shown as a spread, landscape images are shown alone\r\n"
            L"  - Press 1 for Single, 2 for Spread, 3 for Auto\r\n"
            L"  - You can set the cover (first page) to always display as a single page in settings\r\n"
            L"  - Animated GIF/WebP files play simultaneously on both pages in spread view\r\n"
            L"\r\n"
            L"## Zoom (Enlarge / Reduce)\r\n"
            L"  - Hold Ctrl and scroll the mouse wheel for smooth zooming\r\n"
            L"  - Use toolbar buttons to choose \"Fit Window\", \"Fit Width\", etc.\r\n"
            L"  - Press Ctrl+0 to reset to original size\r\n"
            L"\r\n"
            L"## Rotation\r\n"
            L"  - Rotate images using the toolbar button or Ctrl+R\r\n"
            L"\r\n"
            L"## Fullscreen\r\n"
            L"  - Press F11 to toggle fullscreen\r\n"
            L"  - Press Esc to return to normal view\r\n"},
        {L"help.text.tree",
            L"## Three Modes\r\n"
            L"  - Normal Mode: Shows favorites, frequently used folders, and drives\r\n"
            L"  - Bookshelf Mode: Shows only the archives and folders you have registered\r\n"
            L"    You can create categories to organize them\r\n"
            L"  - History Mode: Lists previously opened folders and archives, newest first\r\n"
            L"\r\n"
            L"## Favorites\r\n"
            L"  - Right-click a folder or archive and select \"Add to Favorites\"\r\n"
            L"  - Favorites appear at the top of the tree\r\n"
            L"\r\n"
            L"## Bookshelf\r\n"
            L"  - Press the Bookshelf button to switch to Bookshelf mode\r\n"
            L"  - Right-click and select \"Add to Bookshelf\" to register items in a category\r\n"
            L"  - Right-click a category to rename or delete it\r\n"
            L"  - Right-click a folder and select \"Add Folder as Shelf\" to register\r\n"
            L"    all archives inside at once\r\n"
            L"\r\n"
            L"## Sorting\r\n"
            L"  - Use the sort button to sort by name, date, size, or type\r\n"
            L"  - The current sort order is shown on the button\r\n"},
        {L"help.text.file",
            L"## File List Display\r\n"
            L"  - Use the buttons at the top to switch between List view and Grid (thumbnail) view\r\n"
            L"  - In List view, drag column headers to change the sort order\r\n"
            L"  - Column widths and order are preserved for the next launch\r\n"
            L"\r\n"
            L"## Renaming\r\n"
            L"  - Select a file or folder and press F2 to edit the name in place\r\n"
            L"  - You can also use \"Rename\" from the right-click menu\r\n"
            L"  - Press Enter to confirm, Esc to cancel\r\n"
            L"\r\n"
            L"## Deleting Files\r\n"
            L"  - Press Delete or use the right-click menu to delete\r\n"
            L"  - Deleted files are moved to the Recycle Bin, so you can restore them\r\n"
            L"\r\n"
            L"## Filtering Files\r\n"
            L"  - Type in the input field below the file list to show only files\r\n"
            L"    whose names match\r\n"
            L"\r\n"
            L"## Right-Click Menu\r\n"
            L"  - Right-click a file for various actions\r\n"
            L"  - Open with Default App: Open in another application such as an image editor\r\n"
            L"  - Show in Explorer: Open the file's location in Explorer\r\n"
            L"  - Also includes Copy Path, Add to Favorites/Bookshelf,\r\n"
            L"    Rename, Delete, and Properties\r\n"},
        {L"help.text.settings",
            L"## Changing Language\r\n"
            L"  - Supports 12 languages\r\n"
            L"  - Changes take effect immediately in the settings screen\r\n"
            L"\r\n"
            L"## Changing Keyboard Shortcuts\r\n"
            L"  - Customize all action keys in the \"Shortcuts\" tab of the settings screen\r\n"
            L"  - You can assign multiple keys to a single action\r\n"
            L"\r\n"
            L"## Changing Appearance\r\n"
            L"  - Adjust font size, thumbnail size, and hover preview size\r\n"
            L"    in the settings screen\r\n"
            L"  - Changes are applied instantly\r\n"
            L"\r\n"
            L"## Other Settings\r\n"
            L"  - Cache Size: Adjust the amount of memory used to store viewed images\r\n"
            L"  - Loop at Edges: Going past the last page wraps back to the first\r\n"
            L"  - Spread Cover Single: Show the first page alone in spread view\r\n"
            L"  - Show Subfolders: Display images from subfolders as well\r\n"
            L"  - Auto-play Video: Automatically start playback when a video file is selected\r\n"},
    };
}

static void LoadZhCN()
{
    g_strings = {
        {L"tree.desktop",    L"桌面"},
        {L"tree.downloads",  L"下载"},
        {L"tree.documents",  L"文档"},
        {L"tree.pictures",   L"图片"},
        {L"tree.videos",     L"视频"},
        {L"tree.music",      L"音乐"},
        {L"tree.localdisk",  L"本地磁盘"},
        {L"list.name",       L"名称"},
        {L"list.size",       L"大小"},
        {L"list.type",       L"类型"},
        {L"list.date",       L"修改日期"},
        {L"list.filter",     L"筛选"},
        {L"type.folder",     L"文件夹"},
        {L"type.file",       L"文件"},
        {L"type.jpeg",       L"JPEG 图片"},
        {L"type.png",        L"PNG 图片"},
        {L"type.gif",        L"GIF 图片"},
        {L"type.bmp",        L"BMP 图片"},
        {L"type.webp",       L"WebP 图片"},
        {L"type.avif",       L"AVIF 图片"},
        {L"type.tiff",       L"TIFF 图片"},
        {L"type.ico",        L"图标"},
        {L"type.zip",        L"ZIP 压缩包"},
        {L"type.7z",         L"7z 压缩包"},
        {L"type.rar",        L"RAR 压缩包"},
        {L"ctx.fullscreen",      L"全屏 (F11)"},
        {L"ctx.copyimage",       L"复制图片 (Ctrl+C)"},
        {L"ctx.copypath",        L"复制路径"},
        {L"ctx.copyparentpath",  L"复制上级文件夹路径"},
        {L"ctx.openassoc",       L"用默认程序打开"},
        {L"ctx.explorer",        L"在资源管理器中显示"},
        {L"ctx.addfav",          L"添加到收藏夹"},
        {L"ctx.removefav",       L"从收藏夹移除"},
        {L"ctx.rename",          L"重命名"},
        {L"ctx.delete",          L"删除"},
        {L"ctx.properties",      L"属性"},
        {L"dlg.confirm",    L"确认"},
        {L"dlg.delete",     L"确定删除吗？"},
        {L"dlg.settings",   L"设置"},
        {L"status.cantplay",     L"无法播放（请放置 mpv-2.dll）"},
        {L"status.cantopen",     L"无法打开压缩包（未找到 7z.dll 或格式不支持）"},
        {L"status.loading",      L"加载中..."},
        {L"nav.history",    L"历史"},
        {L"nav.favorites",  L"收藏夹"},
        {L"ui.folder",      L"文件夹"},
        {L"ui.address",     L" 地址(A)"},
        {L"ui.bookshelf",   L"书架"},
        {L"ui.history",     L"历史"},
        {L"ui.settings",    L"设置"},
        {L"ui.help",        L"帮助"},
        {L"nav.back",       L"后退"},
        {L"nav.forward",    L"前进"},
        {L"nav.up",         L"上级"},
        {L"nav.refresh",    L"刷新"},
        {L"nav.list",       L"列表视图"},
        {L"nav.grid",       L"网格视图"},
        {L"nav.hover",      L"悬停预览"},
        {L"kb.fit_window",  L"适应窗口"},
        {L"kb.fit_width",   L"适应宽度"},
        {L"kb.fit_height",  L"适应高度"},
        {L"kb.original",    L"原始大小"},
        {L"kb.zoom_in",     L"放大"},
        {L"kb.zoom_out",    L"缩小"},
        {L"kb.view_auto",   L"自动"},
        {L"kb.view_single", L"单页"},
        {L"kb.view_spread", L"双页"},
        {L"kb.binding",     L"装订方向"},
        {L"kb.rotate",      L"旋转"},
        {L"ctx.addshelf",       L"添加到书架"},
        {L"ctx.removeshelf",    L"从书架移除"},
        {L"ctx.newshelf",       L"创建新书架..."},
        {L"ctx.shelfascat",     L"将文件夹添加为书架"},
        {L"ctx.shelfname",      L"创建新书架"},
        {L"ctx.shelfprompt",    L"请输入书架名称:"},
        {L"ctx.deleteshelf",    L"删除此书架"},
        {L"sort.asc",           L"升序"},
        {L"sort.desc",          L"降序"},
        {L"ui.clearall",        L"全部删除"},
        {L"ui.renamethis",      L"重命名"},
        {L"ui.filter",          L"筛选..."},
        {L"drive.network",      L"网络驱动器"},
        {L"drive.removable",    L"可移动磁盘"},
        {L"drive.cdrom",        L"CD/DVD 驱动器"},
        {L"drive.local",        L"本地磁盘"},
        {L"media.loop",         L"循环播放"},
        {L"media.autoplay",     L"自动播放"},
        {L"dlg.bookshelfclear", L"确定删除所有书架记录吗？"},
        {L"dlg.historyclear",   L"确定清除所有历史记录吗？"},
        {L"group.today",        L"今天"},
        {L"group.yesterday",    L"昨天"},
        {L"group.thisweek",     L"本周"},
        {L"group.lastweek",     L"上周"},
        {L"group.older",        L"更早"},
        // 设置对话框
        {L"settings.tab.general",   L"常规"},
        {L"settings.tab.shortcuts", L"快捷键"},
        {L"settings.nav",           L"导航"},
        {L"settings.wrap",          L"循环到开头"},
        {L"settings.spreadfirst",   L"双页首页单独显示"},
        {L"settings.fileload",      L"文件加载"},
        {L"settings.recursive",     L"显示子文件夹图片"},
        {L"settings.media",         L"媒体"},
        {L"settings.autoplay",      L"自动播放"},
        {L"settings.perf",          L"性能"},
        {L"settings.cache",         L"缓存上限"},
        {L"settings.display",       L"显示"},
        {L"settings.threshold",     L"双页判定阈值"},
        {L"settings.thumbsize",     L"网格缩略图大小"},
        {L"settings.previewsize",   L"悬停预览大小"},
        {L"settings.fontsize",      L"字体大小"},
        {L"settings.action",        L"操作"},
        {L"settings.key",           L"按键"},
        {L"settings.addkey",        L"添加按键..."},
        {L"settings.removekey",     L"删除按键"},
        {L"settings.resetkeys",     L"恢复默认"},
        {L"settings.resetall",      L"恢复默认"},
        {L"settings.ok",            L"确定"},
        {L"settings.cancel",        L"取消"},
        {L"settings.resetconfirm",  L"恢复所有默认设置？"},
        {L"settings.keycapture",    L"按键输入"},
        {L"settings.keyprompt",     L"请按下新的按键..."},
        // 帮助对话框
        {L"help.title",         L"帮助 - karikari 使用方法"},
        {L"help.tab.basic",     L"基本操作"},
        {L"help.tab.view",      L"显示模式"},
        {L"help.tab.tree",      L"树形视图"},
        {L"help.tab.file",      L"文件操作"},
        {L"help.tab.settings",  L"设置"},
        {L"help.tab.about",     L"关于"},
        {L"help.version",       L"版本 1.0.0"},
        {L"help.subtitle",      L"最快的图片/视频查看器"},
        {L"help.authorlabel",   L"作者:"},
        {L"help.authorname",    L"megamega39"},
        {L"help.desclabel",     L"说明:"},
        {L"help.desc1",         L"使用 C++ + Win32 API + Direct2D 构建的高性能图片/视频查看器。"},
        {L"help.desc2",         L"leeyez_kai (.NET版) 的原生移植。"},
        {L"help.close",         L"关闭"},
        {L"help.text.basic",
            L"## 查看图片\r\n"
            L"  - 从左侧的树形视图或文件列表中选择文件夹或压缩包打开\r\n"
            L"  - 使用 ← → 键或鼠标滚轮翻页\r\n"
            L"  - 按 Home / End 跳转到第一页或最后一页\r\n"
            L"\r\n"
            L"## 打开压缩包\r\n"
            L"  - 支持 ZIP、7z、RAR、CBZ、CBR、CB7 压缩包\r\n"
            L"  - 可以直接浏览压缩包内的文件夹\r\n"
            L"  - 使用 ↑ ↓ 键切换到同一文件夹中的前后压缩包\r\n"
            L"\r\n"
            L"## 播放视频和音频\r\n"
            L"  - 可以直接播放视频和音频文件\r\n"
            L"  - 按 Space 键切换播放/暂停\r\n"
            L"  - 使用底部的控制栏进行快进、循环和音量调节\r\n"
            L"  - 按播放栏的「A」按钮切换自动播放开/关\r\n"},
        {L"help.text.view",
            L"## 双页显示\r\n"
            L"  - 可以将两页并排显示\r\n"
            L"  - 自动模式下，纵向图片以双页显示，横向图片单独显示\r\n"
            L"  - 按 1 切换单页，按 2 切换双页，按 3 切换自动\r\n"
            L"  - 可以在设置中将封面（第一页）设为始终单独显示\r\n"
            L"  - 动画 GIF/WebP 在双页模式下两页同时播放\r\n"
            L"\r\n"
            L"## 缩放\r\n"
            L"  - 按住 Ctrl 滚动鼠标滚轮可平滑缩放\r\n"
            L"  - 使用工具栏按钮选择「适应窗口」「适应宽度」等\r\n"
            L"  - 按 Ctrl+0 恢复原始大小\r\n"
            L"\r\n"
            L"## 旋转\r\n"
            L"  - 使用工具栏的旋转按钮或 Ctrl+R 旋转图片\r\n"
            L"\r\n"
            L"## 全屏显示\r\n"
            L"  - 按 F11 切换全屏\r\n"
            L"  - 按 Esc 返回正常视图\r\n"},
        {L"help.text.tree",
            L"## 三种模式\r\n"
            L"  - 普通模式: 显示收藏夹、常用文件夹和驱动器\r\n"
            L"  - 书架模式: 仅显示您注册的压缩包和文件夹\r\n"
            L"    可以创建分类进行整理\r\n"
            L"  - 历史模式: 按时间倒序显示以前打开过的文件夹和压缩包\r\n"
            L"\r\n"
            L"## 收藏夹\r\n"
            L"  - 右键点击文件夹或压缩包，选择「添加到收藏夹」\r\n"
            L"  - 收藏夹显示在树形视图的最顶部\r\n"
            L"\r\n"
            L"## 书架\r\n"
            L"  - 点击书架按钮切换到书架模式\r\n"
            L"  - 右键选择「添加到书架」将项目注册到分类中\r\n"
            L"  - 右键点击分类可以重命名或删除\r\n"
            L"  - 右键文件夹选择「将文件夹添加为书架」可以\r\n"
            L"    一次性注册其中的所有压缩包\r\n"
            L"\r\n"
            L"## 排序\r\n"
            L"  - 使用排序按钮按名称、日期、大小或类型排序\r\n"
            L"  - 当前排序方式显示在按钮上\r\n"},
        {L"help.text.file",
            L"## 文件列表显示\r\n"
            L"  - 使用顶部按钮在列表视图和网格（缩略图）视图之间切换\r\n"
            L"  - 在列表视图中，拖动列标题可更改排序顺序\r\n"
            L"  - 列宽和顺序会在下次启动时保留\r\n"
            L"\r\n"
            L"## 重命名\r\n"
            L"  - 选择文件或文件夹后按 F2 即可就地编辑名称\r\n"
            L"  - 也可以使用右键菜单中的「重命名」\r\n"
            L"  - 按 Enter 确认，Esc 取消\r\n"
            L"\r\n"
            L"## 删除文件\r\n"
            L"  - 按 Delete 键或使用右键菜单删除\r\n"
            L"  - 删除的文件会移至回收站，可以恢复\r\n"
            L"\r\n"
            L"## 筛选文件\r\n"
            L"  - 在文件列表下方的输入框中输入文字，\r\n"
            L"    只显示名称匹配的文件\r\n"
            L"\r\n"
            L"## 右键菜单\r\n"
            L"  - 右键点击文件可执行各种操作\r\n"
            L"  - 用默认程序打开: 使用图片编辑器等其他应用打开\r\n"
            L"  - 在资源管理器中显示: 在资源管理器中打开文件所在位置\r\n"
            L"  - 还包括复制路径、添加到收藏夹/书架、\r\n"
            L"    重命名、删除和属性\r\n"},
        {L"help.text.settings",
            L"## 更改语言\r\n"
            L"  - 支持 12 种语言\r\n"
            L"  - 在设置画面更改后立即生效\r\n"
            L"\r\n"
            L"## 更改键盘快捷键\r\n"
            L"  - 在设置画面的「快捷键」标签页中可自定义所有操作的按键\r\n"
            L"  - 可以为一个操作分配多个按键\r\n"
            L"\r\n"
            L"## 更改外观\r\n"
            L"  - 在设置画面中调整字体大小、缩略图大小和悬停预览大小\r\n"
            L"  - 更改立即生效\r\n"
            L"\r\n"
            L"## 其他设置\r\n"
            L"  - 缓存大小: 调整用于存储已查看图片的内存量\r\n"
            L"  - 循环到开头: 翻过最后一页会回到第一页\r\n"
            L"  - 双页封面单独: 双页模式下第一页单独显示\r\n"
            L"  - 显示子文件夹: 同时显示子文件夹中的图片\r\n"
            L"  - 自动播放视频: 选择视频文件时自动开始播放\r\n"},
    };
}

static void LoadZhTW()
{
    g_strings = {
        {L"tree.desktop",    L"桌面"},
        {L"tree.downloads",  L"下載"},
        {L"tree.documents",  L"文件"},
        {L"tree.pictures",   L"圖片"},
        {L"tree.videos",     L"影片"},
        {L"tree.music",      L"音樂"},
        {L"tree.localdisk",  L"本機磁碟"},
        {L"list.name",       L"名稱"},
        {L"list.size",       L"大小"},
        {L"list.type",       L"類型"},
        {L"list.date",       L"修改日期"},
        {L"list.filter",     L"篩選"},
        {L"type.folder",     L"資料夾"},
        {L"type.file",       L"檔案"},
        {L"type.jpeg",       L"JPEG 圖片"},
        {L"type.png",        L"PNG 圖片"},
        {L"type.gif",        L"GIF 圖片"},
        {L"type.bmp",        L"BMP 圖片"},
        {L"type.webp",       L"WebP 圖片"},
        {L"type.avif",       L"AVIF 圖片"},
        {L"type.tiff",       L"TIFF 圖片"},
        {L"type.ico",        L"圖示"},
        {L"type.zip",        L"ZIP 壓縮檔"},
        {L"type.7z",         L"7z 壓縮檔"},
        {L"type.rar",        L"RAR 壓縮檔"},
        {L"ctx.fullscreen",      L"全螢幕 (F11)"},
        {L"ctx.copyimage",       L"複製圖片 (Ctrl+C)"},
        {L"ctx.copypath",        L"複製路徑"},
        {L"ctx.copyparentpath",  L"複製上層資料夾路徑"},
        {L"ctx.openassoc",       L"用預設程式開啟"},
        {L"ctx.explorer",        L"在檔案總管中顯示"},
        {L"ctx.addfav",          L"加入我的最愛"},
        {L"ctx.removefav",       L"從我的最愛移除"},
        {L"ctx.rename",          L"重新命名"},
        {L"ctx.delete",          L"刪除"},
        {L"ctx.properties",      L"內容"},
        {L"dlg.confirm",    L"確認"},
        {L"dlg.delete",     L"確定刪除嗎？"},
        {L"dlg.settings",   L"設定"},
        {L"status.cantplay",     L"無法播放（請放置 mpv-2.dll）"},
        {L"status.cantopen",     L"無法開啟壓縮檔（找不到 7z.dll 或格式不支援）"},
        {L"status.loading",      L"載入中..."},
        {L"nav.history",    L"歷史"},
        {L"nav.favorites",  L"我的最愛"},
        {L"ui.folder",      L"資料夾"},
        {L"ui.address",     L" 地址(A)"},
        {L"ui.bookshelf",   L"書架"},
        {L"ui.history",     L"歷史"},
        {L"ui.settings",    L"設定"},
        {L"ui.help",        L"說明"},
        {L"nav.back",       L"返回"},
        {L"nav.forward",    L"前進"},
        {L"nav.up",         L"上層"},
        {L"nav.refresh",    L"重新整理"},
        {L"nav.list",       L"清單檢視"},
        {L"nav.grid",       L"格狀檢視"},
        {L"nav.hover",      L"懸停預覽"},
        {L"kb.fit_window",  L"符合視窗"},
        {L"kb.fit_width",   L"符合寬度"},
        {L"kb.fit_height",  L"符合高度"},
        {L"kb.original",    L"原始大小"},
        {L"kb.zoom_in",     L"放大"},
        {L"kb.zoom_out",    L"縮小"},
        {L"kb.view_auto",   L"自動"},
        {L"kb.view_single", L"單頁"},
        {L"kb.view_spread", L"雙頁"},
        {L"kb.binding",     L"裝訂方向"},
        {L"kb.rotate",      L"旋轉"},
        {L"ctx.addshelf",       L"加入書架"},
        {L"ctx.removeshelf",    L"從書架移除"},
        {L"ctx.newshelf",       L"建立新書架..."},
        {L"ctx.shelfascat",     L"將資料夾加入為書架"},
        {L"ctx.shelfname",      L"建立新書架"},
        {L"ctx.shelfprompt",    L"請輸入書架名稱:"},
        {L"ctx.deleteshelf",    L"刪除此書架"},
        {L"sort.asc",           L"升冪"},
        {L"sort.desc",          L"降冪"},
        {L"ui.clearall",        L"全部刪除"},
        {L"ui.renamethis",      L"重新命名"},
        {L"ui.filter",          L"篩選..."},
        {L"drive.network",      L"網路磁碟機"},
        {L"drive.removable",    L"卸除式磁碟"},
        {L"drive.cdrom",        L"CD/DVD 光碟機"},
        {L"drive.local",        L"本機磁碟"},
        {L"media.loop",         L"循環播放"},
        {L"media.autoplay",     L"自動播放"},
        {L"dlg.bookshelfclear", L"確定刪除所有書架記錄嗎？"},
        {L"dlg.historyclear",   L"確定清除所有歷史記錄嗎？"},
        {L"group.today",        L"今天"},
        {L"group.yesterday",    L"昨天"},
        {L"group.thisweek",     L"本週"},
        {L"group.lastweek",     L"上週"},
        {L"group.older",        L"更早"},
        // 設定對話框
        {L"settings.tab.general",   L"一般"},
        {L"settings.tab.shortcuts", L"快捷鍵"},
        {L"settings.nav",           L"導覽"},
        {L"settings.wrap",          L"循環到開頭"},
        {L"settings.spreadfirst",   L"雙頁首頁單獨顯示"},
        {L"settings.fileload",      L"檔案載入"},
        {L"settings.recursive",     L"顯示子資料夾圖片"},
        {L"settings.media",         L"媒體"},
        {L"settings.autoplay",      L"自動播放"},
        {L"settings.perf",          L"效能"},
        {L"settings.cache",         L"快取上限"},
        {L"settings.display",       L"顯示"},
        {L"settings.threshold",     L"雙頁判定閾值"},
        {L"settings.thumbsize",     L"格狀縮圖大小"},
        {L"settings.previewsize",   L"懸停預覽大小"},
        {L"settings.fontsize",      L"字型大小"},
        {L"settings.action",        L"動作"},
        {L"settings.key",           L"按鍵"},
        {L"settings.addkey",        L"新增按鍵..."},
        {L"settings.removekey",     L"刪除按鍵"},
        {L"settings.resetkeys",     L"恢復預設"},
        {L"settings.resetall",      L"恢復預設"},
        {L"settings.ok",            L"確定"},
        {L"settings.cancel",        L"取消"},
        {L"settings.resetconfirm",  L"恢復所有預設設定？"},
        {L"settings.keycapture",    L"按鍵輸入"},
        {L"settings.keyprompt",     L"請按下新的按鍵..."},
        // 說明對話框
        {L"help.title",         L"說明 - karikari 使用方法"},
        {L"help.tab.basic",     L"基本操作"},
        {L"help.tab.view",      L"顯示模式"},
        {L"help.tab.tree",      L"樹狀檢視"},
        {L"help.tab.file",      L"檔案操作"},
        {L"help.tab.settings",  L"設定"},
        {L"help.tab.about",     L"關於"},
        {L"help.version",       L"版本 1.0.0"},
        {L"help.subtitle",      L"最快的圖片/影片檢視器"},
        {L"help.authorlabel",   L"作者:"},
        {L"help.authorname",    L"megamega39"},
        {L"help.desclabel",     L"說明:"},
        {L"help.desc1",         L"使用 C++ + Win32 API + Direct2D 建構的高效能圖片/影片檢視器。"},
        {L"help.desc2",         L"leeyez_kai (.NET版) 的原生移植。"},
        {L"help.close",         L"關閉"},
        {L"help.text.basic",
            L"## 檢視圖片\r\n"
            L"  - 從左側的樹狀檢視或檔案清單中選擇資料夾或壓縮檔開啟\r\n"
            L"  - 使用 ← → 鍵或滑鼠滾輪翻頁\r\n"
            L"  - 按 Home / End 跳到第一頁或最後一頁\r\n"
            L"\r\n"
            L"## 開啟壓縮檔\r\n"
            L"  - 支援 ZIP、7z、RAR、CBZ、CBR、CB7 壓縮檔\r\n"
            L"  - 可以直接瀏覽壓縮檔內的資料夾\r\n"
            L"  - 使用 ↑ ↓ 鍵切換到同一資料夾中的前後壓縮檔\r\n"
            L"\r\n"
            L"## 播放影片和音訊\r\n"
            L"  - 可以直接播放影片和音訊檔案\r\n"
            L"  - 按 Space 鍵切換播放/暫停\r\n"
            L"  - 使用底部的控制列進行快轉、循環和音量調整\r\n"
            L"  - 按播放列的「A」按鈕切換自動播放開/關\r\n"},
        {L"help.text.view",
            L"## 雙頁顯示\r\n"
            L"  - 可以將兩頁並排顯示\r\n"
            L"  - 自動模式下，直向圖片以雙頁顯示，橫向圖片單獨顯示\r\n"
            L"  - 按 1 切換單頁，按 2 切換雙頁，按 3 切換自動\r\n"
            L"  - 可以在設定中將封面（第一頁）設為始終單獨顯示\r\n"
            L"  - 動畫 GIF/WebP 在雙頁模式下兩頁同時播放\r\n"
            L"\r\n"
            L"## 縮放\r\n"
            L"  - 按住 Ctrl 滾動滑鼠滾輪可平滑縮放\r\n"
            L"  - 使用工具列按鈕選擇「符合視窗」「符合寬度」等\r\n"
            L"  - 按 Ctrl+0 恢復原始大小\r\n"
            L"\r\n"
            L"## 旋轉\r\n"
            L"  - 使用工具列的旋轉按鈕或 Ctrl+R 旋轉圖片\r\n"
            L"\r\n"
            L"## 全螢幕顯示\r\n"
            L"  - 按 F11 切換全螢幕\r\n"
            L"  - 按 Esc 返回正常檢視\r\n"},
        {L"help.text.tree",
            L"## 三種模式\r\n"
            L"  - 一般模式: 顯示我的最愛、常用資料夾和磁碟機\r\n"
            L"  - 書架模式: 僅顯示您註冊的壓縮檔和資料夾\r\n"
            L"    可以建立分類進行整理\r\n"
            L"  - 歷史模式: 按時間倒序顯示以前開啟過的資料夾和壓縮檔\r\n"
            L"\r\n"
            L"## 我的最愛\r\n"
            L"  - 右鍵點擊資料夾或壓縮檔，選擇「加入我的最愛」\r\n"
            L"  - 我的最愛顯示在樹狀檢視的最頂部\r\n"
            L"\r\n"
            L"## 書架\r\n"
            L"  - 點擊書架按鈕切換到書架模式\r\n"
            L"  - 右鍵選擇「加入書架」將項目註冊到分類中\r\n"
            L"  - 右鍵點擊分類可以重新命名或刪除\r\n"
            L"  - 右鍵資料夾選擇「將資料夾加入為書架」可以\r\n"
            L"    一次註冊其中的所有壓縮檔\r\n"
            L"\r\n"
            L"## 排序\r\n"
            L"  - 使用排序按鈕按名稱、日期、大小或類型排序\r\n"
            L"  - 目前排序方式顯示在按鈕上\r\n"},
        {L"help.text.file",
            L"## 檔案清單顯示\r\n"
            L"  - 使用頂部按鈕在清單檢視和格狀（縮圖）檢視之間切換\r\n"
            L"  - 在清單檢視中，拖曳欄標題可更改排序順序\r\n"
            L"  - 欄寬和順序會在下次啟動時保留\r\n"
            L"\r\n"
            L"## 重新命名\r\n"
            L"  - 選擇檔案或資料夾後按 F2 即可就地編輯名稱\r\n"
            L"  - 也可以使用右鍵選單中的「重新命名」\r\n"
            L"  - 按 Enter 確認，Esc 取消\r\n"
            L"\r\n"
            L"## 刪除檔案\r\n"
            L"  - 按 Delete 鍵或使用右鍵選單刪除\r\n"
            L"  - 刪除的檔案會移至資源回收筒，可以還原\r\n"
            L"\r\n"
            L"## 篩選檔案\r\n"
            L"  - 在檔案清單下方的輸入欄位中輸入文字，\r\n"
            L"    只顯示名稱相符的檔案\r\n"
            L"\r\n"
            L"## 右鍵選單\r\n"
            L"  - 右鍵點擊檔案可執行各種操作\r\n"
            L"  - 用預設程式開啟: 使用圖片編輯器等其他應用程式開啟\r\n"
            L"  - 在檔案總管中顯示: 在檔案總管中開啟檔案所在位置\r\n"
            L"  - 還包括複製路徑、加入我的最愛/書架、\r\n"
            L"    重新命名、刪除和內容\r\n"},
        {L"help.text.settings",
            L"## 變更語言\r\n"
            L"  - 支援 12 種語言\r\n"
            L"  - 在設定畫面變更後立即生效\r\n"
            L"\r\n"
            L"## 變更鍵盤快捷鍵\r\n"
            L"  - 在設定畫面的「快捷鍵」標籤頁中可自訂所有操作的按鍵\r\n"
            L"  - 可以為一個操作指定多個按鍵\r\n"
            L"\r\n"
            L"## 變更外觀\r\n"
            L"  - 在設定畫面中調整字型大小、縮圖大小和懸停預覽大小\r\n"
            L"  - 變更立即生效\r\n"
            L"\r\n"
            L"## 其他設定\r\n"
            L"  - 快取大小: 調整用於儲存已檢視圖片的記憶體量\r\n"
            L"  - 循環到開頭: 翻過最後一頁會回到第一頁\r\n"
            L"  - 雙頁封面單獨: 雙頁模式下第一頁單獨顯示\r\n"
            L"  - 顯示子資料夾: 同時顯示子資料夾中的圖片\r\n"
            L"  - 自動播放影片: 選擇影片檔案時自動開始播放\r\n"},
    };
}

static void LoadKo()
{
    g_strings = {
        {L"tree.desktop",    L"바탕 화면"},
        {L"tree.downloads",  L"다운로드"},
        {L"tree.documents",  L"문서"},
        {L"tree.pictures",   L"사진"},
        {L"tree.videos",     L"동영상"},
        {L"tree.music",      L"음악"},
        {L"tree.localdisk",  L"로컬 디스크"},
        {L"list.name",       L"이름"},
        {L"list.size",       L"크기"},
        {L"list.type",       L"유형"},
        {L"list.date",       L"수정한 날짜"},
        {L"list.filter",     L"필터"},
        {L"type.folder",     L"폴더"},
        {L"type.file",       L"파일"},
        {L"type.jpeg",       L"JPEG 이미지"},
        {L"type.png",        L"PNG 이미지"},
        {L"type.gif",        L"GIF 이미지"},
        {L"type.bmp",        L"BMP 이미지"},
        {L"type.webp",       L"WebP 이미지"},
        {L"type.avif",       L"AVIF 이미지"},
        {L"type.tiff",       L"TIFF 이미지"},
        {L"type.ico",        L"아이콘"},
        {L"type.zip",        L"ZIP 압축 파일"},
        {L"type.7z",         L"7z 압축 파일"},
        {L"type.rar",        L"RAR 압축 파일"},
        {L"ctx.fullscreen",      L"전체 화면 (F11)"},
        {L"ctx.copyimage",       L"이미지 복사 (Ctrl+C)"},
        {L"ctx.copypath",        L"경로 복사"},
        {L"ctx.copyparentpath",  L"상위 폴더 경로 복사"},
        {L"ctx.openassoc",       L"기본 앱으로 열기"},
        {L"ctx.explorer",        L"탐색기에서 표시"},
        {L"ctx.addfav",          L"즐겨찾기에 추가"},
        {L"ctx.removefav",       L"즐겨찾기에서 제거"},
        {L"ctx.rename",          L"이름 바꾸기"},
        {L"ctx.delete",          L"삭제"},
        {L"ctx.properties",      L"속성"},
        {L"dlg.confirm",    L"확인"},
        {L"dlg.delete",     L"삭제하시겠습니까?"},
        {L"dlg.settings",   L"설정"},
        {L"status.cantplay",     L"재생할 수 없습니다 (mpv-2.dll을 배치하세요)"},
        {L"status.cantopen",     L"압축 파일을 열 수 없습니다 (7z.dll을 찾을 수 없거나 지원하지 않는 형식)"},
        {L"status.loading",      L"로딩 중..."},
        {L"nav.history",    L"기록"},
        {L"nav.favorites",  L"즐겨찾기"},
        {L"ui.folder",      L"폴더"},
        {L"ui.address",     L" 주소(A)"},
        {L"ui.bookshelf",   L"서재"},
        {L"ui.history",     L"기록"},
        {L"ui.settings",    L"설정"},
        {L"ui.help",        L"도움말"},
        {L"nav.back",       L"뒤로"},
        {L"nav.forward",    L"앞으로"},
        {L"nav.up",         L"위로"},
        {L"nav.refresh",    L"새로고침"},
        {L"nav.list",       L"목록 보기"},
        {L"nav.grid",       L"그리드 보기"},
        {L"nav.hover",      L"호버 미리보기"},
        {L"kb.fit_window",  L"창에 맞추기"},
        {L"kb.fit_width",   L"너비에 맞추기"},
        {L"kb.fit_height",  L"높이에 맞추기"},
        {L"kb.original",    L"원본 크기"},
        {L"kb.zoom_in",     L"확대"},
        {L"kb.zoom_out",    L"축소"},
        {L"kb.view_auto",   L"자동"},
        {L"kb.view_single", L"단일 페이지"},
        {L"kb.view_spread", L"양면"},
        {L"kb.binding",     L"제본 방향"},
        {L"kb.rotate",      L"회전"},
        {L"ctx.addshelf",       L"서재에 추가"},
        {L"ctx.removeshelf",    L"서재에서 제거"},
        {L"ctx.newshelf",       L"새 서재 만들기..."},
        {L"ctx.shelfascat",     L"폴더를 서재로 추가"},
        {L"ctx.shelfname",      L"새 서재 만들기"},
        {L"ctx.shelfprompt",    L"서재 이름을 입력하세요:"},
        {L"ctx.deleteshelf",    L"이 서재 삭제"},
        {L"sort.asc",           L"오름차순"},
        {L"sort.desc",          L"내림차순"},
        {L"ui.clearall",        L"전체 삭제"},
        {L"ui.renamethis",      L"이름 바꾸기"},
        {L"ui.filter",          L"필터..."},
        {L"drive.network",      L"네트워크 드라이브"},
        {L"drive.removable",    L"이동식 디스크"},
        {L"drive.cdrom",        L"CD/DVD 드라이브"},
        {L"drive.local",        L"로컬 디스크"},
        {L"media.loop",         L"반복 재생"},
        {L"media.autoplay",     L"자동 재생"},
        {L"dlg.bookshelfclear", L"서재의 모든 항목을 삭제하시겠습니까?"},
        {L"dlg.historyclear",   L"모든 기록을 삭제하시겠습니까?"},
        {L"group.today",        L"오늘"},
        {L"group.yesterday",    L"어제"},
        {L"group.thisweek",     L"이번 주"},
        {L"group.lastweek",     L"지난 주"},
        {L"group.older",        L"이전"},
        // 설정 대화상자
        {L"settings.tab.general",   L"일반"},
        {L"settings.tab.shortcuts", L"단축키"},
        {L"settings.nav",           L"탐색"},
        {L"settings.wrap",          L"끝에서 처음으로 순환"},
        {L"settings.spreadfirst",   L"양면 모드에서 첫 페이지 단독 표시"},
        {L"settings.fileload",      L"파일 로드"},
        {L"settings.recursive",     L"하위 폴더 이미지 표시"},
        {L"settings.media",         L"미디어"},
        {L"settings.autoplay",      L"자동 재생"},
        {L"settings.perf",          L"성능"},
        {L"settings.cache",         L"캐시 메모리 제한"},
        {L"settings.display",       L"표시"},
        {L"settings.threshold",     L"양면 판정 임계값"},
        {L"settings.thumbsize",     L"그리드 썸네일 크기"},
        {L"settings.previewsize",   L"호버 미리보기 크기"},
        {L"settings.fontsize",      L"글꼴 크기"},
        {L"settings.action",        L"동작"},
        {L"settings.key",           L"키"},
        {L"settings.addkey",        L"키 추가..."},
        {L"settings.removekey",     L"키 삭제"},
        {L"settings.resetkeys",     L"기본값으로 초기화"},
        {L"settings.resetall",      L"기본값으로 초기화"},
        {L"settings.ok",            L"확인"},
        {L"settings.cancel",        L"취소"},
        {L"settings.resetconfirm",  L"모든 설정을 초기화하시겠습니까?"},
        {L"settings.keycapture",    L"키 입력"},
        {L"settings.keyprompt",     L"새 키를 누르세요..."},
        // 도움말 대화상자
        {L"help.title",         L"도움말 - karikari 사용법"},
        {L"help.tab.basic",     L"기본 조작"},
        {L"help.tab.view",      L"보기 모드"},
        {L"help.tab.tree",      L"트리 보기"},
        {L"help.tab.file",      L"파일 작업"},
        {L"help.tab.settings",  L"설정"},
        {L"help.tab.about",     L"정보"},
        {L"help.version",       L"버전 1.0.0"},
        {L"help.subtitle",      L"가장 빠른 이미지/동영상 뷰어"},
        {L"help.authorlabel",   L"제작자:"},
        {L"help.authorname",    L"megamega39"},
        {L"help.desclabel",     L"설명:"},
        {L"help.desc1",         L"C++ + Win32 API + Direct2D로 구축한 고성능 이미지/동영상 뷰어."},
        {L"help.desc2",         L"leeyez_kai (.NET 버전)의 네이티브 이식."},
        {L"help.close",         L"닫기"},
        {L"help.text.basic",
            L"## 이미지 보기\r\n"
            L"  - 왼쪽의 트리나 파일 목록에서 폴더 또는 압축 파일을 선택하여 열 수 있습니다\r\n"
            L"  - ← → 키 또는 마우스 휠로 페이지를 넘길 수 있습니다\r\n"
            L"  - Home / End를 눌러 첫 페이지 또는 마지막 페이지로 이동합니다\r\n"
            L"\r\n"
            L"## 압축 파일 열기\r\n"
            L"  - ZIP, 7z, RAR, CBZ, CBR, CB7 압축 파일을 지원합니다\r\n"
            L"  - 압축 파일 내의 폴더도 직접 탐색할 수 있습니다\r\n"
            L"  - ↑ ↓ 키로 같은 폴더에 있는 이전/다음 압축 파일로 이동합니다\r\n"
            L"\r\n"
            L"## 동영상 및 오디오 재생\r\n"
            L"  - 동영상 및 오디오 파일을 바로 재생할 수 있습니다\r\n"
            L"  - Space 키로 재생/일시정지를 전환합니다\r\n"
            L"  - 하단의 바에서 탐색, 반복, 볼륨 조절이 가능합니다\r\n"
            L"  - 재생 바의 「A」 버튼으로 자동 재생 켜기/끄기를 전환합니다\r\n"},
        {L"help.text.view",
            L"## 양면 보기\r\n"
            L"  - 두 페이지를 나란히 표시할 수 있습니다\r\n"
            L"  - 자동 모드에서는 세로 이미지는 양면, 가로 이미지는 단독으로 표시합니다\r\n"
            L"  - 1키로 단일, 2키로 양면, 3키로 자동으로 전환합니다\r\n"
            L"  - 설정에서 표지(첫 페이지)를 항상 단독 표시로 설정할 수 있습니다\r\n"
            L"  - 애니메이션 GIF/WebP도 양면에서 동시에 재생됩니다\r\n"
            L"\r\n"
            L"## 확대/축소\r\n"
            L"  - Ctrl을 누른 채 마우스 휠을 굴리면 부드럽게 확대/축소됩니다\r\n"
            L"  - 툴바 버튼으로 「창에 맞추기」「너비에 맞추기」 등을 선택합니다\r\n"
            L"  - Ctrl+0으로 원래 크기로 돌아갑니다\r\n"
            L"\r\n"
            L"## 회전\r\n"
            L"  - 툴바의 회전 버튼 또는 Ctrl+R로 이미지를 회전합니다\r\n"
            L"\r\n"
            L"## 전체 화면\r\n"
            L"  - F11 키로 전체 화면을 전환합니다\r\n"
            L"  - Esc 키로 일반 보기로 돌아갑니다\r\n"},
        {L"help.text.tree",
            L"## 세 가지 모드\r\n"
            L"  - 일반 모드: 즐겨찾기, 자주 사용하는 폴더, 드라이브를 표시합니다\r\n"
            L"  - 서재 모드: 등록한 압축 파일과 폴더만 표시합니다\r\n"
            L"    카테고리를 만들어 정리할 수 있습니다\r\n"
            L"  - 기록 모드: 이전에 열었던 폴더와 압축 파일을 최신 순으로 나열합니다\r\n"
            L"\r\n"
            L"## 즐겨찾기\r\n"
            L"  - 폴더나 압축 파일을 오른쪽 클릭하여 「즐겨찾기에 추가」를 선택합니다\r\n"
            L"  - 즐겨찾기는 트리의 최상단에 표시됩니다\r\n"
            L"\r\n"
            L"## 서재\r\n"
            L"  - 서재 버튼을 눌러 서재 모드로 전환합니다\r\n"
            L"  - 오른쪽 클릭으로 「서재에 추가」를 선택하여 카테고리에 등록합니다\r\n"
            L"  - 카테고리를 오른쪽 클릭하여 이름 변경이나 삭제가 가능합니다\r\n"
            L"  - 폴더를 오른쪽 클릭하고 「폴더를 서재로 추가」를 선택하면\r\n"
            L"    안에 있는 모든 압축 파일을 한꺼번에 등록합니다\r\n"
            L"\r\n"
            L"## 정렬\r\n"
            L"  - 정렬 버튼으로 이름, 날짜, 크기, 유형 순으로 정렬합니다\r\n"
            L"  - 현재 정렬 순서가 버튼에 표시됩니다\r\n"},
        {L"help.text.file",
            L"## 파일 목록 표시\r\n"
            L"  - 상단의 버튼으로 목록 보기와 그리드(썸네일) 보기를 전환합니다\r\n"
            L"  - 목록 보기에서 열 머리글을 드래그하여 정렬 순서를 변경합니다\r\n"
            L"  - 열 너비와 순서는 다음 실행 시에도 유지됩니다\r\n"
            L"\r\n"
            L"## 이름 바꾸기\r\n"
            L"  - 파일이나 폴더를 선택하고 F2를 누르면 이름을 바로 편집할 수 있습니다\r\n"
            L"  - 오른쪽 클릭 메뉴의 「이름 바꾸기」로도 가능합니다\r\n"
            L"  - Enter로 확인, Esc로 취소합니다\r\n"
            L"\r\n"
            L"## 파일 삭제\r\n"
            L"  - Delete 키 또는 오른쪽 클릭 메뉴로 삭제합니다\r\n"
            L"  - 삭제된 파일은 휴지통으로 이동되어 복원할 수 있습니다\r\n"
            L"\r\n"
            L"## 파일 필터링\r\n"
            L"  - 파일 목록 아래의 입력란에 문자를 입력하면\r\n"
            L"    이름이 일치하는 파일만 표시됩니다\r\n"
            L"\r\n"
            L"## 오른쪽 클릭 메뉴\r\n"
            L"  - 파일을 오른쪽 클릭하면 다양한 작업을 수행할 수 있습니다\r\n"
            L"  - 기본 앱으로 열기: 이미지 편집기 등 다른 앱으로 열 수 있습니다\r\n"
            L"  - 탐색기에서 표시: 파일의 위치를 탐색기에서 엽니다\r\n"
            L"  - 경로 복사, 즐겨찾기/서재에 추가,\r\n"
            L"    이름 바꾸기, 삭제, 속성도 사용할 수 있습니다\r\n"},
        {L"help.text.settings",
            L"## 언어 변경\r\n"
            L"  - 12개 언어를 지원합니다\r\n"
            L"  - 설정 화면에서 변경하면 즉시 적용됩니다\r\n"
            L"\r\n"
            L"## 키보드 단축키 변경\r\n"
            L"  - 설정 화면의 「단축키」 탭에서 모든 작업의 키를 변경할 수 있습니다\r\n"
            L"  - 하나의 작업에 여러 키를 등록할 수 있습니다\r\n"
            L"\r\n"
            L"## 외관 변경\r\n"
            L"  - 설정 화면에서 글꼴 크기, 썸네일 크기, 호버 미리보기 크기를 조정합니다\r\n"
            L"  - 변경 사항은 즉시 반영됩니다\r\n"
            L"\r\n"
            L"## 기타 설정\r\n"
            L"  - 캐시 크기: 본 이미지를 저장하는 메모리 양을 조절합니다\r\n"
            L"  - 끝에서 순환: 마지막 페이지에서 다음으로 가면 처음으로 돌아갑니다\r\n"
            L"  - 양면 표지 단독: 양면 보기에서 첫 페이지만 단독 표시합니다\r\n"
            L"  - 하위 폴더 표시: 하위 폴더의 이미지도 함께 표시합니다\r\n"
            L"  - 동영상 자동 재생: 동영상 파일 선택 시 자동으로 재생을 시작합니다\r\n"},
    };
}

static void LoadEs()
{
    g_strings = {
        {L"tree.desktop",    L"Escritorio"},
        {L"tree.downloads",  L"Descargas"},
        {L"tree.documents",  L"Documentos"},
        {L"tree.pictures",   L"Imágenes"},
        {L"tree.videos",     L"Vídeos"},
        {L"tree.music",      L"Música"},
        {L"tree.localdisk",  L"Disco local"},
        {L"list.name",       L"Nombre"},
        {L"list.size",       L"Tamaño"},
        {L"list.type",       L"Tipo"},
        {L"list.date",       L"Fecha de modificación"},
        {L"list.filter",     L"Filtro"},
        {L"type.folder",     L"Carpeta"},
        {L"type.file",       L"Archivo"},
        {L"type.jpeg",       L"Imagen JPEG"},
        {L"type.png",        L"Imagen PNG"},
        {L"type.gif",        L"Imagen GIF"},
        {L"type.bmp",        L"Imagen BMP"},
        {L"type.webp",       L"Imagen WebP"},
        {L"type.avif",       L"Imagen AVIF"},
        {L"type.tiff",       L"Imagen TIFF"},
        {L"type.ico",        L"Icono"},
        {L"type.zip",        L"Archivo ZIP"},
        {L"type.7z",         L"Archivo 7z"},
        {L"type.rar",        L"Archivo RAR"},
        {L"ctx.fullscreen",      L"Pantalla completa (F11)"},
        {L"ctx.copyimage",       L"Copiar imagen (Ctrl+C)"},
        {L"ctx.copypath",        L"Copiar ruta"},
        {L"ctx.copyparentpath",  L"Copiar ruta de carpeta superior"},
        {L"ctx.openassoc",       L"Abrir con app predeterminada"},
        {L"ctx.explorer",        L"Mostrar en Explorador"},
        {L"ctx.addfav",          L"Agregar a favoritos"},
        {L"ctx.removefav",       L"Quitar de favoritos"},
        {L"ctx.rename",          L"Cambiar nombre"},
        {L"ctx.delete",          L"Eliminar"},
        {L"ctx.properties",      L"Propiedades"},
        {L"dlg.confirm",    L"Confirmar"},
        {L"dlg.delete",     L"¿Eliminar?"},
        {L"dlg.settings",   L"Configuración"},
        {L"status.cantplay",     L"No se puede reproducir (coloque mpv-2.dll)"},
        {L"status.cantopen",     L"No se puede abrir el archivo (7z.dll no encontrado)"},
        {L"status.loading",      L"Cargando..."},
        {L"nav.history",    L"Historial"},
        {L"nav.favorites",  L"Favoritos"},
        {L"ui.folder",      L"Carpeta"},
        {L"ui.address",     L" Dirección(A)"},
        {L"ui.bookshelf",   L"Estantería"},
        {L"ui.history",     L"Historial"},
        {L"ui.settings",    L"Configuración"},
        {L"ui.help",        L"Ayuda"},
        {L"nav.back",       L"Atrás"},
        {L"nav.forward",    L"Adelante"},
        {L"nav.up",         L"Arriba"},
        {L"nav.refresh",    L"Actualizar"},
        {L"nav.list",       L"Vista de lista"},
        {L"nav.grid",       L"Vista de cuadrícula"},
        {L"nav.hover",      L"Vista previa"},
        {L"kb.fit_window",  L"Ajustar a ventana"},
        {L"kb.fit_width",   L"Ajustar al ancho"},
        {L"kb.fit_height",  L"Ajustar al alto"},
        {L"kb.original",    L"Tamaño original"},
        {L"kb.zoom_in",     L"Ampliar"},
        {L"kb.zoom_out",    L"Reducir"},
        {L"kb.view_auto",   L"Automático"},
        {L"kb.view_single", L"Página única"},
        {L"kb.view_spread", L"Doble página"},
        {L"kb.binding",     L"Dirección de encuadernación"},
        {L"kb.rotate",      L"Rotar"},
        {L"ctx.addshelf",       L"Agregar a estantería"},
        {L"ctx.removeshelf",    L"Quitar de estantería"},
        {L"ctx.newshelf",       L"Crear nueva estantería..."},
        {L"ctx.shelfascat",     L"Agregar carpeta como estantería"},
        {L"ctx.shelfname",      L"Crear nueva estantería"},
        {L"ctx.shelfprompt",    L"Ingrese el nombre de la estantería:"},
        {L"ctx.deleteshelf",    L"Eliminar esta estantería"},
        {L"sort.asc",           L"Ascendente"},
        {L"sort.desc",          L"Descendente"},
        {L"ui.clearall",        L"Borrar todo"},
        {L"ui.renamethis",      L"Cambiar nombre"},
        {L"ui.filter",          L"Filtro..."},
        {L"drive.network",      L"Unidad de red"},
        {L"drive.removable",    L"Disco extraíble"},
        {L"drive.cdrom",        L"Unidad CD/DVD"},
        {L"drive.local",        L"Disco local"},
        {L"media.loop",         L"Repetir"},
        {L"media.autoplay",     L"Reproducción automática"},
        {L"dlg.bookshelfclear", L"¿Eliminar todos los registros de la estantería?"},
        {L"dlg.historyclear",   L"¿Borrar todo el historial?"},
        {L"group.today",        L"Hoy"},
        {L"group.yesterday",    L"Ayer"},
        {L"group.thisweek",     L"Esta semana"},
        {L"group.lastweek",     L"Semana pasada"},
        {L"group.older",        L"Anterior"},
        // Diálogo de configuración
        {L"settings.tab.general",   L"General"},
        {L"settings.tab.shortcuts", L"Atajos"},
        {L"settings.nav",           L"Navegación"},
        {L"settings.wrap",          L"Repetir en los extremos"},
        {L"settings.spreadfirst",   L"Mostrar primera página sola en doble página"},
        {L"settings.fileload",      L"Carga de archivos"},
        {L"settings.recursive",     L"Mostrar imágenes de subcarpetas"},
        {L"settings.media",         L"Medios"},
        {L"settings.autoplay",      L"Reproducción automática"},
        {L"settings.perf",          L"Rendimiento"},
        {L"settings.cache",         L"Límite de caché"},
        {L"settings.display",       L"Visualización"},
        {L"settings.threshold",     L"Umbral de doble página"},
        {L"settings.thumbsize",     L"Tamaño de miniatura"},
        {L"settings.previewsize",   L"Tamaño de vista previa"},
        {L"settings.fontsize",      L"Tamaño de fuente"},
        {L"settings.action",        L"Acción"},
        {L"settings.key",           L"Tecla"},
        {L"settings.addkey",        L"Añadir tecla..."},
        {L"settings.removekey",     L"Eliminar tecla"},
        {L"settings.resetkeys",     L"Restablecer"},
        {L"settings.resetall",      L"Restablecer"},
        {L"settings.ok",            L"Aceptar"},
        {L"settings.cancel",        L"Cancelar"},
        {L"settings.resetconfirm",  L"¿Restablecer todas las configuraciones?"},
        {L"settings.keycapture",    L"Entrada de tecla"},
        {L"settings.keyprompt",     L"Presione una nueva tecla..."},
        // Diálogo de ayuda
        {L"help.title",         L"Ayuda - Cómo usar karikari"},
        {L"help.tab.basic",     L"Básico"},
        {L"help.tab.view",      L"Modo de vista"},
        {L"help.tab.tree",      L"Vista de árbol"},
        {L"help.tab.file",      L"Operaciones de archivo"},
        {L"help.tab.settings",  L"Configuración"},
        {L"help.tab.about",     L"Acerca de"},
        {L"help.version",       L"Versión 1.0.0"},
        {L"help.subtitle",      L"El visor de imágenes y vídeos más rápido"},
        {L"help.authorlabel",   L"Autor:"},
        {L"help.authorname",    L"megamega39"},
        {L"help.desclabel",     L"Descripción:"},
        {L"help.desc1",         L"Visor de imágenes/vídeos de alto rendimiento construido con C++ + Win32 API + Direct2D."},
        {L"help.desc2",         L"Portación nativa de leeyez_kai (versión .NET)."},
        {L"help.close",         L"Cerrar"},
        {L"help.text.basic",
            L"## Ver imágenes\r\n"
            L"  - Seleccione una carpeta o archivo comprimido desde el árbol o la lista de archivos a la izquierda\r\n"
            L"  - Use las teclas ← → o la rueda del ratón para pasar páginas\r\n"
            L"  - Presione Home / End para ir a la primera o última página\r\n"
            L"\r\n"
            L"## Abrir archivos comprimidos\r\n"
            L"  - Compatible con archivos ZIP, 7z, RAR, CBZ, CBR y CB7\r\n"
            L"  - Puede navegar por las carpetas dentro de los archivos comprimidos\r\n"
            L"  - Use las teclas ↑ ↓ para moverse al archivo comprimido anterior o siguiente en la misma carpeta\r\n"
            L"\r\n"
            L"## Reproducir vídeo y audio\r\n"
            L"  - Los archivos de vídeo y audio se pueden reproducir directamente\r\n"
            L"  - Presione Espacio para alternar reproducción/pausa\r\n"
            L"  - Use la barra inferior para buscar, repetir y ajustar el volumen\r\n"
            L"  - Presione el botón \"A\" en la barra de reproducción para activar/desactivar la reproducción automática\r\n"},
        {L"help.text.view",
            L"## Vista de doble página\r\n"
            L"  - Muestra dos páginas una al lado de la otra\r\n"
            L"  - En modo automático, las imágenes verticales se muestran en doble página, las horizontales solas\r\n"
            L"  - Presione 1 para Única, 2 para Doble, 3 para Automático\r\n"
            L"  - Puede configurar la portada (primera página) para que siempre se muestre sola\r\n"
            L"  - Los GIF/WebP animados se reproducen simultáneamente en ambas páginas\r\n"
            L"\r\n"
            L"## Zoom (Ampliar / Reducir)\r\n"
            L"  - Mantenga Ctrl y use la rueda del ratón para un zoom suave\r\n"
            L"  - Use los botones de la barra de herramientas para elegir \"Ajustar a ventana\", \"Ajustar al ancho\", etc.\r\n"
            L"  - Presione Ctrl+0 para restablecer al tamaño original\r\n"
            L"\r\n"
            L"## Rotación\r\n"
            L"  - Rote imágenes con el botón de la barra de herramientas o Ctrl+R\r\n"
            L"\r\n"
            L"## Pantalla completa\r\n"
            L"  - Presione F11 para alternar pantalla completa\r\n"
            L"  - Presione Esc para volver a la vista normal\r\n"},
        {L"help.text.tree",
            L"## Tres modos\r\n"
            L"  - Modo normal: Muestra favoritos, carpetas frecuentes y unidades\r\n"
            L"  - Modo estantería: Muestra solo los archivos y carpetas que ha registrado\r\n"
            L"    Puede crear categorías para organizarlos\r\n"
            L"  - Modo historial: Lista las carpetas y archivos abiertos anteriormente, más recientes primero\r\n"
            L"\r\n"
            L"## Favoritos\r\n"
            L"  - Haga clic derecho en una carpeta o archivo y seleccione \"Agregar a favoritos\"\r\n"
            L"  - Los favoritos aparecen en la parte superior del árbol\r\n"
            L"\r\n"
            L"## Estantería\r\n"
            L"  - Presione el botón Estantería para cambiar al modo estantería\r\n"
            L"  - Haga clic derecho y seleccione \"Agregar a estantería\" para registrar en una categoría\r\n"
            L"  - Haga clic derecho en una categoría para renombrar o eliminar\r\n"
            L"  - Haga clic derecho en una carpeta y seleccione \"Agregar carpeta como estantería\"\r\n"
            L"    para registrar todos los archivos comprimidos de una vez\r\n"
            L"\r\n"
            L"## Ordenar\r\n"
            L"  - Use el botón de ordenar para ordenar por nombre, fecha, tamaño o tipo\r\n"
            L"  - El orden actual se muestra en el botón\r\n"},
        {L"help.text.file",
            L"## Lista de archivos\r\n"
            L"  - Use los botones superiores para cambiar entre vista de lista y cuadrícula (miniaturas)\r\n"
            L"  - En vista de lista, arrastre los encabezados de columna para cambiar el orden\r\n"
            L"  - El ancho y orden de las columnas se conservan para el próximo inicio\r\n"
            L"\r\n"
            L"## Cambiar nombre\r\n"
            L"  - Seleccione un archivo o carpeta y presione F2 para editar el nombre\r\n"
            L"  - También puede usar \"Cambiar nombre\" del menú contextual\r\n"
            L"  - Presione Enter para confirmar, Esc para cancelar\r\n"
            L"\r\n"
            L"## Eliminar archivos\r\n"
            L"  - Presione Suprimir o use el menú contextual para eliminar\r\n"
            L"  - Los archivos eliminados se mueven a la Papelera de reciclaje\r\n"
            L"\r\n"
            L"## Filtrar archivos\r\n"
            L"  - Escriba en el campo de entrada debajo de la lista de archivos\r\n"
            L"    para mostrar solo los archivos que coincidan\r\n"
            L"\r\n"
            L"## Menú contextual\r\n"
            L"  - Haga clic derecho en un archivo para varias acciones\r\n"
            L"  - Abrir con app predeterminada: Abrir en otra aplicación\r\n"
            L"  - Mostrar en Explorador: Abrir la ubicación del archivo\r\n"
            L"  - También incluye Copiar ruta, Agregar a favoritos/estantería,\r\n"
            L"    Cambiar nombre, Eliminar y Propiedades\r\n"},
        {L"help.text.settings",
            L"## Cambiar idioma\r\n"
            L"  - Compatible con 12 idiomas\r\n"
            L"  - Los cambios se aplican inmediatamente en la pantalla de configuración\r\n"
            L"\r\n"
            L"## Cambiar atajos de teclado\r\n"
            L"  - Personalice todas las teclas en la pestaña \"Atajos\" de la configuración\r\n"
            L"  - Puede asignar varias teclas a una misma acción\r\n"
            L"\r\n"
            L"## Cambiar apariencia\r\n"
            L"  - Ajuste el tamaño de fuente, miniaturas y vista previa en la configuración\r\n"
            L"  - Los cambios se aplican al instante\r\n"
            L"\r\n"
            L"## Otras configuraciones\r\n"
            L"  - Tamaño de caché: Ajuste la memoria para almacenar imágenes vistas\r\n"
            L"  - Repetir en extremos: Al pasar la última página vuelve a la primera\r\n"
            L"  - Portada en doble página: Muestra la primera página sola en modo doble\r\n"
            L"  - Mostrar subcarpetas: Muestra imágenes de subcarpetas también\r\n"
            L"  - Reproducción automática: Inicia la reproducción automáticamente al seleccionar un vídeo\r\n"},
    };
}

static void LoadPtBR()
{
    g_strings = {
        {L"tree.desktop",    L"Área de Trabalho"},
        {L"tree.downloads",  L"Downloads"},
        {L"tree.documents",  L"Documentos"},
        {L"tree.pictures",   L"Imagens"},
        {L"tree.videos",     L"Vídeos"},
        {L"tree.music",      L"Músicas"},
        {L"tree.localdisk",  L"Disco Local"},
        {L"list.name",       L"Nome"},
        {L"list.size",       L"Tamanho"},
        {L"list.type",       L"Tipo"},
        {L"list.date",       L"Data de modificação"},
        {L"list.filter",     L"Filtro"},
        {L"type.folder",     L"Pasta"},
        {L"type.file",       L"Arquivo"},
        {L"type.jpeg",       L"Imagem JPEG"},
        {L"type.png",        L"Imagem PNG"},
        {L"type.gif",        L"Imagem GIF"},
        {L"type.bmp",        L"Imagem BMP"},
        {L"type.webp",       L"Imagem WebP"},
        {L"type.avif",       L"Imagem AVIF"},
        {L"type.tiff",       L"Imagem TIFF"},
        {L"type.ico",        L"Ícone"},
        {L"type.zip",        L"Arquivo ZIP"},
        {L"type.7z",         L"Arquivo 7z"},
        {L"type.rar",        L"Arquivo RAR"},
        {L"ctx.fullscreen",      L"Tela cheia (F11)"},
        {L"ctx.copyimage",       L"Copiar imagem (Ctrl+C)"},
        {L"ctx.copypath",        L"Copiar caminho"},
        {L"ctx.copyparentpath",  L"Copiar caminho da pasta superior"},
        {L"ctx.openassoc",       L"Abrir com app padrão"},
        {L"ctx.explorer",        L"Mostrar no Explorador"},
        {L"ctx.addfav",          L"Adicionar aos favoritos"},
        {L"ctx.removefav",       L"Remover dos favoritos"},
        {L"ctx.rename",          L"Renomear"},
        {L"ctx.delete",          L"Excluir"},
        {L"ctx.properties",      L"Propriedades"},
        {L"dlg.confirm",    L"Confirmar"},
        {L"dlg.delete",     L"Excluir?"},
        {L"dlg.settings",   L"Configurações"},
        {L"status.cantplay",     L"Não é possível reproduzir (coloque mpv-2.dll)"},
        {L"status.cantopen",     L"Não é possível abrir o arquivo (7z.dll não encontrado)"},
        {L"status.loading",      L"Carregando..."},
        {L"nav.history",    L"Histórico"},
        {L"nav.favorites",  L"Favoritos"},
        {L"ui.folder",      L"Pasta"},
        {L"ui.address",     L" Endereço(A)"},
        {L"ui.bookshelf",   L"Estante"},
        {L"ui.history",     L"Histórico"},
        {L"ui.settings",    L"Configurações"},
        {L"ui.help",        L"Ajuda"},
        {L"nav.back",       L"Voltar"},
        {L"nav.forward",    L"Avançar"},
        {L"nav.up",         L"Acima"},
        {L"nav.refresh",    L"Atualizar"},
        {L"nav.list",       L"Lista"},
        {L"nav.grid",       L"Grade"},
        {L"nav.hover",      L"Pré-visualização"},
        {L"kb.fit_window",  L"Ajustar à janela"},
        {L"kb.fit_width",   L"Ajustar à largura"},
        {L"kb.fit_height",  L"Ajustar à altura"},
        {L"kb.original",    L"Tamanho original"},
        {L"kb.zoom_in",     L"Ampliar"},
        {L"kb.zoom_out",    L"Reduzir"},
        {L"kb.view_auto",   L"Automático"},
        {L"kb.view_single", L"Página única"},
        {L"kb.view_spread", L"Página dupla"},
        {L"kb.binding",     L"Direção de encadernação"},
        {L"kb.rotate",      L"Girar"},
        {L"ctx.addshelf",       L"Adicionar à estante"},
        {L"ctx.removeshelf",    L"Remover da estante"},
        {L"ctx.newshelf",       L"Criar nova estante..."},
        {L"ctx.shelfascat",     L"Adicionar pasta como estante"},
        {L"ctx.shelfname",      L"Criar nova estante"},
        {L"ctx.shelfprompt",    L"Digite o nome da estante:"},
        {L"ctx.deleteshelf",    L"Excluir esta estante"},
        {L"sort.asc",           L"Crescente"},
        {L"sort.desc",          L"Decrescente"},
        {L"ui.clearall",        L"Excluir tudo"},
        {L"ui.renamethis",      L"Renomear"},
        {L"ui.filter",          L"Filtro..."},
        {L"drive.network",      L"Unidade de rede"},
        {L"drive.removable",    L"Disco removível"},
        {L"drive.cdrom",        L"Unidade CD/DVD"},
        {L"drive.local",        L"Disco local"},
        {L"media.loop",         L"Repetir"},
        {L"media.autoplay",     L"Reprodução automática"},
        {L"dlg.bookshelfclear", L"Excluir todos os registros da estante?"},
        {L"dlg.historyclear",   L"Limpar todo o histórico?"},
        {L"group.today",        L"Hoje"},
        {L"group.yesterday",    L"Ontem"},
        {L"group.thisweek",     L"Esta semana"},
        {L"group.lastweek",     L"Semana passada"},
        {L"group.older",        L"Anterior"},
        // Diálogo de configurações
        {L"settings.tab.general",   L"Geral"},
        {L"settings.tab.shortcuts", L"Atalhos"},
        {L"settings.nav",           L"Navegação"},
        {L"settings.wrap",          L"Repetir nas extremidades"},
        {L"settings.spreadfirst",   L"Mostrar primeira página sozinha"},
        {L"settings.fileload",      L"Carregamento de arquivos"},
        {L"settings.recursive",     L"Mostrar imagens de subpastas"},
        {L"settings.media",         L"Mídia"},
        {L"settings.autoplay",      L"Reprodução automática"},
        {L"settings.perf",          L"Desempenho"},
        {L"settings.cache",         L"Limite de cache"},
        {L"settings.display",       L"Exibição"},
        {L"settings.threshold",     L"Limite de página dupla"},
        {L"settings.thumbsize",     L"Tamanho da miniatura"},
        {L"settings.previewsize",   L"Tamanho da pré-visualização"},
        {L"settings.fontsize",      L"Tamanho da fonte"},
        {L"settings.action",        L"Ação"},
        {L"settings.key",           L"Tecla"},
        {L"settings.addkey",        L"Adicionar tecla..."},
        {L"settings.removekey",     L"Remover tecla"},
        {L"settings.resetkeys",     L"Restaurar padrão"},
        {L"settings.resetall",      L"Restaurar padrão"},
        {L"settings.ok",            L"OK"},
        {L"settings.cancel",        L"Cancelar"},
        {L"settings.resetconfirm",  L"Restaurar todas as configurações?"},
        {L"settings.keycapture",    L"Entrada de tecla"},
        {L"settings.keyprompt",     L"Pressione uma nova tecla..."},
        // Diálogo de ajuda
        {L"help.title",         L"Ajuda - Como usar o karikari"},
        {L"help.tab.basic",     L"Básico"},
        {L"help.tab.view",      L"Modo de exibição"},
        {L"help.tab.tree",      L"Árvore"},
        {L"help.tab.file",      L"Operações de arquivo"},
        {L"help.tab.settings",  L"Configurações"},
        {L"help.tab.about",     L"Sobre"},
        {L"help.version",       L"Versão 1.0.0"},
        {L"help.subtitle",      L"O visualizador de imagens e vídeos mais rápido"},
        {L"help.authorlabel",   L"Autor:"},
        {L"help.authorname",    L"megamega39"},
        {L"help.desclabel",     L"Descrição:"},
        {L"help.desc1",         L"Visualizador de imagens/vídeos de alto desempenho construído com C++ + Win32 API + Direct2D."},
        {L"help.desc2",         L"Portação nativa do leeyez_kai (versão .NET)."},
        {L"help.close",         L"Fechar"},
        {L"help.text.basic",
            L"## Visualizar imagens\r\n"
            L"  - Selecione uma pasta ou arquivo compactado na árvore ou lista de arquivos à esquerda\r\n"
            L"  - Use as teclas ← → ou a roda do mouse para virar páginas\r\n"
            L"  - Pressione Home / End para ir à primeira ou última página\r\n"
            L"\r\n"
            L"## Abrir arquivos compactados\r\n"
            L"  - Suporta arquivos ZIP, 7z, RAR, CBZ, CBR e CB7\r\n"
            L"  - Você pode navegar pelas pastas dentro dos arquivos compactados\r\n"
            L"  - Use as teclas ↑ ↓ para ir ao arquivo compactado anterior ou próximo na mesma pasta\r\n"
            L"\r\n"
            L"## Reproduzir vídeo e áudio\r\n"
            L"  - Arquivos de vídeo e áudio podem ser reproduzidos diretamente\r\n"
            L"  - Pressione Espaço para alternar reprodução/pausa\r\n"
            L"  - Use a barra inferior para busca, repetição e ajuste de volume\r\n"
            L"  - Pressione o botão \"A\" na barra de reprodução para ativar/desativar reprodução automática\r\n"},
        {L"help.text.view",
            L"## Visualização em página dupla\r\n"
            L"  - Exibe duas páginas lado a lado\r\n"
            L"  - No modo automático, imagens em retrato são mostradas em página dupla, paisagem sozinhas\r\n"
            L"  - Pressione 1 para Única, 2 para Dupla, 3 para Automático\r\n"
            L"  - Você pode configurar a capa (primeira página) para sempre ser exibida sozinha\r\n"
            L"  - GIF/WebP animados são reproduzidos simultaneamente em ambas as páginas\r\n"
            L"\r\n"
            L"## Zoom (Ampliar / Reduzir)\r\n"
            L"  - Segure Ctrl e use a roda do mouse para zoom suave\r\n"
            L"  - Use os botões da barra de ferramentas para \"Ajustar à janela\", \"Ajustar à largura\", etc.\r\n"
            L"  - Pressione Ctrl+0 para redefinir ao tamanho original\r\n"
            L"\r\n"
            L"## Rotação\r\n"
            L"  - Gire imagens com o botão da barra de ferramentas ou Ctrl+R\r\n"
            L"\r\n"
            L"## Tela cheia\r\n"
            L"  - Pressione F11 para alternar tela cheia\r\n"
            L"  - Pressione Esc para voltar à visualização normal\r\n"},
        {L"help.text.tree",
            L"## Três modos\r\n"
            L"  - Modo normal: Mostra favoritos, pastas frequentes e unidades\r\n"
            L"  - Modo estante: Mostra apenas os arquivos e pastas que você registrou\r\n"
            L"    Você pode criar categorias para organizá-los\r\n"
            L"  - Modo histórico: Lista pastas e arquivos abertos anteriormente, mais recentes primeiro\r\n"
            L"\r\n"
            L"## Favoritos\r\n"
            L"  - Clique com o botão direito em uma pasta ou arquivo e selecione \"Adicionar aos Favoritos\"\r\n"
            L"  - Os favoritos aparecem no topo da árvore\r\n"
            L"\r\n"
            L"## Estante\r\n"
            L"  - Pressione o botão Estante para mudar para o modo estante\r\n"
            L"  - Clique com o botão direito e selecione \"Adicionar à Estante\" para registrar em uma categoria\r\n"
            L"  - Clique com o botão direito em uma categoria para renomear ou excluir\r\n"
            L"  - Clique com o botão direito em uma pasta e selecione \"Adicionar pasta como estante\"\r\n"
            L"    para registrar todos os arquivos compactados de uma vez\r\n"
            L"\r\n"
            L"## Ordenação\r\n"
            L"  - Use o botão de ordenação para ordenar por nome, data, tamanho ou tipo\r\n"
            L"  - A ordem atual é mostrada no botão\r\n"},
        {L"help.text.file",
            L"## Lista de arquivos\r\n"
            L"  - Use os botões superiores para alternar entre lista e grade (miniaturas)\r\n"
            L"  - Na visualização em lista, arraste os cabeçalhos de coluna para alterar a ordem\r\n"
            L"  - A largura e ordem das colunas são preservadas para a próxima inicialização\r\n"
            L"\r\n"
            L"## Renomear\r\n"
            L"  - Selecione um arquivo ou pasta e pressione F2 para editar o nome\r\n"
            L"  - Também pode usar \"Renomear\" no menu de contexto\r\n"
            L"  - Pressione Enter para confirmar, Esc para cancelar\r\n"
            L"\r\n"
            L"## Excluir arquivos\r\n"
            L"  - Pressione Delete ou use o menu de contexto para excluir\r\n"
            L"  - Arquivos excluídos são movidos para a Lixeira e podem ser restaurados\r\n"
            L"\r\n"
            L"## Filtrar arquivos\r\n"
            L"  - Digite no campo de entrada abaixo da lista de arquivos\r\n"
            L"    para mostrar apenas os arquivos correspondentes\r\n"
            L"\r\n"
            L"## Menu de contexto\r\n"
            L"  - Clique com o botão direito em um arquivo para várias ações\r\n"
            L"  - Abrir com app padrão: Abrir em outro aplicativo\r\n"
            L"  - Mostrar no Explorador: Abrir a localização do arquivo\r\n"
            L"  - Também inclui Copiar caminho, Adicionar a favoritos/estante,\r\n"
            L"    Renomear, Excluir e Propriedades\r\n"},
        {L"help.text.settings",
            L"## Alterar idioma\r\n"
            L"  - Suporta 12 idiomas\r\n"
            L"  - As alterações são aplicadas imediatamente na tela de configurações\r\n"
            L"\r\n"
            L"## Alterar atalhos de teclado\r\n"
            L"  - Personalize todas as teclas na aba \"Atalhos\" das configurações\r\n"
            L"  - Você pode atribuir várias teclas a uma mesma ação\r\n"
            L"\r\n"
            L"## Alterar aparência\r\n"
            L"  - Ajuste o tamanho da fonte, miniaturas e pré-visualização nas configurações\r\n"
            L"  - As alterações são aplicadas instantaneamente\r\n"
            L"\r\n"
            L"## Outras configurações\r\n"
            L"  - Tamanho do cache: Ajuste a memória para armazenar imagens visualizadas\r\n"
            L"  - Repetir nas extremidades: Ao passar a última página, volta à primeira\r\n"
            L"  - Capa em página dupla: Mostra a primeira página sozinha no modo duplo\r\n"
            L"  - Mostrar subpastas: Exibe imagens de subpastas também\r\n"
            L"  - Reprodução automática: Inicia a reprodução ao selecionar um vídeo\r\n"},
    };
}

static void LoadFr()
{
    g_strings = {
        {L"tree.desktop",    L"Bureau"},
        {L"tree.downloads",  L"Téléchargements"},
        {L"tree.documents",  L"Documents"},
        {L"tree.pictures",   L"Images"},
        {L"tree.videos",     L"Vidéos"},
        {L"tree.music",      L"Musique"},
        {L"tree.localdisk",  L"Disque local"},
        {L"list.name",       L"Nom"},
        {L"list.size",       L"Taille"},
        {L"list.type",       L"Type"},
        {L"list.date",       L"Date de modification"},
        {L"list.filter",     L"Filtre"},
        {L"type.folder",     L"Dossier"},
        {L"type.file",       L"Fichier"},
        {L"type.jpeg",       L"Image JPEG"},
        {L"type.png",        L"Image PNG"},
        {L"type.gif",        L"Image GIF"},
        {L"type.bmp",        L"Image BMP"},
        {L"type.webp",       L"Image WebP"},
        {L"type.avif",       L"Image AVIF"},
        {L"type.tiff",       L"Image TIFF"},
        {L"type.ico",        L"Icône"},
        {L"type.zip",        L"Archive ZIP"},
        {L"type.7z",         L"Archive 7z"},
        {L"type.rar",        L"Archive RAR"},
        {L"ctx.fullscreen",      L"Plein écran (F11)"},
        {L"ctx.copyimage",       L"Copier l'image (Ctrl+C)"},
        {L"ctx.copypath",        L"Copier le chemin"},
        {L"ctx.copyparentpath",  L"Copier le chemin du dossier parent"},
        {L"ctx.openassoc",       L"Ouvrir avec l'app par défaut"},
        {L"ctx.explorer",        L"Afficher dans l'Explorateur"},
        {L"ctx.addfav",          L"Ajouter aux favoris"},
        {L"ctx.removefav",       L"Retirer des favoris"},
        {L"ctx.rename",          L"Renommer"},
        {L"ctx.delete",          L"Supprimer"},
        {L"ctx.properties",      L"Propriétés"},
        {L"dlg.confirm",    L"Confirmer"},
        {L"dlg.delete",     L"Supprimer ?"},
        {L"dlg.settings",   L"Paramètres"},
        {L"status.cantplay",     L"Lecture impossible (placez mpv-2.dll)"},
        {L"status.cantopen",     L"Impossible d'ouvrir l'archive (7z.dll introuvable)"},
        {L"status.loading",      L"Chargement..."},
        {L"nav.history",    L"Historique"},
        {L"nav.favorites",  L"Favoris"},
        {L"ui.folder",      L"Dossier"},
        {L"ui.address",     L" Adresse(A)"},
        {L"ui.bookshelf",   L"Bibliothèque"},
        {L"ui.history",     L"Historique"},
        {L"ui.settings",    L"Paramètres"},
        {L"ui.help",        L"Aide"},
        {L"nav.back",       L"Retour"},
        {L"nav.forward",    L"Avancer"},
        {L"nav.up",         L"Monter"},
        {L"nav.refresh",    L"Actualiser"},
        {L"nav.list",       L"Vue en liste"},
        {L"nav.grid",       L"Vue en grille"},
        {L"nav.hover",      L"Aperçu"},
        {L"kb.fit_window",  L"Ajuster à la fenêtre"},
        {L"kb.fit_width",   L"Ajuster à la largeur"},
        {L"kb.fit_height",  L"Ajuster à la hauteur"},
        {L"kb.original",    L"Taille originale"},
        {L"kb.zoom_in",     L"Agrandir"},
        {L"kb.zoom_out",    L"Réduire"},
        {L"kb.view_auto",   L"Automatique"},
        {L"kb.view_single", L"Page unique"},
        {L"kb.view_spread", L"Double page"},
        {L"kb.binding",     L"Sens de reliure"},
        {L"kb.rotate",      L"Pivoter"},
        {L"ctx.addshelf",       L"Ajouter à la bibliothèque"},
        {L"ctx.removeshelf",    L"Retirer de la bibliothèque"},
        {L"ctx.newshelf",       L"Créer une nouvelle étagère..."},
        {L"ctx.shelfascat",     L"Ajouter le dossier comme étagère"},
        {L"ctx.shelfname",      L"Créer une nouvelle étagère"},
        {L"ctx.shelfprompt",    L"Entrez le nom de l'étagère :"},
        {L"ctx.deleteshelf",    L"Supprimer cette étagère"},
        {L"sort.asc",           L"Croissant"},
        {L"sort.desc",          L"Décroissant"},
        {L"ui.clearall",        L"Tout supprimer"},
        {L"ui.renamethis",      L"Renommer"},
        {L"ui.filter",          L"Filtre..."},
        {L"drive.network",      L"Lecteur réseau"},
        {L"drive.removable",    L"Disque amovible"},
        {L"drive.cdrom",        L"Lecteur CD/DVD"},
        {L"drive.local",        L"Disque local"},
        {L"media.loop",         L"Boucle"},
        {L"media.autoplay",     L"Lecture automatique"},
        {L"dlg.bookshelfclear", L"Supprimer tous les enregistrements de la bibliothèque ?"},
        {L"dlg.historyclear",   L"Effacer tout l'historique ?"},
        {L"group.today",        L"Aujourd'hui"},
        {L"group.yesterday",    L"Hier"},
        {L"group.thisweek",     L"Cette semaine"},
        {L"group.lastweek",     L"Semaine dernière"},
        {L"group.older",        L"Plus ancien"},
        // Boîte de dialogue des paramètres
        {L"settings.tab.general",   L"Général"},
        {L"settings.tab.shortcuts", L"Raccourcis"},
        {L"settings.nav",           L"Navigation"},
        {L"settings.wrap",          L"Boucler aux extrémités"},
        {L"settings.spreadfirst",   L"Première page seule en double page"},
        {L"settings.fileload",      L"Chargement"},
        {L"settings.recursive",     L"Afficher les sous-dossiers"},
        {L"settings.media",         L"Médias"},
        {L"settings.autoplay",      L"Lecture automatique"},
        {L"settings.perf",          L"Performance"},
        {L"settings.cache",         L"Limite du cache"},
        {L"settings.display",       L"Affichage"},
        {L"settings.threshold",     L"Seuil de double page"},
        {L"settings.thumbsize",     L"Taille des vignettes"},
        {L"settings.previewsize",   L"Taille de l'aperçu"},
        {L"settings.fontsize",      L"Taille de police"},
        {L"settings.action",        L"Action"},
        {L"settings.key",           L"Touche"},
        {L"settings.addkey",        L"Ajouter touche..."},
        {L"settings.removekey",     L"Supprimer touche"},
        {L"settings.resetkeys",     L"Par défaut"},
        {L"settings.resetall",      L"Par défaut"},
        {L"settings.ok",            L"OK"},
        {L"settings.cancel",        L"Annuler"},
        {L"settings.resetconfirm",  L"Réinitialiser tous les paramètres ?"},
        {L"settings.keycapture",    L"Saisie de touche"},
        {L"settings.keyprompt",     L"Appuyez sur une nouvelle touche..."},
        // Boîte de dialogue d'aide
        {L"help.title",         L"Aide - Comment utiliser karikari"},
        {L"help.tab.basic",     L"Bases"},
        {L"help.tab.view",      L"Mode d'affichage"},
        {L"help.tab.tree",      L"Arborescence"},
        {L"help.tab.file",      L"Opérations sur les fichiers"},
        {L"help.tab.settings",  L"Paramètres"},
        {L"help.tab.about",     L"À propos"},
        {L"help.version",       L"Version 1.0.0"},
        {L"help.subtitle",      L"La visionneuse d'images et de vidéos la plus rapide"},
        {L"help.authorlabel",   L"Auteur :"},
        {L"help.authorname",    L"megamega39"},
        {L"help.desclabel",     L"Description :"},
        {L"help.desc1",         L"Visionneuse d'images/vidéos haute performance construite avec C++ + Win32 API + Direct2D."},
        {L"help.desc2",         L"Portage natif de leeyez_kai (version .NET)."},
        {L"help.close",         L"Fermer"},
        {L"help.text.basic",
            L"## Visionner des images\r\n"
            L"  - Sélectionnez un dossier ou une archive dans l'arborescence ou la liste de fichiers à gauche\r\n"
            L"  - Utilisez les touches ← → ou la molette de la souris pour tourner les pages\r\n"
            L"  - Appuyez sur Début / Fin pour aller à la première ou dernière page\r\n"
            L"\r\n"
            L"## Ouvrir des archives\r\n"
            L"  - Prend en charge les archives ZIP, 7z, RAR, CBZ, CBR et CB7\r\n"
            L"  - Vous pouvez parcourir les dossiers à l'intérieur des archives\r\n"
            L"  - Utilisez les touches ↑ ↓ pour passer à l'archive précédente ou suivante dans le même dossier\r\n"
            L"\r\n"
            L"## Lire des vidéos et de l'audio\r\n"
            L"  - Les fichiers vidéo et audio peuvent être lus directement\r\n"
            L"  - Appuyez sur Espace pour basculer lecture/pause\r\n"
            L"  - Utilisez la barre inférieure pour la recherche, la répétition et le réglage du volume\r\n"
            L"  - Appuyez sur le bouton « A » de la barre de lecture pour activer/désactiver la lecture automatique\r\n"},
        {L"help.text.view",
            L"## Affichage double page\r\n"
            L"  - Affiche deux pages côte à côte\r\n"
            L"  - En mode automatique, les images portrait s'affichent en double page, les paysages seules\r\n"
            L"  - Appuyez sur 1 pour Simple, 2 pour Double, 3 pour Automatique\r\n"
            L"  - Vous pouvez configurer la couverture (première page) pour qu'elle s'affiche toujours seule\r\n"
            L"  - Les GIF/WebP animés sont lus simultanément sur les deux pages\r\n"
            L"\r\n"
            L"## Zoom (Agrandir / Réduire)\r\n"
            L"  - Maintenez Ctrl et utilisez la molette pour un zoom fluide\r\n"
            L"  - Utilisez les boutons de la barre d'outils pour « Ajuster à la fenêtre », « Ajuster à la largeur », etc.\r\n"
            L"  - Appuyez sur Ctrl+0 pour revenir à la taille originale\r\n"
            L"\r\n"
            L"## Rotation\r\n"
            L"  - Faites pivoter les images avec le bouton de la barre d'outils ou Ctrl+R\r\n"
            L"\r\n"
            L"## Plein écran\r\n"
            L"  - Appuyez sur F11 pour basculer en plein écran\r\n"
            L"  - Appuyez sur Échap pour revenir à la vue normale\r\n"},
        {L"help.text.tree",
            L"## Trois modes\r\n"
            L"  - Mode normal : Affiche les favoris, les dossiers fréquents et les lecteurs\r\n"
            L"  - Mode bibliothèque : Affiche uniquement les archives et dossiers que vous avez enregistrés\r\n"
            L"    Vous pouvez créer des catégories pour les organiser\r\n"
            L"  - Mode historique : Liste les dossiers et archives ouverts précédemment, les plus récents en premier\r\n"
            L"\r\n"
            L"## Favoris\r\n"
            L"  - Faites un clic droit sur un dossier ou une archive et sélectionnez « Ajouter aux favoris »\r\n"
            L"  - Les favoris apparaissent en haut de l'arborescence\r\n"
            L"\r\n"
            L"## Bibliothèque\r\n"
            L"  - Appuyez sur le bouton Bibliothèque pour passer en mode bibliothèque\r\n"
            L"  - Clic droit et sélectionnez « Ajouter à la bibliothèque » pour enregistrer dans une catégorie\r\n"
            L"  - Clic droit sur une catégorie pour renommer ou supprimer\r\n"
            L"  - Clic droit sur un dossier et sélectionnez « Ajouter le dossier comme bibliothèque »\r\n"
            L"    pour enregistrer toutes les archives d'un coup\r\n"
            L"\r\n"
            L"## Tri\r\n"
            L"  - Utilisez le bouton de tri pour trier par nom, date, taille ou type\r\n"
            L"  - L'ordre actuel est affiché sur le bouton\r\n"},
        {L"help.text.file",
            L"## Liste de fichiers\r\n"
            L"  - Utilisez les boutons en haut pour basculer entre vue liste et grille (miniatures)\r\n"
            L"  - En vue liste, glissez les en-têtes de colonne pour changer l'ordre de tri\r\n"
            L"  - La largeur et l'ordre des colonnes sont conservés au prochain lancement\r\n"
            L"\r\n"
            L"## Renommer\r\n"
            L"  - Sélectionnez un fichier ou dossier et appuyez sur F2 pour modifier le nom\r\n"
            L"  - Vous pouvez aussi utiliser « Renommer » dans le menu contextuel\r\n"
            L"  - Appuyez sur Entrée pour confirmer, Échap pour annuler\r\n"
            L"\r\n"
            L"## Supprimer des fichiers\r\n"
            L"  - Appuyez sur Suppr ou utilisez le menu contextuel pour supprimer\r\n"
            L"  - Les fichiers supprimés sont déplacés dans la Corbeille et peuvent être restaurés\r\n"
            L"\r\n"
            L"## Filtrer les fichiers\r\n"
            L"  - Saisissez du texte dans le champ sous la liste de fichiers\r\n"
            L"    pour n'afficher que les fichiers correspondants\r\n"
            L"\r\n"
            L"## Menu contextuel\r\n"
            L"  - Faites un clic droit sur un fichier pour diverses actions\r\n"
            L"  - Ouvrir avec l'app par défaut : Ouvrir dans une autre application\r\n"
            L"  - Afficher dans l'Explorateur : Ouvrir l'emplacement du fichier\r\n"
            L"  - Inclut également Copier le chemin, Ajouter aux favoris/bibliothèque,\r\n"
            L"    Renommer, Supprimer et Propriétés\r\n"},
        {L"help.text.settings",
            L"## Changer de langue\r\n"
            L"  - Prend en charge 12 langues\r\n"
            L"  - Les modifications prennent effet immédiatement dans l'écran des paramètres\r\n"
            L"\r\n"
            L"## Modifier les raccourcis clavier\r\n"
            L"  - Personnalisez toutes les touches dans l'onglet « Raccourcis » des paramètres\r\n"
            L"  - Vous pouvez attribuer plusieurs touches à une même action\r\n"
            L"\r\n"
            L"## Modifier l'apparence\r\n"
            L"  - Ajustez la taille de police, des miniatures et de l'aperçu dans les paramètres\r\n"
            L"  - Les modifications sont appliquées instantanément\r\n"
            L"\r\n"
            L"## Autres paramètres\r\n"
            L"  - Taille du cache : Ajustez la mémoire pour stocker les images consultées\r\n"
            L"  - Boucle aux extrémités : Passer la dernière page revient à la première\r\n"
            L"  - Couverture en page simple : Affiche la première page seule en mode double\r\n"
            L"  - Afficher les sous-dossiers : Affiche aussi les images des sous-dossiers\r\n"
            L"  - Lecture automatique : Démarre automatiquement la lecture en sélectionnant une vidéo\r\n"},
    };
}

static void LoadDe()
{
    g_strings = {
        {L"tree.desktop",    L"Desktop"},
        {L"tree.downloads",  L"Downloads"},
        {L"tree.documents",  L"Dokumente"},
        {L"tree.pictures",   L"Bilder"},
        {L"tree.videos",     L"Videos"},
        {L"tree.music",      L"Musik"},
        {L"tree.localdisk",  L"Lokaler Datenträger"},
        {L"list.name",       L"Name"},
        {L"list.size",       L"Größe"},
        {L"list.type",       L"Typ"},
        {L"list.date",       L"Änderungsdatum"},
        {L"list.filter",     L"Filter"},
        {L"type.folder",     L"Ordner"},
        {L"type.file",       L"Datei"},
        {L"type.jpeg",       L"JPEG-Bild"},
        {L"type.png",        L"PNG-Bild"},
        {L"type.gif",        L"GIF-Bild"},
        {L"type.bmp",        L"BMP-Bild"},
        {L"type.webp",       L"WebP-Bild"},
        {L"type.avif",       L"AVIF-Bild"},
        {L"type.tiff",       L"TIFF-Bild"},
        {L"type.ico",        L"Symbol"},
        {L"type.zip",        L"ZIP-Archiv"},
        {L"type.7z",         L"7z-Archiv"},
        {L"type.rar",        L"RAR-Archiv"},
        {L"ctx.fullscreen",      L"Vollbild (F11)"},
        {L"ctx.copyimage",       L"Bild kopieren (Ctrl+C)"},
        {L"ctx.copypath",        L"Pfad kopieren"},
        {L"ctx.copyparentpath",  L"Übergeordneten Pfad kopieren"},
        {L"ctx.openassoc",       L"Mit Standard-App öffnen"},
        {L"ctx.explorer",        L"Im Explorer anzeigen"},
        {L"ctx.addfav",          L"Zu Favoriten hinzufügen"},
        {L"ctx.removefav",       L"Aus Favoriten entfernen"},
        {L"ctx.rename",          L"Umbenennen"},
        {L"ctx.delete",          L"Löschen"},
        {L"ctx.properties",      L"Eigenschaften"},
        {L"dlg.confirm",    L"Bestätigen"},
        {L"dlg.delete",     L"Löschen?"},
        {L"dlg.settings",   L"Einstellungen"},
        {L"status.cantplay",     L"Wiedergabe nicht möglich (mpv-2.dll platzieren)"},
        {L"status.cantopen",     L"Archiv kann nicht geöffnet werden (7z.dll nicht gefunden)"},
        {L"status.loading",      L"Laden..."},
        {L"nav.history",    L"Verlauf"},
        {L"nav.favorites",  L"Favoriten"},
        {L"ui.folder",      L"Ordner"},
        {L"ui.address",     L" Adresse(A)"},
        {L"ui.bookshelf",   L"Bücherregal"},
        {L"ui.history",     L"Verlauf"},
        {L"ui.settings",    L"Einstellungen"},
        {L"ui.help",        L"Hilfe"},
        {L"nav.back",       L"Zurück"},
        {L"nav.forward",    L"Vorwärts"},
        {L"nav.up",         L"Nach oben"},
        {L"nav.refresh",    L"Aktualisieren"},
        {L"nav.list",       L"Listenansicht"},
        {L"nav.grid",       L"Rasteransicht"},
        {L"nav.hover",      L"Hover-Vorschau"},
        {L"kb.fit_window",  L"An Fenster anpassen"},
        {L"kb.fit_width",   L"An Breite anpassen"},
        {L"kb.fit_height",  L"An Höhe anpassen"},
        {L"kb.original",    L"Originalgröße"},
        {L"kb.zoom_in",     L"Vergrößern"},
        {L"kb.zoom_out",    L"Verkleinern"},
        {L"kb.view_auto",   L"Automatisch"},
        {L"kb.view_single", L"Einzelseite"},
        {L"kb.view_spread", L"Doppelseite"},
        {L"kb.binding",     L"Bindungsrichtung"},
        {L"kb.rotate",      L"Drehen"},
        {L"ctx.addshelf",       L"Zum Bücherregal hinzufügen"},
        {L"ctx.removeshelf",    L"Aus Bücherregal entfernen"},
        {L"ctx.newshelf",       L"Neues Regal erstellen..."},
        {L"ctx.shelfascat",     L"Ordner als Regal hinzufügen"},
        {L"ctx.shelfname",      L"Neues Regal erstellen"},
        {L"ctx.shelfprompt",    L"Regalname eingeben:"},
        {L"ctx.deleteshelf",    L"Dieses Regal löschen"},
        {L"sort.asc",           L"Aufsteigend"},
        {L"sort.desc",          L"Absteigend"},
        {L"ui.clearall",        L"Alle löschen"},
        {L"ui.renamethis",      L"Umbenennen"},
        {L"ui.filter",          L"Filter..."},
        {L"drive.network",      L"Netzlaufwerk"},
        {L"drive.removable",    L"Wechseldatenträger"},
        {L"drive.cdrom",        L"CD/DVD-Laufwerk"},
        {L"drive.local",        L"Lokaler Datenträger"},
        {L"media.loop",         L"Schleife"},
        {L"media.autoplay",     L"Automatische Wiedergabe"},
        {L"dlg.bookshelfclear", L"Alle Einträge im Bücherregal löschen?"},
        {L"dlg.historyclear",   L"Gesamten Verlauf löschen?"},
        {L"group.today",        L"Heute"},
        {L"group.yesterday",    L"Gestern"},
        {L"group.thisweek",     L"Diese Woche"},
        {L"group.lastweek",     L"Letzte Woche"},
        {L"group.older",        L"Älter"},
        // Einstellungsdialog
        {L"settings.tab.general",   L"Allgemein"},
        {L"settings.tab.shortcuts", L"Tastenkürzel"},
        {L"settings.nav",           L"Navigation"},
        {L"settings.wrap",          L"An Enden wiederholen"},
        {L"settings.spreadfirst",   L"Erste Seite einzeln anzeigen"},
        {L"settings.fileload",      L"Datei laden"},
        {L"settings.recursive",     L"Unterordner anzeigen"},
        {L"settings.media",         L"Medien"},
        {L"settings.autoplay",      L"Automatische Wiedergabe"},
        {L"settings.perf",          L"Leistung"},
        {L"settings.cache",         L"Cache-Limit"},
        {L"settings.display",       L"Anzeige"},
        {L"settings.threshold",     L"Schwellenwert für Doppelseite"},
        {L"settings.thumbsize",     L"Miniaturbildgröße"},
        {L"settings.previewsize",   L"Vorschaugröße"},
        {L"settings.fontsize",      L"Schriftgröße"},
        {L"settings.action",        L"Aktion"},
        {L"settings.key",           L"Taste"},
        {L"settings.addkey",        L"Taste hinzufügen..."},
        {L"settings.removekey",     L"Taste entfernen"},
        {L"settings.resetkeys",     L"Zurücksetzen"},
        {L"settings.resetall",      L"Zurücksetzen"},
        {L"settings.ok",            L"OK"},
        {L"settings.cancel",        L"Abbrechen"},
        {L"settings.resetconfirm",  L"Alle Einstellungen zurücksetzen?"},
        {L"settings.keycapture",    L"Tasteneingabe"},
        {L"settings.keyprompt",     L"Neue Taste drücken..."},
        // Hilfedialog
        {L"help.title",         L"Hilfe - Wie man karikari benutzt"},
        {L"help.tab.basic",     L"Grundlagen"},
        {L"help.tab.view",      L"Anzeigemodus"},
        {L"help.tab.tree",      L"Baumansicht"},
        {L"help.tab.file",      L"Dateioperationen"},
        {L"help.tab.settings",  L"Einstellungen"},
        {L"help.tab.about",     L"Über"},
        {L"help.version",       L"Version 1.0.0"},
        {L"help.subtitle",      L"Der schnellste Bild- und Video-Viewer"},
        {L"help.authorlabel",   L"Autor:"},
        {L"help.authorname",    L"megamega39"},
        {L"help.desclabel",     L"Beschreibung:"},
        {L"help.desc1",         L"Hochleistungs-Bild-/Video-Viewer, erstellt mit C++ + Win32 API + Direct2D."},
        {L"help.desc2",         L"Native Portierung von leeyez_kai (.NET-Version)."},
        {L"help.close",         L"Schließen"},
        {L"help.text.basic",
            L"## Bilder ansehen\r\n"
            L"  - Wählen Sie einen Ordner oder ein Archiv aus dem Baum oder der Dateiliste links\r\n"
            L"  - Verwenden Sie die Tasten ← → oder das Mausrad zum Blättern\r\n"
            L"  - Drücken Sie Pos1 / Ende, um zur ersten oder letzten Seite zu springen\r\n"
            L"\r\n"
            L"## Archive öffnen\r\n"
            L"  - Unterstützt ZIP-, 7z-, RAR-, CBZ-, CBR- und CB7-Archive\r\n"
            L"  - Sie können Ordner innerhalb von Archiven direkt durchsuchen\r\n"
            L"  - Verwenden Sie die Tasten ↑ ↓, um zum vorherigen oder nächsten Archiv im selben Ordner zu wechseln\r\n"
            L"\r\n"
            L"## Video und Audio abspielen\r\n"
            L"  - Video- und Audiodateien können direkt abgespielt werden\r\n"
            L"  - Drücken Sie Leertaste, um zwischen Wiedergabe und Pause zu wechseln\r\n"
            L"  - Verwenden Sie die untere Leiste zum Suchen, Wiederholen und Lautstärkeregelung\r\n"
            L"  - Drücken Sie die Taste \"A\" in der Wiedergabeleiste, um die automatische Wiedergabe ein-/auszuschalten\r\n"},
        {L"help.text.view",
            L"## Doppelseitenansicht\r\n"
            L"  - Zeigt zwei Seiten nebeneinander an\r\n"
            L"  - Im Automatikmodus werden Hochformatbilder doppelseitig, Querformatbilder einzeln angezeigt\r\n"
            L"  - Drücken Sie 1 für Einzeln, 2 für Doppelt, 3 für Automatisch\r\n"
            L"  - Sie können das Cover (erste Seite) so einstellen, dass es immer einzeln angezeigt wird\r\n"
            L"  - Animierte GIF/WebP werden auf beiden Seiten gleichzeitig abgespielt\r\n"
            L"\r\n"
            L"## Zoom (Vergrößern / Verkleinern)\r\n"
            L"  - Halten Sie Strg und verwenden Sie das Mausrad für stufenloses Zoomen\r\n"
            L"  - Verwenden Sie die Symbolleisten-Schaltflächen für \"An Fenster anpassen\", \"An Breite anpassen\" usw.\r\n"
            L"  - Drücken Sie Strg+0, um zur Originalgröße zurückzukehren\r\n"
            L"\r\n"
            L"## Drehung\r\n"
            L"  - Drehen Sie Bilder mit der Symbolleisten-Schaltfläche oder Strg+R\r\n"
            L"\r\n"
            L"## Vollbild\r\n"
            L"  - Drücken Sie F11, um den Vollbildmodus umzuschalten\r\n"
            L"  - Drücken Sie Esc, um zur normalen Ansicht zurückzukehren\r\n"},
        {L"help.text.tree",
            L"## Drei Modi\r\n"
            L"  - Normalmodus: Zeigt Favoriten, häufig verwendete Ordner und Laufwerke\r\n"
            L"  - Regalmodus: Zeigt nur die Archive und Ordner, die Sie registriert haben\r\n"
            L"    Sie können Kategorien erstellen, um sie zu organisieren\r\n"
            L"  - Verlaufsmodus: Listet zuvor geöffnete Ordner und Archive auf, neueste zuerst\r\n"
            L"\r\n"
            L"## Favoriten\r\n"
            L"  - Klicken Sie mit der rechten Maustaste auf einen Ordner oder ein Archiv und wählen Sie \"Zu Favoriten hinzufügen\"\r\n"
            L"  - Favoriten erscheinen oben im Baum\r\n"
            L"\r\n"
            L"## Regal\r\n"
            L"  - Drücken Sie die Regal-Schaltfläche, um in den Regalmodus zu wechseln\r\n"
            L"  - Rechtsklick und \"Zum Regal hinzufügen\" wählen, um in einer Kategorie zu registrieren\r\n"
            L"  - Rechtsklick auf eine Kategorie zum Umbenennen oder Löschen\r\n"
            L"  - Rechtsklick auf einen Ordner und \"Ordner als Regal hinzufügen\" wählen,\r\n"
            L"    um alle Archive auf einmal zu registrieren\r\n"
            L"\r\n"
            L"## Sortierung\r\n"
            L"  - Verwenden Sie die Sortierschaltfläche, um nach Name, Datum, Größe oder Typ zu sortieren\r\n"
            L"  - Die aktuelle Sortierreihenfolge wird auf der Schaltfläche angezeigt\r\n"},
        {L"help.text.file",
            L"## Dateiliste\r\n"
            L"  - Verwenden Sie die oberen Schaltflächen, um zwischen Listen- und Rasteransicht (Miniaturbilder) zu wechseln\r\n"
            L"  - In der Listenansicht können Sie Spaltenüberschriften ziehen, um die Sortierreihenfolge zu ändern\r\n"
            L"  - Spaltenbreite und -reihenfolge werden beim nächsten Start beibehalten\r\n"
            L"\r\n"
            L"## Umbenennen\r\n"
            L"  - Wählen Sie eine Datei oder einen Ordner und drücken Sie F2, um den Namen zu bearbeiten\r\n"
            L"  - Sie können auch \"Umbenennen\" im Kontextmenü verwenden\r\n"
            L"  - Drücken Sie Eingabe zum Bestätigen, Esc zum Abbrechen\r\n"
            L"\r\n"
            L"## Dateien löschen\r\n"
            L"  - Drücken Sie Entf oder verwenden Sie das Kontextmenü zum Löschen\r\n"
            L"  - Gelöschte Dateien werden in den Papierkorb verschoben und können wiederhergestellt werden\r\n"
            L"\r\n"
            L"## Dateien filtern\r\n"
            L"  - Geben Sie Text in das Eingabefeld unter der Dateiliste ein,\r\n"
            L"    um nur übereinstimmende Dateien anzuzeigen\r\n"
            L"\r\n"
            L"## Kontextmenü\r\n"
            L"  - Klicken Sie mit der rechten Maustaste auf eine Datei für verschiedene Aktionen\r\n"
            L"  - Mit Standard-App öffnen: In einer anderen Anwendung öffnen\r\n"
            L"  - Im Explorer anzeigen: Den Speicherort der Datei öffnen\r\n"
            L"  - Enthält auch Pfad kopieren, Zu Favoriten/Regal hinzufügen,\r\n"
            L"    Umbenennen, Löschen und Eigenschaften\r\n"},
        {L"help.text.settings",
            L"## Sprache ändern\r\n"
            L"  - Unterstützt 12 Sprachen\r\n"
            L"  - Änderungen werden sofort im Einstellungsbildschirm wirksam\r\n"
            L"\r\n"
            L"## Tastenkombinationen ändern\r\n"
            L"  - Passen Sie alle Tasten im Tab \"Tastenkombinationen\" der Einstellungen an\r\n"
            L"  - Sie können einer Aktion mehrere Tasten zuweisen\r\n"
            L"\r\n"
            L"## Aussehen ändern\r\n"
            L"  - Passen Sie Schriftgröße, Miniaturbildgröße und Vorschaugröße in den Einstellungen an\r\n"
            L"  - Änderungen werden sofort übernommen\r\n"
            L"\r\n"
            L"## Weitere Einstellungen\r\n"
            L"  - Cache-Größe: Speichermenge für angesehene Bilder anpassen\r\n"
            L"  - An Enden wiederholen: Nach der letzten Seite zur ersten zurückkehren\r\n"
            L"  - Cover einzeln: Erste Seite im Doppelseitenmodus einzeln anzeigen\r\n"
            L"  - Unterordner anzeigen: Bilder aus Unterordnern ebenfalls anzeigen\r\n"
            L"  - Automatische Wiedergabe: Wiedergabe startet automatisch bei Auswahl eines Videos\r\n"},
    };
}

static void LoadRu()
{
    g_strings = {
        {L"tree.desktop",    L"Рабочий стол"},
        {L"tree.downloads",  L"Загрузки"},
        {L"tree.documents",  L"Документы"},
        {L"tree.pictures",   L"Изображения"},
        {L"tree.videos",     L"Видео"},
        {L"tree.music",      L"Музыка"},
        {L"tree.localdisk",  L"Локальный диск"},
        {L"list.name",       L"Имя"},
        {L"list.size",       L"Размер"},
        {L"list.type",       L"Тип"},
        {L"list.date",       L"Дата изменения"},
        {L"list.filter",     L"Фильтр"},
        {L"type.folder",     L"Папка"},
        {L"type.file",       L"Файл"},
        {L"type.jpeg",       L"Изображение JPEG"},
        {L"type.png",        L"Изображение PNG"},
        {L"type.gif",        L"Изображение GIF"},
        {L"type.bmp",        L"Изображение BMP"},
        {L"type.webp",       L"Изображение WebP"},
        {L"type.avif",       L"Изображение AVIF"},
        {L"type.tiff",       L"Изображение TIFF"},
        {L"type.ico",        L"Значок"},
        {L"type.zip",        L"Архив ZIP"},
        {L"type.7z",         L"Архив 7z"},
        {L"type.rar",        L"Архив RAR"},
        {L"ctx.fullscreen",      L"Полный экран (F11)"},
        {L"ctx.copyimage",       L"Копировать изображение (Ctrl+C)"},
        {L"ctx.copypath",        L"Копировать путь"},
        {L"ctx.copyparentpath",  L"Копировать путь родительской папки"},
        {L"ctx.openassoc",       L"Открыть приложением по умолчанию"},
        {L"ctx.explorer",        L"Показать в Проводнике"},
        {L"ctx.addfav",          L"Добавить в избранное"},
        {L"ctx.removefav",       L"Удалить из избранного"},
        {L"ctx.rename",          L"Переименовать"},
        {L"ctx.delete",          L"Удалить"},
        {L"ctx.properties",      L"Свойства"},
        {L"dlg.confirm",    L"Подтверждение"},
        {L"dlg.delete",     L"Удалить?"},
        {L"dlg.settings",   L"Настройки"},
        {L"status.cantplay",     L"Воспроизведение невозможно (поместите mpv-2.dll)"},
        {L"status.cantopen",     L"Не удаётся открыть архив (7z.dll не найден)"},
        {L"status.loading",      L"Загрузка..."},
        {L"nav.history",    L"История"},
        {L"nav.favorites",  L"Избранное"},
        {L"ui.folder",      L"Папка"},
        {L"ui.address",     L" Адрес(A)"},
        {L"ui.bookshelf",   L"Книжная полка"},
        {L"ui.history",     L"История"},
        {L"ui.settings",    L"Настройки"},
        {L"ui.help",        L"Справка"},
        {L"nav.back",       L"Назад"},
        {L"nav.forward",    L"Вперёд"},
        {L"nav.up",         L"Вверх"},
        {L"nav.refresh",    L"Обновить"},
        {L"nav.list",       L"Список"},
        {L"nav.grid",       L"Сетка"},
        {L"nav.hover",      L"Предпросмотр"},
        {L"kb.fit_window",  L"По размеру окна"},
        {L"kb.fit_width",   L"По ширине"},
        {L"kb.fit_height",  L"По высоте"},
        {L"kb.original",    L"Исходный размер"},
        {L"kb.zoom_in",     L"Увеличить"},
        {L"kb.zoom_out",    L"Уменьшить"},
        {L"kb.view_auto",   L"Авто"},
        {L"kb.view_single", L"Одна страница"},
        {L"kb.view_spread", L"Разворот"},
        {L"kb.binding",     L"Направление переплёта"},
        {L"kb.rotate",      L"Повернуть"},
        {L"ctx.addshelf",       L"Добавить на книжную полку"},
        {L"ctx.removeshelf",    L"Убрать с книжной полки"},
        {L"ctx.newshelf",       L"Создать новую полку..."},
        {L"ctx.shelfascat",     L"Добавить папку как полку"},
        {L"ctx.shelfname",      L"Создать новую полку"},
        {L"ctx.shelfprompt",    L"Введите название полки:"},
        {L"ctx.deleteshelf",    L"Удалить эту полку"},
        {L"sort.asc",           L"По возрастанию"},
        {L"sort.desc",          L"По убыванию"},
        {L"ui.clearall",        L"Удалить всё"},
        {L"ui.renamethis",      L"Переименовать"},
        {L"ui.filter",          L"Фильтр..."},
        {L"drive.network",      L"Сетевой диск"},
        {L"drive.removable",    L"Съёмный диск"},
        {L"drive.cdrom",        L"CD/DVD-дисковод"},
        {L"drive.local",        L"Локальный диск"},
        {L"media.loop",         L"Зацикливание"},
        {L"media.autoplay",     L"Автовоспроизведение"},
        {L"dlg.bookshelfclear", L"Удалить все записи книжной полки?"},
        {L"dlg.historyclear",   L"Очистить всю историю?"},
        {L"group.today",        L"Сегодня"},
        {L"group.yesterday",    L"Вчера"},
        {L"group.thisweek",     L"На этой неделе"},
        {L"group.lastweek",     L"На прошлой неделе"},
        {L"group.older",        L"Ранее"},
        // Диалог настроек
        {L"settings.tab.general",   L"Общие"},
        {L"settings.tab.shortcuts", L"Горячие клавиши"},
        {L"settings.nav",           L"Навигация"},
        {L"settings.wrap",          L"Зациклить на краях"},
        {L"settings.spreadfirst",   L"Первая страница отдельно"},
        {L"settings.fileload",      L"Загрузка файлов"},
        {L"settings.recursive",     L"Показать подпапки"},
        {L"settings.media",         L"Медиа"},
        {L"settings.autoplay",      L"Автовоспроизведение"},
        {L"settings.perf",          L"Производительность"},
        {L"settings.cache",         L"Лимит кэша"},
        {L"settings.display",       L"Отображение"},
        {L"settings.threshold",     L"Порог двойной страницы"},
        {L"settings.thumbsize",     L"Размер миниатюр"},
        {L"settings.previewsize",   L"Размер предпросмотра"},
        {L"settings.fontsize",      L"Размер шрифта"},
        {L"settings.action",        L"Действие"},
        {L"settings.key",           L"Клавиша"},
        {L"settings.addkey",        L"Добавить клавишу..."},
        {L"settings.removekey",     L"Удалить клавишу"},
        {L"settings.resetkeys",     L"По умолчанию"},
        {L"settings.resetall",      L"По умолчанию"},
        {L"settings.ok",            L"OK"},
        {L"settings.cancel",        L"Отмена"},
        {L"settings.resetconfirm",  L"Сбросить все настройки?"},
        {L"settings.keycapture",    L"Ввод клавиши"},
        {L"settings.keyprompt",     L"Нажмите новую клавишу..."},
        // Диалог справки
        {L"help.title",         L"Справка - Как пользоваться karikari"},
        {L"help.tab.basic",     L"Основы"},
        {L"help.tab.view",      L"Режим отображения"},
        {L"help.tab.tree",      L"Дерево"},
        {L"help.tab.file",      L"Работа с файлами"},
        {L"help.tab.settings",  L"Настройки"},
        {L"help.tab.about",     L"О программе"},
        {L"help.version",       L"Версия 1.0.0"},
        {L"help.subtitle",      L"Самая быстрая программа просмотра изображений и видео"},
        {L"help.authorlabel",   L"Автор:"},
        {L"help.authorname",    L"megamega39"},
        {L"help.desclabel",     L"Описание:"},
        {L"help.desc1",         L"Высокопроизводительный просмотрщик изображений/видео на C++ + Win32 API + Direct2D."},
        {L"help.desc2",         L"Нативный порт leeyez_kai (версия .NET)."},
        {L"help.close",         L"Закрыть"},
        {L"help.text.basic",
            L"## Просмотр изображений\r\n"
            L"  - Выберите папку или архив в дереве или списке файлов слева\r\n"
            L"  - Используйте клавиши ← → или колесо мыши для перелистывания\r\n"
            L"  - Нажмите Home / End для перехода к первой или последней странице\r\n"
            L"\r\n"
            L"## Открытие архивов\r\n"
            L"  - Поддерживаются архивы ZIP, 7z, RAR, CBZ, CBR и CB7\r\n"
            L"  - Можно просматривать папки внутри архивов\r\n"
            L"  - Используйте клавиши ↑ ↓ для перехода к предыдущему или следующему архиву в той же папке\r\n"
            L"\r\n"
            L"## Воспроизведение видео и аудио\r\n"
            L"  - Видео и аудио файлы можно воспроизводить напрямую\r\n"
            L"  - Нажмите Пробел для переключения воспроизведения/паузы\r\n"
            L"  - Используйте нижнюю панель для перемотки, повтора и регулировки громкости\r\n"
            L"  - Нажмите кнопку «A» на панели воспроизведения для включения/выключения автовоспроизведения\r\n"},
        {L"help.text.view",
            L"## Двухстраничный режим\r\n"
            L"  - Отображение двух страниц рядом\r\n"
            L"  - В автоматическом режиме портретные изображения показываются парой, альбомные — по одному\r\n"
            L"  - Нажмите 1 для одиночного, 2 для двойного, 3 для автоматического режима\r\n"
            L"  - Можно настроить обложку (первую страницу) для отображения всегда по одной\r\n"
            L"  - Анимированные GIF/WebP воспроизводятся одновременно на обеих страницах\r\n"
            L"\r\n"
            L"## Масштабирование\r\n"
            L"  - Удерживайте Ctrl и используйте колесо мыши для плавного масштабирования\r\n"
            L"  - Используйте кнопки панели инструментов для «Подогнать под окно», «Подогнать по ширине» и т.д.\r\n"
            L"  - Нажмите Ctrl+0 для возврата к исходному размеру\r\n"
            L"\r\n"
            L"## Поворот\r\n"
            L"  - Поворачивайте изображения кнопкой панели инструментов или Ctrl+R\r\n"
            L"\r\n"
            L"## Полноэкранный режим\r\n"
            L"  - Нажмите F11 для переключения полноэкранного режима\r\n"
            L"  - Нажмите Esc для возврата к обычному виду\r\n"},
        {L"help.text.tree",
            L"## Три режима\r\n"
            L"  - Обычный режим: Показывает избранное, часто используемые папки и диски\r\n"
            L"  - Режим полки: Показывает только зарегистрированные архивы и папки\r\n"
            L"    Можно создавать категории для организации\r\n"
            L"  - Режим истории: Список ранее открытых папок и архивов, сначала новейшие\r\n"
            L"\r\n"
            L"## Избранное\r\n"
            L"  - Щёлкните правой кнопкой мыши по папке или архиву и выберите «Добавить в избранное»\r\n"
            L"  - Избранное отображается вверху дерева\r\n"
            L"\r\n"
            L"## Полка\r\n"
            L"  - Нажмите кнопку Полка для переключения в режим полки\r\n"
            L"  - Щёлкните правой кнопкой и выберите «Добавить на полку» для регистрации в категории\r\n"
            L"  - Щёлкните правой кнопкой по категории для переименования или удаления\r\n"
            L"  - Щёлкните правой кнопкой по папке и выберите «Добавить папку как полку»,\r\n"
            L"    чтобы зарегистрировать все архивы сразу\r\n"
            L"\r\n"
            L"## Сортировка\r\n"
            L"  - Используйте кнопку сортировки для сортировки по имени, дате, размеру или типу\r\n"
            L"  - Текущий порядок сортировки отображается на кнопке\r\n"},
        {L"help.text.file",
            L"## Список файлов\r\n"
            L"  - Используйте верхние кнопки для переключения между списком и сеткой (миниатюры)\r\n"
            L"  - В режиме списка перетаскивайте заголовки столбцов для изменения порядка сортировки\r\n"
            L"  - Ширина и порядок столбцов сохраняются при следующем запуске\r\n"
            L"\r\n"
            L"## Переименование\r\n"
            L"  - Выберите файл или папку и нажмите F2 для редактирования имени\r\n"
            L"  - Также можно использовать «Переименовать» из контекстного меню\r\n"
            L"  - Нажмите Enter для подтверждения, Esc для отмены\r\n"
            L"\r\n"
            L"## Удаление файлов\r\n"
            L"  - Нажмите Delete или используйте контекстное меню для удаления\r\n"
            L"  - Удалённые файлы перемещаются в Корзину и могут быть восстановлены\r\n"
            L"\r\n"
            L"## Фильтрация файлов\r\n"
            L"  - Введите текст в поле ввода под списком файлов,\r\n"
            L"    чтобы показать только совпадающие файлы\r\n"
            L"\r\n"
            L"## Контекстное меню\r\n"
            L"  - Щёлкните правой кнопкой по файлу для различных действий\r\n"
            L"  - Открыть в приложении по умолчанию: Открыть в другом приложении\r\n"
            L"  - Показать в Проводнике: Открыть расположение файла\r\n"
            L"  - Также включает Копировать путь, Добавить в избранное/на полку,\r\n"
            L"    Переименовать, Удалить и Свойства\r\n"},
        {L"help.text.settings",
            L"## Смена языка\r\n"
            L"  - Поддерживается 12 языков\r\n"
            L"  - Изменения вступают в силу немедленно на экране настроек\r\n"
            L"\r\n"
            L"## Изменение сочетаний клавиш\r\n"
            L"  - Настройте все клавиши на вкладке «Сочетания клавиш» в настройках\r\n"
            L"  - Можно назначить несколько клавиш одному действию\r\n"
            L"\r\n"
            L"## Изменение внешнего вида\r\n"
            L"  - Настройте размер шрифта, миниатюр и предпросмотра в настройках\r\n"
            L"  - Изменения применяются мгновенно\r\n"
            L"\r\n"
            L"## Другие настройки\r\n"
            L"  - Размер кэша: Настройте объём памяти для хранения просмотренных изображений\r\n"
            L"  - Зацикливание: При переходе за последнюю страницу возврат к первой\r\n"
            L"  - Обложка отдельно: Первая страница отображается отдельно в двухстраничном режиме\r\n"
            L"  - Показать подпапки: Отображать изображения из подпапок\r\n"
            L"  - Автовоспроизведение: Автоматический запуск воспроизведения при выборе видео\r\n"},
    };
}

static void LoadVi()
{
    g_strings = {
        {L"tree.desktop",    L"Màn hình nền"},
        {L"tree.downloads",  L"Tải xuống"},
        {L"tree.documents",  L"Tài liệu"},
        {L"tree.pictures",   L"Hình ảnh"},
        {L"tree.videos",     L"Video"},
        {L"tree.music",      L"Nhạc"},
        {L"tree.localdisk",  L"Ổ đĩa cục bộ"},
        {L"list.name",       L"Tên"},
        {L"list.size",       L"Kích thước"},
        {L"list.type",       L"Loại"},
        {L"list.date",       L"Ngày sửa đổi"},
        {L"list.filter",     L"Bộ lọc"},
        {L"type.folder",     L"Thư mục"},
        {L"type.file",       L"Tệp"},
        {L"type.jpeg",       L"Ảnh JPEG"},
        {L"type.png",        L"Ảnh PNG"},
        {L"type.gif",        L"Ảnh GIF"},
        {L"type.bmp",        L"Ảnh BMP"},
        {L"type.webp",       L"Ảnh WebP"},
        {L"type.avif",       L"Ảnh AVIF"},
        {L"type.tiff",       L"Ảnh TIFF"},
        {L"type.ico",        L"Biểu tượng"},
        {L"type.zip",        L"Tệp nén ZIP"},
        {L"type.7z",         L"Tệp nén 7z"},
        {L"type.rar",        L"Tệp nén RAR"},
        {L"ctx.fullscreen",      L"Toàn màn hình (F11)"},
        {L"ctx.copyimage",       L"Sao chép ảnh (Ctrl+C)"},
        {L"ctx.copypath",        L"Sao chép đường dẫn"},
        {L"ctx.copyparentpath",  L"Sao chép đường dẫn thư mục cha"},
        {L"ctx.openassoc",       L"Mở bằng ứng dụng mặc định"},
        {L"ctx.explorer",        L"Hiển thị trong Explorer"},
        {L"ctx.addfav",          L"Thêm vào yêu thích"},
        {L"ctx.removefav",       L"Xóa khỏi yêu thích"},
        {L"ctx.rename",          L"Đổi tên"},
        {L"ctx.delete",          L"Xóa"},
        {L"ctx.properties",      L"Thuộc tính"},
        {L"dlg.confirm",    L"Xác nhận"},
        {L"dlg.delete",     L"Xóa?"},
        {L"dlg.settings",   L"Cài đặt"},
        {L"status.cantplay",     L"Không thể phát (hãy đặt mpv-2.dll)"},
        {L"status.cantopen",     L"Không thể mở tệp nén (không tìm thấy 7z.dll)"},
        {L"status.loading",      L"Đang tải..."},
        {L"nav.history",    L"Lịch sử"},
        {L"nav.favorites",  L"Yêu thích"},
        {L"ui.folder",      L"Thư mục"},
        {L"ui.address",     L" Địa chỉ(A)"},
        {L"ui.bookshelf",   L"Tủ sách"},
        {L"ui.history",     L"Lịch sử"},
        {L"ui.settings",    L"Cài đặt"},
        {L"ui.help",        L"Trợ giúp"},
        {L"nav.back",       L"Quay lại"},
        {L"nav.forward",    L"Tiến"},
        {L"nav.up",         L"Lên"},
        {L"nav.refresh",    L"Làm mới"},
        {L"nav.list",       L"Danh sách"},
        {L"nav.grid",       L"Lưới"},
        {L"nav.hover",      L"Xem trước"},
        {L"kb.fit_window",  L"Vừa cửa sổ"},
        {L"kb.fit_width",   L"Vừa chiều rộng"},
        {L"kb.fit_height",  L"Vừa chiều cao"},
        {L"kb.original",    L"Kích thước gốc"},
        {L"kb.zoom_in",     L"Phóng to"},
        {L"kb.zoom_out",    L"Thu nhỏ"},
        {L"kb.view_auto",   L"Tự động"},
        {L"kb.view_single", L"Trang đơn"},
        {L"kb.view_spread", L"Trang đôi"},
        {L"kb.binding",     L"Hướng đóng gáy"},
        {L"kb.rotate",      L"Xoay"},
        {L"ctx.addshelf",       L"Thêm vào tủ sách"},
        {L"ctx.removeshelf",    L"Xóa khỏi tủ sách"},
        {L"ctx.newshelf",       L"Tạo tủ sách mới..."},
        {L"ctx.shelfascat",     L"Thêm thư mục làm tủ sách"},
        {L"ctx.shelfname",      L"Tạo tủ sách mới"},
        {L"ctx.shelfprompt",    L"Nhập tên tủ sách:"},
        {L"ctx.deleteshelf",    L"Xóa tủ sách này"},
        {L"sort.asc",           L"Tăng dần"},
        {L"sort.desc",          L"Giảm dần"},
        {L"ui.clearall",        L"Xóa tất cả"},
        {L"ui.renamethis",      L"Đổi tên"},
        {L"ui.filter",          L"Bộ lọc..."},
        {L"drive.network",      L"Ổ đĩa mạng"},
        {L"drive.removable",    L"Đĩa di động"},
        {L"drive.cdrom",        L"Ổ đĩa CD/DVD"},
        {L"drive.local",        L"Ổ đĩa cục bộ"},
        {L"media.loop",         L"Lặp lại"},
        {L"media.autoplay",     L"Tự động phát"},
        {L"dlg.bookshelfclear", L"Xóa tất cả mục trong tủ sách?"},
        {L"dlg.historyclear",   L"Xóa toàn bộ lịch sử?"},
        {L"group.today",        L"Hôm nay"},
        {L"group.yesterday",    L"Hôm qua"},
        {L"group.thisweek",     L"Tuần này"},
        {L"group.lastweek",     L"Tuần trước"},
        {L"group.older",        L"Trước đó"},
        // Hộp thoại cài đặt
        {L"settings.tab.general",   L"Chung"},
        {L"settings.tab.shortcuts", L"Phím tắt"},
        {L"settings.nav",           L"Điều hướng"},
        {L"settings.wrap",          L"Lặp ở cuối"},
        {L"settings.spreadfirst",   L"Trang đầu đơn trong chế độ đôi"},
        {L"settings.fileload",      L"Tải tệp"},
        {L"settings.recursive",     L"Hiển thị ảnh thư mục con"},
        {L"settings.media",         L"Phương tiện"},
        {L"settings.autoplay",      L"Tự động phát"},
        {L"settings.perf",          L"Hiệu suất"},
        {L"settings.cache",         L"Giới hạn bộ nhớ đệm"},
        {L"settings.display",       L"Hiển thị"},
        {L"settings.threshold",     L"Ngưỡng trang đôi"},
        {L"settings.thumbsize",     L"Kích thước ảnh nhỏ"},
        {L"settings.previewsize",   L"Kích thước xem trước"},
        {L"settings.fontsize",      L"Cỡ chữ"},
        {L"settings.action",        L"Hành động"},
        {L"settings.key",           L"Phím"},
        {L"settings.addkey",        L"Thêm phím..."},
        {L"settings.removekey",     L"Xóa phím"},
        {L"settings.resetkeys",     L"Mặc định"},
        {L"settings.resetall",      L"Mặc định"},
        {L"settings.ok",            L"OK"},
        {L"settings.cancel",        L"Hủy"},
        {L"settings.resetconfirm",  L"Đặt lại tất cả?"},
        {L"settings.keycapture",    L"Nhập phím"},
        {L"settings.keyprompt",     L"Nhấn phím mới..."},
        // Hộp thoại trợ giúp
        {L"help.title",         L"Trợ giúp - Cách sử dụng karikari"},
        {L"help.tab.basic",     L"Cơ bản"},
        {L"help.tab.view",      L"Chế độ xem"},
        {L"help.tab.tree",      L"Cây thư mục"},
        {L"help.tab.file",      L"Thao tác tệp"},
        {L"help.tab.settings",  L"Cài đặt"},
        {L"help.tab.about",     L"Giới thiệu"},
        {L"help.version",       L"Phiên bản 1.0.0"},
        {L"help.subtitle",      L"Trình xem ảnh và video nhanh nhất"},
        {L"help.authorlabel",   L"Tác giả:"},
        {L"help.authorname",    L"megamega39"},
        {L"help.desclabel",     L"Mô tả:"},
        {L"help.desc1",         L"Trình xem ảnh/video hiệu suất cao được xây dựng bằng C++ + Win32 API + Direct2D."},
        {L"help.desc2",         L"Phiên bản gốc của leeyez_kai (phiên bản .NET)."},
        {L"help.close",         L"Đóng"},
        {L"help.text.basic",
            L"## Xem ảnh\r\n"
            L"  - Chọn thư mục hoặc tệp nén từ cây hoặc danh sách tệp bên trái\r\n"
            L"  - Sử dụng phím ← → hoặc con lăn chuột để lật trang\r\n"
            L"  - Nhấn Home / End để đến trang đầu hoặc trang cuối\r\n"
            L"\r\n"
            L"## Mở tệp nén\r\n"
            L"  - Hỗ trợ tệp ZIP, 7z, RAR, CBZ, CBR và CB7\r\n"
            L"  - Có thể duyệt thư mục bên trong tệp nén\r\n"
            L"  - Sử dụng phím ↑ ↓ để chuyển đến tệp nén trước hoặc sau trong cùng thư mục\r\n"
            L"\r\n"
            L"## Phát video và âm thanh\r\n"
            L"  - Có thể phát trực tiếp tệp video và âm thanh\r\n"
            L"  - Nhấn Space để chuyển đổi phát/tạm dừng\r\n"
            L"  - Sử dụng thanh bên dưới để tua, lặp và điều chỉnh âm lượng\r\n"
            L"  - Nhấn nút \"A\" trên thanh phát để bật/tắt tự động phát\r\n"},
        {L"help.text.view",
            L"## Xem trang đôi\r\n"
            L"  - Hiển thị hai trang cạnh nhau\r\n"
            L"  - Ở chế độ tự động, ảnh dọc hiển thị trang đôi, ảnh ngang hiển thị đơn\r\n"
            L"  - Nhấn 1 cho Đơn, 2 cho Đôi, 3 cho Tự động\r\n"
            L"  - Có thể cài đặt trang bìa (trang đầu) luôn hiển thị đơn\r\n"
            L"  - GIF/WebP động phát đồng thời trên cả hai trang\r\n"
            L"\r\n"
            L"## Thu phóng\r\n"
            L"  - Giữ Ctrl và cuộn con lăn chuột để thu phóng mượt mà\r\n"
            L"  - Sử dụng nút thanh công cụ để chọn \"Vừa cửa sổ\", \"Vừa chiều rộng\", v.v.\r\n"
            L"  - Nhấn Ctrl+0 để trở về kích thước gốc\r\n"
            L"\r\n"
            L"## Xoay\r\n"
            L"  - Xoay ảnh bằng nút thanh công cụ hoặc Ctrl+R\r\n"
            L"\r\n"
            L"## Toàn màn hình\r\n"
            L"  - Nhấn F11 để chuyển đổi toàn màn hình\r\n"
            L"  - Nhấn Esc để quay lại chế độ bình thường\r\n"},
        {L"help.text.tree",
            L"## Ba chế độ\r\n"
            L"  - Chế độ bình thường: Hiển thị mục yêu thích, thư mục thường dùng và ổ đĩa\r\n"
            L"  - Chế độ kệ sách: Chỉ hiển thị tệp nén và thư mục bạn đã đăng ký\r\n"
            L"    Có thể tạo danh mục để sắp xếp\r\n"
            L"  - Chế độ lịch sử: Liệt kê thư mục và tệp nén đã mở, mới nhất trước\r\n"
            L"\r\n"
            L"## Mục yêu thích\r\n"
            L"  - Nhấp chuột phải vào thư mục hoặc tệp nén và chọn \"Thêm vào yêu thích\"\r\n"
            L"  - Mục yêu thích hiển thị ở đầu cây\r\n"
            L"\r\n"
            L"## Kệ sách\r\n"
            L"  - Nhấn nút Kệ sách để chuyển sang chế độ kệ sách\r\n"
            L"  - Nhấp chuột phải và chọn \"Thêm vào kệ sách\" để đăng ký vào danh mục\r\n"
            L"  - Nhấp chuột phải vào danh mục để đổi tên hoặc xóa\r\n"
            L"  - Nhấp chuột phải vào thư mục và chọn \"Thêm thư mục làm kệ sách\"\r\n"
            L"    để đăng ký tất cả tệp nén cùng lúc\r\n"
            L"\r\n"
            L"## Sắp xếp\r\n"
            L"  - Sử dụng nút sắp xếp để sắp theo tên, ngày, kích thước hoặc loại\r\n"
            L"  - Thứ tự sắp xếp hiện tại được hiển thị trên nút\r\n"},
        {L"help.text.file",
            L"## Danh sách tệp\r\n"
            L"  - Sử dụng nút phía trên để chuyển giữa dạng danh sách và lưới (hình thu nhỏ)\r\n"
            L"  - Trong dạng danh sách, kéo tiêu đề cột để thay đổi thứ tự sắp xếp\r\n"
            L"  - Chiều rộng và thứ tự cột được giữ lại cho lần khởi động sau\r\n"
            L"\r\n"
            L"## Đổi tên\r\n"
            L"  - Chọn tệp hoặc thư mục và nhấn F2 để sửa tên\r\n"
            L"  - Cũng có thể dùng \"Đổi tên\" từ menu chuột phải\r\n"
            L"  - Nhấn Enter để xác nhận, Esc để hủy\r\n"
            L"\r\n"
            L"## Xóa tệp\r\n"
            L"  - Nhấn Delete hoặc dùng menu chuột phải để xóa\r\n"
            L"  - Tệp đã xóa được chuyển vào Thùng rác và có thể khôi phục\r\n"
            L"\r\n"
            L"## Lọc tệp\r\n"
            L"  - Nhập văn bản vào ô nhập bên dưới danh sách tệp\r\n"
            L"    để chỉ hiển thị tệp phù hợp\r\n"
            L"\r\n"
            L"## Menu chuột phải\r\n"
            L"  - Nhấp chuột phải vào tệp cho nhiều thao tác\r\n"
            L"  - Mở bằng ứng dụng mặc định: Mở trong ứng dụng khác\r\n"
            L"  - Hiển thị trong Explorer: Mở vị trí tệp\r\n"
            L"  - Còn bao gồm Sao chép đường dẫn, Thêm vào yêu thích/kệ sách,\r\n"
            L"    Đổi tên, Xóa và Thuộc tính\r\n"},
        {L"help.text.settings",
            L"## Đổi ngôn ngữ\r\n"
            L"  - Hỗ trợ 12 ngôn ngữ\r\n"
            L"  - Thay đổi có hiệu lực ngay lập tức trong màn hình cài đặt\r\n"
            L"\r\n"
            L"## Đổi phím tắt\r\n"
            L"  - Tùy chỉnh tất cả phím trong tab \"Phím tắt\" của cài đặt\r\n"
            L"  - Có thể gán nhiều phím cho một thao tác\r\n"
            L"\r\n"
            L"## Đổi giao diện\r\n"
            L"  - Điều chỉnh cỡ chữ, kích thước hình thu nhỏ và xem trước trong cài đặt\r\n"
            L"  - Thay đổi được áp dụng ngay lập tức\r\n"
            L"\r\n"
            L"## Cài đặt khác\r\n"
            L"  - Kích thước bộ nhớ đệm: Điều chỉnh bộ nhớ lưu trữ ảnh đã xem\r\n"
            L"  - Lặp ở cuối: Qua trang cuối sẽ quay lại trang đầu\r\n"
            L"  - Bìa trang đôi: Hiển thị trang đầu đơn trong chế độ trang đôi\r\n"
            L"  - Hiển thị thư mục con: Hiển thị ảnh từ thư mục con\r\n"
            L"  - Tự động phát: Tự động bắt đầu phát khi chọn video\r\n"},
    };
}

static void LoadTh()
{
    g_strings = {
        {L"tree.desktop",    L"เดสก์ท็อป"},
        {L"tree.downloads",  L"ดาวน์โหลด"},
        {L"tree.documents",  L"เอกสาร"},
        {L"tree.pictures",   L"รูปภาพ"},
        {L"tree.videos",     L"วิดีโอ"},
        {L"tree.music",      L"เพลง"},
        {L"tree.localdisk",  L"ดิสก์ภายใน"},
        {L"list.name",       L"ชื่อ"},
        {L"list.size",       L"ขนาด"},
        {L"list.type",       L"ประเภท"},
        {L"list.date",       L"วันที่แก้ไข"},
        {L"list.filter",     L"ตัวกรอง"},
        {L"type.folder",     L"โฟลเดอร์"},
        {L"type.file",       L"ไฟล์"},
        {L"type.jpeg",       L"ภาพ JPEG"},
        {L"type.png",        L"ภาพ PNG"},
        {L"type.gif",        L"ภาพ GIF"},
        {L"type.bmp",        L"ภาพ BMP"},
        {L"type.webp",       L"ภาพ WebP"},
        {L"type.avif",       L"ภาพ AVIF"},
        {L"type.tiff",       L"ภาพ TIFF"},
        {L"type.ico",        L"ไอคอน"},
        {L"type.zip",        L"ไฟล์ ZIP"},
        {L"type.7z",         L"ไฟล์ 7z"},
        {L"type.rar",        L"ไฟล์ RAR"},
        {L"ctx.fullscreen",      L"เต็มหน้าจอ (F11)"},
        {L"ctx.copyimage",       L"คัดลอกภาพ (Ctrl+C)"},
        {L"ctx.copypath",        L"คัดลอกเส้นทาง"},
        {L"ctx.copyparentpath",  L"คัดลอกเส้นทางโฟลเดอร์หลัก"},
        {L"ctx.openassoc",       L"เปิดด้วยแอปเริ่มต้น"},
        {L"ctx.explorer",        L"แสดงใน Explorer"},
        {L"ctx.addfav",          L"เพิ่มในรายการโปรด"},
        {L"ctx.removefav",       L"ลบจากรายการโปรด"},
        {L"ctx.rename",          L"เปลี่ยนชื่อ"},
        {L"ctx.delete",          L"ลบ"},
        {L"ctx.properties",      L"คุณสมบัติ"},
        {L"dlg.confirm",    L"ยืนยัน"},
        {L"dlg.delete",     L"ลบ?"},
        {L"dlg.settings",   L"การตั้งค่า"},
        {L"status.cantplay",     L"ไม่สามารถเล่นได้ (วาง mpv-2.dll)"},
        {L"status.cantopen",     L"ไม่สามารถเปิดไฟล์ได้ (ไม่พบ 7z.dll)"},
        {L"status.loading",      L"กำลังโหลด..."},
        {L"nav.history",    L"ประวัติ"},
        {L"nav.favorites",  L"รายการโปรด"},
        {L"ui.folder",      L"โฟลเดอร์"},
        {L"ui.address",     L" ที่อยู่(A)"},
        {L"ui.bookshelf",   L"ชั้นหนังสือ"},
        {L"ui.history",     L"ประวัติ"},
        {L"ui.settings",    L"การตั้งค่า"},
        {L"ui.help",        L"ช่วยเหลือ"},
        {L"nav.back",       L"ย้อนกลับ"},
        {L"nav.forward",    L"ไปข้างหน้า"},
        {L"nav.up",         L"ขึ้น"},
        {L"nav.refresh",    L"รีเฟรช"},
        {L"nav.list",       L"รายการ"},
        {L"nav.grid",       L"ตาราง"},
        {L"nav.hover",      L"แสดงตัวอย่าง"},
        {L"kb.fit_window",  L"พอดีหน้าต่าง"},
        {L"kb.fit_width",   L"พอดีความกว้าง"},
        {L"kb.fit_height",  L"พอดีความสูง"},
        {L"kb.original",    L"ขนาดจริง"},
        {L"kb.zoom_in",     L"ขยาย"},
        {L"kb.zoom_out",    L"ย่อ"},
        {L"kb.view_auto",   L"อัตโนมัติ"},
        {L"kb.view_single", L"หน้าเดียว"},
        {L"kb.view_spread", L"สองหน้า"},
        {L"kb.binding",     L"ทิศทางการเข้าเล่ม"},
        {L"kb.rotate",      L"หมุน"},
        {L"ctx.addshelf",       L"เพิ่มในชั้นหนังสือ"},
        {L"ctx.removeshelf",    L"ลบออกจากชั้นหนังสือ"},
        {L"ctx.newshelf",       L"สร้างชั้นหนังสือใหม่..."},
        {L"ctx.shelfascat",     L"เพิ่มโฟลเดอร์เป็นชั้นหนังสือ"},
        {L"ctx.shelfname",      L"สร้างชั้นหนังสือใหม่"},
        {L"ctx.shelfprompt",    L"กรอกชื่อชั้นหนังสือ:"},
        {L"ctx.deleteshelf",    L"ลบชั้นหนังสือนี้"},
        {L"sort.asc",           L"น้อยไปมาก"},
        {L"sort.desc",          L"มากไปน้อย"},
        {L"ui.clearall",        L"ลบทั้งหมด"},
        {L"ui.renamethis",      L"เปลี่ยนชื่อ"},
        {L"ui.filter",          L"ตัวกรอง..."},
        {L"drive.network",      L"ไดรฟ์เครือข่าย"},
        {L"drive.removable",    L"ดิสก์แบบถอดได้"},
        {L"drive.cdrom",        L"ไดรฟ์ CD/DVD"},
        {L"drive.local",        L"ดิสก์ภายใน"},
        {L"media.loop",         L"เล่นวนซ้ำ"},
        {L"media.autoplay",     L"เล่นอัตโนมัติ"},
        {L"dlg.bookshelfclear", L"ลบรายการชั้นหนังสือทั้งหมด?"},
        {L"dlg.historyclear",   L"ล้างประวัติทั้งหมด?"},
        {L"group.today",        L"วันนี้"},
        {L"group.yesterday",    L"เมื่อวาน"},
        {L"group.thisweek",     L"สัปดาห์นี้"},
        {L"group.lastweek",     L"สัปดาห์ที่แล้ว"},
        {L"group.older",        L"ก่อนหน้า"},
        // กล่องโต้ตอบการตั้งค่า
        {L"settings.tab.general",   L"ทั่วไป"},
        {L"settings.tab.shortcuts", L"แป้นพิมพ์ลัด"},
        {L"settings.nav",           L"การนำทาง"},
        {L"settings.wrap",          L"วนซ้ำที่ปลาย"},
        {L"settings.spreadfirst",   L"หน้าแรกเดี่ยวในโหมดคู่"},
        {L"settings.fileload",      L"โหลดไฟล์"},
        {L"settings.recursive",     L"แสดงภาพจากโฟลเดอร์ย่อย"},
        {L"settings.media",         L"สื่อ"},
        {L"settings.autoplay",      L"เล่นอัตโนมัติ"},
        {L"settings.perf",          L"ประสิทธิภาพ"},
        {L"settings.cache",         L"จำกัดแคช"},
        {L"settings.display",       L"การแสดงผล"},
        {L"settings.threshold",     L"เกณฑ์หน้าคู่"},
        {L"settings.thumbsize",     L"ขนาดภาพขนาดย่อ"},
        {L"settings.previewsize",   L"ขนาดตัวอย่าง"},
        {L"settings.fontsize",      L"ขนาดตัวอักษร"},
        {L"settings.action",        L"การดำเนินการ"},
        {L"settings.key",           L"ปุ่ม"},
        {L"settings.addkey",        L"เพิ่มปุ่ม..."},
        {L"settings.removekey",     L"ลบปุ่ม"},
        {L"settings.resetkeys",     L"ค่าเริ่มต้น"},
        {L"settings.resetall",      L"ค่าเริ่มต้น"},
        {L"settings.ok",            L"ตกลง"},
        {L"settings.cancel",        L"ยกเลิก"},
        {L"settings.resetconfirm",  L"รีเซ็ตการตั้งค่าทั้งหมด?"},
        {L"settings.keycapture",    L"ป้อนปุ่ม"},
        {L"settings.keyprompt",     L"กดปุ่มใหม่..."},
        // กล่องโต้ตอบช่วยเหลือ
        {L"help.title",         L"ช่วยเหลือ - วิธีใช้ karikari"},
        {L"help.tab.basic",     L"พื้นฐาน"},
        {L"help.tab.view",      L"โหมดมุมมอง"},
        {L"help.tab.tree",      L"มุมมองต้นไม้"},
        {L"help.tab.file",      L"จัดการไฟล์"},
        {L"help.tab.settings",  L"ตั้งค่า"},
        {L"help.tab.about",     L"เกี่ยวกับ"},
        {L"help.version",       L"เวอร์ชัน 1.0.0"},
        {L"help.subtitle",      L"โปรแกรมดูภาพและวิดีโอที่เร็วที่สุด"},
        {L"help.authorlabel",   L"ผู้พัฒนา:"},
        {L"help.authorname",    L"megamega39"},
        {L"help.desclabel",     L"คำอธิบาย:"},
        {L"help.desc1",         L"โปรแกรมดูภาพ/วิดีโอประสิทธิภาพสูงสร้างด้วย C++ + Win32 API + Direct2D"},
        {L"help.desc2",         L"พอร์ตเนทีฟจาก leeyez_kai (เวอร์ชัน .NET)"},
        {L"help.close",         L"ปิด"},
        {L"help.text.basic",
            L"## ดูภาพ\r\n"
            L"  - เลือกโฟลเดอร์หรือไฟล์บีบอัดจากต้นไม้หรือรายการไฟล์ทางซ้าย\r\n"
            L"  - ใช้ปุ่ม ← → หรือล้อเลื่อนเมาส์เพื่อเปลี่ยนหน้า\r\n"
            L"  - กด Home / End เพื่อไปหน้าแรกหรือหน้าสุดท้าย\r\n"
            L"\r\n"
            L"## เปิดไฟล์บีบอัด\r\n"
            L"  - รองรับไฟล์ ZIP, 7z, RAR, CBZ, CBR และ CB7\r\n"
            L"  - สามารถเรียกดูโฟลเดอร์ภายในไฟล์บีบอัดได้\r\n"
            L"  - ใช้ปุ่ม ↑ ↓ เพื่อไปยังไฟล์บีบอัดก่อนหน้าหรือถัดไปในโฟลเดอร์เดียวกัน\r\n"
            L"\r\n"
            L"## เล่นวิดีโอและเสียง\r\n"
            L"  - สามารถเล่นไฟล์วิดีโอและเสียงได้โดยตรง\r\n"
            L"  - กด Space เพื่อสลับเล่น/หยุดชั่วคราว\r\n"
            L"  - ใช้แถบด้านล่างเพื่อเลื่อน, วนซ้ำ และปรับระดับเสียง\r\n"
            L"  - กดปุ่ม \"A\" บนแถบเล่นเพื่อเปิด/ปิดเล่นอัตโนมัติ\r\n"},
        {L"help.text.view",
            L"## มุมมองสองหน้า\r\n"
            L"  - แสดงสองหน้าเคียงข้างกัน\r\n"
            L"  - ในโหมดอัตโนมัติ ภาพแนวตั้งแสดงเป็นสองหน้า ภาพแนวนอนแสดงเดี่ยว\r\n"
            L"  - กด 1 สำหรับเดี่ยว, 2 สำหรับคู่, 3 สำหรับอัตโนมัติ\r\n"
            L"  - สามารถตั้งค่าหน้าปก (หน้าแรก) ให้แสดงเดี่ยวเสมอได้\r\n"
            L"  - GIF/WebP แบบเคลื่อนไหวเล่นพร้อมกันทั้งสองหน้า\r\n"
            L"\r\n"
            L"## ซูม (ขยาย / ย่อ)\r\n"
            L"  - กด Ctrl ค้างแล้วเลื่อนล้อเมาส์เพื่อซูมอย่างราบรื่น\r\n"
            L"  - ใช้ปุ่มแถบเครื่องมือเพื่อเลือก \"พอดีหน้าต่าง\", \"พอดีความกว้าง\" ฯลฯ\r\n"
            L"  - กด Ctrl+0 เพื่อกลับเป็นขนาดเดิม\r\n"
            L"\r\n"
            L"## หมุน\r\n"
            L"  - หมุนภาพด้วยปุ่มแถบเครื่องมือหรือ Ctrl+R\r\n"
            L"\r\n"
            L"## เต็มจอ\r\n"
            L"  - กด F11 เพื่อสลับเต็มจอ\r\n"
            L"  - กด Esc เพื่อกลับสู่มุมมองปกติ\r\n"},
        {L"help.text.tree",
            L"## สามโหมด\r\n"
            L"  - โหมดปกติ: แสดงรายการโปรด, โฟลเดอร์ที่ใช้บ่อย และไดรฟ์\r\n"
            L"  - โหมดชั้นหนังสือ: แสดงเฉพาะไฟล์บีบอัดและโฟลเดอร์ที่คุณลงทะเบียน\r\n"
            L"    สามารถสร้างหมวดหมู่เพื่อจัดระเบียบได้\r\n"
            L"  - โหมดประวัติ: แสดงรายการโฟลเดอร์และไฟล์บีบอัดที่เปิดก่อนหน้า ล่าสุดก่อน\r\n"
            L"\r\n"
            L"## รายการโปรด\r\n"
            L"  - คลิกขวาที่โฟลเดอร์หรือไฟล์บีบอัดแล้วเลือก \"เพิ่มในรายการโปรด\"\r\n"
            L"  - รายการโปรดปรากฏที่ด้านบนของต้นไม้\r\n"
            L"\r\n"
            L"## ชั้นหนังสือ\r\n"
            L"  - กดปุ่มชั้นหนังสือเพื่อสลับไปโหมดชั้นหนังสือ\r\n"
            L"  - คลิกขวาแล้วเลือก \"เพิ่มในชั้นหนังสือ\" เพื่อลงทะเบียนในหมวดหมู่\r\n"
            L"  - คลิกขวาที่หมวดหมู่เพื่อเปลี่ยนชื่อหรือลบ\r\n"
            L"  - คลิกขวาที่โฟลเดอร์แล้วเลือก \"เพิ่มโฟลเดอร์เป็นชั้นหนังสือ\"\r\n"
            L"    เพื่อลงทะเบียนไฟล์บีบอัดทั้งหมดพร้อมกัน\r\n"
            L"\r\n"
            L"## เรียงลำดับ\r\n"
            L"  - ใช้ปุ่มเรียงลำดับเพื่อเรียงตามชื่อ, วันที่, ขนาด หรือประเภท\r\n"
            L"  - ลำดับการเรียงปัจจุบันแสดงบนปุ่ม\r\n"},
        {L"help.text.file",
            L"## รายการไฟล์\r\n"
            L"  - ใช้ปุ่มด้านบนเพื่อสลับระหว่างมุมมองรายการและตาราง (ภาพย่อ)\r\n"
            L"  - ในมุมมองรายการ ลากหัวคอลัมน์เพื่อเปลี่ยนลำดับการเรียง\r\n"
            L"  - ความกว้างและลำดับคอลัมน์จะถูกเก็บไว้สำหรับการเปิดครั้งถัดไป\r\n"
            L"\r\n"
            L"## เปลี่ยนชื่อ\r\n"
            L"  - เลือกไฟล์หรือโฟลเดอร์แล้วกด F2 เพื่อแก้ไขชื่อ\r\n"
            L"  - สามารถใช้ \"เปลี่ยนชื่อ\" จากเมนูคลิกขวาได้เช่นกัน\r\n"
            L"  - กด Enter เพื่อยืนยัน, Esc เพื่อยกเลิก\r\n"
            L"\r\n"
            L"## ลบไฟล์\r\n"
            L"  - กด Delete หรือใช้เมนูคลิกขวาเพื่อลบ\r\n"
            L"  - ไฟล์ที่ลบจะถูกย้ายไปถังขยะและสามารถกู้คืนได้\r\n"
            L"\r\n"
            L"## กรองไฟล์\r\n"
            L"  - พิมพ์ข้อความในช่องป้อนข้อมูลด้านล่างรายการไฟล์\r\n"
            L"    เพื่อแสดงเฉพาะไฟล์ที่ตรงกัน\r\n"
            L"\r\n"
            L"## เมนูคลิกขวา\r\n"
            L"  - คลิกขวาที่ไฟล์เพื่อดำเนินการต่างๆ\r\n"
            L"  - เปิดด้วยแอปเริ่มต้น: เปิดในแอปพลิเคชันอื่น\r\n"
            L"  - แสดงใน Explorer: เปิดตำแหน่งของไฟล์\r\n"
            L"  - ยังรวมถึงคัดลอกเส้นทาง, เพิ่มในรายการโปรด/ชั้นหนังสือ,\r\n"
            L"    เปลี่ยนชื่อ, ลบ และคุณสมบัติ\r\n"},
        {L"help.text.settings",
            L"## เปลี่ยนภาษา\r\n"
            L"  - รองรับ 12 ภาษา\r\n"
            L"  - การเปลี่ยนแปลงมีผลทันทีในหน้าจอตั้งค่า\r\n"
            L"\r\n"
            L"## เปลี่ยนปุ่มลัด\r\n"
            L"  - ปรับแต่งปุ่มทั้งหมดในแท็บ \"ปุ่มลัด\" ของการตั้งค่า\r\n"
            L"  - สามารถกำหนดหลายปุ่มให้กับการดำเนินการเดียวได้\r\n"
            L"\r\n"
            L"## เปลี่ยนรูปลักษณ์\r\n"
            L"  - ปรับขนาดตัวอักษร, ขนาดภาพย่อ และขนาดตัวอย่างในการตั้งค่า\r\n"
            L"  - การเปลี่ยนแปลงจะถูกใช้ทันที\r\n"
            L"\r\n"
            L"## การตั้งค่าอื่นๆ\r\n"
            L"  - ขนาดแคช: ปรับหน่วยความจำสำหรับเก็บภาพที่ดูแล้ว\r\n"
            L"  - วนซ้ำที่ปลาย: เลยหน้าสุดท้ายจะกลับไปหน้าแรก\r\n"
            L"  - ปกหน้าเดี่ยว: แสดงหน้าแรกเดี่ยวในโหมดสองหน้า\r\n"
            L"  - แสดงโฟลเดอร์ย่อย: แสดงภาพจากโฟลเดอร์ย่อยด้วย\r\n"
            L"  - เล่นอัตโนมัติ: เริ่มเล่นอัตโนมัติเมื่อเลือกวิดีโอ\r\n"},
    };
}

// 言語コードとロード関数のテーブル
struct LangEntry {
    const wchar_t* code;
    void (*loader)();
};

static const LangEntry g_langTable[] = {
    { L"ja",    LoadJa },
    { L"en",    LoadEn },
    { L"zh-CN", LoadZhCN },
    { L"zh-TW", LoadZhTW },
    { L"ko",    LoadKo },
    { L"es",    LoadEs },
    { L"pt-BR", LoadPtBR },
    { L"fr",    LoadFr },
    { L"de",    LoadDe },
    { L"ru",    LoadRu },
    { L"vi",    LoadVi },
    { L"th",    LoadTh },
};

static const std::wstring kEmpty;

void I18nInit(const std::wstring& lang) { I18nSetLang(lang); }

void I18nSetLang(const std::wstring& lang)
{
    g_lang = lang;
    for (const auto& entry : g_langTable)
    {
        if (lang == entry.code) { entry.loader(); return; }
    }
    LoadJa(); // デフォルト日本語
}

const std::wstring& I18nGet(const std::wstring& key)
{
    auto it = g_strings.find(key);
    if (it != g_strings.end()) return it->second;
    return kEmpty;
}

const std::wstring& I18nGetLang() { return g_lang; }
