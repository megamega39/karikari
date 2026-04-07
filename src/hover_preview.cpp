#include "hover_preview.h"
#include "archive.h"
#include "filelist.h"
#include "stream_cache.h"
#include <wincodec.h>
#include <wrl/client.h>
#include <list>
#include <tuple>

using Microsoft::WRL::ComPtr;

static constexpr int kHoverPreviewSize = 320;

int g_hoverItemIndex = -1;
std::wstring g_hoverPath;
std::atomic<int> g_hoverGeneration{0};

// ホバーキャッシュ（HBITMAP LRU、最大30枚）
struct HoverCacheEntry { std::wstring key; HBITMAP hBmp; UINT w, h; };
static std::list<HoverCacheEntry> g_hoverCache;
static std::unordered_map<std::wstring, std::list<HoverCacheEntry>::iterator> g_hoverCacheMap;
static constexpr int kHoverCacheMax = 30;

static HBITMAP HoverCacheGet(const std::wstring& key, UINT& w, UINT& h)
{
    auto it = g_hoverCacheMap.find(key);
    if (it == g_hoverCacheMap.end()) return nullptr;
    g_hoverCache.splice(g_hoverCache.begin(), g_hoverCache, it->second);
    w = it->second->w; h = it->second->h;
    return it->second->hBmp;
}

static void HoverCachePut(const std::wstring& key, HBITMAP hBmp, UINT w, UINT h)
{
    auto it = g_hoverCacheMap.find(key);
    if (it != g_hoverCacheMap.end())
    { DeleteObject(it->second->hBmp); g_hoverCache.erase(it->second); g_hoverCacheMap.erase(it); }

    while ((int)g_hoverCache.size() >= kHoverCacheMax)
    { auto& last = g_hoverCache.back(); DeleteObject(last.hBmp); g_hoverCacheMap.erase(last.key); g_hoverCache.pop_back(); }

    g_hoverCache.push_front({ key, hBmp, w, h });
    g_hoverCacheMap[key] = g_hoverCache.begin();
}

static bool g_hoverClassRegistered = false;

static LRESULT CALLBACK HoverPreviewProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_PAINT)
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        HBITMAP hBmp = (HBITMAP)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        if (hBmp)
        {
            RECT rc; GetClientRect(hwnd, &rc);
            HDC hdcMem = CreateCompatibleDC(hdc);
            HGDIOBJ hOld = SelectObject(hdcMem, hBmp);
            BITMAP bm; GetObject(hBmp, sizeof(bm), &bm);
            StretchBlt(hdc, 0, 0, rc.right, rc.bottom, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
            SelectObject(hdcMem, hOld);
            DeleteDC(hdcMem);
        }
        else
        {
            RECT rc; GetClientRect(hwnd, &rc);
            FillRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    if (msg == WM_DESTROY)
    {
        HBITMAP hBmp = (HBITMAP)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        if (hBmp) DeleteObject(hBmp);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void ShowHoverPreviewWindow(const std::wstring& path, HBITMAP hBmp, UINT dstW, UINT dstH, POINT screenPos)
{
    HideHoverPreview();
    if (!g_hoverClassRegistered)
    {
        WNDCLASSEXW wc = {}; wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = HoverPreviewProc; wc.hInstance = g_app.hInstance;
        wc.lpszClassName = L"KarikariHoverPreview"; wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        RegisterClassExW(&wc); g_hoverClassRegistered = true;
    }
    int wx = screenPos.x + 16, wy = screenPos.y + 16;
    HMONITOR hMon = MonitorFromPoint(screenPos, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = {}; mi.cbSize = sizeof(mi);
    GetMonitorInfoW(hMon, &mi);
    if (wx + (int)dstW > mi.rcWork.right) wx = screenPos.x - (int)dstW - 8;
    if (wy + (int)dstH > mi.rcWork.bottom) wy = mi.rcWork.bottom - (int)dstH;

    // HBITMAPをコピー（キャッシュに所有権があるので）
    HDC hdcScreen = GetDC(nullptr);
    HDC hdcSrc = CreateCompatibleDC(hdcScreen); HDC hdcDst = CreateCompatibleDC(hdcScreen);
    HBITMAP hCopy = CreateCompatibleBitmap(hdcScreen, dstW, dstH);
    SelectObject(hdcSrc, hBmp); SelectObject(hdcDst, hCopy);
    BitBlt(hdcDst, 0, 0, dstW, dstH, hdcSrc, 0, 0, SRCCOPY);
    DeleteDC(hdcSrc); DeleteDC(hdcDst); ReleaseDC(nullptr, hdcScreen);

    g_app.hwndHoverPreview = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
        L"KarikariHoverPreview", nullptr, WS_POPUP | WS_BORDER,
        wx, wy, dstW, dstH, g_app.wnd.hwndMain, nullptr, g_app.hInstance, nullptr);
    if (g_app.hwndHoverPreview)
    { SetWindowLongPtrW(g_app.hwndHoverPreview, GWLP_USERDATA, (LONG_PTR)hCopy); ShowWindow(g_app.hwndHoverPreview, SW_SHOWNOACTIVATE); }
    else DeleteObject(hCopy);
}

void ShowHoverPreview(const std::wstring& path, POINT screenPos)
{
    HideHoverPreview();

    // キャッシュチェック（即表示）
    UINT cw, ch;
    HBITMAP cached = HoverCacheGet(path, cw, ch);
    if (cached)
    {
        ShowHoverPreviewWindow(path, cached, cw, ch, screenPos);
        return;
    }

    // 非同期デコード: バックグラウンドスレッドで実行
    int generation = ++g_hoverGeneration;
    std::wstring pathCopy = path;
    POINT posCopy = screenPos;

    struct HoverResult { int generation; HBITMAP hBmp; UINT w, h; POINT pos; wchar_t path[MAX_PATH]; };

    PTP_WORK work = CreateThreadpoolWork([](PTP_CALLBACK_INSTANCE, PVOID ctx, PTP_WORK work) {
        auto* params = (std::tuple<std::wstring, int, POINT>*)ctx;
        auto& [path, gen, pos] = *params;

        if (gen != g_hoverGeneration.load()) { delete params; CloseThreadpoolWork(work); return; }

        // COM初期化
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        ComPtr<IWICImagingFactory> factory;
        CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.GetAddressOf()));
        if (!factory) { CoUninitialize(); delete params; CloseThreadpoolWork(work); return; }

        ComPtr<IWICBitmapDecoder> decoder;
        ComPtr<IWICBitmapFrameDecode> frame;
        ComPtr<IWICStream> wicStream;
        std::vector<BYTE> buffer;
        HRESULT hr;

        std::wstring arcPath, entryPath;
        if (SplitArchivePath(path, arcPath, entryPath))
        {
            auto cachedStream = StreamCacheGet(path);
            if (!cachedStream)
            {
                if (!ExtractToMemory(arcPath, entryPath, buffer))
                { CoUninitialize(); delete params; CloseThreadpoolWork(work); return; }
                StreamCachePut(path, std::move(buffer));
                cachedStream = StreamCacheGet(path);
            }
            if (!cachedStream)
            { CoUninitialize(); delete params; CloseThreadpoolWork(work); return; }
            hr = factory->CreateStream(wicStream.GetAddressOf());
            if (SUCCEEDED(hr)) hr = wicStream->InitializeFromMemory(
                const_cast<BYTE*>(cachedStream->data()), (DWORD)cachedStream->size());
            if (SUCCEEDED(hr)) hr = factory->CreateDecoderFromStream(wicStream.Get(), nullptr, WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf());
        }
        else if (IsArchiveFile(path))
        {
            std::vector<ArchiveEntryRef> entries;
            if (!OpenArchiveAndGetEntries(path, entries) || entries.empty())
            { CoUninitialize(); delete params; CloseThreadpoolWork(work); return; }
            std::wstring firstImage;
            for (int i = 0; i < (int)entries.size(); i++)
                if (IsImageFile(entries[i].path)) { firstImage = entries[i].path; break; }
            if (firstImage.empty() || !ExtractToMemory(path, firstImage, buffer))
            { CoUninitialize(); delete params; CloseThreadpoolWork(work); return; }
            hr = factory->CreateStream(wicStream.GetAddressOf());
            if (SUCCEEDED(hr)) hr = wicStream->InitializeFromMemory(buffer.data(), (DWORD)buffer.size());
            if (SUCCEEDED(hr)) hr = factory->CreateDecoderFromStream(wicStream.Get(), nullptr, WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf());
        }
        else
        {
            hr = factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf());
        }

        if (FAILED(hr) || !decoder || gen != g_hoverGeneration.load())
        { CoUninitialize(); delete params; CloseThreadpoolWork(work); return; }
        hr = decoder->GetFrame(0, frame.GetAddressOf());
        if (FAILED(hr)) { CoUninitialize(); delete params; CloseThreadpoolWork(work); return; }

        UINT imgW, imgH;
        frame->GetSize(&imgW, &imgH);
        if (imgW == 0 || imgH == 0) { CoUninitialize(); delete params; CloseThreadpoolWork(work); return; }

        float scale = std::min((float)kHoverPreviewSize / imgW, (float)kHoverPreviewSize / imgH);
        if (scale > 1.0f) scale = 1.0f;
        UINT dstW = std::max(1U, (UINT)(imgW * scale));
        UINT dstH = std::max(1U, (UINT)(imgH * scale));

        ComPtr<IWICBitmapScaler> scaler;
        factory->CreateBitmapScaler(scaler.GetAddressOf());
        scaler->Initialize(frame.Get(), dstW, dstH, WICBitmapInterpolationModeLinear);
        ComPtr<IWICFormatConverter> converter;
        factory->CreateFormatConverter(converter.GetAddressOf());
        converter->Initialize(scaler.Get(), GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = dstW; bmi.bmiHeader.biHeight = -(int)dstH;
        bmi.bmiHeader.biPlanes = 1; bmi.bmiHeader.biBitCount = 32;
        void* pBits = nullptr;
        HDC hdcScreen = GetDC(nullptr);
        HBITMAP hBmp = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
        ReleaseDC(nullptr, hdcScreen);
        if (!hBmp) { CoUninitialize(); delete params; CloseThreadpoolWork(work); return; }
        converter->CopyPixels(nullptr, dstW * 4, dstW * dstH * 4, (BYTE*)pBits);

        // UIスレッドに通知
        auto* result = new HoverResult();
        result->generation = gen; result->hBmp = hBmp; result->w = dstW; result->h = dstH; result->pos = pos;
        wcsncpy_s(result->path, path.c_str(), MAX_PATH - 1);
        if (!PostMessageW(g_app.wnd.hwndMain, WM_HOVER_PREVIEW_DONE, 0, (LPARAM)result))
        { DeleteObject(hBmp); delete result; }

        CoUninitialize();
        delete params;
        CloseThreadpoolWork(work);
    }, new std::tuple<std::wstring, int, POINT>(pathCopy, generation, posCopy), nullptr);

    if (work) SubmitThreadpoolWork(work);
}

void HideHoverPreview()
{
    if (g_app.hwndHoverPreview)
    {
        DestroyWindow(g_app.hwndHoverPreview);
        g_app.hwndHoverPreview = nullptr;
    }
}

void HandleHoverPreviewDone(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    // バックグラウンドでデコード完了 → UIスレッドで表示
    struct HoverResult { int generation; HBITMAP hBmp; UINT w, h; POINT pos; wchar_t path[MAX_PATH]; };
    auto* r = (HoverResult*)lParam;
    if (r)
    {
        if (r->generation == g_hoverGeneration.load() && r->hBmp)
        {
            HoverCachePut(r->path, r->hBmp, r->w, r->h);
            // ウィンドウ表示
            HideHoverPreview();
            if (!g_hoverClassRegistered)
            {
                WNDCLASSEXW wc = {}; wc.cbSize = sizeof(wc);
                wc.lpfnWndProc = HoverPreviewProc; wc.hInstance = g_app.hInstance;
                wc.lpszClassName = L"KarikariHoverPreview"; wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
                RegisterClassExW(&wc); g_hoverClassRegistered = true;
            }
            POINT screenPos = r->pos;
            int wx = screenPos.x + 16, wy = screenPos.y + 16;
            HMONITOR hMon = MonitorFromPoint(screenPos, MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi = {}; mi.cbSize = sizeof(mi);
            GetMonitorInfoW(hMon, &mi);
            if (wx + (int)r->w > mi.rcWork.right) wx = screenPos.x - (int)r->w - 8;
            if (wy + (int)r->h > mi.rcWork.bottom) wy = mi.rcWork.bottom - (int)r->h;

            // HBITMAPをコピー（キャッシュに所有権があるので）
            HDC hdcScreen = GetDC(nullptr);
            HDC hdcSrc = CreateCompatibleDC(hdcScreen);
            HDC hdcDst = CreateCompatibleDC(hdcScreen);
            HBITMAP hCopy = CreateCompatibleBitmap(hdcScreen, r->w, r->h);
            SelectObject(hdcSrc, r->hBmp);
            SelectObject(hdcDst, hCopy);
            BitBlt(hdcDst, 0, 0, r->w, r->h, hdcSrc, 0, 0, SRCCOPY);
            DeleteDC(hdcSrc); DeleteDC(hdcDst); ReleaseDC(nullptr, hdcScreen);

            g_app.hwndHoverPreview = CreateWindowExW(
                WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
                L"KarikariHoverPreview", nullptr, WS_POPUP | WS_BORDER,
                wx, wy, r->w, r->h, g_app.wnd.hwndMain, nullptr, g_app.hInstance, nullptr);
            if (g_app.hwndHoverPreview)
            {
                SetWindowLongPtrW(g_app.hwndHoverPreview, GWLP_USERDATA, (LONG_PTR)hCopy);
                ShowWindow(g_app.hwndHoverPreview, SW_SHOWNOACTIVATE);
            }
            else
                DeleteObject(hCopy);
        }
        else if (r->hBmp)
            DeleteObject(r->hBmp);
        delete r;
    }
}

void CleanupHoverPreview()
{
    HideHoverPreview();
    for (auto& entry : g_hoverCache)
        DeleteObject(entry.hBmp);
    g_hoverCache.clear();
    g_hoverCacheMap.clear();
}
