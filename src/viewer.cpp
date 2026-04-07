#include "viewer.h"
#include "decoder.h"
#include "nav.h"
#include "cache.h"
#include "stream_cache.h"
#include "utils.h"
#include "media.h"
#include "window.h"
#include "viewer_toolbar.h"
#include "archive.h"
#include <mutex>
#include <atomic>

static bool LoadBitmapCached(const std::wstring& path, ComPtr<ID2D1Bitmap>& out);
static constexpr UINT_PTR kAnimTimerId = 42;

static HRESULT CreateViewerRenderTarget(HWND hwnd)
{
    auto& dc = g_app.viewer.deviceContext;
    if (!dc) return E_FAIL;

    // 古いリソースを解放
    dc->SetTarget(nullptr);
    g_app.viewer.backBuffer.Reset();
    g_app.viewer.swapChain.Reset();

    RECT rc;
    GetClientRect(hwnd, &rc);
    UINT width = std::max(1L, rc.right - rc.left);
    UINT height = std::max(1L, rc.bottom - rc.top);

    // DXGIファクトリ取得
    ComPtr<IDXGIDevice> dxgiDevice;
    HRESULT hr = g_app.viewer.d3dDevice.As(&dxgiDevice);
    if (FAILED(hr)) return E_FAIL;
    ComPtr<IDXGIAdapter> adapter;
    hr = dxgiDevice->GetAdapter(adapter.GetAddressOf());
    if (FAILED(hr)) return E_FAIL;
    ComPtr<IDXGIFactory2> dxgiFactory;
    hr = adapter->GetParent(IID_PPV_ARGS(dxgiFactory.GetAddressOf()));
    if (FAILED(hr)) return E_FAIL;

    // スワップチェーン作成
    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

    hr = dxgiFactory->CreateSwapChainForHwnd(
        g_app.viewer.d3dDevice.Get(), hwnd, &desc,
        nullptr, nullptr, g_app.viewer.swapChain.GetAddressOf());
    if (FAILED(hr)) return hr;

    // バックバッファ取得 → D2Dターゲット設定
    ComPtr<IDXGISurface> surface;
    hr = g_app.viewer.swapChain->GetBuffer(0, IID_PPV_ARGS(surface.GetAddressOf()));
    if (FAILED(hr)) return hr;

    D2D1_BITMAP_PROPERTIES1 bmpProps = {};
    bmpProps.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bmpProps.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    bmpProps.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
    float dpiX, dpiY;
    dc->GetDpi(&dpiX, &dpiY);
    bmpProps.dpiX = dpiX;
    bmpProps.dpiY = dpiY;

    hr = dc->CreateBitmapFromDxgiSurface(surface.Get(), &bmpProps,
                                          g_app.viewer.backBuffer.GetAddressOf());
    if (FAILED(hr)) return hr;

    dc->SetTarget(g_app.viewer.backBuffer.Get());
    return S_OK;
}

static float CalcBaseScale(D2D1_SIZE_F rtSize, D2D1_SIZE_F bmpSize)
{
    switch (g_app.viewer.scaleMode)
    {
    case FitWindow:
    {
        float s = std::min(rtSize.width / bmpSize.width, rtSize.height / bmpSize.height);
        return std::min(s, 1.0f);
    }
    case FitWidth:
        return rtSize.width / bmpSize.width;
    case FitHeight:
        return rtSize.height / bmpSize.height;
    case Original:
        return 1.0f;
    }
    return 1.0f;
}

// 単独ページ描画
static void PaintSingle(ID2D1RenderTarget* rt)
{
    if (!g_app.viewer.bitmap) return;

    auto rtSize = rt->GetSize();
    auto bmpSize = g_app.viewer.bitmap->GetSize();

    // 回転90/270度時は幅高さを入れ替えてスケール計算
    bool rotated = (g_app.viewer.rotation == 90 || g_app.viewer.rotation == 270);
    D2D1_SIZE_F logicalSize = rotated
        ? D2D1::SizeF(bmpSize.height, bmpSize.width)
        : bmpSize;

    float baseScale = CalcBaseScale(rtSize, logicalSize);
    float scale = baseScale * g_app.viewer.zoom;

    float drawW = logicalSize.width * scale;
    float drawH = logicalSize.height * scale;
    float cx = rtSize.width / 2.0f + g_app.viewer.scrollX;
    float cy = rtSize.height / 2.0f + g_app.viewer.scrollY;

    auto interpMode = (scale > 2.0f)
        ? D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR
        : D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;

    // 回転変換
    if (g_app.viewer.rotation != 0)
    {
        rt->SetTransform(
            D2D1::Matrix3x2F::Rotation((float)g_app.viewer.rotation, D2D1::Point2F(cx, cy)));
    }

    // 回転後の描画矩形（回転前の座標系で原画像サイズ基準）
    float origDrawW = bmpSize.width * scale;
    float origDrawH = bmpSize.height * scale;
    float x = cx - origDrawW / 2.0f;
    float y = cy - origDrawH / 2.0f;

    rt->DrawBitmap(g_app.viewer.bitmap.Get(),
        D2D1::RectF(x, y, x + origDrawW, y + origDrawH),
        1.0f, interpMode);

    if (g_app.viewer.rotation != 0)
        rt->SetTransform(D2D1::Matrix3x2F::Identity());
}

// 見開き描画: 2枚を左右に隙間なく中央配置
static void PaintSpread(ID2D1RenderTarget* rt)
{
    if (!g_app.viewer.bitmap || !g_app.viewer.bitmap2) return;

    auto rtSize = rt->GetSize();
    auto s1 = g_app.viewer.bitmap->GetSize();
    auto s2 = g_app.viewer.bitmap2->GetSize();

    // 左右どちらのページか（RTL対応）
    ID2D1Bitmap* leftBmp = g_app.viewer.isRTL ? g_app.viewer.bitmap2.Get() : g_app.viewer.bitmap.Get();
    ID2D1Bitmap* rightBmp = g_app.viewer.isRTL ? g_app.viewer.bitmap.Get() : g_app.viewer.bitmap2.Get();
    auto leftSize = leftBmp->GetSize();
    auto rightSize = rightBmp->GetSize();

    // 高さ基準でス���ール合わせ
    float scaleL = rtSize.height / leftSize.height;
    float scaleR = rtSize.height / rightSize.height;

    float drawLW = leftSize.width * scaleL;
    float drawLH = leftSize.height * scaleL;
    float drawRW = rightSize.width * scaleR;
    float drawRH = rightSize.height * scaleR;

    float totalW = drawLW + drawRW;

    // 合計幅がビューアーを超える場合は縮小
    if (totalW > rtSize.width)
    {
        float shrink = rtSize.width / totalW;
        drawLW *= shrink;
        drawLH *= shrink;
        drawRW *= shrink;
        drawRH *= shrink;
        totalW = rtSize.width;
    }

    // FitWindowで等倍超え防止
    if (g_app.viewer.scaleMode == FitWindow)
    {
        float maxScale = std::max(drawLW / leftSize.width, drawRW / rightSize.width);
        if (maxScale > 1.0f)
        {
            float fix = 1.0f / maxScale;
            drawLW *= fix; drawLH *= fix;
            drawRW *= fix; drawRH *= fix;
            totalW = drawLW + drawRW;
        }
    }

    // ズーム適用
    drawLW *= g_app.viewer.zoom; drawLH *= g_app.viewer.zoom;
    drawRW *= g_app.viewer.zoom; drawRH *= g_app.viewer.zoom;
    totalW *= g_app.viewer.zoom;

    float startX = (rtSize.width - totalW) / 2.0f + g_app.viewer.scrollX;
    float maxDrawH = std::max(drawLH, drawRH);
    float baseY = (rtSize.height - maxDrawH) / 2.0f + g_app.viewer.scrollY;

    // 左ページ
    float ly = baseY + (maxDrawH - drawLH) / 2.0f;
    rt->DrawBitmap(leftBmp,
        D2D1::RectF(startX, ly, startX + drawLW, ly + drawLH),
        1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);

    // 右ページ
    float rx = startX + drawLW;
    float ry = baseY + (maxDrawH - drawRH) / 2.0f;
    rt->DrawBitmap(rightBmp,
        D2D1::RectF(rx, ry, rx + drawRW, ry + drawRH),
        1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
}

static void OnViewerPaint(HWND hwnd)
{
    if (!g_app.viewer.deviceContext || !g_app.viewer.swapChain) return;

    g_app.viewer.deviceContext->BeginDraw();
    g_app.viewer.deviceContext->Clear(D2D1::ColorF(D2D1::ColorF::Black));

    if (g_app.viewer.isSpreadActive && g_app.viewer.bitmap2)
        PaintSpread(g_app.viewer.deviceContext.Get());
    else
        PaintSingle(g_app.viewer.deviceContext.Get());

    HRESULT hr = g_app.viewer.deviceContext->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET)
    {
        g_app.viewer.deviceContext->SetTarget(nullptr);
        g_app.viewer.deviceContext.Reset();
        g_app.viewer.backBuffer.Reset();
        g_app.viewer.swapChain.Reset();
        g_app.viewer.d3dDevice.Reset();
        g_app.viewer.bitmap.Reset();
        g_app.viewer.bitmap2.Reset();
        CacheClear(); // キャッシュ内の無効ビットマップを全削除
        if (SUCCEEDED(InitD2D()))
            CreateViewerRenderTarget(hwnd);
        // デバイスロスト回復後に現在の画像を再デコード・再表示
        if (g_app.nav.currentFileIndex >= 0)
            GoToFile(g_app.nav.currentFileIndex);
    }
    else if (SUCCEEDED(hr))
    {
        // 静止画はVSyncなし（即時表示）、アニメ時はVSyncあり
        if (g_app.viewer.isAnimating)
            g_app.viewer.swapChain->Present(1, 0);
        else
            g_app.viewer.swapChain->Present(0, 0);
    }
}

static bool IsImageOverflowing()
{
    if (!g_app.viewer.bitmap || !g_app.viewer.deviceContext) return false;

    auto rtSize = g_app.viewer.deviceContext->GetSize();
    auto bmpSize = g_app.viewer.bitmap->GetSize();
    float baseScale = CalcBaseScale(rtSize, bmpSize);
    float scale = baseScale * g_app.viewer.zoom;

    return (bmpSize.width * scale > rtSize.width) ||
           (bmpSize.height * scale > rtSize.height);
}

static LRESULT CALLBACK ViewerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        DragAcceptFiles(hwnd, TRUE);
        CreateViewerRenderTarget(hwnd);
        return 0;

    case WM_SIZE:
        if (g_app.viewer.deviceContext && g_app.viewer.swapChain)
        {
            RECT rc;
            GetClientRect(hwnd, &rc);
            UINT width = std::max(1L, rc.right - rc.left);
            UINT height = std::max(1L, rc.bottom - rc.top);

            // バックバッファ解放 → リサイズ → 再取得
            g_app.viewer.deviceContext->SetTarget(nullptr);
            g_app.viewer.backBuffer.Reset();
            HRESULT hr = g_app.viewer.swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
            {
                // デバイスロスト: D2Dデバイスも完全再作成
                g_app.viewer.deviceContext.Reset();
                g_app.viewer.swapChain.Reset();
                g_app.viewer.d3dDevice.Reset();
                g_app.viewer.bitmap.Reset();
                g_app.viewer.bitmap2.Reset();
                CacheClear();
                if (SUCCEEDED(InitD2D()))
                    CreateViewerRenderTarget(hwnd);
                if (g_app.nav.currentFileIndex >= 0)
                    GoToFile(g_app.nav.currentFileIndex);
            }
            else if (SUCCEEDED(hr))
            {
                ComPtr<IDXGISurface> surface;
                hr = g_app.viewer.swapChain->GetBuffer(0, IID_PPV_ARGS(surface.GetAddressOf()));
                if (SUCCEEDED(hr))
                {
                    D2D1_BITMAP_PROPERTIES1 bmpProps = {};
                    bmpProps.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
                    bmpProps.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
                    bmpProps.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
                    float dpiX, dpiY;
                    g_app.viewer.deviceContext->GetDpi(&dpiX, &dpiY);
                    bmpProps.dpiX = dpiX;
                    bmpProps.dpiY = dpiY;
                    g_app.viewer.deviceContext->CreateBitmapFromDxgiSurface(
                        surface.Get(), &bmpProps, g_app.viewer.backBuffer.GetAddressOf());
                    g_app.viewer.deviceContext->SetTarget(g_app.viewer.backBuffer.Get());
                }
            }
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        OnViewerPaint(hwnd);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_MOUSEWHEEL:
    {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        bool ctrl = (GET_KEYSTATE_WPARAM(wParam) & MK_CONTROL) != 0;

        if (ctrl)
        {
            ViewerZoomSmooth(delta > 0);
        }
        else
        {
            int step = delta > 0 ? -GetPagesPerView() : GetPagesPerView();
            GoToFile(g_app.nav.currentFileIndex + step);
        }
        return 0;
    }

    case WM_TIMER:
    {
        if (wParam == kAnimTimerId && g_app.viewer.isAnimating && !g_app.viewer.animFrames.empty())
        {
            // 次のフレームへ
            g_app.viewer.animCurrentFrame = (g_app.viewer.animCurrentFrame + 1) % (int)g_app.viewer.animFrames.size();
            g_app.viewer.bitmap = g_app.viewer.animFrames[g_app.viewer.animCurrentFrame];
            InvalidateRect(hwnd, nullptr, FALSE);

            // 次のフレームのディレイでタイマー再設定
            KillTimer(hwnd, kAnimTimerId);
            int delay = g_app.viewer.animDelays[g_app.viewer.animCurrentFrame];
            g_app.viewer.animTimer = SetTimer(hwnd, kAnimTimerId, delay, nullptr);
        }
        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        // パン開始準備（5px threshold で実際のパン開始を判定）
        g_app.viewer.panStart = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
        g_app.viewer.panScrollStartX = g_app.viewer.scrollX;
        g_app.viewer.panScrollStartY = g_app.viewer.scrollY;
        g_app.viewer.isPanning = false; // まだ開始しない
        SetCapture(hwnd);
        return 0;
    }

    case WM_MOUSEMOVE:
    {
        if (GetCapture() == hwnd)
        {
            int mx = (short)LOWORD(lParam);
            int my = (short)HIWORD(lParam);
            int dx = mx - g_app.viewer.panStart.x;
            int dy = my - g_app.viewer.panStart.y;

            if (!g_app.viewer.isPanning)
            {
                // 5px threshold
                if ((dx * dx + dy * dy) >= 25 && IsImageOverflowing())
                {
                    g_app.viewer.isPanning = true;
                    SetCursor(LoadCursorW(nullptr, IDC_SIZEALL));
                }
            }

            if (g_app.viewer.isPanning)
            {
                float newX = g_app.viewer.panScrollStartX + dx;
                float newY = g_app.viewer.panScrollStartY + dy;
                if (newX != g_app.viewer.scrollX || newY != g_app.viewer.scrollY)
                {
                    g_app.viewer.scrollX = newX;
                    g_app.viewer.scrollY = newY;
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
        }
        return 0;
    }

    case WM_LBUTTONUP:
    {
        if (GetCapture() == hwnd)
        {
            g_app.viewer.isPanning = false;
            ReleaseCapture();
            SetCursor(LoadCursorW(nullptr, IDC_ARROW));
        }
        return 0;
    }

    case WM_LBUTTONDBLCLK:
        PostMessageW(g_app.wnd.hwndMain, WM_TOGGLE_FULLSCREEN, 0, 0);
        return 0;

    case WM_RBUTTONUP:
    {
        POINT pt = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
        ClientToScreen(hwnd, &pt);
        PostMessageW(g_app.wnd.hwndMain, WM_SHOW_CONTEXT_MENU, 1, MAKELPARAM(pt.x, pt.y));
        return 0;
    }

    case WM_DROPFILES:
    {
        HDROP hDrop = (HDROP)wParam;
        wchar_t path[MAX_PATH];
        if (DragQueryFileW(hDrop, 0, path, MAX_PATH))
            NavigateTo(path);
        DragFinish(hDrop);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool RegisterViewerClass(HINSTANCE hInst)
{
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = ViewerWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"KarikariViewer";
    return RegisterClassExW(&wc) != 0;
}

HWND CreateViewer(HWND parent, HINSTANCE hInst)
{
    HWND hwnd = CreateWindowExW(
        WS_EX_ACCEPTFILES,
        L"KarikariViewer", nullptr,
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        0, 0, 100, 100,
        parent, (HMENU)(UINT_PTR)IDC_VIEWER, hInst, nullptr);
    g_app.wnd.hwndViewer = hwnd;
    return hwnd;
}

// アニメーションGIF/WebPのフレーム読み込み
static bool TryLoadAnimation(const std::wstring& path)
{
    if (!g_app.viewer.wicFactory || !g_app.viewer.deviceContext) return false;

    // .gif/.webp 以外はアニメーションチェック不要（高速スキップ）
    const wchar_t* ext = PathFindExtensionW(path.c_str());
    if (!ext) return false;
    if (_wcsicmp(ext, L".gif") != 0 && _wcsicmp(ext, L".webp") != 0) return false;

    ComPtr<IWICBitmapDecoder> decoder;
    HRESULT hr = g_app.viewer.wicFactory->CreateDecoderFromFilename(
        path.c_str(), nullptr, GENERIC_READ,
        WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf());
    if (FAILED(hr)) return false;

    UINT frameCount = 0;
    decoder->GetFrameCount(&frameCount);
    if (frameCount <= 1) return false; // 静止画

    g_app.viewer.animFrames.clear();
    g_app.viewer.animDelays.clear();

    size_t totalAnimMem = 0;
    constexpr size_t kMaxAnimMem = 100ULL * 1024 * 1024; // 100MB
    constexpr UINT kMaxAnimFrames = 50;

    for (UINT i = 0; i < frameCount && i < kMaxAnimFrames; i++)
    {
        ComPtr<IWICBitmapFrameDecode> frame;
        hr = decoder->GetFrame(i, frame.GetAddressOf());
        if (FAILED(hr)) break;

        // フレーム遅延時間をメタデータから取得
        int delay = 100; // デフォルト100ms
        ComPtr<IWICMetadataQueryReader> meta;
        if (SUCCEEDED(frame->GetMetadataQueryReader(meta.GetAddressOf())))
        {
            PROPVARIANT val;
            PropVariantInit(&val);
            // GIF: /grctlext/Delay (1/100秒単位)
            if (SUCCEEDED(meta->GetMetadataByName(L"/grctlext/Delay", &val)))
            {
                if (val.vt == VT_UI2) delay = val.uiVal * 10; // 1/100秒 → ms
                PropVariantClear(&val);
            }
            // WebP: /imgdesc/Delay (ms単位、形式による)
        }
        if (delay < 20) delay = 20; // 最低20ms

        // D2D Bitmap に変換
        ComPtr<IWICFormatConverter> converter;
        hr = g_app.viewer.wicFactory->CreateFormatConverter(converter.GetAddressOf());
        if (FAILED(hr)) break;
        hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
        if (FAILED(hr)) break;

        ComPtr<ID2D1Bitmap> bmp;
        hr = g_app.viewer.deviceContext->CreateBitmapFromWicBitmap(converter.Get(), nullptr, bmp.GetAddressOf());
        if (FAILED(hr)) break;

        g_app.viewer.animFrames.push_back(bmp);
        g_app.viewer.animDelays.push_back(delay);

        // メモリ上限チェック
        auto sz = bmp->GetSize();
        totalAnimMem += (size_t)(sz.width * sz.height * 4);
        if (totalAnimMem > kMaxAnimMem) break;
    }

    return g_app.viewer.animFrames.size() > 1;
}

void ViewerStopAnimation()
{
    if (g_app.viewer.animTimer)
    {
        KillTimer(g_app.wnd.hwndViewer, kAnimTimerId);
        g_app.viewer.animTimer = 0;
    }
    g_app.viewer.isAnimating = false;
    g_app.viewer.animFrames.clear();
    g_app.viewer.animDelays.clear();
    g_app.viewer.animCurrentFrame = 0;
}

void ViewerLoadImage(const std::wstring& path)
{
    if (!g_app.viewer.deviceContext) return;

    ViewerStopAnimation();
    g_app.viewer.bitmap.Reset();
    g_app.viewer.bitmap2.Reset();
    g_app.viewer.isSpreadActive = false;
    g_app.viewer.rotation = 0;

    // アニメーション試行
    if (TryLoadAnimation(path))
    {
        g_app.viewer.isAnimating = true;
        g_app.viewer.animCurrentFrame = 0;
        g_app.viewer.bitmap = g_app.viewer.animFrames[0];
        g_app.nav.currentPath = path;
        g_app.viewer.zoom = 1.0f;
        g_app.viewer.scrollX = 0.0f;
        g_app.viewer.scrollY = 0.0f;

        // 最初のフレームのディレイでタイマー開始
        g_app.viewer.animTimer = SetTimer(g_app.wnd.hwndViewer, kAnimTimerId, g_app.viewer.animDelays[0], nullptr);
        InvalidateRect(g_app.wnd.hwndViewer, nullptr, FALSE);
        return;
    }

    // 通常の静止画
    if (LoadBitmapCached(path, g_app.viewer.bitmap))
    {
        g_app.nav.currentPath = path;
        g_app.viewer.zoom = 1.0f;
        g_app.viewer.scrollX = 0.0f;
        g_app.viewer.scrollY = 0.0f;
        InvalidateRect(g_app.wnd.hwndViewer, nullptr, FALSE);
    }
}

// --- 非同期デコード ---
// COM RAII ラッパー（スレッドごとに初期化/解放）
struct AsyncComInitGuard {
    bool initialized = false;
    AsyncComInitGuard() { initialized = SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)); }
    ~AsyncComInitGuard() { if (initialized) CoUninitialize(); }
};
static thread_local AsyncComInitGuard tl_asyncComInit;
static thread_local ComPtr<IWICImagingFactory> tl_asyncWicFactory;

static IWICImagingFactory* GetAsyncWicFactory()
{
    (void)tl_asyncComInit;
    if (!tl_asyncWicFactory)
    {
        CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                         IID_PPV_ARGS(tl_asyncWicFactory.GetAddressOf()));
    }
    return tl_asyncWicFactory.Get();
}

static std::atomic<int> g_asyncDecodeGeneration{0}; // キャンセル用世代番号

int GetAsyncDecodeGeneration() { return g_asyncDecodeGeneration.load(); }

void ViewerLoadImageAsync(const std::wstring& path)
{
    // キャッシュヒット時は即表示
    auto cached = CacheGet(path);
    if (cached)
    {
        ViewerStopAnimation();
        g_app.viewer.bitmap = cached;
        g_app.viewer.bitmap2.Reset();
        g_app.viewer.isSpreadActive = false;
        g_app.viewer.rotation = 0;
        g_app.nav.currentPath = path;
        g_app.viewer.zoom = 1.0f;
        g_app.viewer.scrollX = 0.0f;
        g_app.viewer.scrollY = 0.0f;
        InvalidateRect(g_app.wnd.hwndViewer, nullptr, FALSE);
        return;
    }

    // キャッシュミス: 前の画像を維持したままバックグラウンドでデコード
    int generation = ++g_asyncDecodeGeneration;
    std::wstring pathCopy = path;

    struct AsyncDecodeResult {
        int generation;
        ComPtr<IWICBitmap> wicBitmap;
        std::wstring path;
    };

    auto result = std::make_unique<AsyncDecodeResult>(AsyncDecodeResult{ generation, {}, pathCopy });

    PTP_WORK work = CreateThreadpoolWork([](PTP_CALLBACK_INSTANCE, PVOID ctx, PTP_WORK work) {
        // unique_ptr で所有権を再取得（スコープアウト時に自動delete）
        std::unique_ptr<AsyncDecodeResult> r((AsyncDecodeResult*)ctx);
        IWICImagingFactory* factory = GetAsyncWicFactory();
        if (!factory) { CloseThreadpoolWork(work); return; }

        // 世代チェック（新しいリクエストが来ていたらスキップ）
        if (r->generation != g_asyncDecodeGeneration.load()) { CloseThreadpoolWork(work); return; }

        HRESULT hr;
        std::wstring arcPath, entryPath;
        if (SplitArchivePath(r->path, arcPath, entryPath))
        {
            // 書庫内画像: ストリームキャッシュ → ExtractToMemory → デコード（ゼロコピー）
            auto cachedStream = StreamCacheGet(r->path);
            if (!cachedStream)
            {
                std::vector<BYTE> buffer;
                if (!ExtractToMemory(arcPath, entryPath, buffer))
                { CloseThreadpoolWork(work); return; }
                StreamCachePut(r->path, std::move(buffer));
                cachedStream = StreamCacheGet(r->path);
            }
            if (!cachedStream || r->generation != g_asyncDecodeGeneration.load()) { CloseThreadpoolWork(work); return; }
            hr = DecodeMemoryToWicBitmap(cachedStream->data(), cachedStream->size(), factory, r->wicBitmap.GetAddressOf());
        }
        else
        {
            hr = DecodeToWicBitmap(r->path, factory, r->wicBitmap.GetAddressOf());
        }

        if (FAILED(hr) || !r->wicBitmap || r->generation != g_asyncDecodeGeneration.load())
        { CloseThreadpoolWork(work); return; }

        // UIスレッドに通知（パスを構造体で渡す）
        auto* msg = new AsyncDecodeDoneMsg{ r->wicBitmap.Detach(), r->path, r->generation };
        if (!PostMessageW(g_app.wnd.hwndMain, WM_ASYNC_DECODE_DONE, 0, (LPARAM)msg)) {
            msg->wicBmp->Release();
            delete msg;
        }
        // r はスコープアウト時に自動delete
        CloseThreadpoolWork(work);
    }, result.get(), nullptr);

    if (work) { result.release(); SubmitThreadpoolWork(work); } // 所有権をスレッドプールに移転
    // work が null の場合、result のデストラクタが自動的に delete する
}

// 単一画像をキャッシュ付きで取得するヘルパー
static bool LoadBitmapCached(const std::wstring& path, ComPtr<ID2D1Bitmap>& out)
{
    auto cached = CacheGet(path);
    if (cached) { out = cached; return true; }

    if (SUCCEEDED(DecodeImage(path, g_app.viewer.deviceContext.Get(), out.GetAddressOf())))
    {
        CachePut(path, out);
        return true;
    }
    return false;
}

void ViewerShowSpread(const std::wstring& path1, const std::wstring& path2)
{
    if (!g_app.viewer.deviceContext) return;

    ViewerStopAnimation();
    g_app.viewer.bitmap.Reset();
    g_app.viewer.bitmap2.Reset();
    g_app.viewer.isSpreadActive = false;

    bool ok1 = LoadBitmapCached(path1, g_app.viewer.bitmap);
    bool ok2 = LoadBitmapCached(path2, g_app.viewer.bitmap2);

    if (ok1 && ok2)
    {
        g_app.viewer.isSpreadActive = true;
        g_app.nav.currentPath = path1;
    }
    else if (ok1)
    {
        g_app.nav.currentPath = path1;
    }

    g_app.viewer.zoom = 1.0f;
    g_app.viewer.scrollX = 0.0f;
    g_app.viewer.scrollY = 0.0f;
    InvalidateRect(g_app.wnd.hwndViewer, nullptr, FALSE);
}

// 画像サイズキャッシュ（プリフェッチスレッドからも書き込まれるため mutex 保護）
static std::mutex g_sizeCacheMutex;
static std::unordered_map<std::wstring, std::pair<UINT, UINT>> g_sizeCache;

// WIC で画像サイズだけ高速取得（キャッシュ付き）
// cacheOnly=true: キャッシュミス時にWICデコーダを開かずfalseを返す（UIブロック回避）
static bool GetImageSize(const std::wstring& path, UINT& w, UINT& h, bool cacheOnly = false)
{
    // キャッシュヒット（mutex保護）
    {
        std::lock_guard<std::mutex> lock(g_sizeCacheMutex);
        auto it = g_sizeCache.find(path);
        if (it != g_sizeCache.end())
        {
            w = it->second.first;
            h = it->second.second;
            return true;
        }
    }

    // キャッシュオンリーモード: WICデコーダを開かない（高速フォールバック）
    if (cacheOnly) return false;

    if (!g_app.viewer.wicFactory) return false;

    ComPtr<IWICBitmapDecoder> decoder;
    HRESULT hr = g_app.viewer.wicFactory->CreateDecoderFromFilename(
        path.c_str(), nullptr, GENERIC_READ,
        WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf());
    if (FAILED(hr)) return false;

    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, frame.GetAddressOf());
    if (FAILED(hr)) return false;

    if (FAILED(frame->GetSize(&w, &h))) return false;

    // キャッシュに保存（mutex保護、1000超過時は半分削除）
    {
        std::lock_guard<std::mutex> lock(g_sizeCacheMutex);
        if (g_sizeCache.size() > 1000) {
            auto it = g_sizeCache.begin();
            for (int i = 0; i < 500 && it != g_sizeCache.end(); i++)
                it = g_sizeCache.erase(it);
        }
        g_sizeCache[path] = { w, h };
    }
    return true;
}

// プリフェッチスレッドから画像サイズキャッシュを先読みする
void CacheImageSize(const std::wstring& path, UINT w, UINT h)
{
    std::lock_guard<std::mutex> lock(g_sizeCacheMutex);
    if (g_sizeCache.size() > 1000) {
        auto it = g_sizeCache.begin();
        for (int i = 0; i < 500 && it != g_sizeCache.end(); i++)
            it = g_sizeCache.erase(it);
    }
    g_sizeCache[path] = { w, h };
}

// 縦向き判定（w/h <= 1.0 なら縦向き）
// cacheOnly=true: キャッシュミス時はtrue（縦向き扱い=見開き可能）を返す
static bool IsPortrait(const std::wstring& path, bool cacheOnly = false)
{
    UINT w = 0, h = 0;
    if (!GetImageSize(path, w, h, cacheOnly)) return true; // 取得失敗は縦向き扱い
    if (w == 0 || h == 0) return true;
    return (float)w / h <= 1.0f;
}

// 見開き判定結果キャッシュ
static std::unordered_map<int, bool> g_spreadCache;
void ClearSpreadCache() { g_spreadCache.clear(); }

bool ShouldShowSpread(int index)
{
    // キャッシュヒット
    auto it = g_spreadCache.find(index);
    if (it != g_spreadCache.end()) return it->second;

    if (g_app.viewer.viewMode == 1) return false; // 単独モード

    // 動画・音声は見開きの対象外
    {
        int n = (int)g_app.nav.viewableFiles.size();
        if (index >= 0 && index < n && IsMediaFile(g_app.nav.viewableFiles[index])) return false;
        if (index + 1 < n && IsMediaFile(g_app.nav.viewableFiles[index + 1])) return false;
    }

    if (g_app.viewer.viewMode == 2)
    {
        // 強制見開き: 次ファイルがあれば見開き
        return (index + 1) < (int)g_app.nav.viewableFiles.size();
    }

    // 自動モード
    int total = (int)g_app.nav.viewableFiles.size();
    if (index == 0) return false;         // 表紙は単独
    if (index + 1 >= total) return false; // 最後のページは単独

    const auto& currentPath = g_app.nav.viewableFiles[index];
    const auto& nextPath = g_app.nav.viewableFiles[index + 1];

    // メディアファイルは見開き不可
    if (IsMediaFile(currentPath) || IsMediaFile(nextPath)) return false;

    // 現在のページが横向き → 単独（キャッシュオンリーでUIブロック回避）
    if (!IsPortrait(currentPath, true)) { g_spreadCache[index] = false; return false; }

    // 次のページが横向き → 単独（キャッシュオンリーでUIブロック回避）
    if (!IsPortrait(nextPath, true)) { g_spreadCache[index] = false; return false; }

    // 両方縦向き → 見開き
    g_spreadCache[index] = true;
    return true;
}

int GetPagesPerView()
{
    return g_app.viewer.isSpreadActive ? 2 : 1;
}

int ViewerGetEffectiveZoomPercent()
{
    return (int)(g_app.viewer.zoom * 100.0f + 0.5f);
}

void ViewerSetScaleMode(ScaleMode mode)
{
    g_app.viewer.scaleMode = mode;
    g_app.viewer.zoom = 1.0f;
    g_app.viewer.scrollX = 0.0f;
    g_app.viewer.scrollY = 0.0f;
    InvalidateRect(g_app.wnd.hwndViewer, nullptr, FALSE);
    UpdateZoomLabel();
}

void ViewerSetViewMode(int mode)
{
    g_app.viewer.viewMode = mode;
    ClearSpreadCache(); // モード変更時にキャッシュクリア
    if (g_app.nav.currentFileIndex >= 0)
        GoToFile(g_app.nav.currentFileIndex);
}

void ViewerToggleBinding()
{
    g_app.viewer.isRTL = !g_app.viewer.isRTL;
    InvalidateRect(g_app.wnd.hwndViewer, nullptr, FALSE);
}

void ViewerZoomIn()
{
    // 25%刻み: 現在値を25%単位で切り上げ
    int cur = ViewerGetEffectiveZoomPercent();
    int next = ((cur / 25) + 1) * 25;
    if (next <= cur) next += 25;
    g_app.viewer.zoom = std::min(next / 100.0f, kZoomMax);
    InvalidateRect(g_app.wnd.hwndViewer, nullptr, FALSE);
    UpdateZoomLabel();
}

void ViewerZoomOut()
{
    // 25%刻み: 現在値を25%単位で切り下げ
    int cur = ViewerGetEffectiveZoomPercent();
    int next = ((cur - 1) / 25) * 25;
    if (next < 25) next = 25;
    g_app.viewer.zoom = std::max(next / 100.0f, kZoomMin);
    InvalidateRect(g_app.wnd.hwndViewer, nullptr, FALSE);
    UpdateZoomLabel();
}

void ViewerZoomSmooth(bool zoomIn)
{
    // なめらかズーム（ホイール用）: ×1.1 / ×0.9
    g_app.viewer.zoom *= zoomIn ? 1.1f : (1.0f / 1.1f);
    g_app.viewer.zoom = std::clamp(g_app.viewer.zoom, kZoomMin, kZoomMax);
    InvalidateRect(g_app.wnd.hwndViewer, nullptr, FALSE);
    UpdateZoomLabel();
}

void ViewerRotateCW()
{
    g_app.viewer.rotation = (g_app.viewer.rotation + 90) % 360;
    InvalidateRect(g_app.wnd.hwndViewer, nullptr, FALSE);
}

void ViewerRotateCCW()
{
    g_app.viewer.rotation = (g_app.viewer.rotation + 270) % 360;
    InvalidateRect(g_app.wnd.hwndViewer, nullptr, FALSE);
}

void ViewerSetBitmap(ComPtr<ID2D1Bitmap> bmp, const std::wstring& path)
{
    ViewerStopAnimation();
    g_app.viewer.bitmap = bmp;
    g_app.viewer.bitmap2.Reset();
    g_app.viewer.isSpreadActive = false;
    g_app.nav.currentPath = path;
    g_app.viewer.zoom = 1.0f;
    g_app.viewer.scrollX = 0.0f;
    g_app.viewer.scrollY = 0.0f;
    InvalidateRect(g_app.wnd.hwndViewer, nullptr, FALSE);
}

void ViewerResetView()
{
    g_app.viewer.zoom = 1.0f;
    g_app.viewer.scrollX = 0.0f;
    g_app.viewer.scrollY = 0.0f;
}
