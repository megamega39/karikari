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
#include "settings.h"
#include <mutex>
#include <atomic>
#include <webp/decode.h>
#include <webp/demux.h>

// 型安全なWebPAnimDecoderアクセサ（AnimState用）
static WebPAnimDecoder* GetWebPDecoder(AnimState& a) { return static_cast<WebPAnimDecoder*>(a.webpDecoder); }
static void DeleteWebPDecoder(AnimState& a) {
    if (a.webpDecoder) { WebPAnimDecoderDelete(static_cast<WebPAnimDecoder*>(a.webpDecoder)); a.webpDecoder = nullptr; }
}

// AnimState::Reset の実装
void AnimStateReset(AnimState& a) {
    if (a.webpDecoder) DeleteWebPDecoder(a);
    a.webpFileData.clear();
    a.webpFileData.shrink_to_fit();
    a.webpCanvasW = a.webpCanvasH = 0;
    a.webpPrevTimestamp = 0;
    a.gifDecoder.Reset();
    a.gifCanvas.Reset();
    a.gifPrevCanvas.Reset();
    a.gifCanvasW = a.gifCanvasH = 0;
    a.gifFrameCount = a.gifCurrentFrame = 0;
    a.type = AnimState::None;
    a.nextDelay = 100;
    a.elapsed = 0;
}

static bool LoadBitmapCached(const std::wstring& path, ComPtr<ID2D1Bitmap>& out);
static bool GetImageSize(const std::wstring& path, UINT& w, UINT& h, bool cacheOnly, UINT* outFrameCount);
static std::atomic<int> g_asyncDecodeGeneration{0};
static constexpr UINT_PTR kAnimTimerId = 42;
static bool GifDecodeNextFrame(AnimState& anim, ComPtr<ID2D1Bitmap>& outBmp, int& outDelay);
static bool WebPDecodeNextFrame(AnimState& anim, ComPtr<ID2D1Bitmap>& outBmp, int& outDelay);

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
static void PaintSingle(ID2D1DeviceContext* rt)
{
    if (!g_app.viewer.bitmap) return;

    auto rtSize = rt->GetSize();
    // DPI非依存のピクセルサイズを使用
    auto px = g_app.viewer.bitmap->GetPixelSize();
    D2D1_SIZE_F bmpSize = { (float)px.width, (float)px.height };

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
        ? D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR
        : D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC;

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

    D2D1_RECT_F destRect = D2D1::RectF(x, y, x + origDrawW, y + origDrawH);
    rt->DrawBitmap(g_app.viewer.bitmap.Get(), &destRect, 1.0f, interpMode, nullptr);

    if (g_app.viewer.rotation != 0)
        rt->SetTransform(D2D1::Matrix3x2F::Identity());
}

// 見開き描画: 2枚を左右に隙間なく中央配置
static void PaintSpread(ID2D1DeviceContext* rt)
{
    if (!g_app.viewer.bitmap || !g_app.viewer.bitmap2) return;

    auto rtSize = rt->GetSize();

    // 左右どちらのページか（RTL対応）
    ID2D1Bitmap* leftBmp = g_app.viewer.isRTL ? g_app.viewer.bitmap2.Get() : g_app.viewer.bitmap.Get();
    ID2D1Bitmap* rightBmp = g_app.viewer.isRTL ? g_app.viewer.bitmap.Get() : g_app.viewer.bitmap2.Get();
    // DPI非依存のピクセルサイズを使用（アニメGIF/WebPと静止画でDPIが異なるため）
    auto lp = leftBmp->GetPixelSize();
    auto rp = rightBmp->GetPixelSize();
    D2D1_SIZE_F leftSize = { (float)lp.width, (float)lp.height };
    D2D1_SIZE_F rightSize = { (float)rp.width, (float)rp.height };

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
    D2D1_RECT_F leftRect = D2D1::RectF(startX, ly, startX + drawLW, ly + drawLH);
    rt->DrawBitmap(leftBmp, &leftRect, 1.0f,
        D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC, nullptr);

    // 右ページ
    float rx = startX + drawLW;
    float ry = baseY + (maxDrawH - drawRH) / 2.0f;
    D2D1_RECT_F rightRect = D2D1::RectF(rx, ry, rx + drawRW, ry + drawRH);
    rt->DrawBitmap(rightBmp, &rightRect, 1.0f,
        D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC, nullptr);
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
    auto px = g_app.viewer.bitmap->GetPixelSize();
    D2D1_SIZE_F bmpSize = { (float)px.width, (float)px.height };
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
        if (wParam != kAnimTimerId || !g_app.viewer.isAnimating) return 0;

        // 最小化時はアニメーション更新を停止（CPU/メモリ節約）
        if (IsIconic(GetAncestor(hwnd, GA_ROOT)))
            return 0;

        KillTimer(hwnd, kAnimTimerId);

        DWORD now = GetTickCount();
        DWORD dt = now - g_app.viewer.lastAnimTick;
        if (dt > 5000) dt = 16; // 異常値ガード
        g_app.viewer.lastAnimTick = now;
        bool needRepaint = false;

        // anim1 (bitmap)
        auto& a1 = g_app.viewer.anim1;
        if (a1.IsActive())
        {
            a1.elapsed += (int)dt;
            if (a1.elapsed >= a1.nextDelay)
            {
                a1.elapsed = 0;
                int delay = 100;
                bool ok = false;
                if (a1.type == AnimState::Gif)
                    ok = GifDecodeNextFrame(a1, g_app.viewer.bitmap, delay);
                else if (a1.type == AnimState::WebP)
                    ok = WebPDecodeNextFrame(a1, g_app.viewer.bitmap, delay);
                if (ok) a1.nextDelay = delay;
                needRepaint = true;
            }
        }

        // anim2 (bitmap2)
        auto& a2 = g_app.viewer.anim2;
        if (a2.IsActive())
        {
            a2.elapsed += (int)dt;
            if (a2.elapsed >= a2.nextDelay)
            {
                a2.elapsed = 0;
                int delay = 100;
                bool ok = false;
                if (a2.type == AnimState::Gif)
                    ok = GifDecodeNextFrame(a2, g_app.viewer.bitmap2, delay);
                else if (a2.type == AnimState::WebP)
                    ok = WebPDecodeNextFrame(a2, g_app.viewer.bitmap2, delay);
                if (ok) a2.nextDelay = delay;
                needRepaint = true;
            }
        }

        // レガシーアニメ（animFrames）
        if (!a1.IsActive() && !a2.IsActive() && !g_app.viewer.animFrames.empty())
        {
            g_app.viewer.animCurrentFrame = (g_app.viewer.animCurrentFrame + 1) % (int)g_app.viewer.animFrames.size();
            g_app.viewer.bitmap = g_app.viewer.animFrames[g_app.viewer.animCurrentFrame];
            needRepaint = true;
        }

        if (needRepaint)
            InvalidateRect(hwnd, nullptr, FALSE);

        // 次回タイマー間隔
        int r1 = a1.IsActive() ? (std::max)(1, a1.nextDelay - a1.elapsed) : INT_MAX;
        int r2 = a2.IsActive() ? (std::max)(1, a2.nextDelay - a2.elapsed) : INT_MAX;
        int next = (std::min)(r1, r2);
        if (next == INT_MAX)
        {
            // レガシーフォールバック
            if (!g_app.viewer.animFrames.empty())
                next = g_app.viewer.animDelays[g_app.viewer.animCurrentFrame];
            else
                next = 100;
        }
        g_app.viewer.animTimer = SetTimer(hwnd, kAnimTimerId, next, nullptr);
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

// === GIFメタデータヘルパー ===
static UINT GetMetadataUI2(IWICMetadataQueryReader* meta, const wchar_t* name, UINT def)
{
    PROPVARIANT val; PropVariantInit(&val);
    if (SUCCEEDED(meta->GetMetadataByName(name, &val)) && val.vt == VT_UI2)
    { UINT r = val.uiVal; PropVariantClear(&val); return r; }
    PropVariantClear(&val); return def;
}
static UINT GetMetadataUI1(IWICMetadataQueryReader* meta, const wchar_t* name, UINT def)
{
    PROPVARIANT val; PropVariantInit(&val);
    if (SUCCEEDED(meta->GetMetadataByName(name, &val)) && val.vt == VT_UI1)
    { UINT r = val.bVal; PropVariantClear(&val); return r; }
    PropVariantClear(&val); return def;
}

// === GIFストリーミング: 1フレーム合成してD2Dビットマップを返す ===
static bool GifDecodeNextFrame(AnimState& anim, ComPtr<ID2D1Bitmap>& outBmp, int& outDelay)
{
    auto& v = g_app.viewer;
    if (!anim.gifDecoder || !anim.gifCanvas || !v.wicFactory || !v.deviceContext) return false;

    if (anim.gifCurrentFrame >= anim.gifFrameCount)
    {
        // ループ: キャンバスクリアして先頭へ
        anim.gifCurrentFrame = 0;
        ComPtr<IWICBitmapLock> lock;
        WICRect lockRc = { 0, 0, (int)anim.gifCanvasW, (int)anim.gifCanvasH };
        if (FAILED(anim.gifCanvas->Lock(&lockRc, WICBitmapLockWrite, lock.GetAddressOf()))) return false;
        UINT bufSize = 0; BYTE* pBuf = nullptr;
        lock->GetDataPointer(&bufSize, &pBuf);
        if (pBuf) memset(pBuf, 0, bufSize);
    }

    ComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(anim.gifDecoder->GetFrame(anim.gifCurrentFrame, frame.GetAddressOf()))) return false;

    int delay = 100;
    UINT frameLeft = 0, frameTop = 0, disposal = 0;
    ComPtr<IWICMetadataQueryReader> meta;
    if (SUCCEEDED(frame->GetMetadataQueryReader(meta.GetAddressOf())))
    {
        delay = (int)GetMetadataUI2(meta.Get(), L"/grctlext/Delay", 10) * 10;
        disposal = GetMetadataUI1(meta.Get(), L"/grctlext/Disposal", 0);
        frameLeft = GetMetadataUI2(meta.Get(), L"/imgdesc/Left", 0);
        frameTop = GetMetadataUI2(meta.Get(), L"/imgdesc/Top", 0);
    }
    if (delay < 20) delay = 20;
    outDelay = delay;

    // disposal=3: 合成前のキャンバスを保存
    if (disposal == 3 && anim.gifPrevCanvas)
    {
        ComPtr<IWICBitmapLock> srcLock, dstLock;
        WICRect lockRc = { 0, 0, (int)anim.gifCanvasW, (int)anim.gifCanvasH };
        if (SUCCEEDED(anim.gifCanvas->Lock(&lockRc, WICBitmapLockRead, srcLock.GetAddressOf())) &&
            SUCCEEDED(anim.gifPrevCanvas->Lock(&lockRc, WICBitmapLockWrite, dstLock.GetAddressOf())))
        {
            UINT srcSize = 0, dstSize = 0; BYTE* pSrc = nullptr; BYTE* pDst = nullptr;
            srcLock->GetDataPointer(&srcSize, &pSrc);
            dstLock->GetDataPointer(&dstSize, &pDst);
            if (pSrc && pDst) memcpy(pDst, pSrc, (std::min)(srcSize, dstSize));
        }
    }

    // フレームをPBGRA変換
    ComPtr<IWICFormatConverter> converter;
    if (FAILED(v.wicFactory->CreateFormatConverter(converter.GetAddressOf()))) return false;
    if (FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA,
        WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom))) return false;

    UINT frameW = 0, frameH = 0;
    converter->GetSize(&frameW, &frameH);
    UINT frameStride = frameW * 4;
    std::vector<BYTE> framePixels(frameStride * frameH);
    if (FAILED(converter->CopyPixels(nullptr, frameStride, (UINT)framePixels.size(), framePixels.data()))) return false;

    // キャンバスにフレームを合成
    {
        ComPtr<IWICBitmapLock> lock;
        WICRect lockRc = { 0, 0, (int)anim.gifCanvasW, (int)anim.gifCanvasH };
        if (FAILED(anim.gifCanvas->Lock(&lockRc, WICBitmapLockWrite, lock.GetAddressOf()))) return false;
        UINT canvasStride = 0; lock->GetStride(&canvasStride);
        UINT bufSize = 0; BYTE* pCanvas = nullptr;
        lock->GetDataPointer(&bufSize, &pCanvas);
        if (!pCanvas) return false;

        for (UINT y = 0; y < frameH && (frameTop + y) < anim.gifCanvasH; y++)
        {
            BYTE* dst = pCanvas + (frameTop + y) * canvasStride + frameLeft * 4;
            BYTE* src = framePixels.data() + y * frameStride;
            UINT copyW = (std::min)(frameW, anim.gifCanvasW - frameLeft);
            for (UINT x = 0; x < copyW; x++)
            {
                BYTE a = src[x * 4 + 3];
                if (a > 0) memcpy(dst + x * 4, src + x * 4, 4);
            }
        }
    }

    // キャンバスからD2Dビットマップ（CopyFromMemoryで再利用）
    {
        ComPtr<IWICBitmapLock> lock;
        WICRect lockRc = { 0, 0, (int)anim.gifCanvasW, (int)anim.gifCanvasH };
        if (FAILED(anim.gifCanvas->Lock(&lockRc, WICBitmapLockRead, lock.GetAddressOf()))) return false;
        UINT stride = 0; UINT bufSz = 0; BYTE* pBuf = nullptr;
        lock->GetStride(&stride);
        lock->GetDataPointer(&bufSz, &pBuf);

        bool reused = false;
        if (outBmp)
        {
            auto sz = outBmp->GetPixelSize();
            if (sz.width == anim.gifCanvasW && sz.height == anim.gifCanvasH)
                reused = SUCCEEDED(outBmp->CopyFromMemory(nullptr, pBuf, stride));
        }
        if (!reused)
        {
            outBmp.Reset();
            D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
            if (FAILED(v.deviceContext->CreateBitmap(
                D2D1::SizeU(anim.gifCanvasW, anim.gifCanvasH), pBuf, stride, props, outBmp.GetAddressOf()))) return false;
        }
    }

    // disposal 処理
    if (disposal == 2)
    {
        ComPtr<IWICBitmapLock> lock;
        WICRect lockRc = { (int)frameLeft, (int)frameTop, (int)frameW, (int)frameH };
        if (frameLeft + frameW <= anim.gifCanvasW && frameTop + frameH <= anim.gifCanvasH)
            if (SUCCEEDED(anim.gifCanvas->Lock(&lockRc, WICBitmapLockWrite, lock.GetAddressOf())))
            {
                UINT stride = 0; UINT bufSz = 0; BYTE* pBuf = nullptr;
                lock->GetStride(&stride); lock->GetDataPointer(&bufSz, &pBuf);
                if (pBuf) for (UINT y = 0; y < frameH; y++) memset(pBuf + y * stride, 0, frameW * 4);
            }
    }
    else if (disposal == 3 && anim.gifPrevCanvas)
    {
        ComPtr<IWICBitmapLock> srcLock, dstLock;
        WICRect lockRc = { 0, 0, (int)anim.gifCanvasW, (int)anim.gifCanvasH };
        if (SUCCEEDED(anim.gifPrevCanvas->Lock(&lockRc, WICBitmapLockRead, srcLock.GetAddressOf())) &&
            SUCCEEDED(anim.gifCanvas->Lock(&lockRc, WICBitmapLockWrite, dstLock.GetAddressOf())))
        {
            UINT srcSize = 0, dstSize = 0; BYTE* pSrc = nullptr; BYTE* pDst = nullptr;
            srcLock->GetDataPointer(&srcSize, &pSrc);
            dstLock->GetDataPointer(&dstSize, &pDst);
            if (pSrc && pDst) memcpy(pDst, pSrc, (std::min)(srcSize, dstSize));
        }
    }

    anim.gifCurrentFrame++;
    return true;
}

// === WebPストリーミング: 1フレームデコードしてD2Dビットマップを返す ===
static bool WebPDecodeNextFrame(AnimState& anim, ComPtr<ID2D1Bitmap>& outBmp, int& outDelay)
{
    auto* dec = GetWebPDecoder(anim);
    if (!dec) return false;

    if (!WebPAnimDecoderHasMoreFrames(dec))
    {
        // ループ: デコーダを再作成して先頭に戻す
        WebPAnimDecoderDelete(dec);
        WebPData webpData;
        webpData.bytes = anim.webpFileData.data();
        webpData.size = anim.webpFileData.size();
        WebPAnimDecoderOptions decOpts;
        WebPAnimDecoderOptionsInit(&decOpts);
        decOpts.color_mode = MODE_BGRA;
        dec = WebPAnimDecoderNew(&webpData, &decOpts);
        anim.webpDecoder = dec;
        anim.webpPrevTimestamp = 0;
        if (!dec) return false;
    }

    uint8_t* buf = nullptr;
    int timestamp = 0;
    if (!WebPAnimDecoderGetNext(dec, &buf, &timestamp)) return false;

    int delay = timestamp - anim.webpPrevTimestamp;
    if (delay < 20) delay = 20;
    outDelay = delay;
    anim.webpPrevTimestamp = timestamp;

    UINT stride = anim.webpCanvasW * 4;
    bool reused = false;
    if (outBmp)
    {
        auto sz = outBmp->GetPixelSize();
        if (sz.width == anim.webpCanvasW && sz.height == anim.webpCanvasH)
            reused = SUCCEEDED(outBmp->CopyFromMemory(nullptr, buf, stride));
    }
    if (!reused)
    {
        D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
        ComPtr<ID2D1Bitmap> bmp;
        if (SUCCEEDED(g_app.viewer.deviceContext->CreateBitmap(
            D2D1::SizeU(anim.webpCanvasW, anim.webpCanvasH),
            buf, stride, props, bmp.GetAddressOf())))
            outBmp = bmp;
        else
            return false;
    }
    return true;
}

// === ファイルをバイト配列として読み込む ===
static bool ReadFileToBytes(const std::wstring& path, std::vector<BYTE>& out)
{
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER sz;
    GetFileSizeEx(hFile, &sz);
    out.resize((size_t)sz.QuadPart);
    DWORD read;
    ReadFile(hFile, out.data(), (DWORD)out.size(), &read, nullptr);
    CloseHandle(hFile);
    return read == (DWORD)out.size();
}

// === WebPアニメーション読み込み（libwebp） ===
static bool TryLoadWebPAnimation(const std::wstring& path, AnimState& anim, ComPtr<ID2D1Bitmap>& outBmp)
{
    auto& v = g_app.viewer;

    // ファイルorメモリから読み込み
    std::wstring arcPath, entryPath;
    if (SplitArchivePath(path, arcPath, entryPath))
    {
        auto cached = StreamCacheGet(path);
        if (cached)
            anim.webpFileData.assign(cached->begin(), cached->end());
        else
        {
            std::vector<BYTE> buf;
            if (!ExtractToMemory(arcPath, entryPath, buf)) return false;
            anim.webpFileData = std::move(buf);
        }
    }
    else
    {
        if (!ReadFileToBytes(path, anim.webpFileData)) return false;
    }

    WebPData webpData;
    webpData.bytes = anim.webpFileData.data();
    webpData.size = anim.webpFileData.size();

    // アニメーションかチェック
    WebPDemuxer* demux = WebPDemux(&webpData);
    if (!demux) return false;
    UINT frameCount = WebPDemuxGetI(demux, WEBP_FF_FRAME_COUNT);
    anim.webpCanvasW = WebPDemuxGetI(demux, WEBP_FF_CANVAS_WIDTH);
    anim.webpCanvasH = WebPDemuxGetI(demux, WEBP_FF_CANVAS_HEIGHT);
    WebPDemuxDelete(demux);

    if (frameCount <= 1) { anim.webpFileData.clear(); return false; }

    // WebPAnimDecoder作成
    WebPAnimDecoderOptions decOpts;
    WebPAnimDecoderOptionsInit(&decOpts);
    decOpts.color_mode = MODE_BGRA;
    auto* dec = WebPAnimDecoderNew(&webpData, &decOpts);
    if (!dec) { anim.webpFileData.clear(); return false; }
    anim.webpDecoder = dec;
    anim.webpPrevTimestamp = 0;

    // 最初のフレームをデコード
    uint8_t* buf = nullptr;
    int timestamp = 0;
    if (!WebPAnimDecoderGetNext(dec, &buf, &timestamp))
    {
        WebPAnimDecoderDelete(dec);
        anim.webpDecoder = nullptr;
        anim.webpFileData.clear();
        return false;
    }

    UINT stride = anim.webpCanvasW * 4;
    D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
    ComPtr<ID2D1Bitmap> bmp;
    if (FAILED(v.deviceContext->CreateBitmap(
        D2D1::SizeU(anim.webpCanvasW, anim.webpCanvasH), buf, stride, props, bmp.GetAddressOf())))
    {
        WebPAnimDecoderDelete(dec);
        anim.webpDecoder = nullptr;
        anim.webpFileData.clear();
        return false;
    }
    outBmp = bmp;
    anim.webpPrevTimestamp = timestamp;
    anim.nextDelay = (timestamp > 0) ? timestamp : 100;
    anim.elapsed = 0;
    anim.type = AnimState::WebP;
    return true;
}

// === GIFアニメーション読み込み（WICストリーミング） ===
static bool TryLoadGifAnimation(const std::wstring& path, AnimState& anim, ComPtr<ID2D1Bitmap>& outBmp)
{
    auto& v = g_app.viewer;
    if (!v.wicFactory || !v.deviceContext) return false;

    // 書庫内GIF対応: メモリからWICデコーダ作成
    std::wstring arcPath, entryPath;
    if (SplitArchivePath(path, arcPath, entryPath))
    {
        auto cached = StreamCacheGet(path);
        std::vector<BYTE> buf;
        const BYTE* data = nullptr;
        size_t size = 0;
        if (cached) { data = cached->data(); size = cached->size(); }
        else
        {
            if (!ExtractToMemory(arcPath, entryPath, buf)) return false;
            data = buf.data(); size = buf.size();
        }
        ComPtr<IWICStream> stream;
        if (FAILED(v.wicFactory->CreateStream(stream.GetAddressOf()))) return false;
        if (FAILED(stream->InitializeFromMemory(const_cast<BYTE*>(data), (DWORD)size))) return false;
        if (FAILED(v.wicFactory->CreateDecoderFromStream(
            stream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, anim.gifDecoder.GetAddressOf()))) return false;
    }
    else
    {
        if (FAILED(v.wicFactory->CreateDecoderFromFilename(
            path.c_str(), nullptr, GENERIC_READ,
            WICDecodeMetadataCacheOnLoad, anim.gifDecoder.GetAddressOf()))) return false;
    }

    anim.gifDecoder->GetFrameCount(&anim.gifFrameCount);
    if (anim.gifFrameCount <= 1) { anim.gifDecoder.Reset(); return false; }

    // キャンバスサイズ取得（GIFグローバルメタデータ）
    ComPtr<IWICMetadataQueryReader> globalMeta;
    anim.gifCanvasW = 0; anim.gifCanvasH = 0;
    if (SUCCEEDED(anim.gifDecoder->GetMetadataQueryReader(globalMeta.GetAddressOf())))
    {
        anim.gifCanvasW = GetMetadataUI2(globalMeta.Get(), L"/logscrdesc/Width", 0);
        anim.gifCanvasH = GetMetadataUI2(globalMeta.Get(), L"/logscrdesc/Height", 0);
    }
    if (anim.gifCanvasW == 0 || anim.gifCanvasH == 0)
    {
        // フォールバック: 最初のフレームサイズ
        ComPtr<IWICBitmapFrameDecode> f0;
        if (SUCCEEDED(anim.gifDecoder->GetFrame(0, f0.GetAddressOf())))
            f0->GetSize(&anim.gifCanvasW, &anim.gifCanvasH);
    }
    if (anim.gifCanvasW == 0 || anim.gifCanvasH == 0) { anim.gifDecoder.Reset(); return false; }

    // キャンバス作成
    v.wicFactory->CreateBitmap(anim.gifCanvasW, anim.gifCanvasH, GUID_WICPixelFormat32bppPBGRA,
        WICBitmapCacheOnLoad, anim.gifCanvas.GetAddressOf());
    v.wicFactory->CreateBitmap(anim.gifCanvasW, anim.gifCanvasH, GUID_WICPixelFormat32bppPBGRA,
        WICBitmapCacheOnLoad, anim.gifPrevCanvas.GetAddressOf());
    if (!anim.gifCanvas) { anim.gifDecoder.Reset(); return false; }

    anim.gifCurrentFrame = 0;
    anim.type = AnimState::Gif;

    // 最初のフレーム
    int delay = 100;
    if (!GifDecodeNextFrame(anim, outBmp, delay)) { anim.gifDecoder.Reset(); anim.gifCanvas.Reset(); return false; }
    anim.nextDelay = delay;
    anim.elapsed = 0;
    return true;
}

// === アニメーション試行（WebP→GIF の順） ===
bool TryLoadAnimation(const std::wstring& path, AnimState& anim, ComPtr<ID2D1Bitmap>& outBmp)
{
    const wchar_t* ext = PathFindExtensionW(path.c_str());
    if (!ext) return false;
    if (_wcsicmp(ext, L".webp") == 0) return TryLoadWebPAnimation(path, anim, outBmp);
    if (_wcsicmp(ext, L".gif") == 0) return TryLoadGifAnimation(path, anim, outBmp);
    return false;
}

void ViewerStopAnimation()
{
    if (g_app.viewer.animTimer)
    {
        KillTimer(g_app.wnd.hwndViewer, kAnimTimerId);
        g_app.viewer.animTimer = 0;
    }
    g_app.viewer.isAnimating = false;
    AnimStateReset(g_app.viewer.anim1);
    AnimStateReset(g_app.viewer.anim2);

    // レガシー cleanup
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
    if (TryLoadAnimation(path, g_app.viewer.anim1, g_app.viewer.bitmap))
    {
        g_app.viewer.isAnimating = true;
        g_app.viewer.lastAnimTick = GetTickCount();
        g_app.nav.currentPath = path;
        g_app.viewer.zoom = 1.0f;
        g_app.viewer.scrollX = 0.0f;
        g_app.viewer.scrollY = 0.0f;
        g_app.viewer.animTimer = SetTimer(g_app.wnd.hwndViewer, kAnimTimerId, 16, nullptr);
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

// g_asyncDecodeGeneration はファイル先頭で定義済み

int GetAsyncDecodeGeneration() { return g_asyncDecodeGeneration.load(); }
void InvalidateAsyncDecode() { ++g_asyncDecodeGeneration; }

void ViewerStartAnimationIfNeeded(const std::wstring& path1, const std::wstring& path2)
{
    auto isAnimExt = [](const std::wstring& p) {
        const wchar_t* ext = PathFindExtensionW(p.c_str());
        return ext && (_wcsicmp(ext, L".gif") == 0 || _wcsicmp(ext, L".webp") == 0);
    };
    bool a1 = false, a2 = false;
    if (isAnimExt(path1))
        a1 = TryLoadAnimation(path1, g_app.viewer.anim1, g_app.viewer.bitmap);
    if (!path2.empty() && isAnimExt(path2))
        a2 = TryLoadAnimation(path2, g_app.viewer.anim2, g_app.viewer.bitmap2);
    if (a1 || a2) {
        ++g_asyncDecodeGeneration; // アニメ開始後の古い非同期結果を無効化
        g_app.viewer.isAnimating = true;
        g_app.viewer.lastAnimTick = GetTickCount();
        g_app.viewer.animTimer = SetTimer(g_app.wnd.hwndViewer, kAnimTimerId, 16, nullptr);
    }
}

bool IsConfirmedAnimation(const std::wstring& path)
{
    const wchar_t* ext = PathFindExtensionW(path.c_str());
    if (!ext || (_wcsicmp(ext, L".gif") != 0 && _wcsicmp(ext, L".webp") != 0))
        return false;
    UINT w = 0, h = 0, fc = 0;
    if (GetImageSize(path, w, h, true, &fc) && fc > 1) return true;
    return false;
}

void ViewerLoadImageAsync(const std::wstring& path)
{
    // アニメーションGIF/WebPはストリーミングデコード（書庫内対応）
    {
        const wchar_t* ext = PathFindExtensionW(path.c_str());
        if (ext && (_wcsicmp(ext, L".gif") == 0 || _wcsicmp(ext, L".webp") == 0))
        {
            // サイズキャッシュでフレーム数を確認（cacheOnly=trueでI/Oなし）
            UINT tw = 0, th = 0, tfc = 0;
            bool sizeKnown = GetImageSize(path, tw, th, true, &tfc);

            // 確実にアニメーション（フレーム数>1）の場合のみ同期パスで試行
            // サイズ不明の場合は非同期デコードに任せる（メインスレッドで書庫展開しない）
            if (sizeKnown && tfc > 1)
            {
                ViewerStopAnimation();

                ComPtr<ID2D1Bitmap> animBmp;
                AnimState animState;
                if (TryLoadAnimation(path, animState, animBmp))
                {
                    ++g_asyncDecodeGeneration;
                    g_app.viewer.anim1 = std::move(animState);
                    g_app.viewer.bitmap = animBmp;
                    g_app.viewer.bitmap2.Reset();
                    g_app.viewer.isSpreadActive = false;
                    g_app.viewer.isAnimating = true;
                    g_app.viewer.lastAnimTick = GetTickCount();
                    g_app.viewer.rotation = 0;
                    g_app.nav.currentPath = path;
                    g_app.viewer.zoom = 1.0f;
                    g_app.viewer.scrollX = 0.0f;
                    g_app.viewer.scrollY = 0.0f;
                    g_app.viewer.animTimer = SetTimer(g_app.wnd.hwndViewer, kAnimTimerId, 16, nullptr);
                    InvalidateRect(g_app.wnd.hwndViewer, nullptr, FALSE);
                    return;
                }
            }
            // 静止画 or サイズ不明: 通常の非同期デコードへフォールバック
        }
    }

    // キャッシュヒット時は即表示
    auto cached = CacheGet(path);
    if (cached)
    {
        ++g_asyncDecodeGeneration; // 保留中の非同期デコード結果を無効化
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
        // キャッシュヒットでもアニメGIF/WebPならアニメ開始
        ViewerStartAnimationIfNeeded(path);
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

void ViewerLoadSpreadAsync(const std::wstring& path1, const std::wstring& path2)
{
    // 確実にアニメーション（サイズキャッシュでフレーム数>1確認済み）の場合のみ同期パスへ
    if (IsConfirmedAnimation(path1) || IsConfirmedAnimation(path2))
    {
        ViewerShowSpread(path1, path2);
        return;
    }

    // キャッシュヒット時は即座に見開き表示
    auto cached1 = CacheGet(path1);
    auto cached2 = CacheGet(path2);
    if (cached1 && cached2) {
        InvalidateAsyncDecode();
        ViewerStopAnimation();
        g_app.viewer.bitmap = cached1;
        g_app.viewer.bitmap2 = cached2;
        g_app.viewer.isSpreadActive = true;
        g_app.viewer.rotation = 0;
        g_app.nav.currentPath = path1;
        ViewerResetView();
        InvalidateRect(g_app.wnd.hwndViewer, nullptr, FALSE);
        // キャッシュヒットでもアニメ開始
        ViewerStartAnimationIfNeeded(path1, path2);
        return;
    }

    // 非同期デコード
    int generation = ++g_asyncDecodeGeneration;

    struct SpreadDecodeCtx {
        int generation;
        std::wstring path1, path2;
    };
    auto* ctx = new SpreadDecodeCtx{ generation, path1, path2 };

    PTP_WORK work = CreateThreadpoolWork([](PTP_CALLBACK_INSTANCE, PVOID param, PTP_WORK work) {
        auto* c = (SpreadDecodeCtx*)param;
        std::unique_ptr<SpreadDecodeCtx> ctx(c);

        if (ctx->generation != g_asyncDecodeGeneration.load()) { CloseThreadpoolWork(work); return; }

        IWICImagingFactory* factory = GetAsyncWicFactory();
        if (!factory) { CloseThreadpoolWork(work); return; }

        // 1枚目デコード
        ComPtr<IWICBitmap> wic1;
        {
            std::wstring arcPath, entryPath;
            if (SplitArchivePath(ctx->path1, arcPath, entryPath)) {
                auto cached = StreamCacheGet(ctx->path1);
                if (!cached) {
                    std::vector<BYTE> buf;
                    if (ExtractToMemory(arcPath, entryPath, buf)) {
                        StreamCachePut(ctx->path1, std::move(buf));
                        cached = StreamCacheGet(ctx->path1);
                    }
                }
                if (cached)
                    DecodeMemoryToWicBitmap(cached->data(), cached->size(), factory, wic1.GetAddressOf());
            } else {
                DecodeToWicBitmap(ctx->path1, factory, wic1.GetAddressOf());
            }
        }

        if (ctx->generation != g_asyncDecodeGeneration.load()) { CloseThreadpoolWork(work); return; }

        // 2枚目デコード
        ComPtr<IWICBitmap> wic2;
        {
            std::wstring arcPath, entryPath;
            if (SplitArchivePath(ctx->path2, arcPath, entryPath)) {
                auto cached = StreamCacheGet(ctx->path2);
                if (!cached) {
                    std::vector<BYTE> buf;
                    if (ExtractToMemory(arcPath, entryPath, buf)) {
                        StreamCachePut(ctx->path2, std::move(buf));
                        cached = StreamCacheGet(ctx->path2);
                    }
                }
                if (cached)
                    DecodeMemoryToWicBitmap(cached->data(), cached->size(), factory, wic2.GetAddressOf());
            } else {
                DecodeToWicBitmap(ctx->path2, factory, wic2.GetAddressOf());
            }
        }

        if (ctx->generation != g_asyncDecodeGeneration.load()) { CloseThreadpoolWork(work); return; }

        // 画像サイズキャッシュ
        UINT w, h;
        if (wic1 && SUCCEEDED(wic1->GetSize(&w, &h)) && w > 0 && h > 0)
            CacheImageSize(ctx->path1, w, h);
        if (wic2 && SUCCEEDED(wic2->GetSize(&w, &h)) && w > 0 && h > 0)
            CacheImageSize(ctx->path2, w, h);

        auto* msg = new AsyncSpreadDoneMsg{
            wic1 ? wic1.Detach() : nullptr,
            wic2 ? wic2.Detach() : nullptr,
            ctx->path1, ctx->path2, ctx->generation
        };
        if (!PostMessageW(g_app.wnd.hwndMain, WM_ASYNC_SPREAD_DONE, 0, (LPARAM)msg)) {
            if (msg->wicBmp1) msg->wicBmp1->Release();
            if (msg->wicBmp2) msg->wicBmp2->Release();
            delete msg;
        }
        CloseThreadpoolWork(work);
    }, ctx, nullptr);

    if (work) SubmitThreadpoolWork(work);
    else delete ctx;
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

    ++g_asyncDecodeGeneration; // 保留中の非同期デコード結果を無効化
    ViewerStopAnimation();
    g_app.viewer.bitmap.Reset();
    g_app.viewer.bitmap2.Reset();
    g_app.viewer.isSpreadActive = false;

    bool a1 = TryLoadAnimation(path1, g_app.viewer.anim1, g_app.viewer.bitmap);
    if (!a1) LoadBitmapCached(path1, g_app.viewer.bitmap);

    bool a2 = TryLoadAnimation(path2, g_app.viewer.anim2, g_app.viewer.bitmap2);
    if (!a2) LoadBitmapCached(path2, g_app.viewer.bitmap2);

    if (g_app.viewer.bitmap && g_app.viewer.bitmap2)
    {
        g_app.viewer.isSpreadActive = true;
        g_app.nav.currentPath = path1;
    }
    else if (g_app.viewer.bitmap)
    {
        g_app.viewer.isSpreadActive = false;
        g_app.nav.currentPath = path1;
    }

    if (a1 || a2)
    {
        g_app.viewer.isAnimating = true;
        g_app.viewer.lastAnimTick = GetTickCount();
        g_app.viewer.animTimer = SetTimer(g_app.wnd.hwndViewer, kAnimTimerId, 16, nullptr);
    }

    ViewerResetView();
    InvalidateRect(g_app.wnd.hwndViewer, nullptr, FALSE);
}

// 画像サイズキャッシュ（プリフェッチスレッドからも書き込まれるため mutex 保護）
static std::mutex g_sizeCacheMutex;
struct ImageSizeInfo { UINT w, h, frameCount; };
static std::unordered_map<std::wstring, ImageSizeInfo> g_sizeCache;

// WIC で画像サイズだけ高速取得（キャッシュ付き）
// cacheOnly=true: キャッシュミス時にWICデコーダを開かずfalseを返す（UIブロック回避）
static bool GetImageSize(const std::wstring& path, UINT& w, UINT& h, bool cacheOnly = false, UINT* outFrameCount = nullptr)
{
    // キャッシュヒット（mutex保護）
    {
        std::lock_guard<std::mutex> lock(g_sizeCacheMutex);
        auto it = g_sizeCache.find(path);
        if (it != g_sizeCache.end())
        {
            w = it->second.w;
            h = it->second.h;
            if (outFrameCount) *outFrameCount = it->second.frameCount;
            return true;
        }
    }

    // キャッシュオンリーモード: WICデコーダを開かない（高速フォールバック）
    if (cacheOnly) return false;

    if (!g_app.viewer.wicFactory) return false;

    ComPtr<IWICBitmapDecoder> decoder;
    HRESULT hr;

    // 書庫内パスの場合: StreamCacheからメモリデコードでサイズ取得
    std::wstring arcPath, entryPath;
    if (SplitArchivePath(path, arcPath, entryPath))
    {
        auto cached = StreamCacheGet(path);
        if (!cached) return false; // StreamCacheになければ諦める（プリフェッチ待ち）

        ComPtr<IWICStream> stream;
        hr = g_app.viewer.wicFactory->CreateStream(stream.GetAddressOf());
        if (FAILED(hr)) return false;
        hr = stream->InitializeFromMemory(const_cast<BYTE*>(cached->data()), (DWORD)cached->size());
        if (FAILED(hr)) return false;
        hr = g_app.viewer.wicFactory->CreateDecoderFromStream(
            stream.Get(), nullptr, WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf());
    }
    else
    {
        hr = g_app.viewer.wicFactory->CreateDecoderFromFilename(
            path.c_str(), nullptr, GENERIC_READ,
            WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf());
    }
    if (FAILED(hr)) return false;

    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, frame.GetAddressOf());
    if (FAILED(hr)) return false;

    if (FAILED(frame->GetSize(&w, &h))) return false;

    UINT frameCount = 0;
    decoder->GetFrameCount(&frameCount);
    if (outFrameCount) *outFrameCount = frameCount;

    // キャッシュに保存（mutex保護、超過時は半分削除）
    {
        std::lock_guard<std::mutex> lock(g_sizeCacheMutex);
        if (g_sizeCache.size() > 3000) {
            auto it = g_sizeCache.begin();
            for (int i = 0; i < 1500 && it != g_sizeCache.end(); i++)
                it = g_sizeCache.erase(it);
        }
        g_sizeCache[path] = { w, h, frameCount };
    }
    return true;
}

// プリフェッチスレッドから画像サイズキャッシュを先読みする
void CacheImageSize(const std::wstring& path, UINT w, UINT h)
{
    std::lock_guard<std::mutex> lock(g_sizeCacheMutex);
    if (g_sizeCache.size() > 3000) {
        auto it = g_sizeCache.begin();
        for (int i = 0; i < 1500 && it != g_sizeCache.end(); i++)
            it = g_sizeCache.erase(it);
    }
    // 既存エントリのframeCountを保持（プリフェッチではframeCount不明）
    // デフォルト0 = 未取得。IsPortraitでframeCount > 1のみ判定するので0は影響なし
    auto existing = g_sizeCache.find(path);
    UINT fc = (existing != g_sizeCache.end()) ? existing->second.frameCount : 0;
    g_sizeCache[path] = { w, h, fc };
}

// 縦向き判定（w/h <= spreadThreshold なら縦向き）
// cacheOnly=true: キャッシュミス時はtrue（縦向き扱い=見開き可能）を返す
// sizeKnown: サイズが実際に取得できたかどうかを返す（nullptrなら無視）
static bool IsPortrait(const std::wstring& path, bool cacheOnly = false, bool* sizeKnown = nullptr)
{
    UINT w = 0, h = 0, frameCount = 0;
    bool got = GetImageSize(path, w, h, cacheOnly, &frameCount);
    if (sizeKnown) *sizeKnown = got && w > 0 && h > 0;
    if (!got || w == 0 || h == 0) return true; // 取得失敗は縦向き扱い
    const auto& s = GetCachedSettings();
    return (float)w / h <= s.spreadThreshold;
}

// 見開き判定結果キャッシュ
static std::unordered_map<int, bool> g_spreadCache;
static std::mutex g_spreadCacheMutex;
void ClearSpreadCache() { std::lock_guard<std::mutex> lk(g_spreadCacheMutex); g_spreadCache.clear(); }

bool ShouldShowSpread(int index)
{
    // キャッシュヒット（スレッドセーフ）
    {
        std::lock_guard<std::mutex> lk(g_spreadCacheMutex);
        auto it = g_spreadCache.find(index);
        if (it != g_spreadCache.end()) return it->second;
    }

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
    const auto& s = GetCachedSettings();
    if (index == 0 && s.spreadFirstSingle) return false;  // 表紙は単独（設定依存）
    if (index + 1 >= total) return false; // 最後のページは単独

    const auto& currentPath = g_app.nav.viewableFiles[index];
    const auto& nextPath = g_app.nav.viewableFiles[index + 1];

    // メディアファイルは見開き不可
    if (IsMediaFile(currentPath) || IsMediaFile(nextPath)) return false;

    // 現在のページの縦横比チェック（サイズ不明ならWICヘッダ読みで取得）
    bool size1Known = false, size2Known = false;
    bool cur = IsPortrait(currentPath, false, &size1Known);
    bool nxt = IsPortrait(nextPath, false, &size2Known);

    // 横向きと確定した画像がある → 単独（キャッシュに記録）
    if (size1Known && !cur) { std::lock_guard<std::mutex> lk(g_spreadCacheMutex); g_spreadCache[index] = false; return false; }
    if (size2Known && !nxt) { std::lock_guard<std::mutex> lk(g_spreadCacheMutex); g_spreadCache[index] = false; return false; }

    // 両方のサイズが判明して両方縦向き → 見開き（キャッシュに記録）
    if (size1Known && size2Known) { std::lock_guard<std::mutex> lk(g_spreadCacheMutex); g_spreadCache[index] = true; return true; }

    // サイズ不明あり → 単独表示（安全側）。キャッシュには入れない
    // （プリフェッチでサイズが判明したら次回再判定される）
    return false;
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
