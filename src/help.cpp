#include "help.h"
#include "app.h"
#include <commctrl.h>

// === タブテキスト ===
static const wchar_t* g_helpTexts[] = {
    // タブ0: 基本操作
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
    L"  ・再生バーの「A」ボタンで自動再生のオン/オフを切り替えられます\r\n",

    // タブ1: 表示モード
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
    L"  ・Esc キーで元に戻ります\r\n",

    // タブ2: ツリービュー
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
    L"  ・現在の並び順はボタンに表示されています\r\n",

    // タブ3: ファイル操作
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
    L"    名前変更、削除、プロパティが使えます\r\n",

    // タブ4: 設定
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
    L"  ・動画自動再生: 動画ファイルを選んだとき自動的に再生を始めます\r\n",
};

static const wchar_t* g_tabNames[] = {
    L"基本操作", L"表示モード", L"ツリービュー", L"ファイル操作", L"設定", L"このアプリについて"
};
static constexpr int TAB_COUNT = _countof(g_tabNames);
static constexpr int TAB_ABOUT = TAB_COUNT - 1;
static constexpr int TEXT_TAB_COUNT = _countof(g_helpTexts); // テキストタブの数

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
    SetWindowTextW(hDlg, L"ヘルプ - karikari の使い方");

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
        tci.pszText = (LPWSTR)g_tabNames[i];
        SendMessageW(g_helpTabCtrl, TCM_INSERTITEMW, i, (LPARAM)&tci);
    }

    int tx = S(14), ty = S(30);
    int contentW = W - S(20);
    int contentH = DH - S(44) - S(32) - S(8);

    // テキストタブ: 読み取り専用のマルチラインEditコントロール
    for (int i = 0; i < TEXT_TAB_COUNT; i++)
    {
        HWND hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", g_helpTexts[i],
            WS_CHILD | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL,
            tx, ty, contentW, contentH, hDlg, nullptr, g_app.hInstance, nullptr);
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

        AL(MkLabel(hDlg, tx, ay, contentW, S(16), L"バージョン 1.0.0"));
        ay += S(24);
        AL(MkLabel(hDlg, tx, ay, contentW, S(16), L"最速の画像・動画ビューア"));
        ay += S(36);
        AL(MkLabel(hDlg, tx, ay, contentW, S(16), L"作者:", true));
        ay += S(20);
        AL(MkLabel(hDlg, tx + S(8), ay, contentW, S(16), L"megamega39"));
        ay += S(32);
        AL(MkLabel(hDlg, tx, ay, contentW, S(16), L"説明:", true));
        ay += S(20);
        AL(MkLabel(hDlg, tx + S(8), ay, contentW, S(32),
            L"C++ + Win32 API + Direct2D で構築した高速画像/動画ビューア。"));
        ay += S(24);
        AL(MkLabel(hDlg, tx + S(8), ay, contentW, S(16),
            L"leeyez_kai (.NET版) のネイティブ移植。"));
    }

    // 閉じるボタン
    int btnW = S(80), btnH = S(26);
    MkButton(hDlg, (DW - btnW) / 2, DH - S(34), btnW, btnH, IDC_HELP_CLOSE, L"閉じる");

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
