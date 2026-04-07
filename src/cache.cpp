#include "cache.h"
#include <mutex>

struct CacheEntry {
    std::wstring key;
    ComPtr<ID2D1Bitmap> bitmap;
    size_t sizeBytes;
};

static std::list<CacheEntry> g_lruList;
static std::unordered_map<std::wstring, std::list<CacheEntry>::iterator> g_cacheMap;
static size_t g_totalBytes = 0;
static size_t g_maxBytes = 300 * 1024 * 1024;
static std::mutex g_cacheMutex;

void CacheInit(size_t maxBytes, int /*maxEntries*/)
{
    std::lock_guard<std::mutex> lock(g_cacheMutex);

    if (maxBytes == 0)
    {
        // NeeView式: 物理メモリベースの動的上限
        MEMORYSTATUSEX ms = {}; ms.dwLength = sizeof(ms);
        if (GlobalMemoryStatusEx(&ms))
        {
            size_t totalPhys = (size_t)ms.ullTotalPhys;
            size_t half = totalPhys / 2;
            size_t minus2g = (totalPhys > 2ULL * 1024 * 1024 * 1024)
                ? totalPhys - 2ULL * 1024 * 1024 * 1024 : 256ULL * 1024 * 1024;
            g_maxBytes = std::max(half, minus2g);
            // 上限2GB
            g_maxBytes = std::min(g_maxBytes, (size_t)2ULL * 1024 * 1024 * 1024);
        }
    }
    else
    {
        g_maxBytes = maxBytes;
    }
}

static size_t EstimateBitmapSize(ID2D1Bitmap* bmp)
{
    if (!bmp) return 0;
    auto size = bmp->GetPixelSize();  // D2D1_SIZE_U (DPI非依存のピクセル単位)
    return (size_t)size.width * size.height * 4;
}

ComPtr<ID2D1Bitmap> CacheGet(const std::wstring& key)
{
    std::lock_guard<std::mutex> lock(g_cacheMutex);
    auto it = g_cacheMap.find(key);
    if (it == g_cacheMap.end()) return nullptr;

    g_lruList.splice(g_lruList.begin(), g_lruList, it->second);
    return it->second->bitmap; // ComPtr コピー = AddRef 保証、ロック解放後も安全
}

void CachePut(const std::wstring& key, ComPtr<ID2D1Bitmap> bmp)
{
    if (!bmp) return;

    std::lock_guard<std::mutex> lock(g_cacheMutex);

    auto it = g_cacheMap.find(key);
    if (it != g_cacheMap.end())
    {
        g_totalBytes -= it->second->sizeBytes;
        g_lruList.erase(it->second);
        g_cacheMap.erase(it);
    }

    size_t bmpSize = EstimateBitmapSize(bmp.Get());

    while ((!g_lruList.empty()) &&
           (g_totalBytes + bmpSize > g_maxBytes))
    {
        auto& last = g_lruList.back();
        g_totalBytes -= last.sizeBytes;
        g_cacheMap.erase(last.key);
        g_lruList.pop_back();
    }

    CacheEntry entry;
    entry.key = key;
    entry.bitmap = bmp;
    entry.sizeBytes = bmpSize;
    g_lruList.push_front(std::move(entry));
    g_cacheMap[key] = g_lruList.begin();
    g_totalBytes += bmpSize;
}

bool CacheContains(const std::wstring& key)
{
    std::lock_guard<std::mutex> lock(g_cacheMutex);
    return g_cacheMap.find(key) != g_cacheMap.end();
}

void CacheClear()
{
    std::lock_guard<std::mutex> lock(g_cacheMutex);
    g_lruList.clear();
    g_cacheMap.clear();
    g_totalBytes = 0;
}
