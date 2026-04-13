#include "window.h"
#include "viewer.h"
#include "viewer_toolbar.h"
#include "command.h"
#include "context_menu.h"
#include "hover_preview.h"
#include "bookshelf.h"
#include "navbar.h"
#include "addressbar.h"
#include "statusbar.h"
#include "splitter.h"
#include "tree.h"
#include "filelist.h"
#include "nav.h"
#include "settings.h"
#include "history.h"
#include "archive.h"
#include "prefetch.h"
#include "cache.h"
#include "decoder.h"
#include "utils.h"
#include "media.h"
#include "favorites.h"
#include "i18n.h"
#include "fswatcher.h"
#include <shlwapi.h>
#include <shlobj.h>
#include <shellapi.h>
#include <uxtheme.h>
#include "stream_cache.h"
#include <atomic>
#include <tuple>

// グローバルUIフォント
HFONT g_uiFont = nullptr;
HFONT g_uiFontBold = nullptr;

// 基準レイアウト定数（96dpi）
// パス復元用グローバル（CreateMainWindow → WM_RESTORE_LAST_PATH で共有）
std::wstring g_restorePath;
int g_restoreIndex = -1;

// ツリー選択デバウンス用
static constexpr UINT_PTR kTreeNavTimer = 98;
static std::wstring g_pendingTreePath;
HTREEITEM g_treeRClickItem = nullptr; // 右クリックしたツリーアイテム

static constexpr int kNavBarH_96 = 52;
static constexpr int kAddrBarH_96 = 28;
static constexpr int kSplitterW_96 = 6;
static constexpr int kViewerToolbarH_96 = 36;
static constexpr int kMinPanelSize_96 = 100;

static int DpiScale(int value, UINT dpi) { return MulDiv(value, dpi, 96); }

void ApplyFontSize(int fontSize)
{
    int size = -(fontSize + 3); // fontSize=9 → -12
    if (g_uiFont) DeleteObject(g_uiFont);
    if (g_uiFontBold) DeleteObject(g_uiFontBold);
    g_uiFont = CreateFontW(size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Yu Gothic UI");
    g_uiFontBold = CreateFontW(size, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Yu Gothic UI");

    // 全UIウィンドウにWM_SETFONT送信（アイコンフォントは対象外）
    auto set = [](HWND h, HFONT f) { if (h) SendMessageW(h, WM_SETFONT, (WPARAM)f, TRUE); };
    set(g_app.wnd.hwndTree, g_uiFont);
    set(g_app.wnd.hwndList, g_uiFont);
    set(g_app.wnd.hwndStatusBar, g_uiFont);
    set(g_app.wnd.hwndFilterBox, g_uiFont);
    set(g_app.wnd.hwndHistoryList, g_uiFont);
    set(g_app.wnd.hwndHistoryFilter, g_uiFont);
    set(g_app.wnd.hwndHistoryToolbar, g_uiFont);
    set(g_app.wnd.hwndFolderLabel, g_uiFont);
    set(g_app.wnd.hwndBookshelfToolbar, g_uiFont);
    set(g_app.wnd.hwndTreeSortBtn, g_uiFont);
    set(g_app.wnd.hwndBookshelfSortBtn, g_uiFont);
}

void UpdateSortButtonLabels()
{
    TreeSortMode mode = GetTreeSortMode();
    bool desc = GetTreeSortDescending();
    const wchar_t* arrow = desc ? L" \u25BC" : L" \u25B2"; // ▼ ▲
    const wchar_t* names[] = { L"Name", L"Date", L"Size", L"Type" };
    // i18nキーで名前取得
    const wchar_t* keys[] = { L"list.name", L"list.date", L"list.type", L"list.type" };
    std::wstring label;
    switch (mode) {
    case SortByName: label = I18nGet(L"list.name"); break;
    case SortByDate: label = I18nGet(L"list.date"); break;
    case SortBySize: label = I18nGet(L"list.size"); break;
    case SortByType: label = I18nGet(L"list.type"); break;
    }
    label += arrow;

    if (g_app.wnd.hwndTreeSortBtn)
        SetWindowTextW(g_app.wnd.hwndTreeSortBtn, label.c_str());
    if (g_app.wnd.hwndBookshelfSortBtn)
        SetWindowTextW(g_app.wnd.hwndBookshelfSortBtn, label.c_str());
}

void RebuildUI()
{
    // ナビバーツールチップを更新
    UpdateNavbarTooltips();

    // ラベルテキストを現在の言語に更新
    if (g_app.wnd.hwndFolderLabel)
        SetWindowTextW(g_app.wnd.hwndFolderLabel, I18nGet(L"ui.folder").c_str());
    if (g_app.wnd.hwndAddressLabel)
        SetWindowTextW(g_app.wnd.hwndAddressLabel, I18nGet(L"ui.address").c_str());

    // 本棚ツールバーの子コントロールテキスト更新
    if (g_app.wnd.hwndBookshelfToolbar)
    {
        HWND hChild = GetWindow(g_app.wnd.hwndBookshelfToolbar, GW_CHILD);
        int idx = 0;
        while (hChild)
        {
            wchar_t cls[64] = {};
            GetClassNameW(hChild, cls, 64);
            if (_wcsicmp(cls, L"STATIC") == 0 && idx == 1) // 2番目のSTATIC = ラベル
                SetWindowTextW(hChild, I18nGet(L"ui.bookshelf").c_str());
            else if (_wcsicmp(cls, L"BUTTON") == 0)
            {
                UINT id = (UINT)GetWindowLongPtrW(hChild, GWLP_ID);
                if (id == IDM_BOOKSHELF_CLEAR)
                    SetWindowTextW(hChild, I18nGet(L"ui.clearall").c_str());
            }
            if (_wcsicmp(cls, L"STATIC") == 0) idx++;
            hChild = GetWindow(hChild, GW_HWNDNEXT);
        }
    }

    // 履歴ツールバーの子コントロールテキスト更新
    if (g_app.wnd.hwndHistoryToolbar)
    {
        HWND hChild = GetWindow(g_app.wnd.hwndHistoryToolbar, GW_CHILD);
        int idx = 0;
        while (hChild)
        {
            wchar_t cls[64] = {};
            GetClassNameW(hChild, cls, 64);
            if (_wcsicmp(cls, L"STATIC") == 0 && idx == 1) // 2番目のSTATIC = ラベル
                SetWindowTextW(hChild, I18nGet(L"ui.history").c_str());
            else if (_wcsicmp(cls, L"BUTTON") == 0)
            {
                UINT id = (UINT)GetWindowLongPtrW(hChild, GWLP_ID);
                if (id == 1500) // 履歴全削除ボタンID
                    SetWindowTextW(hChild, I18nGet(L"ui.clearall").c_str());
            }
            if (_wcsicmp(cls, L"STATIC") == 0) idx++;
            hChild = GetWindow(hChild, GW_HWNDNEXT);
        }
    }

    // フィルターのキューバナー更新
    if (g_app.wnd.hwndFilterBox)
        SendMessageW(g_app.wnd.hwndFilterBox, EM_SETCUEBANNER, TRUE, (LPARAM)I18nGet(L"ui.filter").c_str());
    if (g_app.wnd.hwndHistoryFilter)
        SendMessageW(g_app.wnd.hwndHistoryFilter, EM_SETCUEBANNER, TRUE, (LPARAM)I18nGet(L"ui.filter").c_str());

    // ツリー再構築（展開状態とスクロール位置を保持）
    RefreshTree();

    // ファイルリストのカラムヘッダーを言語に合わせて更新
    if (g_app.wnd.hwndList)
    {
        LVCOLUMNW lvc = {};
        lvc.mask = LVCF_TEXT;
        lvc.pszText = (LPWSTR)I18nGet(L"list.name").c_str();
        SendMessageW(g_app.wnd.hwndList, LVM_SETCOLUMNW, 0, (LPARAM)&lvc);
        lvc.pszText = (LPWSTR)I18nGet(L"list.size").c_str();
        SendMessageW(g_app.wnd.hwndList, LVM_SETCOLUMNW, 1, (LPARAM)&lvc);
        lvc.pszText = (LPWSTR)I18nGet(L"list.type").c_str();
        SendMessageW(g_app.wnd.hwndList, LVM_SETCOLUMNW, 2, (LPARAM)&lvc);
        lvc.pszText = (LPWSTR)I18nGet(L"list.date").c_str();
        SendMessageW(g_app.wnd.hwndList, LVM_SETCOLUMNW, 3, (LPARAM)&lvc);
    }

    // ソートボタンラベル更新
    UpdateSortButtonLabels();

    // ファイルリスト再描画（種類テキスト等の更新）
    PopulateListView();

    // レイアウト更新
    LayoutChildren(g_app.wnd.hwndMain);

    // 全体再描画
    InvalidateRect(g_app.wnd.hwndMain, nullptr, TRUE);
}

void LayoutChildren(HWND hwndParent)
{
    RECT rc;
    GetClientRect(hwndParent, &rc);
    int clientW = rc.right;
    int clientH = rc.bottom;
    if (clientW <= 0 || clientH <= 0) return; // 最小化時はスキップ

    // DPI スケール値をキャッシュ（DPI変更時のみ再計算）
    static UINT cachedDpi = 0;
    static int kNavBarH = 0, kAddrBarH = 0, kSplitterW = 0, kViewerToolbarH = 0, kMinPanelSize = 0;
    UINT dpi = GetDpiForWindow(hwndParent);
    if (dpi != cachedDpi)
    {
        cachedDpi = dpi;
        kNavBarH = DpiScale(kNavBarH_96, dpi);
        kAddrBarH = DpiScale(kAddrBarH_96, dpi);
        kSplitterW = DpiScale(kSplitterW_96, dpi);
        kViewerToolbarH = DpiScale(kViewerToolbarH_96, dpi);
        kMinPanelSize = DpiScale(kMinPanelSize_96, dpi);
    }

    // 全画面時はビューアーまたはメディアプレーヤーを全面配置
    if (g_app.isFullscreen)
    {
        if (g_app.nav.isMediaMode)
        {
            if (g_app.wnd.hwndViewer) ShowWindow(g_app.wnd.hwndViewer, SW_HIDE);
            if (g_app.wnd.hwndMediaPlayer)
            {
                ShowWindow(g_app.wnd.hwndMediaPlayer, SW_SHOW);
                MoveWindow(g_app.wnd.hwndMediaPlayer, 0, 0, clientW, clientH, TRUE);
            }
        }
        else
        {
            if (g_app.wnd.hwndMediaPlayer) ShowWindow(g_app.wnd.hwndMediaPlayer, SW_HIDE);
            if (g_app.wnd.hwndViewer)
                MoveWindow(g_app.wnd.hwndViewer, 0, 0, clientW, clientH, TRUE);
        }
        return;
    }

    int y = 0;

    // ナビバー
    if (g_app.wnd.hwndNavBar)
    {
        int rightW = 0;
        if (g_app.wnd.hwndNavBarRight)
        {
            // 右ツールバーのボタン数×ボタン幅
            int btnCount = (int)SendMessageW(g_app.wnd.hwndNavBarRight, TB_BUTTONCOUNT, 0, 0);
            rightW = btnCount * 48;
            MoveWindow(g_app.wnd.hwndNavBarRight, clientW - rightW, y, rightW, kNavBarH, TRUE);
        }
        MoveWindow(g_app.wnd.hwndNavBar, 0, y, clientW - rightW, kNavBarH, TRUE);
        y += kNavBarH;
    }

    // アドレスバー
    if (g_app.wnd.hwndAddressLabel)
    {
        LayoutAddressBar(0, y, clientW, kAddrBarH);
        y += kAddrBarH;
    }

    // ステータスバー
    int statusBarH = 0;
    if (g_app.wnd.hwndStatusBar)
    {
        SendMessageW(g_app.wnd.hwndStatusBar, WM_SIZE, 0, 0);
        RECT sbrc;
        GetWindowRect(g_app.wnd.hwndStatusBar, &sbrc);
        statusBarH = sbrc.bottom - sbrc.top;
    }

    // メイン領域
    int mainH = clientH - y - statusBarH;
    if (mainH < 0) mainH = 0;
    int mainY = y;

    // スプリッター位置をクランプ
    g_app.mainSplitPos = std::max(kMinPanelSize,
                         std::min(g_app.mainSplitPos, clientW - kMinPanelSize - kSplitterW));

    int leftW = g_app.mainSplitPos;
    int rightX = leftW + kSplitterW;
    int rightW = clientW - rightX;

    // メインスプリッター（垂直バー）
    if (g_app.wnd.hwndMainSplitter)
        MoveWindow(g_app.wnd.hwndMainSplitter, leftW, mainY, kSplitterW, mainH, TRUE);

    // --- 左サイドバー ---
    constexpr int headerH = 24; // 全モード統一（通常/本棚/履歴）
    bool shelfMode = g_app.wnd.hwndBookshelfToolbar && IsWindowVisible(g_app.wnd.hwndBookshelfToolbar);

    g_app.sidebarSplitPos = std::max(kMinPanelSize,
                            std::min(g_app.sidebarSplitPos, mainH - kMinPanelSize - kSplitterW));

    int treeH = g_app.sidebarSplitPos - headerH;
    if (treeH < 0) treeH = 0;
    int listY = g_app.sidebarSplitPos + kSplitterW;
    int listH = mainH - listY;

    {
        int sortBtnW = 80;
        int sortBtnH = 22;
        int sortBtnPad = (headerH - sortBtnH) / 2; // 垂直中央
        if (!shelfMode)
        {
            if (g_app.wnd.hwndFolderLabel)
                MoveWindow(g_app.wnd.hwndFolderLabel, 0, mainY, leftW - sortBtnW, headerH, TRUE);
            if (g_app.wnd.hwndTreeSortBtn)
                MoveWindow(g_app.wnd.hwndTreeSortBtn, leftW - sortBtnW, mainY + sortBtnPad, sortBtnW, sortBtnH, TRUE);
        }
        else
        {
            MoveWindow(g_app.wnd.hwndBookshelfToolbar, 0, mainY, leftW, headerH, TRUE);
            if (g_app.wnd.hwndBookshelfSortBtn)
                MoveWindow(g_app.wnd.hwndBookshelfSortBtn, leftW - sortBtnW, sortBtnPad, sortBtnW, sortBtnH, TRUE);
        }
    }

    if (g_app.wnd.hwndTree)
        MoveWindow(g_app.wnd.hwndTree, 0, mainY + headerH, leftW, treeH, TRUE);

    if (g_app.wnd.hwndSidebarSplitter)
        MoveWindow(g_app.wnd.hwndSidebarSplitter, 0, mainY + headerH + treeH, leftW, kSplitterW, TRUE);

    int filterH = 24;
    int sbW = GetSystemMetrics(SM_CXVSCROLL);
    if (g_app.wnd.hwndFilterBox)
        MoveWindow(g_app.wnd.hwndFilterBox, 0, mainY + listY, leftW - sbW, filterH, TRUE);
    // フィルター右横のパッド（スクロールバー幅分の灰色背景）
    if (g_app.wnd.hwndFilterPad)
        MoveWindow(g_app.wnd.hwndFilterPad, leftW - sbW, mainY + listY, sbW, filterH, TRUE);

    if (g_app.wnd.hwndList)
    {
        MoveWindow(g_app.wnd.hwndList, 0, mainY + listY + filterH, leftW, listH - filterH, TRUE);
        // グリッドモード: リサイズ時にアイコンを再配置
        SendMessageW(g_app.wnd.hwndList, LVM_ARRANGE, LVA_DEFAULT, 0);
    }

    // 履歴モード: ツリー部分を履歴ツールバー+履歴ListViewに置き換え（ファイルリストは残す）
    if (g_app.wnd.hwndHistoryToolbar && IsWindowVisible(g_app.wnd.hwndHistoryToolbar))
    {
        int htbH = headerH;
        // ツリー領域（folderLabel + tree）を使う
        int histAreaH = headerH + treeH;
        MoveWindow(g_app.wnd.hwndHistoryToolbar, 0, mainY, leftW, htbH, TRUE);
        if (g_app.wnd.hwndHistoryFilter)
            MoveWindow(g_app.wnd.hwndHistoryFilter, 128, 3, leftW - 138, 22, TRUE);
        if (g_app.wnd.hwndHistoryList)
        {
            int histListH = histAreaH - htbH;
            if (histListH < 50) histListH = 50;
            MoveWindow(g_app.wnd.hwndHistoryList, 0, mainY + htbH, leftW, histListH, TRUE);
            // 列幅を動的調整
            RECT lvrc;
            GetClientRect(g_app.wnd.hwndHistoryList, &lvrc);
            int timeColW = 60;
            int nameColW = lvrc.right - timeColW;
            if (nameColW < 50) nameColW = 50;
            LVCOLUMNW lvc = {};
            lvc.mask = LVCF_WIDTH;
            lvc.cx = nameColW;
            SendMessageW(g_app.wnd.hwndHistoryList, LVM_SETCOLUMNW, 0, (LPARAM)&lvc);
            lvc.cx = timeColW;
            SendMessageW(g_app.wnd.hwndHistoryList, LVM_SETCOLUMNW, 1, (LPARAM)&lvc);
        }
    }

    // --- 右: ツールバー（左右2分割）は常に表示 ---
    auto GetToolbarWidth = [](HWND tb) -> int {
        int count = (int)SendMessageW(tb, TB_BUTTONCOUNT, 0, 0);
        if (count <= 0) return 200;
        RECT rc;
        SendMessageW(tb, TB_GETITEMRECT, count - 1, (LPARAM)&rc);
        return rc.right + 4;
    };

    if (g_app.wnd.hwndViewerTbLeft)
    {
        ShowWindow(g_app.wnd.hwndViewerTbLeft, SW_SHOW);
        int lbW = GetToolbarWidth(g_app.wnd.hwndViewerTbLeft);
        MoveWindow(g_app.wnd.hwndViewerTbLeft, rightX, mainY, lbW, kViewerToolbarH, TRUE);
    }
    if (g_app.wnd.hwndViewerTbRight)
    {
        ShowWindow(g_app.wnd.hwndViewerTbRight, SW_SHOW);
        int rbW = GetToolbarWidth(g_app.wnd.hwndViewerTbRight);
        MoveWindow(g_app.wnd.hwndViewerTbRight, rightX + rightW - rbW, mainY, rbW, kViewerToolbarH, TRUE);
    }

    int contentY = mainY + kViewerToolbarH;
    int contentH = mainH - kViewerToolbarH;

    if (g_app.nav.isMediaMode)
    {
        // メディアモード: ビューアー非表示、メディアプレーヤー表示（ツールバーの下）
        if (g_app.wnd.hwndViewer) ShowWindow(g_app.wnd.hwndViewer, SW_HIDE);
        if (g_app.wnd.hwndMediaPlayer)
        {
            ShowWindow(g_app.wnd.hwndMediaPlayer, SW_SHOW);
            MoveWindow(g_app.wnd.hwndMediaPlayer, rightX, contentY, rightW, contentH, TRUE);
        }
    }
    else
    {
        // 画像モード
        if (g_app.wnd.hwndMediaPlayer) ShowWindow(g_app.wnd.hwndMediaPlayer, SW_HIDE);
        if (g_app.wnd.hwndViewer)
        {
            ShowWindow(g_app.wnd.hwndViewer, SW_SHOW);
            MoveWindow(g_app.wnd.hwndViewer, rightX, contentY, rightW, contentH, TRUE);
            // ワーカースレッド用にビューサイズをキャッシュ（GetClientRect代替）
            g_app.viewer.cachedViewW.store(rightW, std::memory_order_relaxed);
            g_app.viewer.cachedViewH.store(contentH, std::memory_order_relaxed);
        }
    }
    LayoutViewerToolbarLabels();
}

// 日付グループ判定
static std::wstring GetDateGroup(const FILETIME& ft)
{
    SYSTEMTIME stNow, stEntry;
    GetLocalTime(&stNow);
    FILETIME ftLocal;
    FileTimeToLocalFileTime(&ft, &ftLocal);
    FileTimeToSystemTime(&ftLocal, &stEntry);

    // 日数差を計算
    SYSTEMTIME st0 = stNow; st0.wHour = st0.wMinute = st0.wSecond = st0.wMilliseconds = 0;
    FILETIME ft0; SystemTimeToFileTime(&st0, &ft0);
    ULARGE_INTEGER ulNow, ulEntry;
    ulNow.LowPart = ft0.dwLowDateTime; ulNow.HighPart = ft0.dwHighDateTime;
    ulEntry.LowPart = ftLocal.dwLowDateTime; ulEntry.HighPart = ftLocal.dwHighDateTime;

    if (ulEntry.QuadPart >= ulNow.QuadPart) return L"today";
    INT64 diffDays = (ulNow.QuadPart - ulEntry.QuadPart) / (10000000ULL * 60 * 60 * 24);
    if (diffDays < 1) return L"today";
    if (diffDays < 2) return L"yesterday";
    if (diffDays < 7) return L"thisweek";
    if (diffDays < 14) return L"lastweek";
    return L"older";
}

// 時刻フォーマット
static std::wstring FormatAccessTime(const FILETIME& ft)
{
    FILETIME ftLocal;
    SYSTEMTIME st;
    FileTimeToLocalFileTime(&ft, &ftLocal);
    FileTimeToSystemTime(&ftLocal, &st);

    wchar_t buf[32];
    SYSTEMTIME stNow; GetLocalTime(&stNow);
    if (st.wYear == stNow.wYear && st.wMonth == stNow.wMonth && st.wDay == stNow.wDay)
        swprintf_s(buf, _countof(buf), L"%02d:%02d", st.wHour, st.wMinute);
    else
        swprintf_s(buf, _countof(buf), L"%02d/%02d %02d:%02d", st.wMonth, st.wDay, st.wHour, st.wMinute);
    return buf;
}

std::wstring g_historyFilterText;

void BuildHistoryList()
{
    HWND hList = g_app.wnd.hwndHistoryList;
    if (!hList) return;

    SendMessageW(hList, WM_SETREDRAW, FALSE, 0);
    SendMessageW(hList, LVM_DELETEALLITEMS, 0, 0);
    // グループ削除
    SendMessageW(hList, LVM_REMOVEALLGROUPS, 0, 0);

    // グループ作成
    struct { int id; const wchar_t* name; } groups[] = {
        { 0, I18nGet(L"group.today").c_str() }, { 1, I18nGet(L"group.yesterday").c_str() }, { 2, I18nGet(L"group.thisweek").c_str() }, { 3, I18nGet(L"group.lastweek").c_str() }, { 4, I18nGet(L"group.older").c_str() }
    };
    for (auto& g : groups)
    {
        LVGROUP lvg = {};
        lvg.cbSize = sizeof(lvg);
        lvg.mask = LVGF_HEADER | LVGF_GROUPID;
        lvg.pszHeader = (LPWSTR)g.name;
        lvg.iGroupId = g.id;
        SendMessageW(hList, LVM_INSERTGROUP, (WPARAM)-1, (LPARAM)&lvg);
    }

    auto& hist = HistoryGetAll();
    int idx = 0;
    for (auto& entry : hist)
    {
        // フィルター
        if (!g_historyFilterText.empty())
        {
            std::wstring lowerName = ToLowerW(entry.name);
            std::wstring lowerFilter = ToLowerW(g_historyFilterText);
            if (lowerName.find(lowerFilter) == std::wstring::npos) continue;
        }

        // 日付グループ
        std::wstring group = GetDateGroup(entry.accessTime);
        int groupId = (group == L"today") ? 0 : (group == L"yesterday") ? 1 :
                      (group == L"thisweek") ? 2 : (group == L"lastweek") ? 3 : 4;

        // アイコン
        int icon = GetFileIconIndex(entry.name, false);

        // 列0: ファイル名
        LVITEMW lvi = {};
        lvi.mask = LVIF_TEXT | LVIF_GROUPID | LVIF_IMAGE | LVIF_PARAM;
        lvi.iItem = idx;
        lvi.pszText = (LPWSTR)entry.name.c_str();
        lvi.iGroupId = groupId;
        lvi.iImage = icon;
        lvi.lParam = (LPARAM)idx;
        int insertedIdx = (int)SendMessageW(hList, LVM_INSERTITEMW, 0, (LPARAM)&lvi);

        // 列1: 時刻（挿入された実際のインデックスを使用）
        if (insertedIdx >= 0)
        {
            std::wstring timeStr = FormatAccessTime(entry.accessTime);
            LVITEMW lvi2 = {};
            lvi2.iSubItem = 1;
            lvi2.pszText = (LPWSTR)timeStr.c_str();
            SendMessageW(hList, LVM_SETITEMTEXTW, insertedIdx, (LPARAM)&lvi2);
        }
        idx++;
    }

    // 列幅を調整（名前=残り幅、時刻=80px）
    RECT rc;
    GetClientRect(hList, &rc);
    int timeColW = 60;
    LVCOLUMNW lvc = {};
    lvc.mask = LVCF_WIDTH;
    lvc.cx = rc.right - timeColW;
    SendMessageW(hList, LVM_SETCOLUMNW, 0, (LPARAM)&lvc);
    lvc.cx = timeColW;
    SendMessageW(hList, LVM_SETCOLUMNW, 1, (LPARAM)&lvc);

    SendMessageW(hList, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hList, nullptr, TRUE);
}


static void HandleNotify(HWND hwnd, LPNMHDR pnm)
{
    // 履歴ListView選択変更
    if (pnm->hwndFrom == g_app.wnd.hwndHistoryList && pnm->code == LVN_ITEMCHANGED)
    {
        auto* pnmv = (LPNMLISTVIEW)pnm;
        if ((pnmv->uNewState & LVIS_SELECTED) && !(pnmv->uOldState & LVIS_SELECTED))
        {
            // 選択されたアイテムのインデックスから履歴エントリを取得
            // フィルター適用時はlParamからマッピング不要、表示名から検索
            wchar_t buf[512] = {};
            LVITEMW lvi = {};
            lvi.mask = LVIF_TEXT;
            lvi.iItem = pnmv->iItem;
            lvi.pszText = buf;
            lvi.cchTextMax = 512;
            SendMessageW(g_app.wnd.hwndHistoryList, LVM_GETITEMTEXTW, pnmv->iItem, (LPARAM)&lvi);

            // 表示名からタブ以前を取得して履歴から検索
            std::wstring displayName = buf;
            auto tabPos = displayName.find(L'\t');
            if (tabPos != std::wstring::npos) displayName = displayName.substr(0, tabPos);

            auto& hist = HistoryGetAll();
            for (auto& entry : hist)
            {
                if (entry.name == displayName)
                {
                    NavigateTo(entry.path);
                    break;
                }
            }
        }
        return;
    }

    // ナビバー ツールチップ
    // ツールチップ共通ヘルパー: アクション名からショートカットキー文字列を取得
    auto TipWithKey = [](const std::wstring& label, const wchar_t* actionName) -> std::wstring {
        for (auto& kb : GetKeyBindings()) {
            if (kb.action == actionName && !kb.keys.empty()) {
                return label + L" (" + KeyBindingToString(kb) + L")";
            }
        }
        return label;
    };

    if (pnm->hwndFrom == g_app.wnd.hwndNavBar && pnm->code == TBN_GETINFOTIPW)
    {
        auto* tip = (LPNMTBGETINFOTIPW)pnm;
        std::wstring s;
        switch (tip->iItem)
        {
        case IDM_NAV_BACK:      s = TipWithKey(I18nGet(L"nav.back"), L"nav_back"); break;
        case IDM_NAV_FORWARD:   s = TipWithKey(I18nGet(L"nav.forward"), L"nav_forward"); break;
        case IDM_NAV_UP:        s = TipWithKey(I18nGet(L"nav.up"), L"nav_up"); break;
        case IDM_NAV_REFRESH:   s = TipWithKey(I18nGet(L"nav.refresh"), L"refresh"); break;
        case IDM_NAV_BOOKSHELF: s = TipWithKey(I18nGet(L"ui.bookshelf"), L"bookshelf"); break;
        case IDM_NAV_HISTORY:   s = TipWithKey(I18nGet(L"ui.history"), L"history"); break;
        case IDM_NAV_LIST:      s = TipWithKey(I18nGet(L"nav.list"), L"list_view"); break;
        case IDM_NAV_GRID:      s = TipWithKey(I18nGet(L"nav.grid"), L"grid_view"); break;
        case IDM_NAV_HOVER:     s = TipWithKey(I18nGet(L"nav.hover"), L"hover_preview"); break;
        case IDM_NAV_SETTINGS:  s = TipWithKey(I18nGet(L"ui.settings"), L"settings"); break;
        case IDM_NAV_HELP:      s = TipWithKey(I18nGet(L"ui.help"), L"help"); break;
        }
        if (!s.empty() && tip->pszText && tip->cchTextMax > 0)
            wcsncpy_s(tip->pszText, tip->cchTextMax, s.c_str(), _TRUNCATE);
        return;
    }

    // ビューアーツールバー ツールチップ
    if ((pnm->hwndFrom == g_app.wnd.hwndViewerTbLeft || pnm->hwndFrom == g_app.wnd.hwndViewerTbRight) && pnm->code == TBN_GETINFOTIPW)
    {
        auto* tip = (LPNMTBGETINFOTIPW)pnm;
        std::wstring s;
        switch (tip->iItem)
        {
        case IDM_VIEW_FIRST:      s = TipWithKey(L"最初のページ", L"first_page"); break;
        case IDM_VIEW_PREV:       s = TipWithKey(L"前のページ", L"prev_page"); break;
        case IDM_VIEW_NEXT:       s = TipWithKey(L"次のページ", L"next_page"); break;
        case IDM_VIEW_LAST:       s = TipWithKey(L"最後のページ", L"last_page"); break;
        case IDM_VIEW_FIT_WINDOW: s = TipWithKey(I18nGet(L"kb.fit_window"), L"fit_window"); break;
        case IDM_VIEW_FIT_WIDTH:  s = TipWithKey(I18nGet(L"kb.fit_width"), L"fit_width"); break;
        case IDM_VIEW_FIT_HEIGHT: s = TipWithKey(I18nGet(L"kb.fit_height"), L"fit_height"); break;
        case IDM_VIEW_ORIGINAL:   s = TipWithKey(I18nGet(L"kb.original"), L"original_size"); break;
        case IDM_VIEW_ZOOMIN:     s = TipWithKey(I18nGet(L"kb.zoom_in"), L"zoom_in"); break;
        case IDM_VIEW_ZOOMOUT:    s = TipWithKey(I18nGet(L"kb.zoom_out"), L"zoom_out"); break;
        case IDM_VIEW_AUTO:       s = TipWithKey(I18nGet(L"kb.view_auto"), L"view_auto"); break;
        case IDM_VIEW_SINGLE:     s = TipWithKey(I18nGet(L"kb.view_single"), L"view_single"); break;
        case IDM_VIEW_SPREAD:     s = TipWithKey(I18nGet(L"kb.view_spread"), L"view_spread"); break;
        case IDM_VIEW_BINDING:    s = TipWithKey(I18nGet(L"kb.binding"), L"toggle_binding"); break;
        case IDM_VIEW_ROTATE_CW:  s = TipWithKey(I18nGet(L"kb.rotate"), L"rotate_cw"); break;
        }
        if (!s.empty())
            wcsncpy_s(tip->pszText, tip->cchTextMax, s.c_str(), _TRUNCATE);
        return;
    }
    if (pnm->hwndFrom == g_app.wnd.hwndNavBar)
    {
        if (pnm->code == TBN_DROPDOWN)
        {
            auto pnmtb = (LPNMTOOLBARW)pnm;
            POINT pt; GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();

            if (pnmtb->iItem == IDM_NAV_BACK)
            {
                // 戻る履歴ドロップダウン
                auto& back = g_app.nav.historyBack;
                int count = std::min((int)back.size(), 20);
                for (int i = 0; i < count; i++)
                {
                    int idx = (int)back.size() - 1 - i;
                    const auto& p = back[idx];
                    auto pos = p.find_last_of(L'\\');
                    std::wstring name = (pos != std::wstring::npos) ? p.substr(pos + 1) : p;
                    AppendMenuW(hMenu, MF_STRING, IDM_HIST_DROPDOWN_BACK + i, name.c_str());
                }

                if (count > 0)
                {
                    UINT cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
                    if (cmd >= IDM_HIST_DROPDOWN_BACK && cmd < IDM_HIST_DROPDOWN_BACK + count)
                    {
                        int idx = (int)back.size() - 1 - (cmd - IDM_HIST_DROPDOWN_BACK);
                        std::wstring path = back[idx];
                        for (int j = (int)back.size() - 1; j > idx; j--)
                        {
                            g_app.nav.historyForward.push_back(back[j]);
                            back.pop_back();
                        }
                        if (g_app.nav.inArchiveMode && !g_app.nav.currentArchive.empty())
                            g_app.nav.historyForward.push_back(g_app.nav.currentArchive);
                        back.pop_back();
                        NavigateTo(path, { false, true });
                    }
                }
            }
            else if (pnmtb->iItem == IDM_NAV_FORWARD)
            {
                // 進む履歴ドロップダウン
                auto& fwd = g_app.nav.historyForward;
                int count = std::min((int)fwd.size(), 20);
                for (int i = 0; i < count; i++)
                {
                    int idx = (int)fwd.size() - 1 - i;
                    const auto& p = fwd[idx];
                    auto pos = p.find_last_of(L'\\');
                    std::wstring name = (pos != std::wstring::npos) ? p.substr(pos + 1) : p;
                    AppendMenuW(hMenu, MF_STRING, IDM_HIST_DROPDOWN_FWD + i, name.c_str());
                }

                if (count > 0)
                {
                    UINT cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
                    if (cmd >= IDM_HIST_DROPDOWN_FWD && cmd < IDM_HIST_DROPDOWN_FWD + count)
                    {
                        int idx = (int)fwd.size() - 1 - (cmd - IDM_HIST_DROPDOWN_FWD);
                        std::wstring path = fwd[idx];
                        if (g_app.nav.inArchiveMode && !g_app.nav.currentArchive.empty())
                            g_app.nav.historyBack.push_back(g_app.nav.currentArchive);
                        for (int j = (int)fwd.size() - 1; j > idx; j--)
                        {
                            g_app.nav.historyBack.push_back(fwd[j]);
                            fwd.pop_back();
                        }
                        fwd.pop_back();
                        NavigateTo(path, { false, true });
                    }
                }
            }
            DestroyMenu(hMenu);
            return;
        }
    }
    else if (pnm->hwndFrom == g_app.wnd.hwndTree)
    {
        if (pnm->code == TVN_ITEMEXPANDINGW)
        {
            auto pnmtv = (LPNMTREEVIEWW)pnm;
            if (pnmtv->action == TVE_EXPAND)
            {
                ExpandTreeNode(pnmtv->itemNew.hItem);
                // 展開後、展開したアイテムが見えるようにする（スクロール位置は変えない）
            }
        }
        else if (pnm->code == TVN_SELCHANGEDW)
        {
            if (!g_app.isRevealing)
            {
                auto pnmtv = (LPNMTREEVIEWW)pnm;
                std::wstring path = GetTreeItemPath(pnmtv->itemNew.hItem);

                // 本棚モード: CAT/ITEM タグ処理
                if (GetTreeMode() == 1 && !path.empty())
                {
                    if (path == L"BOOKSHELF_ROOT")
                    {
                        // ルートノード: 何もしない
                    }
                    else if (path.size() > 4 && path.substr(0, 4) == L"CAT:")
                    {
                        // カテゴリ選択: アイテムをListViewに表示
                        std::wstring catId = path.substr(4);
                        g_app.nav.fileItems.clear();
                        g_app.nav.viewableFiles.clear();
                        auto& cats = BookshelfGetCategories();
                        for (auto& cat : cats)
                        {
                            if (cat.id == catId)
                            {
                                for (auto& item : cat.items)
                                {
                                    FileItem fi;
                                    fi.name = item.name;
                                    fi.fullPath = item.path;
                                    DWORD attr = GetFileAttributesW(item.path.c_str());
                                    fi.isDirectory = (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
                                    fi.fileSize = 0;
                                    fi.lastWriteTime = {};
                                    g_app.nav.fileItems.push_back(std::move(fi));
                                }
                                UpdateAddressBar(cat.name);
                                break;
                            }
                        }
                        PopulateListView();
                    }
                    else if (path.size() > 5 && path.substr(0, 5) == L"ITEM:")
                    {
                        std::wstring itemPath = path.substr(5);
                        NavigateTo(itemPath);
                    }
                }
                // 通常モード: デバウンス
                else if (!path.empty())
                {
                    // お気に入り子ノード: ツリー展開を抑止（leeyez_kai準拠）
                    if (GetTreeMode() == 0 && IsFavoritesChild(pnmtv->itemNew.hItem))
                    {
                        NavigateTo(path, { true, false });
                    }
                    else
                    {
                        g_pendingTreePath = path;
                        SetTimer(g_app.wnd.hwndMain, kTreeNavTimer, 200, nullptr);
                    }
                }
            }
        }
        else if (pnm->code == TVN_DELETEITEMW)
        {
            auto pnmtv = (LPNMTREEVIEWW)pnm;
            delete[] (wchar_t*)pnmtv->itemOld.lParam;
        }
        else if (pnm->code == NM_RCLICK)
        {
            // ツリー右クリック
            POINT pt; GetCursorPos(&pt);
            POINT clientPt = pt;
            ScreenToClient(g_app.wnd.hwndTree, &clientPt);
            TVHITTESTINFO ht = {}; ht.pt = clientPt;
            HTREEITEM hItem = (HTREEITEM)SendMessageW(g_app.wnd.hwndTree, TVM_HITTEST, 0, (LPARAM)&ht);
            g_treeRClickItem = hItem; // 右クリックしたアイテムを記憶
            if (hItem)
            {
                std::wstring treePath = GetTreeItemPath(hItem);
                // 本棚モード
                if (GetTreeMode() == 1 && treePath.size() > 5 && treePath.substr(0, 5) == L"ITEM:")
                {
                    treePath = treePath.substr(5);
                    ShowFileContextMenu(hwnd, treePath, pt);
                }
                else if (GetTreeMode() == 1 && treePath.size() > 4 && treePath.substr(0, 4) == L"CAT:")
                {
                    // カテゴリ右クリック: 本棚削除/名前変更メニュー
                    HMENU hMenu = CreatePopupMenu();
                    AppendMenuW(hMenu, MF_STRING, 1, I18nGet(L"ctx.deleteshelf").c_str());
                    AppendMenuW(hMenu, MF_STRING, 2, I18nGet(L"ui.renamethis").c_str());
                    UINT cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, nullptr);
                    DestroyMenu(hMenu);
                    if (cmd == 1)
                    {
                        std::wstring catId = treePath.substr(4);
                        BookshelfRemoveCategory(catId);
                        ShowBookshelfTree();
                    }
                    else if (cmd == 2)
                    {
                        // インラインラベル編集を開始（右クリックしたノードを直接編集）
                        SendMessageW(g_app.wnd.hwndTree, TVM_EDITLABEL, 0, (LPARAM)g_treeRClickItem);
                    }
                }
                else if (GetTreeMode() == 1 && treePath == L"BOOKSHELF_ROOT")
                {
                    // ルートは何もしない
                }
                else if (!treePath.empty())
                {
                    ShowFileContextMenu(hwnd, treePath, pt);
                }
            }
        }
        else if (pnm->code == TVN_BEGINLABELEDITW)
        {
            auto pdi = (LPNMTVDISPINFOW)pnm;
            std::wstring tag = GetTreeItemPath(pdi->item.hItem);
            // 本棚カテゴリ: 編集許可
            if (GetTreeMode() == 1 && tag.size() > 4 && tag.substr(0, 4) == L"CAT:")
            {
                SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, FALSE);
            }
            // 本棚ITEM:ノード: 実ファイル/フォルダのリネーム許可
            else if (GetTreeMode() == 1 && tag.size() > 5 && tag.substr(0, 5) == L"ITEM:")
            {
                std::wstring realPath = tag.substr(5);
                if (!PathIsRootW(realPath.c_str()))
                {
                    SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, FALSE);
                    HWND hEdit = (HWND)SendMessageW(g_app.wnd.hwndTree, TVM_GETEDITCONTROL, 0, 0);
                    if (hEdit)
                    {
                        std::wstring name = realPath;
                        auto sl = name.find_last_of(L'\\');
                        if (sl != std::wstring::npos) name = name.substr(sl + 1);
                        auto dot = name.find_last_of(L'.');
                        if (dot != std::wstring::npos)
                            PostMessageW(hEdit, EM_SETSEL, 0, (LPARAM)dot);
                    }
                }
                else
                    SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, TRUE);
            }
            // 通常ファイル/フォルダ: 特殊フォルダ・ドライブ以外は編集許可
            else if (!tag.empty() && !PathIsRootW(tag.c_str()) && tag != L"BOOKSHELF_ROOT"
                && !(tag.size() > 4 && tag.substr(0, 4) == L"CAT:"))
            {
                // 特殊フォルダチェック
                bool isSpecial = false;
                static const KNOWNFOLDERID specialFolders[] = {
                    FOLDERID_Desktop, FOLDERID_Downloads, FOLDERID_Documents,
                    FOLDERID_Pictures, FOLDERID_Videos, FOLDERID_Music
                };
                for (auto& fid : specialFolders)
                {
                    wchar_t* knownPath = nullptr;
                    if (SUCCEEDED(SHGetKnownFolderPath(fid, 0, nullptr, &knownPath)))
                    {
                        if (_wcsicmp(tag.c_str(), knownPath) == 0) isSpecial = true;
                        CoTaskMemFree(knownPath);
                        if (isSpecial) break;
                    }
                }
                SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, isSpecial ? TRUE : FALSE);
                if (!isSpecial)
                {
                    // 編集開始時に拡張子前まで選択
                    HWND hEdit = (HWND)SendMessageW(g_app.wnd.hwndTree, TVM_GETEDITCONTROL, 0, 0);
                    if (hEdit)
                    {
                        std::wstring name = tag;
                        auto sl = name.find_last_of(L'\\');
                        if (sl != std::wstring::npos) name = name.substr(sl + 1);
                        auto dot = name.find_last_of(L'.');
                        if (dot != std::wstring::npos)
                            PostMessageW(hEdit, EM_SETSEL, 0, (LPARAM)dot);
                    }
                }
            }
            else
            {
                SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, TRUE); // 編集拒否
            }
        }
        else if (pnm->code == TVN_ENDLABELEDITW)
        {
            auto pdi = (LPNMTVDISPINFOW)pnm;
            if (pdi->item.pszText && pdi->item.pszText[0] != L'\0')
            {
                std::wstring tag = GetTreeItemPath(pdi->item.hItem);
                // 本棚カテゴリのリネーム
                if (tag.size() > 4 && tag.substr(0, 4) == L"CAT:")
                {
                    std::wstring catId = tag.substr(4);
                    BookshelfRenameCategory(catId, pdi->item.pszText);
                    SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, TRUE);
                }
                // 本棚ITEM:ノードのリネーム（実ファイル/フォルダ）
                else if (tag.size() > 5 && tag.substr(0, 5) == L"ITEM:")
                {
                    std::wstring realPath = tag.substr(5);
                    std::wstring parentDir = realPath;
                    auto lastSlash = parentDir.find_last_of(L'\\');
                    if (lastSlash != std::wstring::npos)
                        parentDir = parentDir.substr(0, lastSlash + 1);
                    std::wstring newPath = parentDir + pdi->item.pszText;

                    // 書庫ファイルのロック解放
                    if (IsArchiveFile(realPath))
                        CloseCurrentArchive();
                    if (_wcsicmp(realPath.c_str(), g_app.nav.currentPath.c_str()) == 0)
                    {
                        ViewerStopAnimation();
                        g_app.viewer.bitmap.Reset();
                        g_app.viewer.bitmap2.Reset();
                        MediaStop();
                    }

                    if (MoveFileW(realPath.c_str(), newPath.c_str()))
                    {
                        // ツリーのlParamを新パスに更新
                        std::wstring newTag = L"ITEM:" + newPath;
                        wchar_t* newStr = new wchar_t[newTag.size() + 1];
                        wcscpy_s(newStr, newTag.size() + 1, newTag.c_str());
                        TVITEMW tvi = {};
                        tvi.mask = TVIF_PARAM;
                        tvi.hItem = pdi->item.hItem;
                        SendMessageW(g_app.wnd.hwndTree, TVM_GETITEMW, 0, (LPARAM)&tvi);
                        delete[] (wchar_t*)tvi.lParam;
                        tvi.lParam = (LPARAM)newStr;
                        SendMessageW(g_app.wnd.hwndTree, TVM_SETITEMW, 0, (LPARAM)&tvi);
                        // 本棚データも更新
                        BookshelfUpdateItemPath(realPath, newPath);
                        SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, TRUE);
                        LoadFolder(g_app.nav.currentFolder);
                        PopulateListView();
                    }
                    else
                        SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, FALSE);
                }
                // 通常ファイル/フォルダのリネーム
                else if (!tag.empty() && !PathIsRootW(tag.c_str()))
                {
                    std::wstring parentDir = tag;
                    auto lastSlash = parentDir.find_last_of(L'\\');
                    if (lastSlash != std::wstring::npos)
                        parentDir = parentDir.substr(0, lastSlash + 1);
                    std::wstring newPath = parentDir + pdi->item.pszText;

                    // 書庫ファイルのロック解放
                    if (IsArchiveFile(tag))
                        CloseCurrentArchive();
                    // 表示中のファイルのロック解放
                    if (_wcsicmp(tag.c_str(), g_app.nav.currentPath.c_str()) == 0)
                    {
                        ViewerStopAnimation();
                        g_app.viewer.bitmap.Reset();
                        g_app.viewer.bitmap2.Reset();
                        MediaStop();
                    }

                    if (MoveFileW(tag.c_str(), newPath.c_str()))
                    {
                        // ツリーのlParamを新パスに更新
                        wchar_t* newStr = new wchar_t[newPath.size() + 1];
                        wcscpy_s(newStr, newPath.size() + 1, newPath.c_str());
                        TVITEMW tvi = {};
                        tvi.mask = TVIF_PARAM;
                        tvi.hItem = pdi->item.hItem;
                        SendMessageW(g_app.wnd.hwndTree, TVM_GETITEMW, 0, (LPARAM)&tvi);
                        delete[] (wchar_t*)tvi.lParam;
                        tvi.lParam = (LPARAM)newStr;
                        SendMessageW(g_app.wnd.hwndTree, TVM_SETITEMW, 0, (LPARAM)&tvi);

                        SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, TRUE);
                        // ファイルリスト・ツリー更新
                        LoadFolder(g_app.nav.currentFolder);
                        PopulateListView();
                        RefreshTree();
                    }
                    else
                    {
                        SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, FALSE);
                    }
                }
            }
        }
    }
    else if (pnm->hwndFrom == g_app.wnd.hwndList)
    {
        if (pnm->code == LVN_GETDISPINFOW)
        {
            // 仮想リスト: 要求されたアイテムのデータを返す
            auto* pdi = (NMLVDISPINFOW*)pnm;
            int idx = pdi->item.iItem;
            if (idx >= 0 && idx < (int)g_app.nav.fileItems.size())
            {
                auto& item = g_app.nav.fileItems[idx];

                if (pdi->item.mask & LVIF_TEXT)
                {
                    // スレッドローカル static バッファで文字列を返す
                    static std::wstring s_sizeBuf, s_typeBuf, s_dateBuf;
                    switch (pdi->item.iSubItem)
                    {
                    case 0: pdi->item.pszText = (LPWSTR)item.name.c_str(); break;
                    case 1:
                        s_sizeBuf = item.isDirectory ? L"" : FormatFileSize(item.fileSize);
                        pdi->item.pszText = (LPWSTR)s_sizeBuf.c_str();
                        break;
                    case 2:
                        s_typeBuf = GetFileType(item.name, item.isDirectory);
                        pdi->item.pszText = (LPWSTR)s_typeBuf.c_str();
                        break;
                    case 3:
                        s_dateBuf = FormatFileTime(item.lastWriteTime);
                        pdi->item.pszText = (LPWSTR)s_dateBuf.c_str();
                        break;
                    }
                }
                if (pdi->item.mask & LVIF_IMAGE)
                {
                    int thumbIdx = GetThumbnailIndex(idx);
                    if (thumbIdx >= 0)
                        pdi->item.iImage = thumbIdx; // グリッドモード: サムネイルImageListのインデックス
                    else
                        pdi->item.iImage = GetFileIconIndex(item.name, item.isDirectory); // リストモード: システムImageList
                }
            }
            return;
        }
        else if (pnm->code == LVN_ITEMCHANGED)
        {
            // シングルクリック（選択変更）→ 画像ならプレビュー表示
            if (g_app.suppressListNotify) return;

            auto pnmlv = (LPNMLISTVIEW)pnm;
            if ((pnmlv->uNewState & LVIS_SELECTED) && !(pnmlv->uOldState & LVIS_SELECTED))
            {
                int sel = pnmlv->iItem;
                if (sel >= 0 && sel < (int)g_app.nav.fileItems.size())
                {
                    auto& item = g_app.nav.fileItems[sel];
                    if (!item.isDirectory && (IsImageFile(item.name) || IsMediaFile(item.name)))
                    {
                        g_app.suppressListNotify = true;
                        for (int i = 0; i < (int)g_app.nav.viewableFiles.size(); i++)
                        {
                            if (_wcsicmp(g_app.nav.viewableFiles[i].c_str(), item.fullPath.c_str()) == 0)
                            {
                                GoToFile(i);
                                break;
                            }
                        }
                        g_app.suppressListNotify = false;
                    }
                }
            }
        }
        else if (pnm->code == NM_DBLCLK)
        {
            // ダブルクリック: フォルダ/書庫ならナビゲート
            int sel = (int)SendMessageW(g_app.wnd.hwndList, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
            if (sel >= 0 && sel < (int)g_app.nav.fileItems.size())
            {
                auto& item = g_app.nav.fileItems[sel];
                if (item.isDirectory || IsArchiveFile(item.name))
                {
                    std::wstring navPath = item.fullPath;
                    NavigateTo(navPath);
                }
            }
        }
        else if (pnm->code == LVN_COLUMNCLICK)
        {
            auto pnmlv = (LPNMLISTVIEW)pnm;
            SortFileList(pnmlv->iSubItem);
        }
        // LVN_ENDLABELEDITW は LVS_OWNERDATA では不要（F2でダイアログ名前変更）
        else if (pnm->code == NM_RCLICK)
        {
            // ファイルリスト右クリック
            int sel = (int)SendMessageW(g_app.wnd.hwndList, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
            if (sel >= 0 && sel < (int)g_app.nav.fileItems.size())
            {
                POINT pt; GetCursorPos(&pt);
                ShowFileContextMenu(hwnd, g_app.nav.fileItems[sel].fullPath, pt);
            }
        }
    }
}

void SetMainTitle(const std::wstring& path)
{
    std::wstring title = L"karikari - " + path;
    SetWindowTextW(g_app.wnd.hwndMain, title.c_str());
}

void ToggleFullscreen(HWND hwnd)
{
    if (!g_app.isFullscreen)
    {
        // 全画面へ
        g_app.savedStyle = GetWindowLongW(hwnd, GWL_STYLE);
        g_app.savedExStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
        GetWindowRect(hwnd, &g_app.savedRect);

        // モニター情報取得
        HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = {}; mi.cbSize = sizeof(mi);
        GetMonitorInfoW(hMon, &mi);

        SetWindowLongW(hwnd, GWL_STYLE, g_app.savedStyle & ~(WS_CAPTION | WS_THICKFRAME));
        SetWindowPos(hwnd, HWND_TOP,
                     mi.rcMonitor.left, mi.rcMonitor.top,
                     mi.rcMonitor.right - mi.rcMonitor.left,
                     mi.rcMonitor.bottom - mi.rcMonitor.top,
                     SWP_FRAMECHANGED);

        // 全UI非表示（メディアプレーヤー/ビューアーはLayoutChildrenで制御）
        for (HWND h : { g_app.wnd.hwndNavBar, g_app.wnd.hwndNavBarRight,
                        g_app.wnd.hwndAddressLabel, g_app.wnd.hwndAddressEdit,
                        g_app.wnd.hwndFolderLabel, g_app.wnd.hwndTreeSortBtn,
                        g_app.wnd.hwndTree, g_app.wnd.hwndFilterBox, g_app.wnd.hwndFilterPad,
                        g_app.wnd.hwndList, g_app.wnd.hwndMainSplitter,
                        g_app.wnd.hwndSidebarSplitter, g_app.wnd.hwndViewerTbLeft,
                        g_app.wnd.hwndViewerTbRight, g_app.wnd.hwndStatusBar,
                        g_app.wnd.hwndBookshelfToolbar, g_app.wnd.hwndBookshelfSortBtn,
                        g_app.wnd.hwndHistoryToolbar, g_app.wnd.hwndHistoryList,
                        g_app.wnd.hwndHistoryFilter })
            if (h) ShowWindow(h, SW_HIDE);

        g_app.isFullscreen = true;
    }
    else
    {
        // 全画面解除
        SetWindowLongW(hwnd, GWL_STYLE, g_app.savedStyle);
        SetWindowLongW(hwnd, GWL_EXSTYLE, g_app.savedExStyle);
        SetWindowPos(hwnd, nullptr,
                     g_app.savedRect.left, g_app.savedRect.top,
                     g_app.savedRect.right - g_app.savedRect.left,
                     g_app.savedRect.bottom - g_app.savedRect.top,
                     SWP_FRAMECHANGED | SWP_NOZORDER);

        // 共通UI再表示
        for (HWND h : { g_app.wnd.hwndNavBar, g_app.wnd.hwndNavBarRight,
                        g_app.wnd.hwndAddressLabel, g_app.wnd.hwndAddressEdit,
                        g_app.wnd.hwndSidebarSplitter, g_app.wnd.hwndFilterBox, g_app.wnd.hwndFilterPad,
                        g_app.wnd.hwndList, g_app.wnd.hwndMainSplitter,
                        g_app.wnd.hwndViewerTbLeft, g_app.wnd.hwndViewerTbRight,
                        g_app.wnd.hwndStatusBar })
            if (h) ShowWindow(h, SW_SHOW);
        // モード別UI再表示
        if (GetTreeMode() == 0)
        {
            // 通常モード
            if (g_app.wnd.hwndFolderLabel) ShowWindow(g_app.wnd.hwndFolderLabel, SW_SHOW);
            if (g_app.wnd.hwndTreeSortBtn) ShowWindow(g_app.wnd.hwndTreeSortBtn, SW_SHOW);
            if (g_app.wnd.hwndTree) ShowWindow(g_app.wnd.hwndTree, SW_SHOW);
        }
        else if (GetTreeMode() == 1)
        {
            // 本棚モード
            if (g_app.wnd.hwndBookshelfToolbar) ShowWindow(g_app.wnd.hwndBookshelfToolbar, SW_SHOW);
            if (g_app.wnd.hwndTree) ShowWindow(g_app.wnd.hwndTree, SW_SHOW);
        }
        else if (GetTreeMode() == 2)
        {
            // 履歴モード
            if (g_app.wnd.hwndHistoryToolbar) ShowWindow(g_app.wnd.hwndHistoryToolbar, SW_SHOW);
            if (g_app.wnd.hwndHistoryList) ShowWindow(g_app.wnd.hwndHistoryList, SW_SHOW);
            if (g_app.wnd.hwndHistoryFilter) ShowWindow(g_app.wnd.hwndHistoryFilter, SW_SHOW);
        }

        g_app.isFullscreen = false;
    }
    LayoutChildren(hwnd);
}

static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;

        CreateNavBar(hwnd, hInst);
        CreateNavBarRight(hwnd, hInst);
        // 本棚・履歴・ホバーボタンをチェック可能に（モード中ハイライト）
        for (UINT cmd : { IDM_NAV_HOVER, IDM_NAV_BOOKSHELF, IDM_NAV_HISTORY })
        {
            TBBUTTONINFOW tbi = {};
            tbi.cbSize = sizeof(tbi);
            tbi.dwMask = TBIF_STYLE;
            SendMessageW(g_app.wnd.hwndNavBar, TB_GETBUTTONINFOW, cmd, (LPARAM)&tbi);
            tbi.fsStyle |= BTNS_CHECK;
            SendMessageW(g_app.wnd.hwndNavBar, TB_SETBUTTONINFOW, cmd, (LPARAM)&tbi);
        }
        CreateAddressBar(hwnd, hInst);
        CreateStatusBar(hwnd, hInst);

        // 「フォルダ」ラベル（TreeViewの上）
        g_app.wnd.hwndFolderLabel = CreateWindowExW(
            0, L"STATIC", I18nGet(L"ui.folder").c_str(),
            WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
            0, 0, 100, 20,
            hwnd, nullptr, hInst, nullptr);
        {
            static HFONT hLabelFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
            SendMessageW(g_app.wnd.hwndFolderLabel, WM_SETFONT, (WPARAM)hLabelFont, TRUE);
        }

        // ツリーソートボタン（フォルダラベル右端に配置、ソート名表示）
        g_app.wnd.hwndTreeSortBtn = CreateWindowExW(
            0, L"BUTTON", L"",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT,
            0, 0, 80, 22,
            hwnd, (HMENU)(UINT_PTR)IDM_TREE_SORT, hInst, nullptr);
        if (g_uiFont)
            SendMessageW(g_app.wnd.hwndTreeSortBtn, WM_SETFONT, (WPARAM)g_uiFont, TRUE);

        // 本棚ツールバー（本棚モード時のみ表示: アイコン+ラベル+全削除+ソート）
        {
            static HFONT hShelfFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
            g_app.wnd.hwndBookshelfToolbar = CreateWindowExW(0, L"STATIC", L"",
                WS_CHILD | SS_NOTIFY, 0, 0, 100, 28, hwnd, nullptr, hInst, nullptr);

            HWND hIcon = CreateWindowExW(0, L"STATIC", L"\uE82D",
                WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, 4, 3, 20, 22, g_app.wnd.hwndBookshelfToolbar, nullptr, hInst, nullptr);
            static HFONT hIconFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, ANTIALIASED_QUALITY, 0, L"Segoe Fluent Icons");
            SendMessageW(hIcon, WM_SETFONT, (WPARAM)(hIconFont ? hIconFont : hShelfFont), TRUE);

            HWND hLabel = CreateWindowExW(0, L"STATIC", I18nGet(L"ui.bookshelf").c_str(),
                WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, 26, 3, 40, 22, g_app.wnd.hwndBookshelfToolbar, nullptr, hInst, nullptr);
            SendMessageW(hLabel, WM_SETFONT, (WPARAM)hShelfFont, TRUE);

            HWND hClear = CreateWindowExW(0, L"BUTTON", I18nGet(L"ui.clearall").c_str(),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 70, 3, 52, 22, g_app.wnd.hwndBookshelfToolbar,
                (HMENU)(UINT_PTR)IDM_BOOKSHELF_CLEAR, hInst, nullptr);
            SendMessageW(hClear, WM_SETFONT, (WPARAM)hShelfFont, TRUE);

            g_app.wnd.hwndBookshelfSortBtn = CreateWindowExW(0, L"BUTTON", L"",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_FLAT, 0, 0, 80, 22, g_app.wnd.hwndBookshelfToolbar,
                (HMENU)(UINT_PTR)IDM_BOOKSHELF_SORT, hInst, nullptr);
            SendMessageW(g_app.wnd.hwndBookshelfSortBtn, WM_SETFONT, (WPARAM)(g_uiFont ? g_uiFont : hShelfFont), TRUE);

            // STATICの子ボタンのWM_COMMANDをメインウィンドウに転送
            SetWindowSubclass(g_app.wnd.hwndBookshelfToolbar, [](HWND h, UINT msg, WPARAM wp, LPARAM lp,
                UINT_PTR, DWORD_PTR) -> LRESULT {
                if (msg == WM_COMMAND)
                    return SendMessageW(GetParent(h), WM_COMMAND, wp, lp);
                return DefSubclassProc(h, msg, wp, lp);
            }, 0, 0);
        }

        // サイドバー: TreeView + ListView
        g_app.wnd.hwndTree = CreateWindowExW(
            0, WC_TREEVIEWW, nullptr,
            WS_CHILD | WS_VISIBLE
            | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS | TVS_NOHSCROLL | TVS_EDITLABELS,
            0, 0, 100, 100,
            hwnd, (HMENU)(UINT_PTR)IDC_TREEVIEW, hInst, nullptr);

        // TreeViewサブクラス: ショートカットキーを親に転送（ListViewサブクラスと同様）
        SetWindowSubclass(g_app.wnd.hwndTree, [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
            UINT_PTR, DWORD_PTR) -> LRESULT {
            if (msg == WM_KEYDOWN)
            {
                switch (wParam)
                {
                case VK_LEFT: case VK_RIGHT:  // ページ送り → 親に転送
                case VK_HOME: case VK_END:    // 最初/最後 → 親に転送
                case VK_RETURN:               // 展開/折畳み → 親に転送
                case VK_F2: case VK_F5: case VK_F11: case VK_F1:
                case VK_DELETE: case VK_ESCAPE: case VK_SPACE:
                    SendMessageW(GetParent(hwnd), WM_KEYDOWN, wParam, lParam);
                    return 0;
                }
                // 英字キー（W, B, C等のショートカット）も転送
                if (wParam >= 'A' && wParam <= 'Z')
                {
                    SendMessageW(GetParent(hwnd), WM_KEYDOWN, wParam, lParam);
                    return 0;
                }
                if (wParam >= '0' && wParam <= '9')
                {
                    SendMessageW(GetParent(hwnd), WM_KEYDOWN, wParam, lParam);
                    return 0;
                }
                // VK_UP/VK_DOWN はTreeViewデフォルト動作（ツリーナビゲーション）に任せる
            }
            // WM_CHARのビープ音を抑制（Enter等の制御文字）
            if (msg == WM_CHAR && (wParam == L'\r' || wParam == L'\n' || wParam == L'\t' || wParam == L'\x1b'))
                return 0;
            // ホバープレビュー: ツリー上の書庫ファイルにマウスを乗せるとプレビュー
            if (msg == WM_MOUSEMOVE && g_app.hoverPreviewEnabled)
            {
                TVHITTESTINFO ht = {};
                ht.pt = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
                HTREEITEM hItem = (HTREEITEM)SendMessageW(hwnd, TVM_HITTEST, 0, (LPARAM)&ht);
                static HTREEITEM s_lastHoverItem = nullptr;
                if (hItem != s_lastHoverItem)
                {
                    s_lastHoverItem = hItem;
                    KillTimer(GetParent(hwnd), kHoverTimerId);
                    HideHoverPreview();
                    if (hItem && (ht.flags & TVHT_ONITEM))
                    {
                        std::wstring path = GetTreeItemPath(hItem);
                        // 本棚モードのITEM:タグからパスを抽出
                        if (path.size() > 5 && path.substr(0, 5) == L"ITEM:")
                            path = path.substr(5);
                        if (!path.empty() && IsArchiveFile(path))
                        {
                            g_hoverPath = path;
                            SetTimer(GetParent(hwnd), kHoverTimerId, kHoverDelayMs, nullptr);
                        }
                    }
                    TRACKMOUSEEVENT tme = {}; tme.cbSize = sizeof(tme);
                    tme.dwFlags = TME_LEAVE; tme.hwndTrack = hwnd;
                    TrackMouseEvent(&tme);
                }
            }
            if (msg == WM_MOUSELEAVE)
            {
                KillTimer(GetParent(hwnd), kHoverTimerId);
                HideHoverPreview();
            }
            return DefSubclassProc(hwnd, msg, wParam, lParam);
        }, 0, 0);

        g_app.wnd.hwndList = CreateWindowExW(
            0, WC_LISTVIEWW, nullptr,
            WS_CHILD | WS_VISIBLE
            | LVS_REPORT | LVS_OWNERDATA | LVS_SHOWSELALWAYS,
            0, 0, 100, 100,
            hwnd, (HMENU)(UINT_PTR)IDC_LISTVIEW, hInst, nullptr);

        // Explorer テーマ適用（Windows Vista+ モダンスタイル）— ListViewのみ
        SetWindowTheme(g_app.wnd.hwndList, L"Explorer", nullptr);

        // ListView フルロウ選択
        SendMessageW(g_app.wnd.hwndList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                     LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_HEADERDRAGDROP);

        // 背景色を薄い水色に設定（leeyez_kai準拠 #EBF4FF）
        SendMessageW(g_app.wnd.hwndList, LVM_SETBKCOLOR, 0, (LPARAM)RGB(235, 244, 255));
        SendMessageW(g_app.wnd.hwndList, LVM_SETTEXTBKCOLOR, 0, (LPARAM)RGB(235, 244, 255));

        // ListViewサブクラス: 上下キーを無効化（ページ送りは左右キーのみ）
        SetWindowSubclass(g_app.wnd.hwndList, [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
            UINT_PTR, DWORD_PTR) -> LRESULT {
            if (msg == WM_KEYDOWN)
            {
                switch (wParam)
                {
                case VK_LEFT: case VK_RIGHT:
                case VK_UP: case VK_DOWN:
                case VK_HOME: case VK_END:
                case VK_RETURN:
                case VK_F2: case VK_F5: case VK_F11: case VK_F1:
                case VK_DELETE: case VK_ESCAPE: case VK_SPACE:
                    SendMessageW(GetParent(hwnd), WM_KEYDOWN, wParam, lParam);
                    return 0;
                }
                if (wParam >= 'A' && wParam <= 'Z')
                { SendMessageW(GetParent(hwnd), WM_KEYDOWN, wParam, lParam); return 0; }
                if (wParam >= '0' && wParam <= '9')
                { SendMessageW(GetParent(hwnd), WM_KEYDOWN, wParam, lParam); return 0; }
            }
            // WM_CHARのビープ音を抑制（Enter等の制御文字）
            if (msg == WM_CHAR && (wParam == L'\r' || wParam == L'\n' || wParam == L'\t' || wParam == L'\x1b'))
                return 0;
            // ホバープレビュー: マウス移動でアイテム検出
            if (msg == WM_MOUSEMOVE && g_app.hoverPreviewEnabled)
            {
                LVHITTESTINFO ht = {};
                ht.pt = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
                int idx = (int)SendMessageW(hwnd, LVM_HITTEST, 0, (LPARAM)&ht);
                if (idx != g_hoverItemIndex)
                {
                    g_hoverItemIndex = idx;
                    KillTimer(GetParent(hwnd), kHoverTimerId);
                    HideHoverPreview();
                    if (idx >= 0 && idx < (int)g_app.nav.fileItems.size())
                    {
                        auto& item = g_app.nav.fileItems[idx];
                        if (IsImageFile(item.name) || IsArchiveFile(item.name))
                        {
                            g_hoverPath = item.fullPath;
                            SetTimer(GetParent(hwnd), kHoverTimerId, kHoverDelayMs, nullptr);
                        }
                    }
                    TRACKMOUSEEVENT tme = {}; tme.cbSize = sizeof(tme);
                    tme.dwFlags = TME_LEAVE; tme.hwndTrack = hwnd;
                    TrackMouseEvent(&tme);
                }
            }
            if (msg == WM_MOUSELEAVE)
            {
                g_hoverItemIndex = -1;
                KillTimer(GetParent(hwnd), kHoverTimerId);
                HideHoverPreview();
            }
            return DefSubclassProc(hwnd, msg, wParam, lParam);
        }, 0, 0);

        // フィルターボックス
        g_app.wnd.hwndFilterBox = CreateWindowExW(
            0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            0, 0, 100, 24,
            hwnd, nullptr, hInst, nullptr);
        {
            static HFONT hFilterFont = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
            SendMessageW(g_app.wnd.hwndFilterBox, WM_SETFONT, (WPARAM)hFilterFont, TRUE);
            SendMessageW(g_app.wnd.hwndFilterBox, EM_SETCUEBANNER, TRUE, (LPARAM)I18nGet(L"ui.filter").c_str());
        }
        // フィルター右横パッド（スクロールバー幅分の灰色背景）
        g_app.wnd.hwndFilterPad = CreateWindowExW(
            0, L"STATIC", L"",
            WS_CHILD | WS_VISIBLE,
            0, 0, 20, 24,
            hwnd, nullptr, hInst, nullptr);

        // ListViewカラム
        LVCOLUMNW lvc = {};
        lvc.mask = LVCF_TEXT | LVCF_WIDTH;
        lvc.cx = 200;
        lvc.pszText = (LPWSTR)I18nGet(L"list.name").c_str();
        SendMessageW(g_app.wnd.hwndList, LVM_INSERTCOLUMNW, 0, (LPARAM)&lvc);
        lvc.cx = 80;
        lvc.pszText = (LPWSTR)I18nGet(L"list.size").c_str();
        SendMessageW(g_app.wnd.hwndList, LVM_INSERTCOLUMNW, 1, (LPARAM)&lvc);
        lvc.cx = 90;
        lvc.pszText = (LPWSTR)I18nGet(L"list.type").c_str();
        SendMessageW(g_app.wnd.hwndList, LVM_INSERTCOLUMNW, 2, (LPARAM)&lvc);
        lvc.cx = 130;
        lvc.pszText = (LPWSTR)I18nGet(L"list.date").c_str();
        SendMessageW(g_app.wnd.hwndList, LVM_INSERTCOLUMNW, 3, (LPARAM)&lvc);

        // 保存された列順序・幅を復元
        {
            AppSettings colSettings;
            if (LoadSettings(colSettings))
            {
                if (colSettings.columnOrder.size() == 4)
                    ListView_SetColumnOrderArray(g_app.wnd.hwndList, 4, colSettings.columnOrder.data());
                if (colSettings.columnWidths.size() == 4)
                    for (int i = 0; i < 4; i++)
                        ListView_SetColumnWidth(g_app.wnd.hwndList, i, colSettings.columnWidths[i]);
            }
        }

        // 履歴ツールバー（初期非表示）
        {
            static HFONT hHistFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
            g_app.wnd.hwndHistoryToolbar = CreateWindowExW(0, L"STATIC", L"",
                WS_CHILD | SS_NOTIFY, 0, 0, 100, 28, hwnd, nullptr, hInst, nullptr);

            HWND hIcon = CreateWindowExW(0, L"STATIC", L"\uE81C",
                WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, 4, 3, 20, 22, g_app.wnd.hwndHistoryToolbar, nullptr, hInst, nullptr);
            static HFONT hIconFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, ANTIALIASED_QUALITY, 0, L"Segoe Fluent Icons");
            SendMessageW(hIcon, WM_SETFONT, (WPARAM)(hIconFont ? hIconFont : hHistFont), TRUE);

            HWND hLabel = CreateWindowExW(0, L"STATIC", I18nGet(L"ui.history").c_str(),
                WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE, 26, 3, 40, 22, g_app.wnd.hwndHistoryToolbar, nullptr, hInst, nullptr);
            SendMessageW(hLabel, WM_SETFONT, (WPARAM)hHistFont, TRUE);

            HWND hClear = CreateWindowExW(0, L"BUTTON", I18nGet(L"ui.clearall").c_str(),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 70, 3, 52, 22, g_app.wnd.hwndHistoryToolbar, (HMENU)1500, hInst, nullptr);
            SendMessageW(hClear, WM_SETFONT, (WPARAM)hHistFont, TRUE);

            g_app.wnd.hwndHistoryFilter = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 128, 3, 100, 22, g_app.wnd.hwndHistoryToolbar, nullptr, hInst, nullptr);
            SendMessageW(g_app.wnd.hwndHistoryFilter, WM_SETFONT, (WPARAM)hHistFont, TRUE);
            SendMessageW(g_app.wnd.hwndHistoryFilter, EM_SETCUEBANNER, TRUE, (LPARAM)I18nGet(L"ui.filter").c_str());

            // STATICの子コントロールのWM_COMMANDをメインウィンドウに転送
            SetWindowSubclass(g_app.wnd.hwndHistoryToolbar, [](HWND h, UINT msg, WPARAM wp, LPARAM lp,
                UINT_PTR, DWORD_PTR) -> LRESULT {
                if (msg == WM_COMMAND)
                    return SendMessageW(GetParent(h), WM_COMMAND, wp, lp);
                return DefSubclassProc(h, msg, wp, lp);
            }, 0, 0);
        }

        // 履歴ListView（初期非表示）
        g_app.wnd.hwndHistoryList = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
            WS_CHILD | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_NOCOLUMNHEADER,
            0, 0, 100, 100, hwnd, nullptr, hInst, nullptr);
        {
            SendMessageW(g_app.wnd.hwndHistoryList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
            // 背景色
            SendMessageW(g_app.wnd.hwndHistoryList, LVM_SETBKCOLOR, 0, (LPARAM)RGB(235, 244, 255));
            SendMessageW(g_app.wnd.hwndHistoryList, LVM_SETTEXTBKCOLOR, 0, (LPARAM)RGB(235, 244, 255));
            // 2列（ヘッダ非表示）: 名前（左寄せ） + 時刻（右寄せ）
            LVCOLUMNW lvc = {};
            lvc.mask = LVCF_WIDTH | LVCF_FMT;
            lvc.fmt = LVCFMT_LEFT;
            lvc.cx = 300;
            SendMessageW(g_app.wnd.hwndHistoryList, LVM_INSERTCOLUMNW, 0, (LPARAM)&lvc);
            lvc.fmt = LVCFMT_RIGHT;
            lvc.cx = 60;
            SendMessageW(g_app.wnd.hwndHistoryList, LVM_INSERTCOLUMNW, 1, (LPARAM)&lvc);
            // グループ有効化
            SendMessageW(g_app.wnd.hwndHistoryList, LVM_ENABLEGROUPVIEW, TRUE, 0);
            // システムイメージリスト
            SHFILEINFOW sfi = {};
            HIMAGELIST hSysImg = (HIMAGELIST)SHGetFileInfoW(L"C:\\", 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
            if (hSysImg) SendMessageW(g_app.wnd.hwndHistoryList, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)hSysImg);

            // 履歴リストにホバープレビュー用サブクラス
            SetWindowSubclass(g_app.wnd.hwndHistoryList, [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                UINT_PTR, DWORD_PTR) -> LRESULT {
                if (msg == WM_MOUSEMOVE && g_app.hoverPreviewEnabled)
                {
                    LVHITTESTINFO ht = {};
                    ht.pt = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
                    int idx = (int)SendMessageW(hwnd, LVM_HITTEST, 0, (LPARAM)&ht);
                    static int s_lastHistHoverIdx = -1;
                    if (idx != s_lastHistHoverIdx)
                    {
                        s_lastHistHoverIdx = idx;
                        KillTimer(GetParent(hwnd), kHoverTimerId);
                        HideHoverPreview();
                        if (idx >= 0)
                        {
                            // 表示名を取得して履歴からパスを検索
                            wchar_t buf[512] = {};
                            LVITEMW lvi = {};
                            lvi.iItem = idx; lvi.mask = LVIF_TEXT;
                            lvi.pszText = buf; lvi.cchTextMax = 512;
                            SendMessageW(hwnd, LVM_GETITEMW, 0, (LPARAM)&lvi);
                            std::wstring name = buf;
                            auto tabPos = name.find(L'\t');
                            if (tabPos != std::wstring::npos) name = name.substr(0, tabPos);
                            auto& hist = HistoryGetAll();
                            for (auto& entry : hist)
                            {
                                if (entry.name == name)
                                {
                                    g_hoverPath = entry.path;
                                    SetTimer(GetParent(hwnd), kHoverTimerId, kHoverDelayMs, nullptr);
                                    break;
                                }
                            }
                        }
                        TRACKMOUSEEVENT tme = {}; tme.cbSize = sizeof(tme);
                        tme.dwFlags = TME_LEAVE; tme.hwndTrack = hwnd;
                        TrackMouseEvent(&tme);
                    }
                }
                if (msg == WM_MOUSELEAVE)
                {
                    KillTimer(GetParent(hwnd), kHoverTimerId);
                    HideHoverPreview();
                }
                return DefSubclassProc(hwnd, msg, wParam, lParam);
            }, 0, 0);
        }

        // スプリッター
        g_app.wnd.hwndMainSplitter = CreateSplitter(hwnd, hInst, true, IDC_MAIN_SPLITTER);
        g_app.wnd.hwndSidebarSplitter = CreateSplitter(hwnd, hInst, false, IDC_SIDEBAR_SPLITTER);

        // ビューアーツールバー + ビューアー
        CreateViewerToolbars(hwnd, hInst);
        CreateViewer(hwnd, hInst);

        // メディアプレーヤー（初期非表示）
        // MediaPlayer は遅延初期化（MediaPlay 初回呼び出し時に作成）

        // フォント・サムネイル・プレビューサイズを設定値で初期化
        {
            AppSettings initSettings;
            LoadSettings(initSettings);
            ApplyFontSize(initSettings.fontSize);
            ApplyThumbnailSize(initSettings.thumbnailSize);
            ApplyPreviewSize(initSettings.previewSize);
        }

        InitListView();
        // ツリーソート設定 + ビューアー設定を復元
        {
            AppSettings s;
            LoadSettings(s);
            TreeSortMode mode = SortByName;
            if (s.treeSortMode == L"Date") mode = SortByDate;
            else if (s.treeSortMode == L"Size") mode = SortBySize;
            else if (s.treeSortMode == L"Type") mode = SortByType;
            SetTreeSortMode(mode, s.treeSortDescending);
            UpdateSortButtonLabels();

            g_app.viewer.viewMode = s.viewMode;
            g_app.viewer.isRTL = s.isRTL;
            if (s.scaleMode >= 0 && s.scaleMode <= 3)
                g_app.viewer.scaleMode = (ScaleMode)s.scaleMode;
        }
        InitFolderTree();
        LayoutChildren(hwnd);

        return 0;
    }

    case WM_SIZE:
        LayoutChildren(hwnd);
        return 0;

    case WM_KEYDOWN:
        HandleKeyDown(hwnd, wParam);
        return 0;

    case WM_CHAR:
        // メインウィンドウでの未処理WM_CHARのビープ音を抑制
        return 0;

    case WM_CTLCOLORSTATIC:
    {
        HWND hCtrl = (HWND)lParam;
        // フォルダラベル、本棚ツールバー内、フィルターパッドはデフォルト（灰色）
        if (hCtrl == g_app.wnd.hwndFolderLabel ||
            hCtrl == g_app.wnd.hwndFilterPad ||
            hCtrl == g_app.wnd.hwndBookshelfToolbar ||
            (g_app.wnd.hwndBookshelfToolbar && GetParent(hCtrl) == g_app.wnd.hwndBookshelfToolbar))
            break;
        // その他のSTATIC（アドレスバー等）は白背景
        {
            HDC hdc = (HDC)wParam;
            SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
            SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
            static HBRUSH hWhiteBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
            return (LRESULT)hWhiteBrush;
        }
    }

    case WM_COMMAND:
        // フィルターボックスのテキスト変更
        if (HIWORD(wParam) == EN_CHANGE && (HWND)lParam == g_app.wnd.hwndFilterBox)
        {
            wchar_t buf[256] = {};
            GetWindowTextW(g_app.wnd.hwndFilterBox, buf, 255);
            SetFileListFilter(buf);
            return 0;
        }
        // 履歴フィルター
        if (HIWORD(wParam) == EN_CHANGE && (HWND)lParam == g_app.wnd.hwndHistoryFilter)
        {
            wchar_t buf[256] = {};
            GetWindowTextW(g_app.wnd.hwndHistoryFilter, buf, 255);
            g_historyFilterText = buf;
            BuildHistoryList();
            return 0;
        }
        // 履歴「全削除」ボタン
        if (LOWORD(wParam) == 1500)
        {
            if (MessageBoxW(hwnd, I18nGet(L"dlg.historyclear").c_str(), I18nGet(L"dlg.confirm").c_str(), MB_YESNO | MB_ICONQUESTION) == IDYES)
            {
                HistoryClear();
                BuildHistoryList();
            }
            return 0;
        }
        HandleCommand(hwnd, LOWORD(wParam));
        return 0;

    case WM_NOTIFY:
    {
        LPNMHDR pnm = (LPNMHDR)lParam;
        // TBN_DROPDOWNはTBDDRET_DEFAULTを返す必要がある
        if (pnm->hwndFrom == g_app.wnd.hwndNavBar && pnm->code == TBN_DROPDOWN)
        {
            HandleNotify(hwnd, pnm);
            return TBDDRET_DEFAULT;
        }
        // TVN_BEGINLABELEDIT / TVN_ENDLABELEDIT は戻り値が必要
        if (pnm->hwndFrom == g_app.wnd.hwndTree &&
            (pnm->code == TVN_BEGINLABELEDITW || pnm->code == TVN_ENDLABELEDITW))
        {
            HandleNotify(hwnd, pnm);
            return GetWindowLongPtrW(hwnd, DWLP_MSGRESULT);
        }
        HandleNotify(hwnd, pnm);
        return 0;
    }

    case WM_PREFETCH_DONE:
    {
        // 現在のメッセージ + キューに溜まった同種メッセージをバッチ処理
        // （GPU転送を連続実行して効率化）
        auto* msg = reinterpret_cast<PrefetchDoneMsg*>(lParam);
        if (msg)
        {
            if (msg->wicBmp && g_app.viewer.deviceContext && !msg->path.empty())
            {
                ComPtr<ID2D1Bitmap> d2dBmp;
                if (SUCCEEDED(WicBitmapToD2D(msg->wicBmp, g_app.viewer.deviceContext.Get(), d2dBmp.GetAddressOf())))
                    CachePut(msg->path, d2dBmp);
            }
            if (msg->wicBmp) msg->wicBmp->Release();
            delete msg;
        }

        // キューに溜まった WM_PREFETCH_DONE も連続処理（ループ上限付き）
        MSG pendingMsg;
        for (int batch = 0; batch < 8; batch++)
        {
            if (!PeekMessageW(&pendingMsg, hwnd, WM_PREFETCH_DONE, WM_PREFETCH_DONE, PM_REMOVE))
                break;
            auto* pm = reinterpret_cast<PrefetchDoneMsg*>(pendingMsg.lParam);
            if (pm)
            {
                if (pm->wicBmp && g_app.viewer.deviceContext && !pm->path.empty())
                {
                    ComPtr<ID2D1Bitmap> d2dBmp;
                    if (SUCCEEDED(WicBitmapToD2D(pm->wicBmp, g_app.viewer.deviceContext.Get(), d2dBmp.GetAddressOf())))
                        CachePut(pm->path, d2dBmp);
                }
                if (pm->wicBmp) pm->wicBmp->Release();
                delete pm;
            }
        }
        return 0;
    }

    case WM_ASYNC_DECODE_DONE:
    {
        auto* msg = (AsyncDecodeDoneMsg*)lParam;
        if (!msg) return 0;

        if (msg->generation != GetAsyncDecodeGeneration()) {
            if (msg->wicBmp) msg->wicBmp->Release();
            delete msg;
            return 0;
        }

        if (msg->wicBmp && g_app.viewer.deviceContext)
        {
            ComPtr<ID2D1Bitmap> d2dBmp;
            if (SUCCEEDED(WicBitmapToD2D(msg->wicBmp, g_app.viewer.deviceContext.Get(), d2dBmp.GetAddressOf())))
            {
                CachePut(msg->path, d2dBmp);
                ViewerStopAnimation();
                g_app.viewer.bitmap = d2dBmp;
                g_app.viewer.bitmap2.Reset();
                g_app.viewer.isSpreadActive = false;
                g_app.viewer.zoom = 1.0f;
                g_app.viewer.scrollX = 0.0f;
                g_app.viewer.scrollY = 0.0f;
                g_app.nav.currentPath = msg->path;
                InvalidateRect(g_app.wnd.hwndViewer, nullptr, FALSE);

                // アニメーション開始（非同期デコード完了後、StreamCacheにデータあり）
                ViewerStartAnimationIfNeeded(msg->path);

                auto size = d2dBmp->GetPixelSize();
                int total = (int)g_app.nav.viewableFiles.size();
                wchar_t pageInfo[64];
                swprintf_s(pageInfo, _countof(pageInfo), L"%d / %d", g_app.nav.currentFileIndex + 1, total);
                UpdateStatusBar(msg->path, (int)size.width, (int)size.height, pageInfo, 0);
            }
        }
        if (msg->wicBmp) msg->wicBmp->Release();
        delete msg;
        return 0;
    }

    case WM_ASYNC_SPREAD_DONE:
    {
        auto* msg = reinterpret_cast<AsyncSpreadDoneMsg*>(lParam);
        if (!msg) return 0;
        if (msg->generation != GetAsyncDecodeGeneration()) {
            if (msg->wicBmp1) msg->wicBmp1->Release();
            if (msg->wicBmp2) msg->wicBmp2->Release();
            delete msg;
            return 0;
        }
        if (g_app.viewer.deviceContext) {
            ComPtr<ID2D1Bitmap> bmp1, bmp2;
            bool ok1 = msg->wicBmp1 && SUCCEEDED(WicBitmapToD2D(msg->wicBmp1, g_app.viewer.deviceContext.Get(), bmp1.GetAddressOf()));
            bool ok2 = msg->wicBmp2 && SUCCEEDED(WicBitmapToD2D(msg->wicBmp2, g_app.viewer.deviceContext.Get(), bmp2.GetAddressOf()));
            if (ok1) CachePut(msg->path1, bmp1);
            if (ok2) CachePut(msg->path2, bmp2);
            if (ok1 && ok2) {
                ViewerStopAnimation();
                g_app.viewer.bitmap = bmp1;
                g_app.viewer.bitmap2 = bmp2;
                g_app.viewer.isSpreadActive = true;
                g_app.viewer.rotation = 0;
                g_app.nav.currentPath = msg->path1;
                g_app.viewer.zoom = 1.0f;
                g_app.viewer.scrollX = 0.0f;
                g_app.viewer.scrollY = 0.0f;
                InvalidateRect(g_app.wnd.hwndViewer, nullptr, FALSE);

                // アニメーション開始（非同期デコード完了後、StreamCacheにデータあり）
                ViewerStartAnimationIfNeeded(msg->path1, msg->path2);

                // ステータスバー更新
                auto size = bmp1->GetPixelSize();
                int total = (int)g_app.nav.viewableFiles.size();
                wchar_t pageInfo[64];
                swprintf_s(pageInfo, _countof(pageInfo), L"%d / %d", g_app.nav.currentFileIndex + 1, total);
                UpdateStatusBar(msg->path1, (int)size.width, (int)size.height, pageInfo, 0);
            } else if (ok1) {
                ViewerSetBitmap(bmp1, msg->path1);
            }
        }
        if (msg->wicBmp1) msg->wicBmp1->Release();
        if (msg->wicBmp2) msg->wicBmp2->Release();
        delete msg;
        return 0;
    }

    case WM_HOVER_PREVIEW_DONE:
        HandleHoverPreviewDone(hwnd, wParam, lParam);
        return 0;

    case WM_APP + 50: // インラインリネームキャンセル（遅延処理）
        CancelInlineRename();
        return 0;

    case WM_APP + 8: // WM_THUMB_DONE
        OnThumbnailDone(wParam, lParam);
        return 0;

    case WM_ARCHIVE_LOADED:
    {
        int generation = (int)wParam;
        auto* result = reinterpret_cast<ArchiveLoadResult*>(lParam);
        if (!result) return 0;
        if (generation != GetArchiveLoadGeneration()) {
            delete result;
            return 0;
        }
        ApplyArchiveLoadResult(result);
        delete result;
        return 0;
    }

    case WM_TOGGLE_FULLSCREEN:
        ToggleFullscreen(hwnd);
        return 0;

    case WM_FOLDER_CHANGED:
    {
        // フォルダ内容が変更された → 再読み込み
        if (!g_app.nav.currentFolder.empty() && !g_app.nav.inArchiveMode)
        {
            int prevIndex = g_app.nav.currentFileIndex;
            LoadFolder(g_app.nav.currentFolder);
            PopulateListView();
            // 表示中のファイルインデックスを復元
            if (prevIndex >= 0 && prevIndex < (int)g_app.nav.viewableFiles.size())
                ListViewSelectItem(prevIndex);
        }
        return 0;
    }

    // WM_DEFERRED_INIT は廃止（WM_CREATE 内で同期実行に統合）

    case WM_RESTORE_LAST_PATH:
    {
        // 現在は CreateMainWindow 内で同期実行のため通常は到達しない
        if (!g_restorePath.empty())
        {
            NavigateTo(g_restorePath);
            if (g_restoreIndex >= 0)
                GoToFile(g_restoreIndex);
            g_restorePath.clear();
        }
        return 0;
    }

    case WM_SHOW_CONTEXT_MENU:
    {
        bool isViewer = (wParam == 1);
        int x = (short)LOWORD(lParam), y = (short)HIWORD(lParam);
        ShowContextMenu(hwnd, x, y, isViewer);
        return 0;
    }

    case WM_SPLITTER_MOVED:
    {
        UINT id = (UINT)wParam;
        int newPos = (int)lParam;
        if (id == IDC_MAIN_SPLITTER)
        {
            g_app.mainSplitPos = newPos;
            LayoutChildren(hwnd);
        }
        else if (id == IDC_SIDEBAR_SPLITTER)
        {
            UINT dpi = GetDpiForWindow(hwnd);
            int mainY = DpiScale(kNavBarH_96, dpi) + DpiScale(kAddrBarH_96, dpi);
            g_app.sidebarSplitPos = newPos - mainY;
            LayoutChildren(hwnd);
        }
        return 0;
    }

    case WM_TIMER:
    {
        if (wParam == kHoverTimerId)
        {
            KillTimer(hwnd, kHoverTimerId);
            if (g_app.hoverPreviewEnabled && !g_hoverPath.empty())
            {
                POINT pt;
                GetCursorPos(&pt);
                ShowHoverPreview(g_hoverPath, pt);
            }
            return 0;
        }
        if (wParam == kTreeNavTimer)
        {
            KillTimer(hwnd, kTreeNavTimer);
            if (!g_pendingTreePath.empty() && GetTreeMode() == 0)
            {
                NavigateTo(g_pendingTreePath, { true, false });
            }
            g_pendingTreePath.clear();
        }
        if (wParam == 100) // kIdlePrefetchTimer（nav.cppで定義）
        {
            KillTimer(hwnd, 100);
            int idx = g_app.nav.currentFileIndex;
            int dir = g_app.nav.lastNavDirection;
            if (dir == 0) dir = 1;
            if (idx >= 0 && !g_app.nav.viewableFiles.empty())
                PrefetchIdleStart(idx, dir);
        }
        return 0;
    }

    case WM_DESTROY:
    {
        // ツリーキャッシュ保存（アイコン + サブフォルダ）
        SaveTreeCaches();

        // 設定保存（既存設定を読み込んでから上書き）
        AppSettings settings;
        LoadSettings(settings);
        WINDOWPLACEMENT wp = {};
        wp.length = sizeof(wp);
        GetWindowPlacement(hwnd, &wp);
        settings.windowRect = wp.rcNormalPosition;
        settings.maximized = (wp.showCmd == SW_SHOWMAXIMIZED);
        settings.mainSplitPos = g_app.mainSplitPos;
        settings.sidebarSplitPos = g_app.sidebarSplitPos;
        settings.lastPath = g_app.nav.inArchiveMode ? g_app.nav.currentArchive : g_app.nav.currentFolder;
        settings.lastFileIndex = g_app.nav.currentFileIndex;
        settings.lastImagePath = g_app.nav.currentPath;
        settings.viewMode = g_app.viewer.viewMode;
        settings.isRTL = g_app.viewer.isRTL;
        settings.scaleMode = (int)g_app.viewer.scaleMode;

        // ファイルリスト列順序・幅を保存
        if (g_app.wnd.hwndList)
        {
            int colCount = Header_GetItemCount(ListView_GetHeader(g_app.wnd.hwndList));
            if (colCount > 0)
            {
                settings.columnOrder.resize(colCount);
                ListView_GetColumnOrderArray(g_app.wnd.hwndList, colCount, settings.columnOrder.data());
                settings.columnWidths.resize(colCount);
                for (int i = 0; i < colCount; i++)
                    settings.columnWidths[i] = ListView_GetColumnWidth(g_app.wnd.hwndList, i);
            }
        }

        SaveSettings(settings);

        // 表示中の画像をキャッシュに保存（次回起動時に即表示）
        if (!g_app.nav.currentPath.empty())
        {
            wchar_t cachePath[MAX_PATH];
            GetModuleFileNameW(nullptr, cachePath, MAX_PATH);
            PathRemoveFileSpecW(cachePath);
            PathAppendW(cachePath, L"last_view.cache");

            std::wstring arcPath, entryPath;
            if (SplitArchivePath(g_app.nav.currentPath, arcPath, entryPath))
            {
                // 書庫内画像: 展開してバイト列を保存
                std::vector<BYTE> buf;
                if (ExtractToMemory(arcPath, entryPath, buf) && !buf.empty())
                {
                    HANDLE hf = CreateFileW(cachePath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
                    if (hf != INVALID_HANDLE_VALUE)
                    {
                        DWORD w; WriteFile(hf, buf.data(), (DWORD)buf.size(), &w, nullptr);
                        CloseHandle(hf);
                    }
                }
            }
            else
            {
                // 通常ファイル: そのままコピー
                CopyFileW(g_app.nav.currentPath.c_str(), cachePath, FALSE);
            }
        }

        g_app.viewer.bitmap.Reset();
        g_app.viewer.bitmap2.Reset();
        if (g_app.viewer.deviceContext) g_app.viewer.deviceContext->SetTarget(nullptr);
        g_app.viewer.backBuffer.Reset();
        g_app.viewer.swapChain.Reset();
        g_app.viewer.deviceContext.Reset();
        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool RegisterMainWindow(HINSTANCE hInst)
{
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"KarikariWindow";
    return RegisterClassExW(&wc) != 0;
}

HWND CreateMainWindow(HINSTANCE hInst, int nCmdShow)
{
    // 設定復元
    AppSettings settings;
    bool hasSettings = LoadSettings(settings);

    int x = CW_USEDEFAULT, y = CW_USEDEFAULT, w = 1200, h = 800;
    if (hasSettings)
    {
        x = settings.windowRect.left;
        y = settings.windowRect.top;
        w = settings.windowRect.right - settings.windowRect.left;
        h = settings.windowRect.bottom - settings.windowRect.top;
        if (w < 400) w = 400;
        if (h < 300) h = 300;
        g_app.mainSplitPos = settings.mainSplitPos;
        g_app.sidebarSplitPos = settings.sidebarSplitPos;
    }

    HWND hwnd = CreateWindowExW(
        0,
        L"KarikariWindow", L"karikari",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        x, y, w, h,
        nullptr, nullptr, hInst, nullptr);

    if (!hwnd) return nullptr;

    g_app.wnd.hwndMain = hwnd;

    // Step 1: ウィンドウ表示
    if (hasSettings && settings.maximized)
        ShowWindow(hwnd, SW_SHOWMAXIMIZED);
    else
        ShowWindow(hwnd, nCmdShow);

    // スプリッター位置を再適用（WM_SIZE でクランプされた可能性があるため）
    if (hasSettings)
    {
        g_app.mainSplitPos = settings.mainSplitPos;
        g_app.sidebarSplitPos = settings.sidebarSplitPos;
        LayoutChildren(hwnd);
    }

    // Step 2: キャッシュ画像を即座に表示（書庫Open不要、~50ms）
    {
        wchar_t cachePath[MAX_PATH];
        GetModuleFileNameW(nullptr, cachePath, MAX_PATH);
        PathRemoveFileSpecW(cachePath);
        PathAppendW(cachePath, L"last_view.cache");

        if (GetFileAttributesW(cachePath) != INVALID_FILE_ATTRIBUTES && g_app.viewer.deviceContext)
        {
            ViewerLoadImage(std::wstring(cachePath));
            UpdateWindow(hwnd); // キャッシュ画像を即座に描画
        }
        else
        {
            UpdateWindow(hwnd); // キャッシュなし → 空のUIを描画
        }
    }

    // Step 3: 書庫/フォルダの完全復元（重い処理、UIは既に表示済み）
    if (hasSettings && !settings.lastPath.empty())
    {
        NavigateTo(settings.lastPath);
        if (settings.lastFileIndex >= 0)
            GoToFile(settings.lastFileIndex);
    }

    return hwnd;
}

