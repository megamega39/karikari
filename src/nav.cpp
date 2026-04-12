#include "nav.h"
#include "filelist.h"
#include "utils.h"
#include "settings.h"
#include "fswatcher.h"
#include <unordered_set>
#include <atomic>
#include <sstream>
#include "viewer.h"
#include "addressbar.h"
#include "statusbar.h"
#include "archive.h"
#include "decoder.h"
#include "cache.h"
#include "stream_cache.h"
#include "history.h"
#include "prefetch.h"
#include "tree.h"
#include "media.h"
#include "window.h"
#include "viewer_toolbar.h"
#include <shlwapi.h>

static std::atomic<int> g_archiveLoadGeneration{0};

// スクロール速度判定用
static ULONGLONG g_lastGoToTime = 0;
std::atomic<int> g_scrollSpeed{0}; // 0=通常, 1=高速 (prefetch.cppから参照)

// ナビゲーション設定キャッシュ（ファイルスコープ）
static bool g_navWrap = true;
static bool g_navWrapLoaded = false;
static bool g_navSettingsChanged = false;

// 書庫内ファイルリストをListViewに表示
static void LoadArchiveToList(const std::wstring& archivePath)
{
    g_app.nav.fileItems.clear();
    g_app.nav.viewableFiles.clear();
    g_app.nav.currentFileIndex = -1;
    g_app.nav.inArchiveMode = true;
    g_app.nav.currentArchive = archivePath;

    // 書庫エントリを所有権付き vector で取得
    std::vector<ArchiveEntryRef> entryRefs;

    if (!OpenArchiveAndGetEntries(archivePath, entryRefs) || entryRefs.empty())
    {
        UpdateStatusBar(archivePath, 0, 0, L"書庫を開けません（7z.dll が見つからないか非対応形式）");
        g_app.nav.inArchiveMode = false;
        PopulateListView();
        return;
    }

    // 書庫ファイル自体の更新日時を取得
    FILETIME archiveTime = {};
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesExW(archivePath.c_str(), GetFileExInfoStandard, &fad))
        archiveTime = fad.ftLastWriteTime;

    // 画像・メディアファイルを追加（重複除去: unordered_set で O(1)）
    std::unordered_set<std::wstring> seenNames;
    g_app.nav.fileItems.reserve((int)entryRefs.size());
    for (int i = 0; i < (int)entryRefs.size(); i++)
    {
        if (!IsImageFile(entryRefs[i].path) && !IsMediaFile(entryRefs[i].path)) continue;

        // 大文字小文字無視の重複��ェック
        std::wstring lowerName = ToLowerW(entryRefs[i].path);
        if (!seenNames.insert(lowerName).second) continue;

        FileItem item;
        item.name = entryRefs[i].path;
        item.fullPath = archivePath + L"!" + entryRefs[i].path;
        item.isDirectory = false;
        item.fileSize = entryRefs[i].size;
        item.lastWriteTime = archiveTime;
        g_app.nav.fileItems.push_back(std::move(item));
    }

    // ソート（名前順）
    std::sort(g_app.nav.fileItems.begin(), g_app.nav.fileItems.end(),
        [](const FileItem& a, const FileItem& b) {
            return _wcsicmp(a.name.c_str(), b.name.c_str()) < 0;
        });

    // 画像ファイルリスト構築（fileItemsと1:1対応）+ O(1)検索用インデックスマップ
    g_app.nav.viewableFileIndex.clear();
    g_app.nav.fileItemIndex.clear();
    for (int i = 0; i < (int)g_app.nav.fileItems.size(); i++)
    {
        auto& item = g_app.nav.fileItems[i];
        int idx = (int)g_app.nav.viewableFiles.size();
        g_app.nav.viewableFiles.push_back(item.fullPath);
        std::wstring key = ToLowerW(item.fullPath);
        g_app.nav.viewableFileIndex[key] = idx;
        g_app.nav.fileItemIndex[key] = i;
    }

    PopulateListView();
}

// 書庫内画像を1枚デコードしてビットマップに格納
static bool LoadArchiveBitmap(const std::wstring& archivePath, const std::wstring& entryPath,
                              ComPtr<ID2D1Bitmap>& outBmp)
{
    std::wstring cacheKey = archivePath + L"!" + entryPath;
    auto cached = CacheGet(cacheKey);
    if (cached) { outBmp = cached; return true; }

    // ストリームキャッシュ確認（展開済みバイトデータ、ゼロコピー）
    auto cachedStream = StreamCacheGet(cacheKey);
    if (!cachedStream)
    {
        std::vector<BYTE> buffer;
        if (!ExtractToMemory(archivePath, entryPath, buffer)) return false;
        StreamCachePut(cacheKey, std::move(buffer));
        cachedStream = StreamCacheGet(cacheKey);
    }
    if (!cachedStream || !g_app.viewer.deviceContext) return false;

    if (FAILED(DecodeImageFromMemory(cachedStream->data(), cachedStream->size(),
                                     g_app.viewer.deviceContext.Get(),
                                     outBmp.GetAddressOf())))
        return false;

    CachePut(cacheKey, outBmp);
    return true;
}

// 書庫内画像をメモリ展開して表示
static void ShowArchiveImage(const std::wstring& archivePath, const std::wstring& entryPath)
{
    if (!g_app.viewer.deviceContext) return;

    std::wstring cacheKey = archivePath + L"!" + entryPath;

    g_app.viewer.bitmap.Reset();
    g_app.viewer.bitmap2.Reset();
    g_app.viewer.isSpreadActive = false;

    // キャッシュチェック
    auto cached = CacheGet(cacheKey);
    if (cached)
    {
        g_app.viewer.bitmap = cached;
    }
    else
    {
        // ストリームキャッシュ確認（展開済みバイトデータ、ゼロコピー）
        auto cachedStream = StreamCacheGet(cacheKey);
        if (!cachedStream)
        {
            std::vector<BYTE> buffer;
            if (!ExtractToMemory(archivePath, entryPath, buffer)) return;
            StreamCachePut(cacheKey, std::move(buffer));
            cachedStream = StreamCacheGet(cacheKey);
        }
        if (!cachedStream) return;

        if (FAILED(DecodeImageFromMemory(cachedStream->data(), cachedStream->size(),
                                         g_app.viewer.deviceContext.Get(),
                                         g_app.viewer.bitmap.GetAddressOf())))
            return;

        CachePut(cacheKey, g_app.viewer.bitmap);
    }

    if (g_app.viewer.bitmap)
    {
        ViewerSetBitmap(g_app.viewer.bitmap, cacheKey);
    }
}

void NavigateTo(const std::wstring& path, NavigateOptions opts)
{
    if (path.empty()) return;

    // 書庫内パス判定（! を含む）
    std::wstring arcPath, entryPath;
    if (SplitArchivePath(path, arcPath, entryPath))
    {
        // 書庫がまだ開かれていなければ開く
        if (g_app.nav.currentArchive.empty() || _wcsicmp(g_app.nav.currentArchive.c_str(), arcPath.c_str()) != 0)
        {
            LoadArchiveToList(arcPath);
            UpdateAddressBar(arcPath);
            SetMainTitle(arcPath);
        }

        if (IsImageFile(entryPath))
        {
            // viewableFiles 内のインデックスを O(1) 検索
            std::wstring key = ToLowerW(path);
            auto it = g_app.nav.viewableFileIndex.find(key);
            if (it != g_app.nav.viewableFileIndex.end())
            {
                GoToFile(it->second);
                return;
            }
            ShowArchiveImage(arcPath, entryPath);
        }
        return;
    }

    DWORD attr = GetFileAttributesW(path.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) return;

    if (attr & FILE_ATTRIBUTE_DIRECTORY)
    {
        if (opts.updateHistory)
        {
            // 書庫モードから来た場合のみ書庫パスを履歴に入れる（フォルダは入れない）
            if (g_app.nav.inArchiveMode && !g_app.nav.currentArchive.empty())
            {
                g_app.nav.historyBack.push_back(g_app.nav.currentArchive);
                g_app.nav.historyForward.clear();
            }
        }

        g_app.nav.inArchiveMode = false;
        g_app.nav.currentArchive.clear();

        if (GetTreeMode() == 0)
            ShowNormalTree(); // 通常モード時のみ（本棚/履歴モードはそのまま）
        LoadFolder(path);
        PopulateListView();
        UpdateAddressBar(path);
        FsWatcherStart(path); // フォルダ変更監視開始
        if ((GetTreeMode() == 0 || GetTreeMode() == 1) && opts.syncTreeSelection)
            SelectTreePath(path);

        SetMainTitle(path);
    }
    else if (IsArchiveFile(path))
    {
        // 書庫ファイル → 書庫内エントリを非同期で読み込み
        if (opts.updateHistory)
        {
            if (g_app.nav.inArchiveMode && !g_app.nav.currentArchive.empty())
            {
                g_app.nav.historyBack.push_back(g_app.nav.currentArchive);
                g_app.nav.historyForward.clear();
            }
        }

        // プリフェッチキャンセル
        PrefetchCancel();

        // UI即時更新（ローディング表示）
        g_app.nav.inArchiveMode = true;
        g_app.nav.currentArchive = path;
        g_app.nav.fileItems.clear();
        g_app.nav.viewableFiles.clear();
        g_app.nav.viewableFileIndex.clear();
        g_app.nav.fileItemIndex.clear();
        g_app.nav.currentFileIndex = -1;
        PopulateListView();
        UpdateAddressBar(path);
        SetMainTitle(path);
        UpdateStatusBar(path, 0, 0, L"読み込み中...");

        // 世代番号インクリメント
        int gen = ++g_archiveLoadGeneration;
        NavigateOptions capturedOpts = opts;

        auto* resultPtr = new ArchiveLoadResult{ gen, path, {}, {}, {}, {}, capturedOpts };

        PTP_WORK work = CreateThreadpoolWork([](PTP_CALLBACK_INSTANCE, PVOID ctx, PTP_WORK work) {
            // ワーカースレッドでCOM初期化（7z.dll使用のため必須）
            struct ComGuard { ComGuard() { CoInitializeEx(nullptr, COINIT_MULTITHREADED); } ~ComGuard() { CoUninitialize(); } } comGuard;

            std::unique_ptr<ArchiveLoadResult> result(static_cast<ArchiveLoadResult*>(ctx));

            // 世代チェック
            if (result->generation != g_archiveLoadGeneration.load()) {
                CloseThreadpoolWork(work); return;
            }

            // OpenArchiveAndGetEntries（重い処理）
            std::vector<ArchiveEntryRef> entryRefs;
            if (!OpenArchiveAndGetEntries(result->archivePath, entryRefs)) {
                // 失敗でもUIに通知
                auto* raw = result.release();
                if (!PostMessageW(g_app.wnd.hwndMain, WM_ARCHIVE_LOADED, (WPARAM)raw->generation, (LPARAM)raw))
                    delete raw;
                CloseThreadpoolWork(work); return;
            }

            // 世代チェック
            if (result->generation != g_archiveLoadGeneration.load()) {
                CloseThreadpoolWork(work); return;
            }

            // fileItems構築
            FILETIME archiveTime = {};
            WIN32_FILE_ATTRIBUTE_DATA fad;
            if (GetFileAttributesExW(result->archivePath.c_str(), GetFileExInfoStandard, &fad))
                archiveTime = fad.ftLastWriteTime;

            std::unordered_set<std::wstring> seenNames;
            result->fileItems.reserve(entryRefs.size());
            for (auto& e : entryRefs) {
                if (!IsImageFile(e.path) && !IsMediaFile(e.path)) continue;
                std::wstring lower = ToLowerW(e.path);
                if (!seenNames.insert(lower).second) continue;
                FileItem item;
                item.name = e.path;
                item.fullPath = result->archivePath + L"!" + e.path;
                item.isDirectory = false;
                item.fileSize = e.size;
                item.lastWriteTime = archiveTime;
                result->fileItems.push_back(std::move(item));
            }

            std::sort(result->fileItems.begin(), result->fileItems.end(),
                [](const FileItem& a, const FileItem& b) {
                    return _wcsicmp(a.name.c_str(), b.name.c_str()) < 0;
                });

            for (int i = 0; i < (int)result->fileItems.size(); i++) {
                auto& item = result->fileItems[i];
                int idx = (int)result->viewableFiles.size();
                result->viewableFiles.push_back(item.fullPath);
                std::wstring key = ToLowerW(item.fullPath);
                result->viewableFileIndex[key] = idx;
                result->fileItemIndex[key] = i;
            }

            auto* raw = result.release();
            if (!PostMessageW(g_app.wnd.hwndMain, WM_ARCHIVE_LOADED, (WPARAM)raw->generation, (LPARAM)raw))
                delete raw;
            CloseThreadpoolWork(work);
        }, resultPtr, nullptr);

        if (work) SubmitThreadpoolWork(work);
        else delete resultPtr;
    }
    else if (IsImageFile(path) || IsMediaFile(path))
    {
        g_app.nav.inArchiveMode = false;
        g_app.nav.currentArchive.clear();

        wchar_t folder[MAX_PATH];
        wcscpy_s(folder, path.c_str());
        PathRemoveFileSpecW(folder);
        std::wstring folderStr(folder);

        if (g_app.nav.currentFolder.empty() || _wcsicmp(g_app.nav.currentFolder.c_str(), folderStr.c_str()) != 0)
        {
            LoadFolder(folderStr);
            PopulateListView();
            UpdateAddressBar(folderStr);
        }

        {
            std::wstring key = ToLowerW(path);
            auto it = g_app.nav.viewableFileIndex.find(key);
            if (it != g_app.nav.viewableFileIndex.end())
            {
                GoToFile(it->second);
                return;
            }
        }

        ViewerLoadImage(path);
    }
}

void NavigateBack()
{
    if (g_app.nav.historyBack.empty()) return;

    std::wstring path = g_app.nav.historyBack.back();
    g_app.nav.historyBack.pop_back();

    // 書庫モードの場合のみ現在地を進む履歴に入れる
    if (g_app.nav.inArchiveMode && !g_app.nav.currentArchive.empty())
        g_app.nav.historyForward.push_back(g_app.nav.currentArchive);

    NavigateTo(path, { false, true });
}

void NavigateForward()
{
    if (g_app.nav.historyForward.empty()) return;

    std::wstring path = g_app.nav.historyForward.back();
    g_app.nav.historyForward.pop_back();

    // 書庫モードの場合のみ現在地を戻る履歴に入れる
    if (g_app.nav.inArchiveMode && !g_app.nav.currentArchive.empty())
        g_app.nav.historyBack.push_back(g_app.nav.currentArchive);

    NavigateTo(path, { false, true });
}

void NavigateUp()
{
    if (g_app.nav.inArchiveMode)
    {
        // 書庫モードから親フォルダへ
        wchar_t parent[MAX_PATH];
        wcscpy_s(parent, g_app.nav.currentArchive.c_str());
        PathRemoveFileSpecW(parent);
        NavigateTo(parent);
        return;
    }

    if (g_app.nav.currentFolder.empty()) return;

    wchar_t parent[MAX_PATH];
    wcscpy_s(parent, g_app.nav.currentFolder.c_str());
    if (PathRemoveFileSpecW(parent))
    {
        std::wstring parentStr(parent);
        if (!parentStr.empty() && parentStr != g_app.nav.currentFolder)
            NavigateTo(parentStr);
    }
}

static void ExitMediaMode()
{
    if (g_app.nav.isMediaMode)
    {
        MediaStop();
        g_app.nav.isMediaMode = false;
        LayoutChildren(g_app.wnd.hwndMain);
    }
}

static int ResolveIndex(int index, int total)
{
    // ループナビゲーション（wrapNavigation 設定に従う）
    if (!g_navWrapLoaded || g_navSettingsChanged)
    {
        g_navSettingsChanged = false;
        const auto& s = GetCachedSettings();
        g_navWrap = s.wrapNavigation;
        g_navWrapLoaded = true;
    }

    if (g_navWrap)
    {
        if (index < 0) index = total - 1;
        if (index >= total) index = 0;
    }
    else
    {
        if (index < 0 || index >= total) return -1; // 範囲外はスキップ
    }
    return index;
}

static void PlayMediaFile(int index, const std::wstring& path)
{
    MediaStop();
    g_app.nav.isMediaMode = true;
    g_app.viewer.isSpreadActive = false;
    LayoutChildren(g_app.wnd.hwndMain);
    if (!g_app.nav.inArchiveMode) HistoryAdd(path);

    // 書庫内メディアはテンポラリに展開して再生
    std::wstring arcPath, entryPath;
    if (SplitArchivePath(path, arcPath, entryPath))
    {
        // fileItems からサイズを取得
        ULONGLONG fileSize = 0;
        for (auto& item : g_app.nav.fileItems)
            if (_wcsicmp(item.fullPath.c_str(), path.c_str()) == 0) { fileSize = item.fileSize; break; }

        std::vector<BYTE> buf;
        std::wstring tempPath;
        if (ExtractSmart(arcPath, entryPath, fileSize, buf, tempPath))
        {
            if (!tempPath.empty())
            {
                MediaPlay(tempPath);
            }
            else if (!buf.empty())
            {
                // メモリ展開された場合: テンポラリファイルに書き出して再生
                wchar_t tmp[MAX_PATH];
                GetTempPathW(MAX_PATH, tmp);
                std::wstring dir = std::wstring(tmp) + L"karikari_media\\";
                CreateDirectoryW(dir.c_str(), nullptr);
                std::wstring ext = PathFindExtensionW(entryPath.c_str());
                std::wstring tmpFile = dir + L"media_tmp" + ext;
                HANDLE hFile = CreateFileW(tmpFile.c_str(), GENERIC_WRITE, 0, nullptr,
                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    DWORD written;
                    WriteFile(hFile, buf.data(), (DWORD)buf.size(), &written, nullptr);
                    CloseHandle(hFile);
                    MediaPlay(tmpFile);
                }
            }
        }
    }
    else
    {
        MediaPlay(path);
    }
}

static void DisplayImageFile(int index, const std::wstring& path)
{
    ExitMediaMode();

    if (ShouldShowSpread(index))
    {
        // 見開き判定がtrueなら即座にisSpreadActiveをセット
        // （非同期デコード完了前でもGetPagesPerViewが2を返すようにする）
        g_app.viewer.isSpreadActive = true;

        std::wstring path2 = g_app.nav.viewableFiles[index + 1];

        // 書庫内の見開き
        std::wstring arcPath1, entry1, arcPath2, entry2;
        bool isArc1 = SplitArchivePath(path, arcPath1, entry1);
        bool isArc2 = SplitArchivePath(path2, arcPath2, entry2);

        if (isArc1 && isArc2)
        {
            // 2枚とも書庫内 → キャッシュヒットなら即見開き、ミスなら非同期デコード
            auto cached1 = CacheGet(path);
            std::wstring key2 = arcPath2 + L"!" + entry2;
            auto cached2 = CacheGet(key2);
            if (cached1 && cached2)
            {
                ViewerStopAnimation();
                g_app.viewer.bitmap = cached1;
                g_app.viewer.bitmap2 = cached2;
                g_app.viewer.isSpreadActive = true;
                g_app.viewer.rotation = 0;
                g_app.nav.currentPath = path;
                ViewerResetView();
                InvalidateRect(g_app.wnd.hwndViewer, nullptr, FALSE);
            }
            else
            {
                ViewerLoadSpreadAsync(path, path2);
            }
        }
        else
        {
            // 通常ファイル見開き: 両方キャッシュヒットなら即見開き、ミスなら非同期デコード
            auto cached1 = CacheGet(path);
            auto cached2 = CacheGet(path2);
            if (cached1 && cached2)
            {
                ViewerShowSpread(path, path2);
            }
            else
            {
                ViewerLoadSpreadAsync(path, path2);
            }
        }
    }
    else
    {
        // 単独表示: 非同期デコードでUIブロック回避
        ViewerLoadImageAsync(path);
    }
}

static void SyncListViewAndStatus(int index, const std::wstring& path)
{
    int total = (int)g_app.nav.viewableFiles.size();

    // ListView の選択を同期 + ファイルサイズ取得
    int lvIndex = g_app.nav.inArchiveMode ? index : -1;
    ULONGLONG fileSize = 0;
    {
        // 書庫モードでは viewableFiles と fileItems が1:1対応
        if (g_app.nav.inArchiveMode && index >= 0 && index < (int)g_app.nav.fileItems.size())
        {
            fileSize = g_app.nav.fileItems[index].fileSize;
        }
        else
        {
            // fileItemIndex で O(1) 逆引き
            std::wstring key = ToLowerW(path);
            auto fit = g_app.nav.fileItemIndex.find(key);
            if (fit != g_app.nav.fileItemIndex.end())
            {
                lvIndex = fit->second;
                fileSize = g_app.nav.fileItems[fit->second].fileSize;
            }
        }
        // 見開き時のみ2アイテム選択（書庫モードで画像表示中のみ）
        int selectCount = 1;
        if (g_app.viewer.isSpreadActive && g_app.nav.inArchiveMode)
            selectCount = 2;
        ListViewSelectItem(lvIndex, selectCount);
    }

    // ステータスバー更新
    wchar_t pageInfo[64];
    swprintf_s(pageInfo, _countof(pageInfo), L"%d / %d", index + 1, total);

    if (g_app.viewer.bitmap)
    {
        auto size = g_app.viewer.bitmap->GetSize();
        UpdateStatusBar(path, (int)size.width, (int)size.height, pageInfo, fileSize);
    }
    else
    {
        UpdateStatusBar(path, 0, 0, pageInfo, fileSize);
    }

    // ビューアーツールバー更新
    UpdatePageCounter(index + 1, total);
    UpdateZoomLabel();

    // タイトルバーにファイル名 + ページ情報
    std::wstring fileName = PathBaseName(path);
    wchar_t title[512];
    swprintf_s(title, _countof(title), L"%s - %d/%d - karikari", fileName.c_str(), index + 1, total);
    SetWindowTextW(g_app.wnd.hwndMain, title);
}

void GoToFile(int index)
{
    int total = (int)g_app.nav.viewableFiles.size();
    if (total == 0) return;
    index = ResolveIndex(index, total);
    if (index < 0) return;

    // スクロール速度判定
    ULONGLONG now = GetTickCount64();
    if (g_lastGoToTime > 0 && (now - g_lastGoToTime) < 200)
        g_scrollSpeed = 1;
    else
        g_scrollSpeed = 0;
    g_lastGoToTime = now;

    int prevIndex = g_app.nav.currentFileIndex;
    g_app.nav.lastNavDirection = (index >= prevIndex) ? 1 : -1;
    g_app.nav.currentFileIndex = index;
    std::wstring path = g_app.nav.viewableFiles[index]; // コピー（安全）

    if (IsMediaFile(path))
        PlayMediaFile(index, path);
    else
        DisplayImageFile(index, path);

    SyncListViewAndStatus(index, path);
    PrefetchStart(index, g_app.nav.lastNavDirection);
}

void NavResetSettings()
{
    g_navSettingsChanged = true;
    InvalidateSettingsCache();
}

void NavigateToSiblingArchive(int direction)
{
    if (g_app.nav.currentArchive.empty()) return;

    // 親フォルダを取得
    wchar_t parentDir[MAX_PATH];
    wcscpy_s(parentDir, g_app.nav.currentArchive.c_str());
    PathRemoveFileSpecW(parentDir);

    // 親フォルダ内の書庫ファイルをソート済みで列挙
    std::wstring search = std::wstring(parentDir) + L"\\*";
    std::vector<std::wstring> archives;

    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileExW(search.c_str(), FindExInfoBasic, &fd,
                                     FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do
    {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        if (fd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) continue;
        if (IsArchiveFile(fd.cFileName))
            archives.push_back(std::wstring(parentDir) + L"\\" + fd.cFileName);
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);

    if (archives.empty()) return;

    // 名前順ソート
    std::sort(archives.begin(), archives.end(),
        [](const std::wstring& a, const std::wstring& b) {
            return _wcsicmp(a.c_str(), b.c_str()) < 0;
        });

    // 現在の書庫のインデックスを探す
    int curIdx = -1;
    for (int i = 0; i < (int)archives.size(); i++)
    {
        if (_wcsicmp(archives[i].c_str(), g_app.nav.currentArchive.c_str()) == 0)
        { curIdx = i; break; }
    }
    if (curIdx < 0) return;

    int nextIdx = curIdx + direction;
    if (nextIdx < 0 || nextIdx >= (int)archives.size()) return;

    NavigateTo(archives[nextIdx]);
}

int GetArchiveLoadGeneration() { return g_archiveLoadGeneration.load(); }

void ApplyArchiveLoadResult(ArchiveLoadResult* result)
{
    // 書庫パスが変わっていたら適用しない（別の書庫に移動済み）
    if (_wcsicmp(g_app.nav.currentArchive.c_str(), result->archivePath.c_str()) != 0)
        return;

    g_app.nav.fileItems = std::move(result->fileItems);
    g_app.nav.viewableFiles = std::move(result->viewableFiles);
    g_app.nav.viewableFileIndex = std::move(result->viewableFileIndex);
    g_app.nav.fileItemIndex = std::move(result->fileItemIndex);

    PopulateListView();

    if (g_app.nav.fileItems.empty())
    {
        UpdateStatusBar(result->archivePath, 0, 0, L"書庫を開けません（7z.dll が見つからないか非対応形式）");
        g_app.nav.inArchiveMode = false;
        return;
    }

    if (GetTreeMode() != 2) HistoryAdd(result->archivePath);
    if ((GetTreeMode() == 0 || GetTreeMode() == 1) && result->opts.syncTreeSelection)
        SelectTreePath(result->archivePath);

    if (!g_app.nav.viewableFiles.empty())
    {
        PrefetchStart(0, 1);
        GoToFile(0);
    }
}

// === 戻る/進む履歴の永続化 ===

static std::wstring GetNavHistoryPath()
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    PathRemoveFileSpecW(path);
    PathAppendW(path, L"nav_history.txt");
    return path;
}

void NavHistorySave()
{
    constexpr int kMaxEntries = 20;
    std::wstring data;
    data += L"[back]\n";
    auto& back = g_app.nav.historyBack;
    int startB = (int)back.size() > kMaxEntries ? (int)back.size() - kMaxEntries : 0;
    for (int i = startB; i < (int)back.size(); i++)
        data += back[i] + L"\n";
    data += L"[forward]\n";
    auto& fwd = g_app.nav.historyForward;
    int startF = (int)fwd.size() > kMaxEntries ? (int)fwd.size() - kMaxEntries : 0;
    for (int i = startF; i < (int)fwd.size(); i++)
        data += fwd[i] + L"\n";
    WriteWStringToFile(GetNavHistoryPath(), data);
}

void NavHistoryLoad()
{
    std::wstring data = ReadFileToWString(GetNavHistoryPath());
    if (data.empty()) return;

    std::wistringstream iss(data);
    std::wstring line;
    int section = -1; // 0=back, 1=forward
    while (std::getline(iss, line))
    {
        if (!line.empty() && line.back() == L'\r')
            line.pop_back();
        if (line == L"[back]") { section = 0; continue; }
        if (line == L"[forward]") { section = 1; continue; }
        if (line.empty()) continue;
        if (section == 0) g_app.nav.historyBack.push_back(line);
        else if (section == 1) g_app.nav.historyForward.push_back(line);
    }
}
