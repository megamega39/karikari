#include "command.h"
#include "viewer.h"
#include "viewer_toolbar.h"
#include "context_menu.h"
#include "bookshelf.h"
#include "navbar.h"
#include "addressbar.h"
#include "tree.h"
#include "filelist.h"
#include "nav.h"
#include "settings.h"
#include "history.h"
#include "archive.h"
#include "media.h"
#include "window.h"
#include <shellapi.h>
#include <shlwapi.h>

// 履歴関連（window.cpp内のstatic関数を呼び出す必要があるため、extern宣言）
extern void BuildHistoryList();
extern std::wstring g_historyFilterText;

// ToggleFullscreen（window.cppに残っている）
extern void ToggleFullscreen(HWND hwnd);

// === UIモード切替ヘルパー ===
static void ShowNormalUI()
{
    ShowWindow(g_app.wnd.hwndFolderLabel, SW_SHOW);
    if (g_app.wnd.hwndTreeSortBtn) ShowWindow(g_app.wnd.hwndTreeSortBtn, SW_SHOW);
    ShowWindow(g_app.wnd.hwndTree, SW_SHOW);
    ShowWindow(g_app.wnd.hwndSidebarSplitter, SW_SHOW);
    ShowWindow(g_app.wnd.hwndFilterBox, SW_SHOW);
    ShowWindow(g_app.wnd.hwndList, SW_SHOW);
    if (g_app.wnd.hwndFolderLabel)
        SetWindowTextW(g_app.wnd.hwndFolderLabel, L"フォルダ");
}

static void HideHistoryUI()
{
    ShowWindow(g_app.wnd.hwndHistoryToolbar, SW_HIDE);
    ShowWindow(g_app.wnd.hwndHistoryList, SW_HIDE);
    if (g_app.wnd.hwndHistoryFilter) ShowWindow(g_app.wnd.hwndHistoryFilter, SW_HIDE);
}

static void HideBookshelfUI()
{
    if (g_app.wnd.hwndBookshelfToolbar) ShowWindow(g_app.wnd.hwndBookshelfToolbar, SW_HIDE);
}

static void HideNormalUI()
{
    ShowWindow(g_app.wnd.hwndFolderLabel, SW_HIDE);
    if (g_app.wnd.hwndTreeSortBtn) ShowWindow(g_app.wnd.hwndTreeSortBtn, SW_HIDE);
}

static void RevealCurrentPath()
{
    if (g_app.nav.inArchiveMode && !g_app.nav.currentArchive.empty())
        SelectTreePath(g_app.nav.currentArchive);
    else if (!g_app.nav.currentFolder.empty())
        SelectTreePath(g_app.nav.currentFolder);
}

void HandleCommand(HWND hwnd, UINT cmd)
{
    switch (cmd)
    {
    // ナビバー
    case IDM_NAV_BACK:    NavigateBack(); break;
    case IDM_NAV_FORWARD: NavigateForward(); break;
    case IDM_NAV_UP:      NavigateUp(); break;
    case IDM_NAV_REFRESH:
        if (!g_app.nav.currentFolder.empty())
        {
            LoadFolder(g_app.nav.currentFolder);
            PopulateListView();
        }
        RefreshTree();
        RevealCurrentPath();
        break;

    // 履歴表示（トグル：もう一度押すと通常に戻る）
    case IDM_NAV_HISTORY:
    {
        if (GetTreeMode() == 2)
        {
            // 履歴モード解除 → 通常モードへ
            ShowNormalTree();
            HideHistoryUI();
            ShowNormalUI();
            LayoutChildren(hwnd);
            RevealCurrentPath();
            break;
        }
        // 他モード → 履歴モードへ
        if (GetTreeMode() == 0) SaveNormalTreeState();
        HideBookshelfUI();
        // 履歴モード有効化
        ShowHistoryTree(); // treeMode=2に設定
        HideNormalUI();
        ShowWindow(g_app.wnd.hwndTree, SW_HIDE);
        // 履歴UI表示
        ShowWindow(g_app.wnd.hwndHistoryToolbar, SW_SHOW);
        ShowWindow(g_app.wnd.hwndHistoryList, SW_SHOW);
        if (g_app.wnd.hwndHistoryFilter)
            ShowWindow(g_app.wnd.hwndHistoryFilter, SW_SHOW);
        // ファイルリスト関連も明示的に表示
        ShowWindow(g_app.wnd.hwndSidebarSplitter, SW_SHOW);
        ShowWindow(g_app.wnd.hwndFilterBox, SW_SHOW);
        ShowWindow(g_app.wnd.hwndList, SW_SHOW);
        // フィルターテキストを保持（モード切替で消さない）
        if (g_app.wnd.hwndHistoryFilter)
            SetWindowTextW(g_app.wnd.hwndHistoryFilter, g_historyFilterText.c_str());
        BuildHistoryList();
        LayoutChildren(hwnd);
        UpdateAddressBar(L"履歴");
        break;
    }

    // 本棚表示（トグル：もう一度押すと通常に戻る）
    case IDM_NAV_BOOKSHELF:
    {
        if (GetTreeMode() == 1)
        {
            // 本棚モード解除 → 通常モードへ
            ShowNormalTree();
            HideBookshelfUI();
            ShowNormalUI();
            LayoutChildren(hwnd);
            RevealCurrentPath();
            break;
        }
        // 通常モードの展開状態を保存
        if (GetTreeMode() == 0) SaveNormalTreeState();
        // 他モード → 本棚モードへ
        if (GetTreeMode() == 2) HideHistoryUI();
        ShowNormalUI(); // ツリーとリストを表示
        HideNormalUI(); // フォルダラベル/ソートボタンを非表示
        if (g_app.wnd.hwndBookshelfToolbar) ShowWindow(g_app.wnd.hwndBookshelfToolbar, SW_SHOW);
        ShowBookshelfTree();
        UpdateAddressBar(L"本棚");
        LayoutChildren(hwnd);
        break;
    }

    // ビューアー - ページ送り
    case IDM_VIEW_FIRST: GoToFile(0); break;
    case IDM_VIEW_PREV:  GoToFile(g_app.nav.currentFileIndex - GetPagesPerView()); break;
    case IDM_VIEW_NEXT:  GoToFile(g_app.nav.currentFileIndex + GetPagesPerView()); break;
    case IDM_VIEW_LAST:  GoToFile((int)g_app.nav.viewableFiles.size() - 1); break;

    // ビューアー - スケールモード
    case IDM_VIEW_FIT_WINDOW: ViewerSetScaleMode(FitWindow); break;
    case IDM_VIEW_FIT_WIDTH:  ViewerSetScaleMode(FitWidth); break;
    case IDM_VIEW_FIT_HEIGHT: ViewerSetScaleMode(FitHeight); break;
    case IDM_VIEW_ORIGINAL:   ViewerSetScaleMode(Original); break;

    // ビューアー - ズーム
    case IDM_VIEW_ZOOMIN:  ViewerZoomIn(); break;
    case IDM_VIEW_ZOOMOUT: ViewerZoomOut(); break;
    case IDM_VIEW_ZOOM_RESET:
        g_app.viewer.zoom = 1.0f;
        InvalidateRect(g_app.wnd.hwndViewer, nullptr, FALSE);
        UpdateZoomLabel();
        break;

    // ビューアー - 表示モード
    case IDM_VIEW_AUTO:    ViewerSetViewMode(0); break;
    case IDM_VIEW_SINGLE:  ViewerSetViewMode(1); break;
    case IDM_VIEW_SPREAD:  ViewerSetViewMode(2); break;
    case IDM_VIEW_BINDING: ViewerToggleBinding(); break;

    // ホバープレビュー ON/OFF
    case IDM_NAV_HOVER:
    {
        g_app.hoverPreviewEnabled = !g_app.hoverPreviewEnabled;
        SendMessageW(g_app.wnd.hwndNavBar, TB_CHECKBUTTON, IDM_NAV_HOVER,
                     MAKELONG(g_app.hoverPreviewEnabled, 0));
        if (!g_app.hoverPreviewEnabled && g_app.hwndHoverPreview)
        {
            DestroyWindow(g_app.hwndHoverPreview);
            g_app.hwndHoverPreview = nullptr;
        }
        break;
    }

    // 設定 / ヘルプ
    case IDM_NAV_SETTINGS: ShowSettingsDialog(hwnd); break;
    case IDM_NAV_HELP:
        SendMessageW(hwnd, WM_KEYDOWN, VK_F1, 0); // F1 と同じ
        break;

    // リスト/グリッド表示切替
    case IDM_NAV_LIST: SwitchToListView(); break;
    case IDM_NAV_GRID: SwitchToGridView(); break;

    case IDM_BOOKSHELF_CLEAR:
    {
        if (MessageBoxW(hwnd, L"本棚の登録をすべて削除しますか？", L"確認",
            MB_YESNO | MB_ICONQUESTION) == IDYES)
        {
            BookshelfClear();
            if (GetTreeMode() == 1) ShowBookshelfTree();
        }
        break;
    }

    case IDM_BOOKSHELF_SORT:
    case IDM_TREE_SORT:
    {
        // ソートドロップダウンメニュー表示
        HMENU hMenu = CreatePopupMenu();
        TreeSortMode curMode = GetTreeSortMode();
        bool curDesc = GetTreeSortDescending();

        AppendMenuW(hMenu, MF_STRING | (curMode == SortByName ? MF_CHECKED : 0), 2510, L"名前");
        AppendMenuW(hMenu, MF_STRING | (curMode == SortByDate ? MF_CHECKED : 0), 2511, L"更新日時");
        AppendMenuW(hMenu, MF_STRING | (curMode == SortBySize ? MF_CHECKED : 0), 2512, L"サイズ");
        AppendMenuW(hMenu, MF_STRING | (curMode == SortByType ? MF_CHECKED : 0), 2513, L"種類");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING | (!curDesc ? MF_CHECKED : 0), 2514, L"昇順");
        AppendMenuW(hMenu, MF_STRING | (curDesc ? MF_CHECKED : 0), 2515, L"降順");

        RECT rc;
        HWND sortBtn = (cmd == IDM_BOOKSHELF_SORT) ?
            g_app.wnd.hwndBookshelfSortBtn : g_app.wnd.hwndTreeSortBtn;
        GetWindowRect(sortBtn, &rc);
        int sortCmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN,
                                 rc.left, rc.bottom, 0, hwnd, nullptr);
        DestroyMenu(hMenu);

        if (sortCmd >= 2510 && sortCmd <= 2513)
        {
            TreeSortMode newMode = (TreeSortMode)(sortCmd - 2510);
            bool newDesc = (curMode == newMode) ? !curDesc : false; // 同じモード→方向反転
            SetTreeSortMode(newMode, newDesc);
            // 設定保存
            AppSettings s; LoadSettings(s);
            const wchar_t* modeNames[] = { L"Name", L"Date", L"Size", L"Type" };
            s.treeSortMode = modeNames[(int)newMode];
            s.treeSortDescending = newDesc;
            SaveSettings(s);
        }
        else if (sortCmd == 2514) // 昇順
        {
            SetTreeSortMode(curMode, false);
            AppSettings s; LoadSettings(s);
            s.treeSortDescending = false;
            SaveSettings(s);
        }
        else if (sortCmd == 2515) // 降順
        {
            SetTreeSortMode(curMode, true);
            AppSettings s; LoadSettings(s);
            s.treeSortDescending = true;
            SaveSettings(s);
        }
        // 本棚モード時はツリー再構築
        if (sortCmd >= 2510 && GetTreeMode() == 1)
            ShowBookshelfTree();
        break;
    }
    }

    // ビューアーツールバーのボタン状態を更新
    UpdateViewerToolbarState();

    // ナビバーの本棚・履歴ボタンのハイライト更新
    if (g_app.wnd.hwndNavBar)
    {
        SendMessageW(g_app.wnd.hwndNavBar, TB_CHECKBUTTON, IDM_NAV_BOOKSHELF, MAKELONG(GetTreeMode() == 1, 0));
        SendMessageW(g_app.wnd.hwndNavBar, TB_CHECKBUTTON, IDM_NAV_HISTORY, MAKELONG(GetTreeMode() == 2, 0));
    }
}

void HandleKeyDown(HWND hwnd, WPARAM vk)
{
    // テキスト入力中はショートカット無効（Alt/Ctrlキー除く）
    HWND focused = GetFocus();
    if (focused == g_app.wnd.hwndAddressEdit)
    {
        // アドレスバーにフォーカス時はEsc以外スキップ
        if (vk == VK_ESCAPE) { SetFocus(g_app.wnd.hwndMain); return; }
        return;
    }

    bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;

    if (alt)
    {
        if (vk == VK_LEFT)  { NavigateBack(); return; }
        if (vk == VK_RIGHT) { NavigateForward(); return; }
        if (vk == VK_UP)    { NavigateUp(); return; }
        return;
    }

    if (ctrl)
    {
        if (vk == VK_OEM_PLUS || vk == VK_ADD)  { ViewerZoomIn(); return; }
        if (vk == VK_OEM_MINUS || vk == VK_SUBTRACT) { ViewerZoomOut(); return; }
        if (vk == '0' || vk == VK_NUMPAD0) { g_app.viewer.zoom = 1.0f; InvalidateRect(g_app.wnd.hwndViewer, nullptr, FALSE); UpdateZoomLabel(); return; }
        if (vk == 'C') { CopyImageToClipboard(hwnd); return; }
        if (vk == 'R')
        {
            bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            if (shift) ViewerRotateCCW(); else ViewerRotateCW();
            return;
        }
        return;
    }

    switch (vk)
    {
    case VK_LEFT:  GoToFile(g_app.nav.currentFileIndex - GetPagesPerView()); break;
    case VK_RIGHT: GoToFile(g_app.nav.currentFileIndex + GetPagesPerView()); break;
    case VK_HOME:  GoToFile(0); break;
    case VK_END:   GoToFile((int)g_app.nav.viewableFiles.size() - 1); break;
    case VK_F5:
        if (!g_app.nav.currentFolder.empty())
        {
            LoadFolder(g_app.nav.currentFolder);
            PopulateListView();
            if (GetTreeMode() == 0)
                SelectTreePath(g_app.nav.currentFolder);
        }
        break;
    case VK_F1:
    {
        MessageBoxW(hwnd,
            L"karikari - 画像/動画ビューア\n\n"
            L"ショートカットキー:\n"
            L"  ←/→  前/次ページ\n"
            L"  Home/End  最初/最後\n"
            L"  Alt+←/→  戻る/進む\n"
            L"  Alt+↑  親フォルダ\n"
            L"  ↑/↓  ファイルリスト移動\n"
            L"  Enter  フォルダ/書庫を開く\n"
            L"  F5  更新\n"
            L"  F11  全画面\n"
            L"  W  ウィンドウに合わせる\n"
            L"  1/2/3  単ページ/見開き/自動\n"
            L"  B  綴じ方向切替\n"
            L"  L/R  左綴じ/右綴じ\n"
            L"  V  表示モード切替\n"
            L"  C  お気に入り表示\n"
            L"  Space  メディア再生/一時停止\n"
            L"  Ctrl+C  画像コピー\n"
            L"  Ctrl+R  時計回り回転\n"
            L"  Ctrl+Shift+R  反時計回り回転\n"
            L"  Ctrl++/-  ズーム\n"
            L"  Ctrl+0  ズームリセット\n"
            L"  F2  名前変更\n"
            L"  Delete  ファイル削除\n"
            L"  Escape  全画面解除",
            L"karikari ヘルプ", MB_OK | MB_ICONINFORMATION);
        break;
    }
    case VK_F11:   ToggleFullscreen(hwnd); break;
    case VK_UP:
    case VK_DOWN:
    {
        // 書庫モード時: 親フォルダ内の前後の書庫に切り替え
        if (g_app.nav.inArchiveMode && !g_app.nav.currentArchive.empty())
        {
            NavigateToSiblingArchive(vk == VK_UP ? -1 : 1);
        }
        // ツリービューにフォーカスがあればツリー操作に任せる（デフォルト動作）
        break;
    }
    case VK_RETURN:
    {
        // ツリーにフォーカス時: 展開/折畳み切替
        if (focused == g_app.wnd.hwndTree)
        {
            HTREEITEM hSel = (HTREEITEM)SendMessageW(g_app.wnd.hwndTree, TVM_GETNEXTITEM, TVGN_CARET, 0);
            if (hSel)
            {
                UINT state = (UINT)SendMessageW(g_app.wnd.hwndTree, TVM_GETITEMSTATE, (WPARAM)hSel, TVIS_EXPANDED);
                SendMessageW(g_app.wnd.hwndTree, TVM_EXPAND,
                    (state & TVIS_EXPANDED) ? TVE_COLLAPSE : TVE_EXPAND, (LPARAM)hSel);
            }
            break;
        }
        // ファイルリスト/その他にフォーカス時: 選択アイテムを開くか、ツリーの展開/折畳み
        int sel = (int)SendMessageW(g_app.wnd.hwndList, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
        if (sel >= 0 && sel < (int)g_app.nav.fileItems.size())
        {
            auto& item = g_app.nav.fileItems[sel];
            if (item.isDirectory || IsArchiveFile(item.name))
            {
                NavigateTo(item.fullPath);
                break;
            }
        }
        // リストで対象がなければツリーの展開/折畳み
        {
            HTREEITEM hSel = (HTREEITEM)SendMessageW(g_app.wnd.hwndTree, TVM_GETNEXTITEM, TVGN_CARET, 0);
            if (hSel)
            {
                UINT state = (UINT)SendMessageW(g_app.wnd.hwndTree, TVM_GETITEMSTATE, (WPARAM)hSel, TVIS_EXPANDED);
                SendMessageW(g_app.wnd.hwndTree, TVM_EXPAND,
                    (state & TVIS_EXPANDED) ? TVE_COLLAPSE : TVE_EXPAND, (LPARAM)hSel);
            }
        }
        break;
    }
    case VK_SPACE:
        if (g_app.nav.isMediaMode) MediaTogglePlayPause();
        break;
    case VK_ESCAPE:
        if (g_app.isFullscreen) ToggleFullscreen(hwnd);
        break;
    case 'W':      ViewerSetScaleMode(FitWindow); break;
    case 'B':      ViewerToggleBinding(); break;
    case 'L':      g_app.viewer.isRTL = false; InvalidateRect(g_app.wnd.hwndViewer, nullptr, FALSE); break;
    case 'R':      g_app.viewer.isRTL = true; InvalidateRect(g_app.wnd.hwndViewer, nullptr, FALSE); break;
    case 'V':      ViewerSetViewMode((g_app.viewer.viewMode + 1) % 3); break;
    case '1':      ViewerSetViewMode(1); break;
    case '2':      ViewerSetViewMode(2); break;
    case '3':      ViewerSetViewMode(0); break;
    case 'C':      // ToggleBookshelf（お気に入り表示切替）
        HandleCommand(hwnd, IDM_NAV_BOOKSHELF);
        break;
    case VK_F2:
    {
        // 名前変更（アドレスバーを入力欄として使用）
        int sel = (int)SendMessageW(g_app.wnd.hwndList, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
        if (sel >= 0 && sel < (int)g_app.nav.fileItems.size() && !g_app.nav.inArchiveMode)
        {
            auto& item = g_app.nav.fileItems[sel];
            // アドレスバーにファイル名を入れて編集モードに
            SetWindowTextW(g_app.wnd.hwndAddressEdit, item.name.c_str());
            ShowWindow(g_app.wnd.hwndAddressEdit, SW_SHOW);
            SetFocus(g_app.wnd.hwndAddressEdit);
            SendMessageW(g_app.wnd.hwndAddressEdit, EM_SETSEL, 0,
                item.name.find_last_of(L'.') != std::wstring::npos ?
                (LPARAM)item.name.find_last_of(L'.') : -1); // 拡張子前まで選択
        }
        break;
    }
    case VK_DELETE:
    {
        // 選択ファイルを削除
        int sel = (int)SendMessageW(g_app.wnd.hwndList, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
        if (sel >= 0 && sel < (int)g_app.nav.fileItems.size())
        {
            auto& item = g_app.nav.fileItems[sel];
            if (!g_app.nav.inArchiveMode)
            {
                std::wstring msg = L"\"" + item.name + L"\" を削除しますか？";
                if (MessageBoxW(hwnd, msg.c_str(), L"確認", MB_YESNO | MB_ICONQUESTION) == IDYES)
                {
                    // ごみ箱へ移動
                    std::wstring path = item.fullPath;
                    path.push_back(L'\0'); // SHFileOperation用のダブルNULL
                    SHFILEOPSTRUCTW op = {};
                    op.wFunc = FO_DELETE;
                    op.pFrom = path.c_str();
                    op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION;
                    if (SHFileOperationW(&op) == 0)
                    {
                        LoadFolder(g_app.nav.currentFolder);
                        PopulateListView();
                    }
                }
            }
        }
        break;
    }
    }
}
