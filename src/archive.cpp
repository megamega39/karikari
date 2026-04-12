#include "archive.h"
#include "utils.h"
#include <shlwapi.h>
#include <objbase.h>
#include <propidl.h>
#include <unordered_map>
#include <mutex>
#include <chrono>

// === 7zip SDK ミニマル型定義 ===
typedef int Int32;
typedef unsigned int UInt32;
typedef long long Int64;
typedef unsigned long long UInt64;

// === 7zip COM インターフェース定義 ===
// GUID は 7zip SDK の Guid.txt に基づく

MIDL_INTERFACE("23170F69-40C1-278A-0000-000300010000")
ISequentialInStream : public IUnknown {
    STDMETHOD(Read)(void *data, UInt32 size, UInt32 *processedSize) PURE;
};

MIDL_INTERFACE("23170F69-40C1-278A-0000-000300030000")
IInStream : public ISequentialInStream {
    STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition) PURE;
};

MIDL_INTERFACE("23170F69-40C1-278A-0000-000300020000")
ISequentialOutStream : public IUnknown {
    STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize) PURE;
};

MIDL_INTERFACE("23170F69-40C1-278A-0000-000500100000")
IProgress : public IUnknown {
    STDMETHOD(SetTotal)(UInt64 total) PURE;
    STDMETHOD(SetCompleted)(const UInt64 *completeValue) PURE;
};

MIDL_INTERFACE("23170F69-40C1-278A-0000-000500200000")
IArchiveExtractCallback : public IProgress {
    STDMETHOD(GetStream)(UInt32 index, ISequentialOutStream **outStream, Int32 askExtractMode) PURE;
    STDMETHOD(PrepareOperation)(Int32 askExtractMode) PURE;
    STDMETHOD(SetOperationResult)(Int32 resultEOperationResult) PURE;
};

MIDL_INTERFACE("23170F69-40C1-278A-0000-000600600000")
IInArchive : public IUnknown {
    STDMETHOD(Open)(IInStream *stream, const UInt64 *maxCheckStartPosition, void *openArchiveCallback) PURE;
    STDMETHOD(Close)() PURE;
    STDMETHOD(GetNumberOfItems)(UInt32 *numItems) PURE;
    STDMETHOD(GetProperty)(UInt32 index, PROPID propID, PROPVARIANT *value) PURE;
    STDMETHOD(Extract)(const UInt32 *indices, UInt32 numItems, Int32 testMode, IArchiveExtractCallback *extractCallback) PURE;
    STDMETHOD(GetArchiveProperty)(PROPID propID, PROPVARIANT *value) PURE;
    STDMETHOD(GetNumberOfProperties)(UInt32 *numProperties) PURE;
    STDMETHOD(GetPropertyInfo)(UInt32 index, BSTR *name, PROPID *propID, VARTYPE *varType) PURE;
    STDMETHOD(GetNumberOfArchiveProperties)(UInt32 *numProperties) PURE;
    STDMETHOD(GetArchivePropertyInfo)(UInt32 index, BSTR *name, PROPID *propID, VARTYPE *varType) PURE;
};

// === Handler GUID定義 ===
// 7zip SDK Guid.txt: Handler GUIDs = {23170F69-40C1-278A-1000-000110XX0000}
static const GUID CLSID_CFormatZip =
    { 0x23170F69, 0x40C1, 0x278A, {0x10,0x00,0x00,0x01,0x10,0x01,0x00,0x00} };
static const GUID CLSID_CFormat7z =
    { 0x23170F69, 0x40C1, 0x278A, {0x10,0x00,0x00,0x01,0x10,0x07,0x00,0x00} };
static const GUID CLSID_CFormatRar =
    { 0x23170F69, 0x40C1, 0x278A, {0x10,0x00,0x00,0x01,0x10,0x03,0x00,0x00} };
static const GUID CLSID_CFormatRar5 =
    { 0x23170F69, 0x40C1, 0x278A, {0x10,0x00,0x00,0x01,0x10,0xCC,0x00,0x00} };

// IInArchive の IID — __uuidof(IInArchive) を使用

// プロパティID
static const PROPID kpidPath = 3;
static const PROPID kpidSize = 7;
static const PROPID kpidIsDir = 6;

// 7z.dll 関数ポインタ
typedef HRESULT (WINAPI *Func_CreateObject)(const GUID* clsID, const GUID* iid, void** outObject);
static HMODULE g_7zDll = nullptr;
static Func_CreateObject g_CreateObject = nullptr;

static const wchar_t* kArchiveExts[] = {
    L".zip", L".7z", L".rar", L".cbz", L".cbr", L".cb7",
};

// === 書庫ハンドルキャッシュ ===
struct ArchiveEntry { UInt32 index; std::wstring path; ULONGLONG size; };

struct CachedArchiveHandle {
    IInArchive* archive = nullptr;
    std::wstring path;
    std::unordered_map<std::wstring, UInt32> entryIndexMap;
    std::vector<ArchiveEntry> entryList;
};

static constexpr int kMaxArchiveCache = 3;

static std::wstring NormalizePath(const std::wstring& p); // 前方宣言

// === エントリリストのディスクキャッシュ ===
#include "utils.h"
#include <sstream>

static std::wstring GetEntryCachePath(const std::wstring& archivePath)
{
    // 書庫パスのハッシュでキャッシュファイル名を決定（複数書庫対応）
    size_t hash = std::hash<std::wstring>{}(archivePath);
    wchar_t name[64];
    swprintf_s(name, L"arc_cache_%zx.dat", hash);

    wchar_t p[MAX_PATH];
    GetModuleFileNameW(nullptr, p, MAX_PATH);
    PathRemoveFileSpecW(p);
    PathAppendW(p, name);
    return p;
}

// 書庫のエントリリストをディスクに保存
static void SaveEntryCache(const std::wstring& archivePath, const std::vector<ArchiveEntry>& entries)
{
    // 書庫ファイルの更新日時を取得（キャッシュ有効性判定用）
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExW(archivePath.c_str(), GetFileExInfoStandard, &fad)) return;

    std::wstring data;
    // ヘッダ: 書庫パス\t更新日時High\t更新日時Low\n
    data += EscapeJsonPath(archivePath) + L"\t" +
            std::to_wstring(fad.ftLastWriteTime.dwHighDateTime) + L"\t" +
            std::to_wstring(fad.ftLastWriteTime.dwLowDateTime) + L"\n";
    // エントリ: index\tpath\tsize\n
    for (auto& e : entries)
        data += std::to_wstring(e.index) + L"\t" + e.path + L"\t" + std::to_wstring(e.size) + L"\n";

    WriteWStringToFile(GetEntryCachePath(archivePath), data);
}

// ディスクキャッシュからエントリリストを復元（成功時 true）
static bool LoadEntryCache(const std::wstring& archivePath,
                           std::vector<ArchiveEntry>& entries,
                           std::unordered_map<std::wstring, UInt32>& indexMap)
{
    std::wstring data = ReadFileToWString(GetEntryCachePath(archivePath));
    if (data.empty()) return false;

    // 最初の行: 書庫パス\t更新日時
    size_t firstNL = data.find(L'\n');
    if (firstNL == std::wstring::npos) return false;

    std::wstring header = data.substr(0, firstNL);
    size_t t1 = header.find(L'\t');
    size_t t2 = header.find(L'\t', t1 + 1);
    if (t1 == std::wstring::npos || t2 == std::wstring::npos) return false;

    std::wstring cachedPath = UnescapeJsonPath(header.substr(0, t1));
    if (_wcsicmp(cachedPath.c_str(), archivePath.c_str()) != 0) return false;

    // 更新日時チェック
    DWORD cachedHigh = (DWORD)_wtoi64(header.substr(t1 + 1, t2 - t1 - 1).c_str());
    DWORD cachedLow = (DWORD)_wtoi64(header.substr(t2 + 1).c_str());

    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExW(archivePath.c_str(), GetFileExInfoStandard, &fad)) return false;
    if (fad.ftLastWriteTime.dwHighDateTime != cachedHigh ||
        fad.ftLastWriteTime.dwLowDateTime != cachedLow) return false;

    // エントリ行をパース
    entries.clear();
    indexMap.clear();
    size_t pos = firstNL + 1;
    while (pos < data.size())
    {
        size_t nl = data.find(L'\n', pos);
        if (nl == std::wstring::npos) nl = data.size();
        std::wstring line = data.substr(pos, nl - pos);
        pos = nl + 1;
        if (line.empty()) continue;

        size_t tab1 = line.find(L'\t');
        size_t tab2 = line.find(L'\t', tab1 + 1);
        if (tab1 == std::wstring::npos || tab2 == std::wstring::npos) continue;

        ArchiveEntry e;
        e.index = (UInt32)_wtoi(line.substr(0, tab1).c_str());
        e.path = line.substr(tab1 + 1, tab2 - tab1 - 1);
        e.size = (ULONGLONG)_wtoi64(line.substr(tab2 + 1).c_str());

        indexMap[NormalizePath(e.path)] = e.index;
        entries.push_back(std::move(e));
    }

    return !entries.empty();
}
static std::list<CachedArchiveHandle> g_archiveCache; // LRU: front=最新
static std::timed_mutex g_archiveMutex;

// 現在アクティブなキャッシュへのポインタ（GetCachedArchive で設定）
static CachedArchiveHandle* g_currentHandle = nullptr;

static void NormalizePathInPlace(std::wstring& p)
{
    for (auto& c : p) { if (c == L'/') c = L'\\'; c = towlower(c); }
}

static std::wstring NormalizePath(const std::wstring& p)
{
    std::wstring r = p;
    NormalizePathInPlace(r);
    return r;
}

// === IInStream 実装 ===
class FileInStream : public IInStream {
    HANDLE hFile_;
    LONG refCount_ = 1;
public:
    FileInStream(HANDLE h) : hFile_(h) {}
    ~FileInStream() { if (hFile_ != INVALID_HANDLE_VALUE) CloseHandle(hFile_); }

    STDMETHOD(QueryInterface)(REFIID iid, void** ppv) override {
        if (iid == __uuidof(IUnknown) || iid == __uuidof(ISequentialInStream) || iid == __uuidof(IInStream)) {
            *ppv = static_cast<IInStream*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    STDMETHOD_(ULONG, AddRef)() override { return InterlockedIncrement(&refCount_); }
    STDMETHOD_(ULONG, Release)() override {
        LONG r = InterlockedDecrement(&refCount_);
        if (r == 0) delete this;
        return r;
    }
    STDMETHOD(Read)(void* data, UInt32 size, UInt32* processedSize) override {
        DWORD read = 0;
        BOOL ok = ReadFile(hFile_, data, size, &read, nullptr);
        if (processedSize) *processedSize = read;
        return ok ? S_OK : HRESULT_FROM_WIN32(GetLastError());
    }
    STDMETHOD(Seek)(Int64 offset, UInt32 seekOrigin, UInt64* newPosition) override {
        LARGE_INTEGER li; li.QuadPart = offset;
        LARGE_INTEGER newPos;
        BOOL ok = SetFilePointerEx(hFile_, li, &newPos, seekOrigin);
        if (newPosition) *newPosition = newPos.QuadPart;
        return ok ? S_OK : HRESULT_FROM_WIN32(GetLastError());
    }
};

// === メモリ出力ストリーム ===
static constexpr size_t kMaxExtractBytes = 2ULL * 1024 * 1024 * 1024; // 2GB 展開上限

class MemoryOutStream : public ISequentialOutStream {
    LONG refCount_ = 1;
    std::vector<BYTE>& buf_;
public:
    MemoryOutStream(std::vector<BYTE>& b) : buf_(b) {}

    STDMETHOD(QueryInterface)(REFIID iid, void** ppv) override {
        if (iid == __uuidof(IUnknown) || iid == __uuidof(ISequentialOutStream)) {
            *ppv = static_cast<ISequentialOutStream*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    STDMETHOD_(ULONG, AddRef)() override { return InterlockedIncrement(&refCount_); }
    STDMETHOD_(ULONG, Release)() override {
        LONG r = InterlockedDecrement(&refCount_);
        if (r == 0) delete this;
        return r;
    }
    STDMETHOD(Write)(const void* data, UInt32 size, UInt32* processedSize) override {
        if (buf_.size() + size > kMaxExtractBytes) {
            if (processedSize) *processedSize = 0;
            return E_OUTOFMEMORY; // ZIP爆弾等のOOM防止
        }
        auto* p = (const BYTE*)data;
        buf_.insert(buf_.end(), p, p + size);
        if (processedSize) *processedSize = size;
        return S_OK;
    }
};

// === IArchiveExtractCallback 実装 ===
class MemoryExtractCallback : public IArchiveExtractCallback {
    LONG refCount_ = 1;
    std::vector<BYTE>& buffer_;
public:
    MemoryExtractCallback(std::vector<BYTE>& buf) : buffer_(buf) { buffer_.clear(); }

    STDMETHOD(QueryInterface)(REFIID iid, void** ppv) override {
        if (iid == __uuidof(IUnknown) || iid == __uuidof(IProgress) || iid == __uuidof(IArchiveExtractCallback)) {
            *ppv = static_cast<IArchiveExtractCallback*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    STDMETHOD_(ULONG, AddRef)() override { return InterlockedIncrement(&refCount_); }
    STDMETHOD_(ULONG, Release)() override {
        LONG r = InterlockedDecrement(&refCount_);
        if (r == 0) delete this;
        return r;
    }
    STDMETHOD(SetTotal)(UInt64) override { return S_OK; }
    STDMETHOD(SetCompleted)(const UInt64*) override { return S_OK; }
    STDMETHOD(GetStream)(UInt32, ISequentialOutStream** outStream, Int32 askExtractMode) override {
        if (askExtractMode != 0) { *outStream = nullptr; return S_OK; }
        *outStream = new MemoryOutStream(buffer_);
        return S_OK;
    }
    STDMETHOD(PrepareOperation)(Int32) override { return S_OK; }
    STDMETHOD(SetOperationResult)(Int32) override { return S_OK; }
};

// === ファイル出力ストリーム（大ファイルをテンプファイルに直接展開） ===
class FileOutStream : public ISequentialOutStream {
    LONG refCount_ = 1;
    HANDLE hFile_;
public:
    FileOutStream(HANDLE h) : hFile_(h) {}
    STDMETHOD(QueryInterface)(REFIID iid, void** ppv) override {
        if (iid == __uuidof(IUnknown) || iid == __uuidof(ISequentialOutStream)) {
            *ppv = static_cast<ISequentialOutStream*>(this); AddRef(); return S_OK;
        }
        *ppv = nullptr; return E_NOINTERFACE;
    }
    STDMETHOD_(ULONG, AddRef)() override { return InterlockedIncrement(&refCount_); }
    STDMETHOD_(ULONG, Release)() override {
        LONG r = InterlockedDecrement(&refCount_);
        if (r == 0) delete this;
        return r;
    }
    STDMETHOD(Write)(const void* data, UInt32 size, UInt32* processedSize) override {
        DWORD written = 0;
        BOOL ok = WriteFile(hFile_, data, size, &written, nullptr);
        if (processedSize) *processedSize = written;
        return ok ? S_OK : E_FAIL;
    }
};

class FileExtractCallback : public IArchiveExtractCallback {
    LONG refCount_ = 1;
    HANDLE hFile_;
public:
    FileExtractCallback(HANDLE h) : hFile_(h) {}
    STDMETHOD(QueryInterface)(REFIID iid, void** ppv) override {
        if (iid == __uuidof(IUnknown) || iid == __uuidof(IProgress) || iid == __uuidof(IArchiveExtractCallback)) {
            *ppv = static_cast<IArchiveExtractCallback*>(this); AddRef(); return S_OK;
        }
        *ppv = nullptr; return E_NOINTERFACE;
    }
    STDMETHOD_(ULONG, AddRef)() override { return InterlockedIncrement(&refCount_); }
    STDMETHOD_(ULONG, Release)() override {
        LONG r = InterlockedDecrement(&refCount_);
        if (r == 0) delete this;
        return r;
    }
    STDMETHOD(SetTotal)(UInt64) override { return S_OK; }
    STDMETHOD(SetCompleted)(const UInt64*) override { return S_OK; }
    STDMETHOD(GetStream)(UInt32, ISequentialOutStream** outStream, Int32 askExtractMode) override {
        if (askExtractMode != 0) { *outStream = nullptr; return S_OK; }
        *outStream = new FileOutStream(hFile_);
        return S_OK;
    }
    STDMETHOD(PrepareOperation)(Int32) override { return S_OK; }
    STDMETHOD(SetOperationResult)(Int32) override { return S_OK; }
};

// パストラバーサル検証（書庫エントリのセキュリティチェック）
static bool IsValidEntryPath(const std::wstring& path)
{
    if (path.find(L"..\\") != std::wstring::npos) return false;
    if (path.find(L"../") != std::wstring::npos) return false;
    if (path == L"..") return false;
    if (path.size() >= 2 && path[1] == L':') return false; // 絶対パス
    if (!path.empty() && (path[0] == L'\\' || path[0] == L'/')) return false;
    return true;
}

// === ヘルパー ===
// ファイル先頭のマジックバイトで RAR5 かどうか判定
static bool IsRar5File(const std::wstring& path)
{
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                               nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    BYTE header[8] = {};
    DWORD read = 0;
    ReadFile(hFile, header, 8, &read, nullptr);
    CloseHandle(hFile);
    // RAR5 signature: "Rar!\x1A\x07\x01\x00"
    return read >= 8 &&
           header[0] == 0x52 && header[1] == 0x61 && header[2] == 0x72 && header[3] == 0x21 &&
           header[4] == 0x1A && header[5] == 0x07 && header[6] == 0x01 && header[7] == 0x00;
}

static const GUID* GetFormatGUID(const std::wstring& path)
{
    const wchar_t* ext = PathFindExtensionW(path.c_str());
    if (!ext) return &CLSID_CFormatZip;
    if (_wcsicmp(ext, L".7z") == 0 || _wcsicmp(ext, L".cb7") == 0) return &CLSID_CFormat7z;
    if (_wcsicmp(ext, L".rar") == 0 || _wcsicmp(ext, L".cbr") == 0)
        return IsRar5File(path) ? &CLSID_CFormatRar5 : &CLSID_CFormatRar;
    return &CLSID_CFormatZip; // zip, cbz
}

// LRU キャッシュ付き書庫オープン（最大3個保持）
static IInArchive* GetCachedArchive(const std::wstring& archivePath)
{
    // LRU キャッシュから探す
    for (auto it = g_archiveCache.begin(); it != g_archiveCache.end(); ++it)
    {
        if (_wcsicmp(it->path.c_str(), archivePath.c_str()) == 0)
        {
            // ヒット → 先頭に移動（LRU更新）
            g_archiveCache.splice(g_archiveCache.begin(), g_archiveCache, it);
            g_currentHandle = &g_archiveCache.front();
            return g_currentHandle->archive;
        }
    }

    // キャッシュミス → 新規オープン
    if (!g_CreateObject) return nullptr;

    const GUID iid = __uuidof(IInArchive);

    // 試行するフォーマットGUIDリスト: 拡張子判定を最初に、失敗したら全形式を試す
    const GUID* allFormats[] = {
        &CLSID_CFormatZip, &CLSID_CFormat7z, &CLSID_CFormatRar, &CLSID_CFormatRar5
    };
    const GUID* primaryGuid = GetFormatGUID(archivePath);

    IInArchive* archive = nullptr;

    // まず拡張子から推定したフォーマットで試行
    auto tryOpen = [&](const GUID* guid) -> bool {
        IInArchive* arc = nullptr;
        HRESULT hr = g_CreateObject(guid, &iid, (void**)&arc);
        if (FAILED(hr) || !arc) return false;

        HANDLE hFile = CreateFileW(archivePath.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                   nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) { arc->Release(); return false; }

        FileInStream* stream = new FileInStream(hFile);
        UInt64 maxCheck = 1 << 22;
        hr = arc->Open(stream, &maxCheck, nullptr);
        stream->Release();

        if (SUCCEEDED(hr)) { archive = arc; return true; }
        arc->Release();
        return false;
    };

    // 1. 拡張子判定のフォーマットで試行
    if (!tryOpen(primaryGuid))
    {
        // 2. 失敗 → 全フォーマットを順番に試行（拡張子判定と同じものはスキップ）
        for (auto* fmt : allFormats)
        {
            if (fmt == primaryGuid) continue; // 既に試した
            if (tryOpen(fmt)) break;
        }
    }

    if (!archive) return nullptr;

    // LRU 上限チェック → 最古を閉じる
    if ((int)g_archiveCache.size() >= kMaxArchiveCache)
    {
        auto& oldest = g_archiveCache.back();
        if (oldest.archive) { oldest.archive->Close(); oldest.archive->Release(); }
        g_archiveCache.pop_back();
    }

    // 先頭に追加
    g_archiveCache.emplace_front();
    auto& handle = g_archiveCache.front();
    handle.archive = archive;
    handle.path = archivePath;

    // ディスクキャッシュからエントリリスト復元を試行（高速）
    if (LoadEntryCache(archivePath, handle.entryList, handle.entryIndexMap))
    {
        g_currentHandle = &handle;
        return archive;
    }

    // キャッシュミス → 7z.dll で全エントリ列挙（遅い）
    handle.entryIndexMap.clear();
    handle.entryList.clear();
    UInt32 numItems = 0;
    archive->GetNumberOfItems(&numItems);
    handle.entryList.reserve(numItems);

    for (UInt32 i = 0; i < numItems; i++)
    {
        PROPVARIANT prop;
        PropVariantInit(&prop);

        archive->GetProperty(i, kpidIsDir, &prop);
        bool isDir = (prop.vt == VT_BOOL && prop.boolVal != VARIANT_FALSE);
        PropVariantClear(&prop);
        if (isDir) continue;

        archive->GetProperty(i, kpidPath, &prop);
        std::wstring path;
        if (prop.vt == VT_BSTR && prop.bstrVal) path = prop.bstrVal;
        PropVariantClear(&prop);
        if (path.empty()) continue;
        if (!IsValidEntryPath(path)) continue; // パストラバーサル防止

        archive->GetProperty(i, kpidSize, &prop);
        ULONGLONG sz = 0;
        if (prop.vt == VT_UI8) sz = prop.uhVal.QuadPart;
        else if (prop.vt == VT_UI4) sz = prop.ulVal;
        PropVariantClear(&prop);

        handle.entryIndexMap[NormalizePath(path)] = i;
        handle.entryList.push_back({ i, path, sz });
    }

    g_currentHandle = &handle;

    // ディスクキャッシュに保存（次回起動時に高速復元）
    SaveEntryCache(archivePath, handle.entryList);

    return archive;
}

// === 公開関数 ===

bool InitArchive()
{
    if (g_7zDll) return true;

    wchar_t dllPath[MAX_PATH];
    GetModuleFileNameW(nullptr, dllPath, MAX_PATH);
    PathRemoveFileSpecW(dllPath);
    PathAppendW(dllPath, L"7z.dll");

    g_7zDll = LoadLibraryW(dllPath);
    if (!g_7zDll)
        g_7zDll = LoadLibraryW(L"C:\\Program Files\\7-Zip\\7z.dll");
    if (!g_7zDll) return false;

    g_CreateObject = (Func_CreateObject)GetProcAddress(g_7zDll, "CreateObject");
    return g_CreateObject != nullptr;
}

void CloseCurrentArchive()
{
    // 全キャッシュをクリア
    for (auto& h : g_archiveCache)
    {
        if (h.archive) { h.archive->Close(); h.archive->Release(); }
    }
    g_archiveCache.clear();
    g_currentHandle = nullptr;
}

void CloseArchive()
{
    CloseCurrentArchive();
    if (g_7zDll) { FreeLibrary(g_7zDll); g_7zDll = nullptr; }
    g_CreateObject = nullptr;
}

bool IsArchiveFile(const std::wstring& path)
{
    return HasExtension(path, kArchiveExts);
}

bool OpenArchiveAndGetEntries(const std::wstring& archivePath,
                              std::vector<ArchiveEntryRef>& outEntries)
{
    std::lock_guard<std::timed_mutex> lock(g_archiveMutex);

    IInArchive* archive = GetCachedArchive(archivePath);
    if (!archive || !g_currentHandle) { outEntries.clear(); return false; }

    outEntries.clear();
    outEntries.reserve(g_currentHandle->entryList.size());
    for (auto& e : g_currentHandle->entryList)
        outEntries.push_back({ e.path, e.size });

    return !outEntries.empty();
}

static DWORD g_mainThreadId = GetCurrentThreadId();

bool ExtractToMemory(const std::wstring& archivePath,
                     const std::wstring& entryPath,
                     std::vector<BYTE>& buffer)
{
    std::unique_lock<std::timed_mutex> lock(g_archiveMutex, std::defer_lock);
    if (GetCurrentThreadId() == g_mainThreadId)
    {
        lock.lock(); // UIスレッド: 即座にロック（待つ）
    }
    else
    {
        // ワーカー: 最大2秒待つ（サムネイル生成等で確実に取得）
        if (!lock.try_lock_for(std::chrono::seconds(2))) return false;
    }
    buffer.clear();

    IInArchive* archive = GetCachedArchive(archivePath);
    if (!archive) return false;

    // インデックスマップで O(1) 検索（in-placeで正規化、コピー削減）
    std::wstring key = entryPath;
    NormalizePathInPlace(key);
    if (!g_currentHandle) return false;
    auto it = g_currentHandle->entryIndexMap.find(key);
    if (it == g_currentHandle->entryIndexMap.end()) return false;

    // 既知サイズで事前確保（再アロケーション削減）
    UInt32 targetIndex = it->second;
    for (auto& e : g_currentHandle->entryList) {
        if (e.index == targetIndex) { buffer.reserve(static_cast<size_t>(e.size)); break; }
    }

    MemoryExtractCallback* cb = new MemoryExtractCallback(buffer);
    HRESULT hr = archive->Extract(&targetIndex, 1, 0, cb);
    cb->Release();

    return SUCCEEDED(hr) && !buffer.empty();
}

// === バッチ展開（プリフェッチ用、1回のロックで複数エントリ展開） ===
bool ExtractBatchToMemory(const std::wstring& archivePath,
                          const std::vector<std::wstring>& entryPaths,
                          std::vector<BatchExtractResult>& results)
{
    results.clear();
    results.resize(entryPaths.size());
    for (size_t i = 0; i < entryPaths.size(); i++)
    {
        results[i].entryPath = entryPaths[i];
        results[i].ok = false;
    }

    std::unique_lock<std::timed_mutex> lock(g_archiveMutex, std::defer_lock);
    if (GetCurrentThreadId() == g_mainThreadId)
        lock.lock();
    else
        if (!lock.try_lock_for(std::chrono::seconds(2))) return false;

    IInArchive* archive = GetCachedArchive(archivePath);
    if (!archive || !g_currentHandle) return false;

    for (size_t i = 0; i < entryPaths.size(); i++)
    {
        std::wstring key = entryPaths[i];
        NormalizePathInPlace(key);
        auto it = g_currentHandle->entryIndexMap.find(key);
        if (it == g_currentHandle->entryIndexMap.end()) continue;

        UInt32 targetIndex = it->second;
        // 既知サイズで事前確保（再アロケーション削減）
        for (auto& e : g_currentHandle->entryList) {
            if (e.index == targetIndex) { results[i].buffer.reserve(static_cast<size_t>(e.size)); break; }
        }
        MemoryExtractCallback* cb = new MemoryExtractCallback(results[i].buffer);
        HRESULT hr = archive->Extract(&targetIndex, 1, 0, cb);
        cb->Release();
        results[i].ok = SUCCEEDED(hr) && !results[i].buffer.empty();
    }
    return true;
}

// === ハイブリッド展開 ===
static constexpr ULONGLONG kMemoryThreshold = 2 * 1024 * 1024; // 2MB
static std::vector<std::wstring> g_tempFiles;
static std::mutex g_tempFilesMutex;

static std::wstring GetTempDir()
{
    wchar_t tmp[MAX_PATH];
    GetTempPathW(MAX_PATH, tmp);
    std::wstring dir = std::wstring(tmp) + L"karikari_media\\";
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

bool ExtractSmart(const std::wstring& archivePath,
                  const std::wstring& entryPath,
                  ULONGLONG fileSize,
                  std::vector<BYTE>& buffer,
                  std::wstring& tempPath)
{
    buffer.clear();
    tempPath.clear();

    if (fileSize < kMemoryThreshold)
    {
        // 小ファイル → メモリ展開
        return ExtractToMemory(archivePath, entryPath, buffer);
    }

    // 大ファイル → 7zからテンポラリファイルに直接展開（メモリ節約）
    std::wstring ext;
    auto dotPos = entryPath.rfind(L'.');
    if (dotPos != std::wstring::npos) ext = entryPath.substr(dotPos);

    wchar_t tmpFile[MAX_PATH];
    GetTempFileNameW(GetTempDir().c_str(), L"kk", 0, tmpFile);
    std::wstring finalPath = std::wstring(tmpFile) + ext;
    DeleteFileW(finalPath.c_str());
    MoveFileW(tmpFile, finalPath.c_str());

    HANDLE hFile = CreateFileW(finalPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        // テンプ作成失敗 → メモリ展開にフォールバック
        return ExtractToMemory(archivePath, entryPath, buffer);
    }

    // 7zから直接ファイルに展開
    bool ok = false;
    {
        std::unique_lock<std::timed_mutex> lock(g_archiveMutex, std::defer_lock);
        if (GetCurrentThreadId() == g_mainThreadId)
            lock.lock();
        else
            if (!lock.try_lock_for(std::chrono::seconds(5))) { CloseHandle(hFile); DeleteFileW(finalPath.c_str()); return false; }

        IInArchive* archive = GetCachedArchive(archivePath);
        if (archive && g_currentHandle)
        {
            std::wstring key = entryPath;
            NormalizePathInPlace(key);
            auto it = g_currentHandle->entryIndexMap.find(key);
            if (it != g_currentHandle->entryIndexMap.end())
            {
                UInt32 idx = it->second;
                auto* cb = new FileExtractCallback(hFile);
                ok = SUCCEEDED(archive->Extract(&idx, 1, 0, cb));
                cb->Release();
            }
        }
    }
    CloseHandle(hFile);

    if (ok)
    {
        { std::lock_guard<std::mutex> lk(g_tempFilesMutex); g_tempFiles.push_back(finalPath); }
        tempPath = finalPath;
        return true;
    }

    DeleteFileW(finalPath.c_str());
    return false;
}

void CleanupTempFiles()
{
    std::lock_guard<std::mutex> lock(g_tempFilesMutex);
    for (auto& f : g_tempFiles)
        DeleteFileW(f.c_str());
    g_tempFiles.clear();

    // テンポラリディレクトリも削除試行（空なら成功）
    wchar_t tmp[MAX_PATH];
    GetTempPathW(MAX_PATH, tmp);
    std::wstring dir = std::wstring(tmp) + L"karikari_media";
    RemoveDirectoryW(dir.c_str());
}

bool SplitArchivePath(const std::wstring& fullPath,
                      std::wstring& archivePath,
                      std::wstring& entryPath)
{
    auto pos = fullPath.rfind(L'!');
    if (pos == std::wstring::npos) return false;

    std::wstring candidate = fullPath.substr(0, pos);
    std::wstring entry = fullPath.substr(pos + 1);

    // ファイルシステムアクセスなし: 拡張子でアーカイブ判定
    if (!IsArchiveFile(candidate)) return false;
    if (entry.empty()) return false;

    archivePath = std::move(candidate);
    entryPath = std::move(entry);
    return true;
}
