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
#include "help.h"
#include "i18n.h"
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
        SetWindowTextW(g_app.wnd.hwndFolderLabel, I18nGet(L"ui.folder").c_str());
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
        if (g_app.nav.inArchiveMode && !g_app.nav.currentArchive.empty())
        {
            LoadArchiveToList(g_app.nav.currentArchive);
        }
        else if (!g_app.nav.currentFolder.empty())
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
        UpdateAddressBar(I18nGet(L"ui.history").c_str());
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
        RevealCurrentPath();
        UpdateAddressBar(I18nGet(L"ui.bookshelf").c_str());
        LayoutChildren(hwnd);
        break;
    }

    // ビューアー - ページ送り
    case IDM_VIEW_FIRST: GoToFile(0); break;
    case IDM_VIEW_PREV:  GoToFile(g_app.nav.currentFileIndex - 1); break;
    case IDM_VIEW_NEXT:  GoToFile(g_app.nav.currentFileIndex + 1); break;
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
    case IDM_VIEW_ROTATE_CW: ViewerRotateCW(); break;

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
        if (MessageBoxW(hwnd, I18nGet(L"dlg.bookshelfclear").c_str(), I18nGet(L"dlg.confirm").c_str(),
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

        AppendMenuW(hMenu, MF_STRING | (curMode == SortByName ? MF_CHECKED : 0), 2510, I18nGet(L"list.name").c_str());
        AppendMenuW(hMenu, MF_STRING | (curMode == SortByDate ? MF_CHECKED : 0), 2511, I18nGet(L"list.date").c_str());
        AppendMenuW(hMenu, MF_STRING | (curMode == SortBySize ? MF_CHECKED : 0), 2512, I18nGet(L"list.size").c_str());
        AppendMenuW(hMenu, MF_STRING | (curMode == SortByType ? MF_CHECKED : 0), 2513, I18nGet(L"list.type").c_str());
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hMenu, MF_STRING | (!curDesc ? MF_CHECKED : 0), 2514, I18nGet(L"sort.asc").c_str());
        AppendMenuW(hMenu, MF_STRING | (curDesc ? MF_CHECKED : 0), 2515, I18nGet(L"sort.desc").c_str());

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
        // ソートボタンラベル更新
        if (sortCmd >= 2510)
            UpdateSortButtonLabels();
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

static void ExecuteAction(HWND hwnd, const std::wstring& action, HWND focused)
{
    // ナビゲーション
    if (action == L"prev_page")
        GoToFile(g_app.nav.currentFileIndex - GetPagesPerView());
    else if (action == L"next_page")
        GoToFile(g_app.nav.currentFileIndex + GetPagesPerView());
    else if (action == L"first_page")
        GoToFile(0);
    else if (action == L"last_page")
        GoToFile((int)g_app.nav.viewableFiles.size() - 1);
    else if (action == L"prev_archive")
    {
        if (g_app.nav.inArchiveMode && !g_app.nav.currentArchive.empty())
            NavigateToSiblingArchive(-1);
    }
    else if (action == L"next_archive")
    {
        if (g_app.nav.inArchiveMode && !g_app.nav.currentArchive.empty())
            NavigateToSiblingArchive(1);
    }
    else if (action == L"nav_back")    NavigateBack();
    else if (action == L"nav_forward") NavigateForward();
    else if (action == L"nav_up")      NavigateUp();
    else if (action == L"open_item")
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
            return;
        }
        // ファイルリスト: 選択アイテムを開く
        int sel = (int)SendMessageW(g_app.wnd.hwndList, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
        if (sel >= 0 && sel < (int)g_app.nav.fileItems.size())
        {
            auto& item = g_app.nav.fileItems[sel];
            if (item.isDirectory || IsArchiveFile(item.name))
            {
                NavigateTo(item.fullPath);
                return;
            }
        }
        // フォールバック: ツリーの展開/折畳み
        HTREEITEM hSel = (HTREEITEM)SendMessageW(g_app.wnd.hwndTree, TVM_GETNEXTITEM, TVGN_CARET, 0);
        if (hSel)
        {
            UINT state = (UINT)SendMessageW(g_app.wnd.hwndTree, TVM_GETITEMSTATE, (WPARAM)hSel, TVIS_EXPANDED);
            SendMessageW(g_app.wnd.hwndTree, TVM_EXPAND,
                (state & TVIS_EXPANDED) ? TVE_COLLAPSE : TVE_EXPAND, (LPARAM)hSel);
        }
    }
    // 表示
    else if (action == L"fit_window")     ViewerSetScaleMode(FitWindow);
    else if (action == L"fit_width")      ViewerSetScaleMode(FitWidth);
    else if (action == L"fit_height")     ViewerSetScaleMode(FitHeight);
    else if (action == L"original_size")  ViewerSetScaleMode(Original);
    else if (action == L"view_single")    ViewerSetViewMode(1);
    else if (action == L"view_spread")    ViewerSetViewMode(2);
    else if (action == L"view_auto")      ViewerSetViewMode(0);
    else if (action == L"view_cycle")     ViewerSetViewMode((g_app.viewer.viewMode + 1) % 3);
    else if (action == L"toggle_binding") ViewerToggleBinding();
    else if (action == L"bind_ltr")       { g_app.viewer.isRTL = false; InvalidateRect(g_app.wnd.hwndViewer, nullptr, FALSE); }
    else if (action == L"bind_rtl")       { g_app.viewer.isRTL = true; InvalidateRect(g_app.wnd.hwndViewer, nullptr, FALSE); }
    else if (action == L"fullscreen")     ToggleFullscreen(hwnd);
    // ズーム
    else if (action == L"zoom_in")    ViewerZoomIn();
    else if (action == L"zoom_out")   ViewerZoomOut();
    else if (action == L"zoom_reset") { g_app.viewer.zoom = 1.0f; InvalidateRect(g_app.wnd.hwndViewer, nullptr, FALSE); UpdateZoomLabel(); }
    // 編集
    else if (action == L"copy_image")  CopyImageToClipboard(hwnd);
    else if (action == L"rotate_cw")   ViewerRotateCW();
    else if (action == L"rotate_ccw")  ViewerRotateCCW();
    else if (action == L"rename")
    {
        if (focused == g_app.wnd.hwndTree)
        {
            // ツリーにフォーカス: インラインラベル編集
            HTREEITEM hSel = (HTREEITEM)SendMessageW(g_app.wnd.hwndTree, TVM_GETNEXTITEM, TVGN_CARET, 0);
            if (hSel)
                SendMessageW(g_app.wnd.hwndTree, TVM_EDITLABEL, 0, (LPARAM)hSel);
        }
        else
        {
            // ファイルリスト: インラインリネーム
            int sel = (int)SendMessageW(g_app.wnd.hwndList, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
            if (sel >= 0 && sel < (int)g_app.nav.fileItems.size() && !g_app.nav.inArchiveMode)
                StartInlineRename(sel);
        }
    }
    else if (action == L"delete_file")
    {
        int sel = (int)SendMessageW(g_app.wnd.hwndList, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
        if (sel >= 0 && sel < (int)g_app.nav.fileItems.size())
        {
            auto& item = g_app.nav.fileItems[sel];
            if (!g_app.nav.inArchiveMode)
            {
                std::wstring msg = L"\"" + item.name + L"\" " + I18nGet(L"dlg.delete");
                std::wstring delPath = item.fullPath;
                if (MessageBoxW(hwnd, msg.c_str(), I18nGet(L"dlg.confirm").c_str(), MB_YESNO | MB_ICONQUESTION) == IDYES)
                {
                    if (_wcsicmp(delPath.c_str(), g_app.nav.currentPath.c_str()) == 0)
                    {
                        ViewerStopAnimation();
                        g_app.viewer.bitmap.Reset();
                        g_app.viewer.bitmap2.Reset();
                        MediaStop();
                    }
                    if (IsArchiveFile(delPath))
                        CloseCurrentArchive();
                    std::wstring shPath = delPath;
                    shPath.push_back(L'\0');
                    SHFILEOPSTRUCTW op = {};
                    op.wFunc = FO_DELETE;
                    op.pFrom = shPath.c_str();
                    op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION;
                    if (SHFileOperationW(&op) == 0)
                    {
                        RemoveFileItemByPath(delPath);
                        RemoveTreeItemByPath(delPath);
                    }
                }
            }
        }
    }
    // メディア
    else if (action == L"play_pause") { if (g_app.nav.isMediaMode) MediaTogglePlayPause(); }
    // その他
    else if (action == L"refresh")    HandleCommand(hwnd, IDM_NAV_REFRESH);
    else if (action == L"help")       ShowHelpDialog(hwnd);
    else if (action == L"escape")     { if (g_app.isFullscreen) ToggleFullscreen(hwnd); }
    // 新規アクション
    else if (action == L"bookshelf")      HandleCommand(hwnd, IDM_NAV_BOOKSHELF);
    else if (action == L"history")        HandleCommand(hwnd, IDM_NAV_HISTORY);
    else if (action == L"list_view")      HandleCommand(hwnd, IDM_NAV_LIST);
    else if (action == L"grid_view")      HandleCommand(hwnd, IDM_NAV_GRID);
    else if (action == L"hover_preview")  HandleCommand(hwnd, IDM_NAV_HOVER);
    else if (action == L"settings")       HandleCommand(hwnd, IDM_NAV_SETTINGS);
}

void HandleKeyDown(HWND hwnd, WPARAM vk)
{
    HWND focused = GetFocus();

    // テキスト入力中はEsc以外スキップ
    if (focused == g_app.wnd.hwndAddressEdit ||
        focused == g_app.wnd.hwndFilterBox ||
        focused == g_app.wnd.hwndHistoryFilter)
    {
        if (vk == VK_ESCAPE) SetFocus(g_app.wnd.hwndMain);
        return;
    }

    bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;

    std::wstring action = FindAction((UINT)vk, ctrl, shift, alt);
    if (!action.empty())
        ExecuteAction(hwnd, action, focused);
}
