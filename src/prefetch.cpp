#include "prefetch.h"
#include "decoder.h"
#include "settings.h"
#include <shlwapi.h>
#include "cache.h"
#include "stream_cache.h"
#include "archive.h"
#include "filelist.h"
#include "viewer.h"
#include <thread>
#include <atomic>
#include <mutex>

// 世代番号方式: ページ切替のたびに世代を進め、古いワーカーは結果をキャッシュに入れるが
// UIスレッドへの通知は最新世代のみ行う。ワーカーを待たないのでページ送りが即座に応答。
static std::atomic<int> g_prefetchGeneration{0};
static std::atomic<bool> g_shuttingDown{false};

// COM RAII ラッパー（スレッドごとに初期化/解放）
struct ComInitGuard {
    bool initialized = false;
    ComInitGuard() { initialized = SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)); }
    ~ComInitGuard() { if (initialized) CoUninitialize(); }
};

// スレッドローカル WICFactory + COM初期化
static thread_local ComInitGuard tl_comInit;
static thread_local ComPtr<IWICImagingFactory> tl_wicFactory;

static IWICImagingFactory* GetThreadWicFactory()
{
    (void)tl_comInit; // COM 初期化を保証
    if (!tl_wicFactory)
    {
        CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                         IID_PPV_ARGS(tl_wicFactory.GetAddressOf()));
    }
    return tl_wicFactory.Get();
}

// GIF/WebPアニメーション（大ファイル）はプリフェッチ除外
static bool ShouldSkipPrefetch(const std::wstring& path)
{
    const wchar_t* ext = PathFindExtensionW(path.c_str());
    if (!ext) return false;
    if (_wcsicmp(ext, L".gif") == 0) return true;
    if (_wcsicmp(ext, L".webp") == 0)
    {
        std::wstring arcPath, entryPath;
        if (!SplitArchivePath(path, arcPath, entryPath))
        {
            WIN32_FILE_ATTRIBUTE_DATA fad;
            if (GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fad))
            {
                ULONGLONG size = ((ULONGLONG)fad.nFileSizeHigh << 32) | fad.nFileSizeLow;
                if (size > 10 * 1024 * 1024) return true;
            }
        }
    }
    return false;
}

// ワーカー本体（世代番号方式）
static void PrefetchWorker(int index, const std::wstring& path, int generation)
{
    if (generation != g_prefetchGeneration.load(std::memory_order_acquire)) return;
    if (ShouldSkipPrefetch(path)) return;
    if (CacheContains(path)) return;

    IWICImagingFactory* factory = GetThreadWicFactory();
    if (!factory) return;

    ComPtr<IWICBitmap> wicBmp;
    HRESULT hr;

    std::wstring arcPath, entryPath;
    if (SplitArchivePath(path, arcPath, entryPath))
    {
        // ストリームキャッシュ確認（ゼロコピー）
        auto cachedStream = StreamCacheGet(path);
        if (!cachedStream)
        {
            std::vector<BYTE> buffer;
            if (!ExtractToMemory(arcPath, entryPath, buffer)) return;
            StreamCachePut(path, std::move(buffer));
            cachedStream = StreamCacheGet(path);
        }
        if (!cachedStream) return;

        hr = DecodeMemoryToWicBitmap(cachedStream->data(), cachedStream->size(), factory, wicBmp.GetAddressOf());
    }
    else
    {
        hr = DecodeToWicBitmap(path, factory, wicBmp.GetAddressOf());
    }

    if (FAILED(hr) || !wicBmp) return;

    // 画像サイズを常にキャッシュ（世代に関係なく）
    UINT w = 0, h = 0;
    if (SUCCEEDED(wicBmp->GetSize(&w, &h)) && w > 0 && h > 0)
        CacheImageSize(path, w, h);

    // UIスレッドへの通知は最新世代のみ（古い世代のインデックスはviewableFilesと不整合の可能性）
    if (generation != g_prefetchGeneration.load(std::memory_order_acquire))
        return; // 古い世代 → キャッシュに入れず破棄（インデックスが無効かもしれない）

    if (g_shuttingDown.load(std::memory_order_acquire)) return;
    auto* msg = new PrefetchDoneMsg{ wicBmp.Detach(), path };
    if (!PostMessageW(g_app.wnd.hwndMain, WM_PREFETCH_DONE, 0, (LPARAM)msg)) {
        msg->wicBmp->Release();
        delete msg;
    }
}

void PrefetchCancel()
{
    // 世代を進めるだけ。ワーカーを待たない（高速）
    ++g_prefetchGeneration;
}

void PrefetchShutdown()
{
    g_shuttingDown.store(true, std::memory_order_release);
    ++g_prefetchGeneration;
}

static bool g_prefetchSettingsChanged = false;

void PrefetchResetSettings()
{
    g_prefetchSettingsChanged = true;
}

extern int g_scrollSpeed; // nav.cppで定義

void PrefetchStart(int currentIndex, int direction)
{
    // 世代を進める（前のワーカーはそのまま走り続けるがUI通知はしない）
    int gen = ++g_prefetchGeneration;

    int total = (int)g_app.nav.viewableFiles.size();
    if (total == 0) return;

    // 設定値から枚数を取得
    static int cachedCount = 0;
    if (cachedCount <= 0 || g_prefetchSettingsChanged)
    {
        g_prefetchSettingsChanged = false;
        const auto& s = GetCachedSettings();
        cachedCount = std::max(1, s.prefetchCount);
    }
    int count = cachedCount;
    if (g_app.nav.inArchiveMode) count = count * 3 / 2;
    if (g_scrollSpeed > 0) count = count * 2; // 高速スクロール時は2倍
    int fwdCount = std::max(1, (count * 3 + 3) / 4);
    int revCount = count - fwdCount;

    std::vector<int> indices;

    // 次ページを最優先
    int nextIdx = currentIndex + direction;
    if (nextIdx >= 0 && nextIdx < total && !CacheContains(g_app.nav.viewableFiles[nextIdx]))
        indices.push_back(nextIdx);

    for (int i = 2; i <= fwdCount; i++)
    {
        int idx = currentIndex + i * direction;
        if (idx >= 0 && idx < total && !CacheContains(g_app.nav.viewableFiles[idx]))
            indices.push_back(idx);
    }

    for (int i = 1; i <= revCount; i++)
    {
        int idx = currentIndex - i * direction;
        if (idx >= 0 && idx < total && !CacheContains(g_app.nav.viewableFiles[idx]))
            indices.push_back(idx);
    }

    // 書庫内画像をバッチ展開
    struct WorkItem { int idx; std::wstring path; int generation; };

    std::wstring batchArcPath;
    std::vector<std::wstring> batchEntries;
    std::vector<int> batchIndices;
    std::vector<int> fileIndices;

    for (int idx : indices)
    {
        std::wstring path = g_app.nav.viewableFiles[idx];
        std::wstring arcPath, entryPath;
        if (SplitArchivePath(path, arcPath, entryPath))
        {
            if (batchArcPath.empty()) batchArcPath = arcPath;
            if (_wcsicmp(batchArcPath.c_str(), arcPath.c_str()) == 0)
            {
                batchEntries.push_back(entryPath);
                batchIndices.push_back(idx);
                continue;
            }
        }
        fileIndices.push_back(idx);
    }

    // 書庫バッチ展開+デコードをバックグラウンドで実行
    if (!batchEntries.empty()) {
        // ワーカーに渡すコンテキスト
        struct BatchPrefetchCtx {
            int generation;
            std::wstring arcPath;
            std::vector<std::wstring> entries;
            std::vector<int> indices;
            std::vector<std::wstring> paths; // viewableFilesのフルパスコピー
        };

        // パスをUIスレッドでコピー
        std::vector<std::wstring> batchPaths;
        for (int idx : batchIndices)
            batchPaths.push_back(g_app.nav.viewableFiles[idx]);

        auto* ctx = new BatchPrefetchCtx{ gen, batchArcPath,
                                           std::move(batchEntries),
                                           std::move(batchIndices),
                                           std::move(batchPaths) };

        PTP_WORK work = CreateThreadpoolWork([](PTP_CALLBACK_INSTANCE, PVOID p, PTP_WORK) {
            std::unique_ptr<BatchPrefetchCtx> ctx(static_cast<BatchPrefetchCtx*>(p));

            if (ctx->generation != g_prefetchGeneration.load()) return;

            // バッチ展開（バックグラウンド）
            std::vector<BatchExtractResult> results;
            ExtractBatchToMemory(ctx->arcPath, ctx->entries, results);

            // StreamCacheに格納
            for (size_t i = 0; i < results.size() && i < ctx->paths.size(); i++) {
                if (results[i].ok && !results[i].buffer.empty())
                    StreamCachePut(ctx->paths[i], std::move(results[i].buffer));
            }

            // 各エントリを順次デコード（StreamCacheヒット→デコードのみ）
            for (size_t i = 0; i < ctx->indices.size(); i++) {
                if (ctx->generation != g_prefetchGeneration.load()) break;
                PrefetchWorker(ctx->indices[i], ctx->paths[i], ctx->generation);
            }
        }, ctx, nullptr);

        if (work) SubmitThreadpoolWork(work);
        else delete ctx;
    }

    // 通常ファイル
    for (int idx : fileIndices)
    {
        if (gen != g_prefetchGeneration.load()) break;
        std::wstring path = g_app.nav.viewableFiles[idx];

        auto* item = new WorkItem{ idx, path, gen };
        PTP_WORK work = CreateThreadpoolWork([](PTP_CALLBACK_INSTANCE, PVOID ctx, PTP_WORK) {
            auto* wi = (WorkItem*)ctx;
            PrefetchWorker(wi->idx, wi->path, wi->generation);
            delete wi;
        }, item, nullptr);
        if (work) SubmitThreadpoolWork(work);
        else delete item;
    }
}
