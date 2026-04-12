#include "filelist.h"
#include "archive.h"
#include "media.h"
#include "utils.h"
#include "i18n.h"
#include "viewer.h"
#include <shlwapi.h>
#include <shlobj.h>
#include <uxtheme.h>
#include <unordered_map>
#include <thread>
#include <atomic>

constexpr UINT WM_THUMB_DONE = WM_APP + 8;


#pragma comment(lib, "shlwapi.lib")

// フィルター用グローバル
static std::wstring g_filterText;
static std::vector<FileItem> g_allFileItems;

static const wchar_t* kImageExts[] = {
    L".jpg", L".jpeg", L".png", L".bmp", L".gif",
    L".webp", L".avif", L".tiff", L".tif", L".ico"
};

bool IsImageFile(const std::wstring& path)
{
    return HasExtension(path, kImageExts);
}

bool IsViewableFile(const std::wstring& name)
{
    return IsImageFile(name) || IsArchiveFile(name) || IsMediaFile(name);
}

// フォルダ・画像・書庫のみ表示対象
static bool IsSupportedFile(const std::wstring& name, bool isDir)
{
    if (isDir) return true;
    return IsViewableFile(name);
}

void LoadFolder(const std::wstring& folderPath)
{
    g_app.nav.fileItems.clear();
    g_app.nav.viewableFiles.clear();
    g_app.nav.currentFolder = folderPath;

    std::wstring searchPath = EnsureTrailingSlash(folderPath) + L'*';

    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileExW(searchPath.c_str(), FindExInfoBasic, &fd,
                                     FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do
    {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0)
            continue;
        // 隠しファイル・システムファイルをスキップ
        if (fd.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))
            continue;

        bool isDir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        // フォルダ・画像・書庫のみ
        if (!IsSupportedFile(fd.cFileName, isDir))
            continue;

        FileItem item;
        item.name = fd.cFileName;
        item.fullPath = EnsureTrailingSlash(folderPath) + fd.cFileName;
        item.isDirectory = isDir;
        item.fileSize = ((ULONGLONG)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
        item.lastWriteTime = fd.ftLastWriteTime;

        g_app.nav.fileItems.push_back(std::move(item));
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);

    // ソート: フォルダ優先、名前順
    std::sort(g_app.nav.fileItems.begin(), g_app.nav.fileItems.end(),
        [](const FileItem& a, const FileItem& b) {
            if (a.isDirectory != b.isDirectory) return a.isDirectory > b.isDirectory;
            return _wcsicmp(a.name.c_str(), b.name.c_str()) < 0;
        });

    // フィルター前の全アイテムを保存
    g_allFileItems = g_app.nav.fileItems;
    g_filterText.clear();

    // fileItems逆引きマップ構築（GoToFile内のListView同期用）
    g_app.nav.fileItemIndex.clear();
    for (int i = 0; i < (int)g_app.nav.fileItems.size(); i++)
    {
        std::wstring key = ToLowerW(g_app.nav.fileItems[i].fullPath);
        g_app.nav.fileItemIndex[key] = i;
    }

    // 画像ファイルリスト構築 + O(1)検索用インデックスマップ
    g_app.nav.viewableFileIndex.clear();
    for (auto& item : g_app.nav.fileItems)
    {
        if (!item.isDirectory && (IsImageFile(item.fullPath) || IsMediaFile(item.fullPath)))
        {
            int idx = (int)g_app.nav.viewableFiles.size();
            g_app.nav.viewableFiles.push_back(item.fullPath);
            std::wstring key = ToLowerW(item.fullPath);
            g_app.nav.viewableFileIndex[key] = idx;
        }
    }
}

std::wstring FormatFileSize(ULONGLONG size)
{
    wchar_t buf[64];
    if (size < 1024)
        swprintf_s(buf, 64, L"%llu B", size);
    else if (size < 1024 * 1024)
        swprintf_s(buf, 64, L"%.1f KB", size / 1024.0);
    else if (size < 1024ULL * 1024 * 1024)
        swprintf_s(buf, 64, L"%.1f MB", size / (1024.0 * 1024.0));
    else
        swprintf_s(buf, 64, L"%.1f GB", size / (1024.0 * 1024.0 * 1024.0));
    return buf;
}

void InitListView()
{
    HWND hwnd = g_app.wnd.hwndList;
    if (!hwnd) return;

    // システムイメージリストをListViewに設定
    SHFILEINFOW sfi = {};
    HIMAGELIST hSysImgList = (HIMAGELIST)SHGetFileInfoW(
        L"C:\\", 0, &sfi, sizeof(sfi),
        SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
    if (hSysImgList)
        SendMessageW(hwnd, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)hSysImgList);
}

std::wstring FormatFileTime(const FILETIME& ft)
{
    if (ft.dwLowDateTime == 0 && ft.dwHighDateTime == 0) return L"";
    FILETIME local;
    SYSTEMTIME st;
    FileTimeToLocalFileTime(&ft, &local);
    FileTimeToSystemTime(&local, &st);
    wchar_t buf[64];
    swprintf_s(buf, 64, L"%04d/%02d/%02d %02d:%02d",
               st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
    return buf;
}

std::wstring GetFileType(const std::wstring& name, bool isDir)
{
    if (isDir) return I18nGet(L"type.folder");
    const wchar_t* ext = PathFindExtensionW(name.c_str());
    if (!ext || !*ext) return I18nGet(L"type.file");

    struct { const wchar_t* ext; const wchar_t* key; } map[] = {
        { L".jpg",  L"type.jpeg" }, { L".jpeg", L"type.jpeg" },
        { L".png",  L"type.png" },  { L".gif",  L"type.gif" },
        { L".bmp",  L"type.bmp" },  { L".webp", L"type.webp" },
        { L".avif", L"type.avif" }, { L".tiff", L"type.tiff" },
        { L".tif",  L"type.tiff" }, { L".ico",  L"type.ico" },
        { L".zip",  L"type.zip" },  { L".cbz",  L"type.zip" },
        { L".7z",   L"type.7z" },   { L".cb7",  L"type.7z" },
        { L".rar",  L"type.rar" },  { L".cbr",  L"type.rar" },
    };
    for (auto& m : map)
        if (_wcsicmp(ext, m.ext) == 0) return I18nGet(m.key);

    return std::wstring(ext + 1) + L" " + I18nGet(L"type.file");
}

// アイコンキャッシュ（拡張子→アイコンインデックス）
static std::unordered_map<std::wstring, int> g_iconCache;
static int g_folderIcon = -1;

int GetFileIconIndex(const std::wstring& path, bool isDir)
{
    if (isDir)
    {
        if (g_folderIcon < 0)
        {
            SHFILEINFOW sfi = {};
            SHGetFileInfoW(L"folder", FILE_ATTRIBUTE_DIRECTORY, &sfi, sizeof(sfi),
                           SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
            g_folderIcon = sfi.iIcon;
        }
        return g_folderIcon;
    }

    // 拡張子でキャッシュ
    const wchar_t* ext = PathFindExtensionW(path.c_str());
    std::wstring key = ToLowerW(ext ? std::wstring(ext) : std::wstring(L""));

    auto it = g_iconCache.find(key);
    if (it != g_iconCache.end()) return it->second;

    SHFILEINFOW sfi = {};
    SHGetFileInfoW(path.c_str(), FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi),
                   SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
    g_iconCache[key] = sfi.iIcon;
    return sfi.iIcon;
}

static bool g_isGridMode = false; // 前方宣言（SwitchToGridViewより前で使用）
void SwitchToGridView(); // 前方宣言

void PopulateListView()
{
    HWND hwnd = g_app.wnd.hwndList;
    if (!hwnd) return;

    // グリッドモード中は再構築が必要
    if (g_isGridMode)
    {
        SwitchToGridView();
        return;
    }

    // LVS_OWNERDATA: アイテム数を設定するだけ（データは LVN_GETDISPINFOW で返す）
    SendMessageW(hwnd, LVM_SETITEMCOUNT, (WPARAM)g_app.nav.fileItems.size(), LVSICF_NOINVALIDATEALL);
    InvalidateRect(hwnd, nullptr, TRUE);
}

// ソート用の静的変数
static int g_sortColumn = 0;
static bool g_sortAscending = true;

static int CompareFileItems(const FileItem& a, const FileItem& b)
{
    // フォルダ優先（通常モード時）
    if (!g_app.nav.inArchiveMode && a.isDirectory != b.isDirectory)
        return a.isDirectory ? -1 : 1;

    int cmp = 0;
    switch (g_sortColumn)
    {
    case 0: // 名前
        cmp = _wcsicmp(a.name.c_str(), b.name.c_str());
        break;
    case 1: // サイズ
        if (a.fileSize < b.fileSize) cmp = -1;
        else if (a.fileSize > b.fileSize) cmp = 1;
        break;
    case 2: // 種類
    {
        const wchar_t* extA = PathFindExtensionW(a.name.c_str());
        const wchar_t* extB = PathFindExtensionW(b.name.c_str());
        cmp = _wcsicmp(extA ? extA : L"", extB ? extB : L"");
        break;
    }
    case 3: // 更新日時
        cmp = CompareFileTime(&a.lastWriteTime, &b.lastWriteTime);
        break;
    }

    return g_sortAscending ? cmp : -cmp;
}

void SortFileList(int column)
{
    if (column == g_sortColumn)
        g_sortAscending = !g_sortAscending;
    else
    {
        g_sortColumn = column;
        g_sortAscending = true;
    }

    std::sort(g_app.nav.fileItems.begin(), g_app.nav.fileItems.end(),
        [](const FileItem& a, const FileItem& b) {
            return CompareFileItems(a, b) < 0;
        });

    // フィルター用バックアップも同期ソート
    g_allFileItems = g_app.nav.fileItems;

    RebuildViewableFiles();
    PopulateListView();
}

void RebuildViewableFiles()
{
    ClearSpreadCache(); // 見開き判定キャッシュをクリア
    g_app.nav.viewableFiles.clear();
    g_app.nav.viewableFileIndex.clear();
    // fileItemsの逆引きマップも再構築
    g_app.nav.fileItemIndex.clear();
    for (int i = 0; i < (int)g_app.nav.fileItems.size(); i++)
    {
        std::wstring key = ToLowerW(g_app.nav.fileItems[i].fullPath);
        g_app.nav.fileItemIndex[key] = i;
    }
    for (auto& item : g_app.nav.fileItems)
    {
        const auto& name = g_app.nav.inArchiveMode ? item.name : item.fullPath;
        if (!item.isDirectory && (IsImageFile(name) || IsMediaFile(name)))
        {
            int idx = (int)g_app.nav.viewableFiles.size();
            g_app.nav.viewableFiles.push_back(item.fullPath);
            std::wstring key = ToLowerW(item.fullPath);
            g_app.nav.viewableFileIndex[key] = idx;
        }
    }
}

void ListViewSelectItem(int index, int selectCount)
{
    HWND hwnd = g_app.wnd.hwndList;
    if (!hwnd) return;

    // 現在の選択を全解除
    int prev = -1;
    while ((prev = (int)SendMessageW(hwnd, LVM_GETNEXTITEM, (WPARAM)prev, LVNI_SELECTED)) >= 0)
    {
        LVITEMW lvi = {};
        lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
        lvi.state = 0;
        SendMessageW(hwnd, LVM_SETITEMSTATE, prev, (LPARAM)&lvi);
    }

    int total = (int)SendMessageW(hwnd, LVM_GETITEMCOUNT, 0, 0);
    for (int i = 0; i < selectCount; i++)
    {
        int sel = index + i;
        if (sel >= 0 && sel < total)
        {
            LVITEMW lvi = {};
            lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
            lvi.state = LVIS_SELECTED | (i == 0 ? LVIS_FOCUSED : 0);
            SendMessageW(hwnd, LVM_SETITEMSTATE, sel, (LPARAM)&lvi);
        }
    }
    if (index >= 0 && index < total)
        SendMessageW(hwnd, LVM_ENSUREVISIBLE, index, FALSE);
}

void SetFileListFilter(const std::wstring& filter)
{
    g_filterText = filter;

    // フィルターが空なら全アイテム表示
    if (filter.empty())
    {
        g_app.nav.fileItems = g_allFileItems;
    }
    else
    {
        // 大文字小文字無視でフィルタリング
        std::wstring lowerFilter = ToLowerW(filter);

        g_app.nav.fileItems.clear();
        for (auto& item : g_allFileItems)
        {
            std::wstring lowerName = ToLowerW(item.name);
            if (lowerName.find(lowerFilter) != std::wstring::npos)
                g_app.nav.fileItems.push_back(item);
        }
    }

    RebuildViewableFiles();
    PopulateListView();
}

// === サムネイルグリッド表示 ===
static constexpr int kThumbSize = 128;
static HIMAGELIST g_thumbImageList = nullptr;
static std::unordered_map<int, int> g_thumbIndexMap;
// g_isGridMode は210行で定義済み

// メモリバッファからデコーダを作成するヘルパー
static HRESULT CreateDecoderFromMemory(IWICImagingFactory* wic, const BYTE* data, size_t size,
                                        IWICBitmapDecoder** ppDecoder)
{
    ComPtr<IWICStream> stream;
    HRESULT hr = wic->CreateStream(stream.GetAddressOf());
    if (FAILED(hr)) return hr;
    hr = stream->InitializeFromMemory(const_cast<BYTE*>(data), (DWORD)size);
    if (FAILED(hr)) return hr;
    return wic->CreateDecoderFromStream(stream.Get(), nullptr,
        WICDecodeMetadataCacheOnDemand, ppDecoder);
}

static HBITMAP CreateThumbnailBitmap(const std::wstring& path, int cx, int cy,
                                     IWICImagingFactory* factory = nullptr,
                                     const BYTE* memData = nullptr, size_t memSize = 0)
{
    IWICImagingFactory* wic = factory ? factory : g_app.viewer.wicFactory.Get();
    if (!wic) return nullptr;

    ComPtr<IWICBitmapDecoder> decoder;
    HRESULT hr;
    if (memData && memSize > 0)
        hr = CreateDecoderFromMemory(wic, memData, memSize, decoder.GetAddressOf());
    else
        hr = wic->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
            WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf());
    if (FAILED(hr)) return nullptr;

    ComPtr<IWICBitmapFrameDecode> frame;
    decoder->GetFrame(0, frame.GetAddressOf());
    if (!frame) return nullptr;

    UINT imgW, imgH;
    frame->GetSize(&imgW, &imgH);
    float scale = std::min((float)cx / imgW, (float)cy / imgH);
    if (scale > 1.0f) scale = 1.0f;
    UINT dstW = std::max(1U, (UINT)(imgW * scale));
    UINT dstH = std::max(1U, (UINT)(imgH * scale));

    // IWICBitmapSourceTransformでネイティブダウンスケール（JPEG高速化）
    ComPtr<IWICBitmapSource> scaledSource;
    ComPtr<IWICBitmapSourceTransform> transform;
    if (SUCCEEDED(frame->QueryInterface(IID_PPV_ARGS(transform.GetAddressOf()))))
    {
        UINT closestW = dstW, closestH = dstH;
        WICPixelFormatGUID pfSrc;
        if (SUCCEEDED(transform->GetClosestSize(&closestW, &closestH)) &&
            SUCCEEDED(transform->GetClosestPixelFormat(&pfSrc)))
        {
            ComPtr<IWICBitmap> nativeBmp;
            if (SUCCEEDED(wic->CreateBitmap(closestW, closestH, pfSrc, WICBitmapCacheOnLoad, nativeBmp.GetAddressOf())))
            {
                ComPtr<IWICBitmapLock> lock;
                WICRect rcLock = { 0, 0, (INT)closestW, (INT)closestH };
                if (SUCCEEDED(nativeBmp->Lock(&rcLock, WICBitmapLockWrite, lock.GetAddressOf())))
                {
                    UINT bufSize = 0; BYTE* buf = nullptr; UINT stride = 0;
                    lock->GetDataPointer(&bufSize, &buf);
                    lock->GetStride(&stride);
                    if (SUCCEEDED(transform->CopyPixels(nullptr, closestW, closestH, &pfSrc, WICBitmapTransformRotate0, stride, bufSize, buf)))
                    {
                        lock.Reset();
                        // さらにリサイズが必要な場合（closestW > dstW）
                        if (closestW > dstW || closestH > dstH)
                        {
                            ComPtr<IWICBitmapScaler> scaler2;
                            wic->CreateBitmapScaler(scaler2.GetAddressOf());
                            if (scaler2) { scaler2->Initialize(nativeBmp.Get(), dstW, dstH, WICBitmapInterpolationModeLinear); scaledSource = scaler2; }
                        }
                        else
                        {
                            dstW = closestW; dstH = closestH;
                            scaledSource = nativeBmp;
                        }
                    }
                }
            }
        }
    }
    // フォールバック: WICBitmapScaler
    if (!scaledSource)
    {
        ComPtr<IWICBitmapScaler> scaler;
        wic->CreateBitmapScaler(scaler.GetAddressOf());
        if (!scaler) return nullptr;
        scaler->Initialize(frame.Get(), dstW, dstH,
            (scale > 0.5f) ? WICBitmapInterpolationModeLinear : WICBitmapInterpolationModeFant);
        scaledSource = scaler;
    }

    ComPtr<IWICFormatConverter> converter;
    wic->CreateFormatConverter(converter.GetAddressOf());
    if (!converter) return nullptr;
    converter->Initialize(scaledSource.Get(), GUID_WICPixelFormat32bppBGRA,
        WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = cx;
    bmi.bmiHeader.biHeight = -cy;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;

    void* pBits = nullptr;
    HBITMAP hbmp = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
    if (!hbmp) return nullptr;

    memset(pBits, 0xFF, cx * cy * 4); // 白背景
    int offX = (cx - (int)dstW) / 2;
    int offY = (cy - (int)dstH) / 2;

    std::vector<BYTE> pixels(dstW * dstH * 4);
    converter->CopyPixels(nullptr, dstW * 4, (UINT)pixels.size(), pixels.data());

    BYTE* dst = (BYTE*)pBits;
    for (UINT row = 0; row < dstH; row++)
    {
        if (offY + (int)row >= cy) break;
        memcpy(dst + (offY + row) * cx * 4 + offX * 4,
               pixels.data() + row * dstW * 4, dstW * 4);
    }
    return hbmp;
}

// サムネイル生成スレッド管理
static std::atomic<bool> g_thumbCancelFlag{false};
static std::thread g_thumbThread;

// 個別サムネイル生成ワーカー（スレッドプールで並行実行）
struct ThumbWorkItem { int idx; std::wstring path; HWND hwndMain; };

static void ThumbWorkerFunc(PTP_CALLBACK_INSTANCE, PVOID ctx, PTP_WORK)
{
    auto* wi = (ThumbWorkItem*)ctx;
    if (g_thumbCancelFlag.load(std::memory_order_relaxed))
    { delete wi; return; }

    HRESULT hrCom = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool comInit = SUCCEEDED(hrCom);
    ComPtr<IWICImagingFactory> factory;
    CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                     IID_PPV_ARGS(factory.GetAddressOf()));
    if (!factory) { if (comInit) CoUninitialize(); delete wi; return; }

    HBITMAP hbmp = nullptr;
    std::wstring arcPath, entryPath;
    if (SplitArchivePath(wi->path, arcPath, entryPath))
    {
        // 書庫内画像
        std::vector<BYTE> buffer;
        if (ExtractToMemory(arcPath, entryPath, buffer) && !buffer.empty())
            hbmp = CreateThumbnailBitmap(wi->path, kThumbSize, kThumbSize, factory.Get(),
                                         buffer.data(), buffer.size());
    }
    else if (IsArchiveFile(wi->path))
    {
        // 書庫ファイル → 中の最初の画像をサムネイルに
        std::vector<ArchiveEntryRef> entries;
        if (OpenArchiveAndGetEntries(wi->path, entries))
        {
            for (auto& e : entries)
            {
                if (IsImageFile(e.path))
                {
                    std::vector<BYTE> buffer;
                    if (ExtractToMemory(wi->path, e.path, buffer) && !buffer.empty())
                        hbmp = CreateThumbnailBitmap(wi->path, kThumbSize, kThumbSize,
                                                     factory.Get(), buffer.data(), buffer.size());
                    break;
                }
            }
        }
    }
    else
    {
        DWORD attr = GetFileAttributesW(wi->path.c_str());
        if (IsImageFile(wi->path))
        {
            // 通常の画像ファイル
            hbmp = CreateThumbnailBitmap(wi->path, kThumbSize, kThumbSize, factory.Get());
        }
        else
        {
            // フォルダ・動画・その他 → シェルサムネイルを試行
            ComPtr<IShellItem> shellItem;
            if (SUCCEEDED(SHCreateItemFromParsingName(wi->path.c_str(), nullptr, IID_PPV_ARGS(shellItem.GetAddressOf()))))
            {
                IShellItemImageFactory* imgFactory = nullptr;
                if (SUCCEEDED(shellItem->QueryInterface(IID_PPV_ARGS(&imgFactory))))
                {
                    SIZE sz = { kThumbSize, kThumbSize };
                    imgFactory->GetImage(sz, SIIGBF_THUMBNAILONLY, &hbmp);
                    if (!hbmp) // THUMBNAILONLY失敗 → アイコンも許可
                        imgFactory->GetImage(sz, SIIGBF_ICONONLY, &hbmp);
                    imgFactory->Release();
                }
            }

            // フォルダでシェルサムネイル取得失敗 → 中の最初の画像を探す
            if (!hbmp && attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
            {
                std::wstring search = wi->path + L"\\*";
                WIN32_FIND_DATAW fd;
                HANDLE hFind = FindFirstFileExW(search.c_str(), FindExInfoBasic, &fd,
                                                 FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH);
                if (hFind != INVALID_HANDLE_VALUE)
                {
                    do {
                        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && IsImageFile(fd.cFileName))
                        {
                            std::wstring imgPath = wi->path + L"\\" + fd.cFileName;
                            hbmp = CreateThumbnailBitmap(imgPath, kThumbSize, kThumbSize, factory.Get());
                            break;
                        }
                    } while (FindNextFileW(hFind, &fd));
                    FindClose(hFind);
                }
            }
        }
    }

    if (hbmp && !g_thumbCancelFlag.load(std::memory_order_relaxed))
    {
        if (!PostMessageW(wi->hwndMain, WM_THUMB_DONE, (WPARAM)wi->idx, (LPARAM)hbmp))
            DeleteObject(hbmp);
    }
    else if (hbmp)
        DeleteObject(hbmp);

    if (comInit) CoUninitialize();
    delete wi;
}

static void ThumbGeneratorThread(std::vector<std::pair<int, std::wstring>> items, HWND hwndMain)
{
    // バッチ単位（12個ずつ）でスレッドプール投入→完了待ち→次バッチ
    constexpr int kBatchSize = 12;
    size_t i = 0;
    while (i < items.size())
    {
        if (g_thumbCancelFlag.load(std::memory_order_relaxed)) break;
        std::vector<PTP_WORK> batch;
        for (int b = 0; b < kBatchSize && i < items.size(); b++, i++)
        {
            if (g_thumbCancelFlag.load(std::memory_order_relaxed)) break;
            auto* wi = new ThumbWorkItem{ items[i].first, items[i].second, hwndMain };
            PTP_WORK work = CreateThreadpoolWork(ThumbWorkerFunc, wi, nullptr);
            if (work) { SubmitThreadpoolWork(work); batch.push_back(work); }
            else delete wi;
        }
        // バッチ完了待ち
        for (auto* w : batch)
        {
            WaitForThreadpoolWorkCallbacks(w, g_thumbCancelFlag.load(std::memory_order_relaxed) ? TRUE : FALSE);
            CloseThreadpoolWork(w);
        }
    }
}

// UIスレッドから呼ばれる: WM_THUMB_DONE ハンドラ
void OnThumbnailDone(WPARAM wParam, LPARAM lParam)
{
    int itemIndex = (int)wParam;
    HBITMAP hbmp = (HBITMAP)lParam;
    if (!hbmp || !g_thumbImageList || !g_isGridMode) { if (hbmp) DeleteObject(hbmp); return; }

    HWND hwnd = g_app.wnd.hwndList;
    if (!hwnd) { DeleteObject(hbmp); return; }

    int imgIdx = ImageList_Add(g_thumbImageList, hbmp, nullptr);
    DeleteObject(hbmp);

    if (imgIdx >= 0)
    {
        LVITEMW lvi = {};
        lvi.mask = LVIF_IMAGE;
        lvi.iItem = itemIndex;
        lvi.iSubItem = 0;
        lvi.iImage = imgIdx;
        ListView_SetItem(hwnd, &lvi);
        ListView_RedrawItems(hwnd, itemIndex, itemIndex);
    }
}

// ListView を新しいスタイルで再作成するヘルパー
static HWND RecreateListView(HWND parent, DWORD style)
{
    HWND oldHwnd = g_app.wnd.hwndList;
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE);

    // 旧 ListView の位置を取得
    RECT rc = {};
    if (oldHwnd) GetWindowRect(oldHwnd, &rc);
    MapWindowPoints(nullptr, parent, (LPPOINT)&rc, 2);

    // 旧 ListView 破棄
    if (oldHwnd) DestroyWindow(oldHwnd);

    // 新 ListView 作成
    HWND hwnd = CreateWindowExW(
        0, WC_LISTVIEWW, nullptr,
        WS_CHILD | WS_VISIBLE | LVS_SHOWSELALWAYS | style,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        parent, (HMENU)(UINT_PTR)IDC_LISTVIEW, hInst, nullptr);

    g_app.wnd.hwndList = hwnd;

    // テーマ + フォント
    SetWindowTheme(hwnd, L"Explorer", nullptr);
    static HFONT hUIFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Yu Gothic UI");
    SendMessageW(hwnd, WM_SETFONT, (WPARAM)hUIFont, TRUE);

    return hwnd;
}

void SwitchToGridView()
{
    if (!g_app.wnd.hwndList) return;

    // 前回のサムネイルスレッドをキャンセル
    g_thumbCancelFlag.store(true, std::memory_order_release);
    if (g_thumbThread.joinable()) g_thumbThread.join();
    g_thumbCancelFlag.store(false, std::memory_order_release);

    g_isGridMode = true;

    // ListView を LVS_ICON モードで再作成（LVS_OWNERDATA の内部状態問題を回避）
    HWND hwnd = RecreateListView(g_app.wnd.hwndMain, LVS_ICON);
    SendMessageW(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_DOUBLEBUFFER);

    // サムネイルImageList作成
    if (g_thumbImageList) { ImageList_Destroy(g_thumbImageList); g_thumbImageList = nullptr; }
    g_thumbIndexMap.clear();
    g_thumbImageList = ImageList_Create(kThumbSize, kThumbSize, ILC_COLOR32, 64, 32);

    // デフォルトアイコン (index 0: 灰色プレースホルダー)
    {
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = kThumbSize; bmi.bmiHeader.biHeight = -kThumbSize;
        bmi.bmiHeader.biPlanes = 1; bmi.bmiHeader.biBitCount = 32;
        void* pBits = nullptr;
        HBITMAP hbmp = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
        if (hbmp) { memset(pBits, 0xE0, kThumbSize * kThumbSize * 4); ImageList_Add(g_thumbImageList, hbmp, nullptr); DeleteObject(hbmp); }
    }

    SendMessageW(hwnd, LVM_SETIMAGELIST, LVSIL_NORMAL, (LPARAM)g_thumbImageList);

    // アイコン間隔を設定（サムネイルサイズ + テキスト余白）
    int spacing = kThumbSize + 20;
    SendMessageW(hwnd, LVM_SETICONSPACING, 0, MAKELPARAM(spacing, spacing + 24));

    // 全アイテムをデフォルトアイコンで即座に挿入
    SendMessageW(hwnd, WM_SETREDRAW, FALSE, 0);
    int count = (int)g_app.nav.fileItems.size();
    std::vector<std::pair<int, std::wstring>> thumbItems;
    for (int i = 0; i < count; i++)
    {
        auto& item = g_app.nav.fileItems[i];

        LVITEMW lvi = {};
        lvi.mask = LVIF_TEXT | LVIF_IMAGE;
        lvi.iItem = i;
        lvi.iSubItem = 0;
        lvi.pszText = (LPWSTR)item.name.c_str();
        lvi.iImage = 0;
        int actualIdx = (int)SendMessageW(hwnd, LVM_INSERTITEMW, 0, (LPARAM)&lvi);

        // 全ファイル種別でサムネイル生成を試行
        thumbItems.push_back({ actualIdx, item.fullPath });
    }
    SendMessageW(hwnd, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hwnd, nullptr, TRUE);

    // バックグラウンドでサムネイル生成開始
    if (!thumbItems.empty())
        g_thumbThread = std::thread(ThumbGeneratorThread, std::move(thumbItems), g_app.wnd.hwndMain);
}

void SwitchToListView()
{
    if (!g_app.wnd.hwndList) return;

    // サムネイルスレッドをキャンセル
    g_thumbCancelFlag.store(true, std::memory_order_release);
    if (g_thumbThread.joinable()) g_thumbThread.join();
    g_thumbCancelFlag.store(false, std::memory_order_release);

    g_isGridMode = false;

    // サムネイルImageListをクリア
    if (g_thumbImageList) { ImageList_Destroy(g_thumbImageList); g_thumbImageList = nullptr; }
    g_thumbIndexMap.clear();

    // ListView を LVS_REPORT | LVS_OWNERDATA モードで再作成
    HWND hwnd = RecreateListView(g_app.wnd.hwndMain, LVS_REPORT | LVS_OWNERDATA);
    SendMessageW(hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                 LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    SendMessageW(hwnd, LVM_SETBKCOLOR, 0, (LPARAM)RGB(235, 244, 255));
    SendMessageW(hwnd, LVM_SETTEXTBKCOLOR, 0, (LPARAM)RGB(235, 244, 255));

    // システムイメージリスト
    SHFILEINFOW sfi = {};
    HIMAGELIST hSysImgList = (HIMAGELIST)SHGetFileInfoW(
        L"C:\\", 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
    if (hSysImgList) SendMessageW(hwnd, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)hSysImgList);

    // カラム再設定
    LVCOLUMNW lvc = {};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;
    lvc.cx = 200; lvc.pszText = (LPWSTR)L"名前";
    SendMessageW(hwnd, LVM_INSERTCOLUMNW, 0, (LPARAM)&lvc);
    lvc.cx = 80; lvc.pszText = (LPWSTR)L"サイズ";
    SendMessageW(hwnd, LVM_INSERTCOLUMNW, 1, (LPARAM)&lvc);
    lvc.cx = 90; lvc.pszText = (LPWSTR)L"種類";
    SendMessageW(hwnd, LVM_INSERTCOLUMNW, 2, (LPARAM)&lvc);
    lvc.cx = 130; lvc.pszText = (LPWSTR)L"更新日時";
    SendMessageW(hwnd, LVM_INSERTCOLUMNW, 3, (LPARAM)&lvc);

    PopulateListView();
}

int GetThumbnailIndex(int fileItemIndex)
{
    if (!g_isGridMode) return -1;
    auto it = g_thumbIndexMap.find(fileItemIndex);
    return (it != g_thumbIndexMap.end()) ? it->second : 0;
}
