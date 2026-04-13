#include "stream_cache.h"
#include <list>
#include <unordered_map>
#include <mutex>

struct StreamCacheEntry {
    std::wstring key;
    std::shared_ptr<const std::vector<BYTE>> data;
    size_t dataSize;
};

static std::list<StreamCacheEntry> g_lruList;
static std::unordered_map<std::wstring, std::list<StreamCacheEntry>::iterator> g_cacheMap;
static size_t g_totalBytes = 0;
// 物理メモリベースの動的上限（メモリの1/6、100MB-1GB範囲）
static size_t CalcMaxStreamCacheBytes() {
    MEMORYSTATUSEX ms = {}; ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms)) {
        size_t limit = (size_t)(ms.ullTotalPhys / 6);
        if (limit < 100ULL * 1024 * 1024) limit = 100ULL * 1024 * 1024;
        if (limit > 1024ULL * 1024 * 1024) limit = 1024ULL * 1024 * 1024;
        return limit;
    }
    return 200ULL * 1024 * 1024;
}
static size_t g_maxBytes = CalcMaxStreamCacheBytes();
static std::mutex g_mutex;

// 1エントリの最大サイズ（50MB以上は格納しない）
static constexpr size_t kMaxEntrySize = 50 * 1024 * 1024;

void StreamCacheInit(size_t maxBytes)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    g_maxBytes = maxBytes;
}

std::shared_ptr<const std::vector<BYTE>> StreamCacheGet(const std::wstring& key)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    auto it = g_cacheMap.find(key);
    if (it == g_cacheMap.end()) return nullptr;

    // LRU: 先頭に移動
    g_lruList.splice(g_lruList.begin(), g_lruList, it->second);
    return it->second->data;
}

static void PutInternal(const std::wstring& key, std::shared_ptr<const std::vector<BYTE>> dataPtr)
{
    size_t dataSize = dataPtr->size();
    if (dataSize == 0 || dataSize > kMaxEntrySize) return;

    // 既存エントリ削除
    auto it = g_cacheMap.find(key);
    if (it != g_cacheMap.end())
    {
        g_totalBytes -= it->second->dataSize;
        g_lruList.erase(it->second);
        g_cacheMap.erase(it);
    }

    // LRU逐出
    while (!g_lruList.empty() && g_totalBytes + dataSize > g_maxBytes)
    {
        auto& last = g_lruList.back();
        g_totalBytes -= last.dataSize;
        g_cacheMap.erase(last.key);
        g_lruList.pop_back();
    }

    StreamCacheEntry entry;
    entry.key = key;
    entry.data = std::move(dataPtr);
    entry.dataSize = dataSize;
    g_lruList.push_front(std::move(entry));
    g_cacheMap[key] = g_lruList.begin();
    g_totalBytes += dataSize;
}

void StreamCachePut(const std::wstring& key, std::vector<BYTE>&& data)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    auto ptr = std::make_shared<const std::vector<BYTE>>(std::move(data));
    PutInternal(key, std::move(ptr));
}

void StreamCacheClear()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    g_lruList.clear();
    g_cacheMap.clear();
    g_totalBytes = 0;
}
