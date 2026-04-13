#include "tree.h"
#include "archive.h"
#include "bookshelf.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include "favorites.h"
#include "history.h"
#include "utils.h"
#include "i18n.h"
#include <shlobj.h>
#include <shlwapi.h>

// お気に入り親ノードのハンドル
static HTREEITEM g_hFavoritesRoot = nullptr;

// TreeView の lParam にフルパスを格納するためヒープ文字列を使う
static wchar_t* AllocPathStr(const std::wstring& path)
{
    size_t len = path.size() + 1;
    wchar_t* buf = new wchar_t[len];
    wcscpy_s(buf, len, path.c_str());
    return buf;
}

// SHGetFileInfo でシステムアイコンインデックスを取得（キャッシュ付き）
static std::unordered_map<std::wstring, int> g_iconIndexCache;
static bool g_iconCacheDirty = false; // ディスク保存が必要か

static std::wstring GetIconCachePath()
{
    wchar_t p[MAX_PATH];
    GetModuleFileNameW(nullptr, p, MAX_PATH);
    PathRemoveFileSpecW(p);
    PathAppendW(p, L"icon_index.cache");
    return p;
}

static void LoadIconCache()
{
    std::wstring data = ReadFileToWString(GetIconCachePath());
    if (data.empty()) return;

    size_t pos = 0;
    while (pos < data.size())
    {
        size_t nl = data.find(L'\n', pos);
        if (nl == std::wstring::npos) nl = data.size();
        std::wstring line = data.substr(pos, nl - pos);
        pos = nl + 1;
        if (line.empty()) continue;

        size_t tab = line.find(L'\t');
        if (tab == std::wstring::npos) continue;

        std::wstring key = line.substr(0, tab);
        int idx = _wtoi(line.substr(tab + 1).c_str());
        g_iconIndexCache[key] = idx;
    }
}

static void SaveIconCache()
{
    if (!g_iconCacheDirty) return;
    std::wstring data;
    data.reserve(g_iconIndexCache.size() * 40);
    for (auto& [key, idx] : g_iconIndexCache)
        data += key + L"\t" + std::to_wstring(idx) + L"\n";
    WriteWStringToFile(GetIconCachePath(), data);
    g_iconCacheDirty = false;
}

// 汎用フォルダアイコン（起動時に1回だけ取得、全フォルダで共有）
static int g_genericFolderIcon = -1;
static int g_genericFolderIconOpen = -1;

// useFileAttrs=true: ファイルシステムにアクセスせず属性だけでアイコン取得（高速）
// ルートレベルのドライブ・特殊フォルダは false、子フォルダ/書庫は true で呼ぶ
static int GetIconIndex(const std::wstring& path, bool open = false, bool useFileAttrs = false)
{
    std::wstring key = path + (open ? L"|O" : L"|C");
    auto it = g_iconIndexCache.find(key);
    if (it != g_iconIndexCache.end()) return it->second;

    SHFILEINFOW sfi = {};
    UINT flags = SHGFI_SYSICONINDEX | SHGFI_SMALLICON;
    if (open) flags |= SHGFI_OPENICON;

    if (useFileAttrs)
    {
        // ファイルシステムにアクセスせず、拡張子/属性だけでアイコンを推定（高速）
        DWORD fileAttr = FILE_ATTRIBUTE_DIRECTORY;
        if (IsArchiveFile(path)) fileAttr = FILE_ATTRIBUTE_NORMAL;
        flags |= SHGFI_USEFILEATTRIBUTES;
        SHGetFileInfoW(path.c_str(), fileAttr, &sfi, sizeof(sfi), flags);
    }
    else
    {
        SHGetFileInfoW(path.c_str(), 0, &sfi, sizeof(sfi), flags);
    }

    g_iconIndexCache[key] = sfi.iIcon;
    g_iconCacheDirty = true;
    return sfi.iIcon;
}

// アイコンインデックスを直接指定して挿入
static HTREEITEM InsertTreeItemWithIcon(HWND hwndTree, HTREEITEM hParent, const std::wstring& text,
                                         const std::wstring& fullPath, bool hasChildren, int icon, int iconOpen = -1)
{
    if (iconOpen < 0) iconOpen = icon;
    TVINSERTSTRUCTW tvis = {};
    tvis.hParent = hParent;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvis.item.pszText = (LPWSTR)text.c_str();
    tvis.item.lParam = (LPARAM)AllocPathStr(fullPath);
    tvis.item.cChildren = hasChildren ? 1 : 0;
    tvis.item.iImage = icon;
    tvis.item.iSelectedImage = iconOpen;
    return (HTREEITEM)SendMessageW(hwndTree, TVM_INSERTITEMW, 0, (LPARAM)&tvis);
}

static HTREEITEM InsertTreeItem(HWND hwndTree, HTREEITEM hParent, const std::wstring& text,
                                const std::wstring& fullPath, bool hasChildren, bool useFileAttrs = false)
{
    int icon, iconOpen;
    if (useFileAttrs && hasChildren && g_genericFolderIcon >= 0)
    {
        icon = g_genericFolderIcon;
        iconOpen = g_genericFolderIconOpen;
    }
    else
    {
        icon = GetIconIndex(fullPath, false, useFileAttrs);
        iconOpen = GetIconIndex(fullPath, true, useFileAttrs);
    }

    TVINSERTSTRUCTW tvis = {};
    tvis.hParent = hParent;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvis.item.pszText = (LPWSTR)text.c_str();
    tvis.item.lParam = (LPARAM)AllocPathStr(fullPath);
    tvis.item.cChildren = hasChildren ? 1 : 0;
    tvis.item.iImage = icon;
    tvis.item.iSelectedImage = iconOpen;

    return (HTREEITEM)SendMessageW(hwndTree, TVM_INSERTITEMW, 0, (LPARAM)&tvis);
}

static std::unordered_map<std::wstring, bool> g_subfoldersCache;
static bool g_subfoldersCacheDirty = false;

static std::wstring GetSubfoldersCachePath()
{
    wchar_t p[MAX_PATH];
    GetModuleFileNameW(nullptr, p, MAX_PATH);
    PathRemoveFileSpecW(p);
    PathAppendW(p, L"subfolders.cache");
    return p;
}

static void LoadSubfoldersCache()
{
    std::wstring data = ReadFileToWString(GetSubfoldersCachePath());
    if (data.empty()) return;
    size_t pos = 0;
    while (pos < data.size())
    {
        size_t nl = data.find(L'\n', pos);
        if (nl == std::wstring::npos) nl = data.size();
        std::wstring line = data.substr(pos, nl - pos);
        pos = nl + 1;
        if (line.empty()) continue;
        size_t tab = line.find(L'\t');
        if (tab == std::wstring::npos) continue;
        g_subfoldersCache[line.substr(0, tab)] = (line[tab + 1] == L'1');
    }
}

static void SaveSubfoldersCache()
{
    if (!g_subfoldersCacheDirty) return;
    std::wstring data;
    data.reserve(g_subfoldersCache.size() * 60);
    for (auto& [key, val] : g_subfoldersCache)
        data += key + L"\t" + (val ? L"1" : L"0") + L"\n";
    WriteWStringToFile(GetSubfoldersCachePath(), data);
    g_subfoldersCacheDirty = false;
}

static bool HasSubfolders(const std::wstring& path)
{
    auto it = g_subfoldersCache.find(path);
    if (it != g_subfoldersCache.end()) return it->second;
    std::wstring search = path;
    if (!search.empty() && search.back() != L'\\') search += L'\\';
    search += L'*';

    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileExW(search.c_str(), FindExInfoBasic, &fd,
                                     FindExSearchLimitToDirectories, nullptr, FIND_FIRST_EX_LARGE_FETCH);
    if (hFind == INVALID_HANDLE_VALUE) return false;

    bool found = false;
    do
    {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (wcscmp(fd.cFileName, L".") != 0 && wcscmp(fd.cFileName, L"..") != 0)
            {
                found = true;
                break;
            }
        }
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);
    g_subfoldersCache[path] = found;
    g_subfoldersCacheDirty = true;
    return found;
}

// 特殊フォルダのパスを取得
static std::wstring GetKnownFolderPath(REFKNOWNFOLDERID folderId)
{
    PWSTR path = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(folderId, 0, nullptr, &path)))
    {
        std::wstring result(path);
        CoTaskMemFree(path);
        return result;
    }
    return L"";
}

void InitFolderTree()
{
    HWND hwnd = g_app.wnd.hwndTree;
    if (!hwnd) return;

    // ディスクキャッシュから復元（SHGetFileInfoW / FindFirstFileExW スキップ）
    LoadIconCache();
    LoadSubfoldersCache();

    // 汎用フォルダアイコンを1回だけ取得（全フォルダノードで共有）
    if (g_genericFolderIcon < 0)
    {
        SHFILEINFOW sfi = {};
        SHGetFileInfoW(L"folder", FILE_ATTRIBUTE_DIRECTORY, &sfi, sizeof(sfi),
            SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
        g_genericFolderIcon = sfi.iIcon;
        SHGetFileInfoW(L"folder", FILE_ATTRIBUTE_DIRECTORY, &sfi, sizeof(sfi),
            SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_OPENICON | SHGFI_USEFILEATTRIBUTES);
        g_genericFolderIconOpen = sfi.iIcon;
    }

    SendMessageW(hwnd, WM_SETREDRAW, FALSE, 0); // 描画停止

    SendMessageW(hwnd, TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT);

    // システムイメージリストをTreeViewに設定（キャッシュ）
    static HIMAGELIST hSysImgList = nullptr;
    if (!hSysImgList)
    {
        SHFILEINFOW sfi = {};
        hSysImgList = (HIMAGELIST)SHGetFileInfoW(
            L"C:\\", 0, &sfi, sizeof(sfi),
            SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
    }
    if (hSysImgList)
        SendMessageW(hwnd, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)hSysImgList);

    // --- お気に入り（赤いハートアイコン、static キャッシュ）---
    {
        static int s_favIcon = -1;
        int favIcon = s_favIcon;

        if (favIcon < 0 && hSysImgList)
        {
            HDC hdcScreen = GetDC(nullptr);
            HDC hdcMem = CreateCompatibleDC(hdcScreen);

            BITMAPINFO bmi = {};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = 16;
            bmi.bmiHeader.biHeight = -16;
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            void* pBits = nullptr;
            HBITMAP hbmp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
            HBITMAP hOld = (HBITMAP)SelectObject(hdcMem, hbmp);

            // Segoe Fluent Icons のハートマーク（❤ U+2764 or U+E006 or U+EB52）
            HFONT hHeartFont = CreateFontW(
                14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Segoe Fluent Icons");
            if (!hHeartFont)
                hHeartFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, 0, 0, ANTIALIASED_QUALITY, 0, L"Segoe MDL2 Assets");

            HFONT hOldFont = (HFONT)SelectObject(hdcMem, hHeartFont);
            memset(pBits, 0, 16 * 16 * 4);
            SetBkMode(hdcMem, TRANSPARENT);
            SetTextColor(hdcMem, RGB(255, 255, 255)); // 白テキスト on 黒背景

            RECT rcText = { 0, 0, 16, 16 };
            DrawTextW(hdcMem, L"\uEB52", 1, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            // 輝度→アルファ、赤色でプリマルチプライ
            BYTE* pixels = (BYTE*)pBits;
            for (int i = 0; i < 16 * 16; i++)
            {
                BYTE alpha = (std::max)({ pixels[i*4], pixels[i*4+1], pixels[i*4+2] });
                pixels[i*4+0] = 0;                          // B
                pixels[i*4+1] = (BYTE)(30 * alpha / 255);   // G
                pixels[i*4+2] = (BYTE)(220 * alpha / 255);  // R (赤)
                pixels[i*4+3] = alpha;
            }

            SelectObject(hdcMem, hOldFont);
            SelectObject(hdcMem, hOld);
            DeleteObject(hHeartFont);

            favIcon = ImageList_Add(hSysImgList, hbmp, nullptr);
            DeleteObject(hbmp);
            DeleteDC(hdcMem);
            ReleaseDC(nullptr, hdcScreen);
        }
        if (favIcon < 0) favIcon = GetIconIndex(GetKnownFolderPath(FOLDERID_Favorites));
        s_favIcon = favIcon; // 2回目以降はスキップ

        TVINSERTSTRUCTW tvis = {};
        tvis.hParent = TVI_ROOT;
        tvis.hInsertAfter = TVI_FIRST;
        tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        tvis.item.pszText = (LPWSTR)I18nGet(L"nav.favorites").c_str();
        tvis.item.lParam = (LPARAM)AllocPathStr(L"");
        tvis.item.cChildren = 1;
        tvis.item.iImage = favIcon;
        tvis.item.iSelectedImage = favIcon;
        g_hFavoritesRoot = (HTREEITEM)SendMessageW(hwnd, TVM_INSERTITEMW, 0, (LPARAM)&tvis);

        // お気に入り子ノード追加
        auto& favs = FavoritesGetAll();
        for (auto& fav : favs)
        {
            auto pos = fav.find_last_of(L'\\');
            std::wstring name = (pos != std::wstring::npos) ? fav.substr(pos + 1) : fav;
            DWORD attr = GetFileAttributesW(fav.c_str());
            bool isDir = (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
            InsertTreeItem(hwnd, g_hFavoritesRoot, name, fav, isDir, true);
        }

        // 展開状態にする
        SendMessageW(hwnd, TVM_EXPAND, TVE_EXPAND, (LPARAM)g_hFavoritesRoot);
    }

    // --- 特殊フォルダ ---
    struct { REFKNOWNFOLDERID id; const wchar_t* key; } specials[] = {
        { FOLDERID_Desktop,    L"tree.desktop" },
        { FOLDERID_Downloads,  L"tree.downloads" },
        { FOLDERID_Documents,  L"tree.documents" },
        { FOLDERID_Pictures,   L"tree.pictures" },
        { FOLDERID_Videos,     L"tree.videos" },
        { FOLDERID_Music,      L"tree.music" },
    };

    for (auto& sf : specials)
    {
        std::wstring path = GetKnownFolderPath(sf.id);
        if (!path.empty())
        {
            // 特殊フォルダは固有アイコンなのでキャッシュを無効化して毎回取得
            g_iconIndexCache.erase(path + L"|C");
            g_iconIndexCache.erase(path + L"|O");
            InsertTreeItem(hwnd, TVI_ROOT, I18nGet(sf.key), path, true);
        }
    }

    // --- セパレータ的に空行は入れられないので、ドライブを続けて追加 ---
    wchar_t drives[256];
    GetLogicalDriveStringsW(255, drives);
    const wchar_t* p = drives;
    while (*p)
    {
        std::wstring drive(p);
        std::wstring driveLetter = drive;
        if (!driveLetter.empty() && driveLetter.back() == L'\\')
            driveLetter.pop_back();

        // ボリュームラベル取得（leeyez_kai準拠: "ラベル (C:)" 形式）
        wchar_t volName[MAX_PATH] = {};
        std::wstring displayName;
        UINT driveType = GetDriveTypeW(drive.c_str());
        if (GetVolumeInformationW(drive.c_str(), volName, MAX_PATH, nullptr, nullptr, nullptr, nullptr, 0) && volName[0])
            displayName = std::wstring(volName) + L" (" + driveLetter + L")";
        else if (driveType == DRIVE_REMOTE)
            displayName = I18nGet(L"drive.network") + L" (" + driveLetter + L")";
        else if (driveType == DRIVE_REMOVABLE)
            displayName = I18nGet(L"drive.removable") + L" (" + driveLetter + L")";
        else if (driveType == DRIVE_CDROM)
            displayName = I18nGet(L"drive.cdrom") + L" (" + driveLetter + L")";
        else
            displayName = I18nGet(L"drive.local") + L" (" + driveLetter + L")";

        InsertTreeItem(hwnd, TVI_ROOT, displayName, drive, true);
        p += wcslen(p) + 1;
    }

    SendMessageW(hwnd, WM_SETREDRAW, TRUE, 0); // 描画再開
    InvalidateRect(hwnd, nullptr, TRUE);

    // アイコンインデックスキャッシュをディスクに保存（次回起動時に高速復元）
    SaveIconCache();
    SaveSubfoldersCache();
}

void RefreshFavoritesInTree()
{
    HWND hwnd = g_app.wnd.hwndTree;
    if (!hwnd || !g_hFavoritesRoot) return;

    // 子ノードを全削除
    HTREEITEM hChild = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)g_hFavoritesRoot);
    while (hChild)
    {
        HTREEITEM hNext = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hChild);
        SendMessageW(hwnd, TVM_DELETEITEM, 0, (LPARAM)hChild);
        hChild = hNext;
    }

    // 再追加
    auto& favs = FavoritesGetAll();
    for (auto& fav : favs)
    {
        auto pos = fav.find_last_of(L'\\');
        std::wstring name = (pos != std::wstring::npos) ? fav.substr(pos + 1) : fav;
        DWORD attr = GetFileAttributesW(fav.c_str());
        bool isDir = (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
        InsertTreeItem(hwnd, g_hFavoritesRoot, name, fav, isDir, true);
    }

    // cChildrenを更新（空なら展開マーク非表示）
    TVITEMW tvi = {};
    tvi.mask = TVIF_CHILDREN;
    tvi.hItem = g_hFavoritesRoot;
    tvi.cChildren = favs.empty() ? 0 : 1;
    SendMessageW(hwnd, TVM_SETITEMW, 0, (LPARAM)&tvi);

    if (!favs.empty())
        SendMessageW(hwnd, TVM_EXPAND, TVE_EXPAND, (LPARAM)g_hFavoritesRoot);
}

bool IsFavoritesChild(HTREEITEM hItem)
{
    if (!hItem || !g_hFavoritesRoot) return false;
    HWND hwnd = g_app.wnd.hwndTree;
    if (!hwnd) return false;
    // 親を再帰的にたどってお気に入りルートに到達するか判定
    HTREEITEM h = hItem;
    for (int depth = 0; depth < 50; depth++)
    {
        HTREEITEM hParent = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_PARENT, (LPARAM)h);
        if (!hParent) return false;
        if (hParent == g_hFavoritesRoot) return true;
        h = hParent;
    }
    return false;
}

// --- ツリーソート（前方定義：ExpandTreeNodeで使用） ---

static TreeSortMode g_treeSortMode = SortByName;
static bool g_treeSortDescending = false;

struct TreeSortItem {
    std::wstring name;
    std::wstring path;
    bool isDir;
    FILETIME lastWrite;
    ULONGLONG size;
};

static int CompareTreeItems(const TreeSortItem& a, const TreeSortItem& b)
{
    if (g_treeSortMode == SortByName || g_treeSortMode == SortByType)
    {
        if (a.isDir != b.isDir) return a.isDir ? -1 : 1;
    }

    int cmp = 0;
    switch (g_treeSortMode)
    {
    case SortByName:
        cmp = _wcsicmp(a.name.c_str(), b.name.c_str());
        break;
    case SortByDate:
        cmp = CompareFileTime(&a.lastWrite, &b.lastWrite);
        break;
    case SortBySize:
        if (a.size < b.size) cmp = -1;
        else if (a.size > b.size) cmp = 1;
        break;
    case SortByType:
    {
        const wchar_t* extA = PathFindExtensionW(a.name.c_str());
        const wchar_t* extB = PathFindExtensionW(b.name.c_str());
        cmp = _wcsicmp(extA, extB);
        if (cmp == 0) cmp = _wcsicmp(a.name.c_str(), b.name.c_str());
        break;
    }
    }
    return g_treeSortDescending ? -cmp : cmp;
}

void ExpandTreeNode(HTREEITEM hItem)
{
    HWND hwnd = g_app.wnd.hwndTree;
    if (!hwnd || !hItem) return;

    // お気に入り親ノードは展開済み（FindFirstFileExスキップ）
    if (hItem == g_hFavoritesRoot) return;

    // 本棚モードのノード: CAT:/BOOKSHELF_ROOTはスキップ、ITEM:は実パスに変換して展開
    std::wstring nodePath = GetTreeItemPath(hItem);
    if (nodePath.substr(0, 4) == L"CAT:" || nodePath == L"BOOKSHELF_ROOT") return;
    if (nodePath.size() > 5 && nodePath.substr(0, 5) == L"ITEM:")
    {
        // ITEM:タグの実パスがフォルダなら、lParamを実パスに書き換えて通常展開
        std::wstring realPath = nodePath.substr(5);
        DWORD a = GetFileAttributesW(realPath.c_str());
        if (a == INVALID_FILE_ATTRIBUTES || !(a & FILE_ATTRIBUTE_DIRECTORY)) return;
        wchar_t* newStr = _wcsdup(realPath.c_str());
        TVITEMW tvi = {};
        tvi.mask = TVIF_PARAM;
        tvi.hItem = hItem;
        tvi.lParam = (LPARAM)newStr;
        SendMessageW(hwnd, TVM_SETITEMW, 0, (LPARAM)&tvi);
        nodePath = realPath;
        // 以降は通常のフォルダ展開として処理
    }

    // 既にダミー以外の子があれば何もしない
    HTREEITEM hChild = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)hItem);
    if (hChild)
    {
        TVITEMW tvi = {};
        tvi.mask = TVIF_PARAM;
        tvi.hItem = hChild;
        SendMessageW(hwnd, TVM_GETITEMW, 0, (LPARAM)&tvi);
        if (tvi.lParam != 0) return; // 既に展開済み

        SendMessageW(hwnd, TVM_DELETEITEM, 0, (LPARAM)hChild);
    }

    std::wstring parentPath = GetTreeItemPath(hItem);
    if (parentPath.empty()) return;

    std::wstring search = parentPath;
    if (!search.empty() && search.back() != L'\\') search += L'\\';
    search += L'*';

    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileExW(search.c_str(), FindExInfoBasic, &fd,
                                     FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
    if (hFind == INVALID_HANDLE_VALUE) return;

    // 列挙結果を一旦収集してからソート
    std::vector<TreeSortItem> sortItems;
    do
    {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) continue;

        bool isDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        bool isArc = !isDir && IsArchiveFile(fd.cFileName);
        if (!isDir && !isArc) continue;

        std::wstring childPath = parentPath;
        if (childPath.back() != L'\\') childPath += L'\\';
        childPath += fd.cFileName;

        TreeSortItem si;
        si.name = fd.cFileName;
        si.path = childPath;
        si.isDir = isDir;
        si.lastWrite = fd.ftLastWriteTime;
        si.size = ((ULONGLONG)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
        sortItems.push_back(std::move(si));
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);

    // ソート
    std::sort(sortItems.begin(), sortItems.end(), [](const TreeSortItem& a, const TreeSortItem& b) {
        return CompareTreeItems(a, b) < 0;
    });

    // ソート済みの順序で挿入
    for (auto& si : sortItems)
        InsertTreeItem(hwnd, hItem, si.name, si.path, si.isDir, true);
}

void SaveTreeCaches()
{
    SaveIconCache();
    SaveSubfoldersCache();
}

TreeSortMode GetTreeSortMode() { return g_treeSortMode; }
bool GetTreeSortDescending() { return g_treeSortDescending; }

// 展開済みノードを再帰的に再ソート
static void ResortNodeRecursive(HWND hwnd, HTREEITEM hParent)
{
    // 子ノードを収集
    std::vector<TreeSortItem> items;
    std::vector<HTREEITEM> childHandles;
    HTREEITEM hChild = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)hParent);
    while (hChild)
    {
        std::wstring path = GetTreeItemPath(hChild);
        // ダミーノード（lParam=0）はスキップ
        TVITEMW tvi = {};
        tvi.mask = TVIF_PARAM;
        tvi.hItem = hChild;
        SendMessageW(hwnd, TVM_GETITEMW, 0, (LPARAM)&tvi);
        if (tvi.lParam == 0) { hChild = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hChild); continue; }

        TreeSortItem si;
        wchar_t text[MAX_PATH] = {};
        TVITEMW ti = {};
        ti.mask = TVIF_TEXT;
        ti.hItem = hChild;
        ti.pszText = text;
        ti.cchTextMax = MAX_PATH;
        SendMessageW(hwnd, TVM_GETITEMW, 0, (LPARAM)&ti);
        si.name = text;
        si.path = path;
        si.isDir = !(IsArchiveFile(path));
        // ファイル情報取得
        WIN32_FILE_ATTRIBUTE_DATA fad = {};
        if (GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fad))
        {
            si.isDir = (fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            si.lastWrite = fad.ftLastWriteTime;
            si.size = ((ULONGLONG)fad.nFileSizeHigh << 32) | fad.nFileSizeLow;
        }
        else
        {
            si.lastWrite = {};
            si.size = 0;
        }
        items.push_back(std::move(si));
        childHandles.push_back(hChild);
        hChild = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hChild);
    }

    if (items.size() < 2) return; // ソート不要

    // ソート順序のインデックスを作成
    std::vector<int> order(items.size());
    for (int i = 0; i < (int)order.size(); i++) order[i] = i;
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        return CompareTreeItems(items[a], items[b]) < 0;
    });

    // TVIS_SORTCHILDREN は使えないのでノードを再配置（TVM_SORTCHILDRENEXは複雑なので削除→再挿入）
    // 現在の展開状態を記録
    std::vector<bool> expanded(items.size());
    for (int i = 0; i < (int)childHandles.size(); i++)
    {
        UINT state = (UINT)SendMessageW(hwnd, TVM_GETITEMSTATE, (WPARAM)childHandles[i], TVIS_EXPANDED);
        expanded[i] = (state & TVIS_EXPANDED) != 0;
    }

    // SORTCHILDRENCB でカスタムソート
    TVSORTCB sortCb = {};
    sortCb.hParent = hParent;
    sortCb.lpfnCompare = [](LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) -> int {
        // lParam はパス文字列ポインタ
        wchar_t* p1 = (wchar_t*)lParam1;
        wchar_t* p2 = (wchar_t*)lParam2;
        if (!p1 || !p2) return 0;

        TreeSortItem a, b;
        a.path = p1;
        a.name = PathFindFileNameW(p1);
        b.path = p2;
        b.name = PathFindFileNameW(p2);

        WIN32_FILE_ATTRIBUTE_DATA fad = {};
        if (GetFileAttributesExW(p1, GetFileExInfoStandard, &fad))
        {
            a.isDir = (fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            a.lastWrite = fad.ftLastWriteTime;
            a.size = ((ULONGLONG)fad.nFileSizeHigh << 32) | fad.nFileSizeLow;
        }
        else { a.isDir = !IsArchiveFile(a.name); a.lastWrite = {}; a.size = 0; }

        if (GetFileAttributesExW(p2, GetFileExInfoStandard, &fad))
        {
            b.isDir = (fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            b.lastWrite = fad.ftLastWriteTime;
            b.size = ((ULONGLONG)fad.nFileSizeHigh << 32) | fad.nFileSizeLow;
        }
        else { b.isDir = !IsArchiveFile(b.name); b.lastWrite = {}; b.size = 0; }

        return CompareTreeItems(a, b);
    };
    sortCb.lParam = 0;
    SendMessageW(hwnd, TVM_SORTCHILDRENCB, 0, (LPARAM)&sortCb);

    // 子ノードを再帰的に処理
    hChild = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)hParent);
    while (hChild)
    {
        UINT state = (UINT)SendMessageW(hwnd, TVM_GETITEMSTATE, (WPARAM)hChild, TVIS_EXPANDED);
        if (state & TVIS_EXPANDED)
            ResortNodeRecursive(hwnd, hChild);
        hChild = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hChild);
    }
}

void SetTreeSortMode(TreeSortMode mode, bool descending)
{
    if (g_treeSortMode == mode && g_treeSortDescending == descending) return;
    g_treeSortMode = mode;
    g_treeSortDescending = descending;

    HWND hwnd = g_app.wnd.hwndTree;
    if (!hwnd || GetTreeMode() != 0) return; // 通常モードのみ

    // 選択ノードを記録
    HTREEITEM hSel = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_CARET, 0);
    std::wstring selPath = hSel ? GetTreeItemPath(hSel) : L"";

    SendMessageW(hwnd, WM_SETREDRAW, FALSE, 0);

    // ルートノードから再帰的に再ソート（お気に入り、特殊フォルダ、ドライブの下を対象）
    HTREEITEM hRoot = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_ROOT, 0);
    while (hRoot)
    {
        UINT state = (UINT)SendMessageW(hwnd, TVM_GETITEMSTATE, (WPARAM)hRoot, TVIS_EXPANDED);
        if (state & TVIS_EXPANDED)
            ResortNodeRecursive(hwnd, hRoot);
        hRoot = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hRoot);
    }

    SendMessageW(hwnd, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hwnd, nullptr, TRUE);

    // 選択復元
    if (!selPath.empty())
    {
        // FindNodeByPath は SelectTreePath の内部で使うが、ここでは簡易に
        // 再選択は既に hSel が有効なはずなのでそのまま
        if (hSel) SendMessageW(hwnd, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hSel);
    }
}

// --- ツリー表示モード切替 ---

static int g_treeMode = 0; // 0=通常, 1=本棚, 2=履歴

// カテゴリ展開状態の記憶（セッション中のみ、再起動でリセット）
static std::unordered_map<std::wstring, bool> g_catExpandState;

// 通常ツリー展開状態の記憶（セッション中のみ）
static std::unordered_set<std::wstring> g_normalExpandedPaths;

// 本棚カテゴリの展開状態を保存
static void SaveBookshelfExpandState()
{
    HWND hwnd = g_app.wnd.hwndTree;
    if (!hwnd) return;
    HTREEITEM hRoot = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_ROOT, 0);
    if (!hRoot) return;
    HTREEITEM hChild = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)hRoot);
    while (hChild)
    {
        std::wstring tag = GetTreeItemPath(hChild);
        if (tag.size() > 4 && tag.substr(0, 4) == L"CAT:")
        {
            UINT state = (UINT)SendMessageW(hwnd, TVM_GETITEMSTATE, (WPARAM)hChild, TVIS_EXPANDED);
            g_catExpandState[tag.substr(4)] = (state & TVIS_EXPANDED) != 0;
        }
        hChild = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hChild);
    }
}

// 再帰的に展開ノードのパスを収集
static void CollectExpandedPaths(HWND hwnd, HTREEITEM hItem, std::unordered_set<std::wstring>& out)
{
    while (hItem)
    {
        UINT state = (UINT)SendMessageW(hwnd, TVM_GETITEMSTATE, (WPARAM)hItem, TVIS_EXPANDED);
        if (state & TVIS_EXPANDED)
        {
            std::wstring path = GetTreeItemPath(hItem);
            if (!path.empty()) out.insert(path);
            HTREEITEM hChild = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)hItem);
            if (hChild) CollectExpandedPaths(hwnd, hChild, out);
        }
        hItem = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hItem);
    }
}

// 保存した展開パスに基づいてノードを展開
static void RestoreExpandedPaths(HWND hwnd, HTREEITEM hItem, const std::unordered_set<std::wstring>& paths, int depth = 0)
{
    if (depth > 10) return; // 深さ制限
    while (hItem)
    {
        std::wstring path = GetTreeItemPath(hItem);
        if (!path.empty() && paths.count(path))
        {
            SendMessageW(hwnd, TVM_EXPAND, TVE_EXPAND, (LPARAM)hItem);
            HTREEITEM hChild = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)hItem);
            if (hChild) RestoreExpandedPaths(hwnd, hChild, paths, depth + 1);
        }
        hItem = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hItem);
    }
}

void ShowBookshelfTree()
{
    HWND hwnd = g_app.wnd.hwndTree;
    if (!hwnd) return;

    // 現在の展開状態を保存（再構築前に）
    if (g_treeMode == 1)
        SaveBookshelfExpandState();

    g_treeMode = 1;
    SendMessageW(hwnd, WM_SETREDRAW, FALSE, 0);
    SendMessageW(hwnd, TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT);
    g_hFavoritesRoot = nullptr;

    // 書庫アイコン
    int arcIcon = GetIconIndex(L"dummy.zip", false, true);

    // 本棚ルートノード
    HTREEITEM hRoot = InsertTreeItemWithIcon(hwnd, TVI_ROOT, I18nGet(L"ui.bookshelf").c_str(), L"BOOKSHELF_ROOT", true, arcIcon);

    // フォルダアイコン
    int folderIcon = g_genericFolderIcon >= 0 ? g_genericFolderIcon : GetIconIndex(L"C:\\", false);
    int folderIconOpen = g_genericFolderIconOpen >= 0 ? g_genericFolderIconOpen : GetIconIndex(L"C:\\", true);

    // カテゴリとアイテムを追加（現在のソート順に従う）
    auto& cats = BookshelfGetCategories();
    TreeSortMode sortMode = GetTreeSortMode();
    bool sortDesc = GetTreeSortDescending();

    // カテゴリをソート順にコピー
    std::vector<size_t> catOrder(cats.size());
    for (size_t i = 0; i < cats.size(); i++) catOrder[i] = i;
    std::sort(catOrder.begin(), catOrder.end(), [&](size_t a, size_t b) {
        int cmp = _wcsicmp(cats[a].name.c_str(), cats[b].name.c_str());
        return sortDesc ? cmp > 0 : cmp < 0;
    });

    for (size_t ci : catOrder)
    {
        auto& cat = cats[ci];
        std::wstring catTag = L"CAT:" + cat.id;
        HTREEITEM hCat = InsertTreeItemWithIcon(hwnd, hRoot, cat.name, catTag, !cat.items.empty(), folderIcon, folderIconOpen);

        // アイテムをソート順にコピー
        std::vector<size_t> itemOrder(cat.items.size());
        for (size_t i = 0; i < cat.items.size(); i++) itemOrder[i] = i;
        std::sort(itemOrder.begin(), itemOrder.end(), [&](size_t a, size_t b) {
            int cmp = _wcsicmp(cat.items[a].name.c_str(), cat.items[b].name.c_str());
            return sortDesc ? cmp > 0 : cmp < 0;
        });

        for (size_t ii : itemOrder)
        {
            auto& item = cat.items[ii];
            std::wstring itemTag = L"ITEM:" + item.path;
            DWORD attr = GetFileAttributesW(item.path.c_str());
            bool isDir = (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
            int itemIcon = isDir ? folderIcon : GetIconIndex(item.path, false, true);
            int itemIconOpen = isDir ? folderIconOpen : itemIcon;
            InsertTreeItemWithIcon(hwnd, hCat, item.name, itemTag, isDir, itemIcon, itemIconOpen);
        }

        // ユーザーが展開した状態を復元（記録がなければ未展開）
        auto it = g_catExpandState.find(cat.id);
        if (it != g_catExpandState.end() && it->second)
            SendMessageW(hwnd, TVM_EXPAND, TVE_EXPAND, (LPARAM)hCat);
    }

    // 本棚ルートは常に展開（子ノード追加後に実行）
    SendMessageW(hwnd, TVM_EXPAND, TVE_EXPAND, (LPARAM)hRoot);

    SendMessageW(hwnd, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hwnd, nullptr, TRUE);
}

void ShowHistoryTree()
{
    HWND hwnd = g_app.wnd.hwndTree;
    if (!hwnd) return;

    // 本棚モードから離れる前に展開状態を保存
    if (g_treeMode == 1)
        SaveBookshelfExpandState();

    g_treeMode = 2;
    SendMessageW(hwnd, WM_SETREDRAW, FALSE, 0);
    SendMessageW(hwnd, TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT);
    g_hFavoritesRoot = nullptr;

    // 履歴をルートレベルに追加（新しい順）
    auto& hist = HistoryGetAll();
    for (auto it = hist.rbegin(); it != hist.rend(); ++it)
    {
        auto pos = it->path.find_last_of(L'\\');
        std::wstring name = (pos != std::wstring::npos) ? it->path.substr(pos + 1) : it->path;
        DWORD attr = GetFileAttributesW(it->path.c_str());
        bool isDir = (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
        bool isArc = !isDir && IsArchiveFile(it->path);
        InsertTreeItem(hwnd, TVI_ROOT, name, it->path, isDir || isArc, true);
    }

    SendMessageW(hwnd, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hwnd, nullptr, TRUE);
}

// 通常ツリーの展開状態を保存（モード切替前に呼ぶ）
void SaveNormalTreeState()
{
    HWND hwnd = g_app.wnd.hwndTree;
    if (!hwnd || g_treeMode != 0) return;
    g_normalExpandedPaths.clear();
    HTREEITEM hRoot = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_ROOT, 0);
    if (hRoot) CollectExpandedPaths(hwnd, hRoot, g_normalExpandedPaths);
}

void RefreshTree()
{
    HWND hwnd = g_app.wnd.hwndTree;
    if (!hwnd) return;

    if (g_treeMode == 1)
    {
        // 本棚モード: ShowBookshelfTreeが展開状態を自動保存/復元
        ShowBookshelfTree();
        return;
    }
    if (g_treeMode == 2) return; // 履歴モードはツリー不使用

    // 通常モード: 展開状態+スクロール位置を保持して再構築
    g_app.isRevealing = true; // 展開/選択によるTVN_SELCHANGEDを無視

    // 1. スクロール位置を保存
    SCROLLINFO si = {}; si.cbSize = sizeof(si); si.fMask = SIF_POS;
    GetScrollInfo(hwnd, SB_VERT, &si);
    int scrollPos = si.nPos;

    // 2. 展開パスを保存
    std::unordered_set<std::wstring> expandedPaths;
    HTREEITEM hRoot = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_ROOT, 0);
    if (hRoot) CollectExpandedPaths(hwnd, hRoot, expandedPaths);

    // 3. ツリーを再構築
    SendMessageW(hwnd, WM_SETREDRAW, FALSE, 0);
    InitFolderTree();

    // 4. 展開パスを復元
    hRoot = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_ROOT, 0);
    if (hRoot && !expandedPaths.empty())
        RestoreExpandedPaths(hwnd, hRoot, expandedPaths);

    // 5. スクロール位置を復元
    si.fMask = SIF_POS;
    si.nPos = scrollPos;
    SetScrollInfo(hwnd, SB_VERT, &si, FALSE);
    SendMessageW(hwnd, WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION, scrollPos), 0);

    SendMessageW(hwnd, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hwnd, nullptr, TRUE);
    g_app.isRevealing = false;
}

void ShowNormalTree()
{
    if (g_treeMode == 0) return; // 既に通常モード

    // 本棚モードから離れる前に展開状態を保存
    if (g_treeMode == 1)
        SaveBookshelfExpandState();

    g_treeMode = 0;
    InitFolderTree();

    // 展開状態を復元
    if (!g_normalExpandedPaths.empty())
    {
        HWND hwnd = g_app.wnd.hwndTree;
        if (hwnd)
        {
            SendMessageW(hwnd, WM_SETREDRAW, FALSE, 0);
            HTREEITEM hRoot = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_ROOT, 0);
            if (hRoot) RestoreExpandedPaths(hwnd, hRoot, g_normalExpandedPaths);
            SendMessageW(hwnd, WM_SETREDRAW, TRUE, 0);
        }
    }
}

int GetTreeMode() { return g_treeMode; }

std::wstring GetTreeItemPath(HTREEITEM hItem)
{
    HWND hwnd = g_app.wnd.hwndTree;
    if (!hwnd || !hItem) return L"";

    TVITEMW tvi = {};
    tvi.mask = TVIF_PARAM;
    tvi.hItem = hItem;
    SendMessageW(hwnd, TVM_GETITEMW, 0, (LPARAM)&tvi);

    if (tvi.lParam)
        return (const wchar_t*)tvi.lParam;
    return L"";
}

// ノードの子を列挙して、指定パスに一致するものを探す
static HTREEITEM FindChildByPath(HWND hwnd, HTREEITEM hParent, const std::wstring& targetPath)
{
    HTREEITEM hChild = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)hParent);
    while (hChild)
    {
        std::wstring childPath = GetTreeItemPath(hChild);
        // 末尾の \ を統一して比較
        std::wstring a = targetPath, b = childPath;
        if (!a.empty() && a.back() == L'\\') a.pop_back();
        if (!b.empty() && b.back() == L'\\') b.pop_back();

        if (_wcsicmp(a.c_str(), b.c_str()) == 0)
            return hChild;

        // targetPath が childPath の子パスかチェック
        std::wstring prefix = b + L"\\";
        if (_wcsnicmp(targetPath.c_str(), prefix.c_str(), prefix.size()) == 0)
            return hChild;

        hChild = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hChild);
    }
    return nullptr;
}

// ツリー全体からパス一致するノードを探して削除
static BOOL RemoveTreeNodeRecursive(HWND hwnd, HTREEITEM hParent, const std::wstring& targetPath)
{
    HTREEITEM hChild = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)hParent);
    while (hChild)
    {
        std::wstring childPath = GetTreeItemPath(hChild);
        // 本棚モードの ITEM: プレフィックスを除去して比較
        std::wstring comparePath = childPath;
        if (comparePath.size() > 5 && comparePath.substr(0, 5) == L"ITEM:")
            comparePath = comparePath.substr(5);

        // 末尾の \ を統一
        std::wstring a = targetPath, b = comparePath;
        if (!a.empty() && a.back() == L'\\') a.pop_back();
        if (!b.empty() && b.back() == L'\\') b.pop_back();

        if (_wcsicmp(a.c_str(), b.c_str()) == 0)
        {
            SendMessageW(hwnd, TVM_DELETEITEM, 0, (LPARAM)hChild);
            return TRUE;
        }

        // 子ノードも再帰検索
        if (RemoveTreeNodeRecursive(hwnd, hChild, targetPath))
            return TRUE;

        hChild = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hChild);
    }
    return FALSE;
}

void RemoveTreeItemByPath(const std::wstring& path)
{
    HWND hwnd = g_app.wnd.hwndTree;
    if (!hwnd || path.empty()) return;

    HTREEITEM hRoot = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_ROOT, 0);
    while (hRoot)
    {
        std::wstring rootPath = GetTreeItemPath(hRoot);
        // ルート自身もチェック
        std::wstring a = path, b = rootPath;
        if (!a.empty() && a.back() == L'\\') a.pop_back();
        if (!b.empty() && b.back() == L'\\') b.pop_back();
        if (_wcsicmp(a.c_str(), b.c_str()) == 0)
        {
            SendMessageW(hwnd, TVM_DELETEITEM, 0, (LPARAM)hRoot);
            return;
        }

        if (RemoveTreeNodeRecursive(hwnd, hRoot, path))
            return;

        hRoot = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hRoot);
    }
}

void SelectTreePath(const std::wstring& path)
{
    HWND hwnd = g_app.wnd.hwndTree;
    if (!hwnd || path.empty()) return;

    g_app.isRevealing = true;

    // 本棚モード: "ITEM:パス" タグでノードを直接検索
    if (g_treeMode == 1)
    {
        std::wstring targetTag = L"ITEM:" + path;
        std::wstring tp = path;
        if (!tp.empty() && tp.back() == L'\\') tp.pop_back();

        HTREEITEM hBestMatch = nullptr;

        // ルート→カテゴリ→アイテムの3階層を走査（全ノード探索）
        HTREEITEM hRoot = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_ROOT, 0);
        if (hRoot)
        {
            HTREEITEM hCat = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)hRoot);
            while (hCat)
            {
                HTREEITEM hItem = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)hCat);
                while (hItem)
                {
                    std::wstring tag = GetTreeItemPath(hItem);

                    // 完全一致
                    if (_wcsicmp(tag.c_str(), targetTag.c_str()) == 0)
                    {
                        hBestMatch = hItem;
                        goto bookshelf_found;
                    }

                    // ITEM:タグのパス���、開いたパスの親ディレクトリか
                    if (tag.size() > 5 && tag.substr(0, 5) == L"ITEM:")
                    {
                        std::wstring itemPath = tag.substr(5);
                        if (!itemPath.empty() && itemPath.back() == L'\\') itemPath.pop_back();
                        std::wstring prefix = itemPath + L"\\";
                        if (_wcsnicmp(tp.c_str(), prefix.c_str(), prefix.size()) == 0)
                            hBestMatch = hItem; // 最も深いマッチを保持（後から上書きOK）
                    }

                    hItem = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hItem);
                }
                hCat = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hCat);
            }
        }

bookshelf_found:
        if (hBestMatch)
        {
            // 親カテゴリを展開
            HTREEITEM hParent = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_PARENT, (LPARAM)hBestMatch);
            if (hParent) SendMessageW(hwnd, TVM_EXPAND, TVE_EXPAND, (LPARAM)hParent);

            // ITEM:フォルダから展開して深く辿る
            SendMessageW(hwnd, WM_SETREDRAW, FALSE, 0);
            for (int depth = 0; depth < 50; depth++)
            {
                std::wstring np = GetTreeItemPath(hBestMatch);
                if (np.size() > 5 && np.substr(0, 5) == L"ITEM:") np = np.substr(5);
                if (!np.empty() && np.back() == L'\\') np.pop_back();
                if (_wcsicmp(np.c_str(), tp.c_str()) == 0) break;

                SendMessageW(hwnd, TVM_EXPAND, TVE_EXPAND, (LPARAM)hBestMatch);
                HTREEITEM hChild = FindChildByPath(hwnd, hBestMatch, path);
                if (!hChild) break;
                hBestMatch = hChild;
            }
            SendMessageW(hwnd, WM_SETREDRAW, TRUE, 0);

            SendMessageW(hwnd, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hBestMatch);
            SendMessageW(hwnd, TVM_ENSUREVISIBLE, 0, (LPARAM)hBestMatch);
        }
        g_app.isRevealing = false;
        return;
    }

    // 通常モード: お気に入り配下を優先検索、次にルートノード
    std::wstring tp = path;
    if (!tp.empty() && tp.back() == L'\\') tp.pop_back();

    HTREEITEM hBest = nullptr;

    // お気に入り配下を先に検索（より深いマッチを優先）
    if (g_hFavoritesRoot)
    {
        HTREEITEM hFavChild = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)g_hFavoritesRoot);
        while (hFavChild)
        {
            std::wstring np = GetTreeItemPath(hFavChild);
            if (!np.empty() && np.back() == L'\\') np.pop_back();

            if (_wcsicmp(np.c_str(), tp.c_str()) == 0)
            { hBest = hFavChild; break; }

            if (!np.empty())
            {
                std::wstring prefix = np + L"\\";
                if (_wcsnicmp(path.c_str(), prefix.c_str(), prefix.size()) == 0)
                { hBest = hFavChild; break; }
            }
            hFavChild = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hFavChild);
        }
    }

    // お気に入りでマッチしなければルートノードを検索
    if (!hBest)
    {
        HTREEITEM hCurrent = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_ROOT, 0);
        while (hCurrent)
        {
            std::wstring nodePath = GetTreeItemPath(hCurrent);
            std::wstring np = nodePath;
            if (!np.empty() && np.back() == L'\\') np.pop_back();

            if (_wcsicmp(np.c_str(), tp.c_str()) == 0)
            { hBest = hCurrent; break; }

            if (!np.empty())
            {
                std::wstring prefix = np + L"\\";
                if (_wcsnicmp(path.c_str(), prefix.c_str(), prefix.size()) == 0)
                { hBest = hCurrent; break; }
            }
            hCurrent = (HTREEITEM)SendMessageW(hwnd, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hCurrent);
        }
    }

    if (!hBest)
    {
        g_app.isRevealing = false;
        return;
    }

    // 展開中は描画停止（白ちらつき防止）
    SendMessageW(hwnd, WM_SETREDRAW, FALSE, 0);

    // 子をたどって展開していく
    for (int depth = 0; depth < 50; depth++) // 深さ制限
    {
        std::wstring nodePath = GetTreeItemPath(hBest);
        std::wstring np = nodePath;
        if (!np.empty() && np.back() == L'\\') np.pop_back();

        if (_wcsicmp(np.c_str(), tp.c_str()) == 0)
            break; // 完全一致、終了

        // 展開してから子を探す
        SendMessageW(hwnd, TVM_EXPAND, TVE_EXPAND, (LPARAM)hBest);

        HTREEITEM hChild = FindChildByPath(hwnd, hBest, path);
        if (!hChild) break;

        hBest = hChild;
    }

    // 描画再開してから選択（選択状態を正しく描画するため）
    SendMessageW(hwnd, WM_SETREDRAW, TRUE, 0);

    // 選択
    SendMessageW(hwnd, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hBest);
    SendMessageW(hwnd, TVM_ENSUREVISIBLE, 0, (LPARAM)hBest);
    InvalidateRect(hwnd, nullptr, TRUE);

    g_app.isRevealing = false;
}
