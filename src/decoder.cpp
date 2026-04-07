#include "decoder.h"
#include "viewer.h"
#include <d2d1_1.h>
#include <d2d1_3.h>
#include <d3d11.h>
#include <dxgi1_2.h>

HRESULT InitD2D()
{
    // D2D 1.1 Factory (マルチスレッド対応)
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED,
                                   g_app.viewer.d2dFactory.GetAddressOf());
    if (FAILED(hr)) return hr;

    // D3D11デバイス作成
    D3D_FEATURE_LEVEL featureLevel;
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                           D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                           nullptr, 0, D3D11_SDK_VERSION,
                           g_app.viewer.d3dDevice.GetAddressOf(),
                           &featureLevel, nullptr);
    if (FAILED(hr)) {
        // ハードウェア失敗時はWARPにフォールバック
        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
                               D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                               nullptr, 0, D3D11_SDK_VERSION,
                               g_app.viewer.d3dDevice.GetAddressOf(),
                               &featureLevel, nullptr);
        if (FAILED(hr)) return hr;
    }

    // DXGI → D2D デバイス → DeviceContext
    ComPtr<IDXGIDevice> dxgiDevice;
    hr = g_app.viewer.d3dDevice.As(&dxgiDevice);
    if (FAILED(hr)) return hr;

    ComPtr<ID2D1Device> d2dDevice;
    hr = g_app.viewer.d2dFactory->CreateDevice(dxgiDevice.Get(), d2dDevice.GetAddressOf());
    if (FAILED(hr)) return hr;

    hr = d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                                        g_app.viewer.deviceContext.GetAddressOf());
    if (FAILED(hr)) return hr;

    // WIC Factory
    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(g_app.viewer.wicFactory.GetAddressOf()));
    return hr;
}

// IWICBitmapSourceTransformでネイティブダウンスケール（JPEG: 1/2, 1/4, 1/8）
// 非対応フォーマットはWICBitmapScalerにフォールバック
static HRESULT DownscaleSource(IWICBitmapSource* frame, IWICImagingFactory* factory,
                                UINT dstW, UINT dstH, ComPtr<IWICBitmapSource>& outSource)
{
    // IWICBitmapSourceTransformを試行（JPEGデコーダはこれをサポート）
    // 最寄りサイズを取得し、IWICBitmapScalerでそのサイズにリサイズ
    // （中間IWICBitmapへのCopyPixelsを回避してRAMコピー削減）
    ComPtr<IWICBitmapSourceTransform> transform;
    if (SUCCEEDED(frame->QueryInterface(IID_PPV_ARGS(transform.GetAddressOf()))))
    {
        UINT closestW = dstW, closestH = dstH;
        if (SUCCEEDED(transform->GetClosestSize(&closestW, &closestH)))
        {
            // JPEGの最寄りサイズ（1/2, 1/4, 1/8等）でScalerを使用
            ComPtr<IWICBitmapScaler> scaler;
            HRESULT hr = factory->CreateBitmapScaler(scaler.GetAddressOf());
            if (SUCCEEDED(hr))
            {
                hr = scaler->Initialize(frame, closestW, closestH,
                    WICBitmapInterpolationModeLinear);
                if (SUCCEEDED(hr))
                {
                    outSource = scaler;
                    return S_OK;
                }
            }
        }
    }

    // フォールバック: WICBitmapScaler（PNG, WebP等）
    UINT imgW = 0, imgH = 0;
    HRESULT hrSize = frame->GetSize(&imgW, &imgH);
    if (FAILED(hrSize) || imgW == 0 || imgH == 0) return E_FAIL;
    float scale = std::min((float)dstW / imgW, (float)dstH / imgH);

    WICBitmapInterpolationMode mode;
    if (scale > 0.75f)
        mode = WICBitmapInterpolationModeHighQualityCubic;
    else if (scale > 0.3f)
        mode = WICBitmapInterpolationModeLinear;
    else
        mode = WICBitmapInterpolationModeFant;

    ComPtr<IWICBitmapScaler> scaler;
    HRESULT hr = factory->CreateBitmapScaler(scaler.GetAddressOf());
    if (FAILED(hr)) return hr;
    hr = scaler->Initialize(frame, dstW, dstH, mode);
    if (FAILED(hr)) return hr;
    outSource = scaler;
    return S_OK;
}

// フレームに対してフォーマット変換 + 条件付きリサイズを適用
static HRESULT ConvertAndOptionalResize(IWICBitmapSource* frame, ID2D1RenderTarget* rt,
                                         IWICImagingFactory* factory, ID2D1Bitmap** ppBitmap)
{
    UINT imgW, imgH;
    frame->GetSize(&imgW, &imgH);

    // 表示領域サイズを取得
    auto rtSize = rt->GetSize();
    UINT viewW = (UINT)rtSize.width;
    UINT viewH = (UINT)rtSize.height;

    // デコード時リサイズ: 元画像が表示領域の1.5倍以上大きい場合
    ComPtr<IWICBitmapSource> source = frame;
    if (viewW > 0 && viewH > 0 && imgW > viewW * 3 / 2 && imgH > viewH * 3 / 2)
    {
        float scale = std::min((float)viewW / imgW, (float)viewH / imgH);
        UINT dstW = std::max(1U, (UINT)(imgW * scale));
        UINT dstH = std::max(1U, (UINT)(imgH * scale));

        ComPtr<IWICBitmapSource> scaled;
        if (SUCCEEDED(DownscaleSource(frame, factory, dstW, dstH, scaled)))
            source = scaled;
    }

    // フォーマット変換
    ComPtr<IWICFormatConverter> converter;
    HRESULT hr = factory->CreateFormatConverter(converter.GetAddressOf());
    if (FAILED(hr)) return hr;

    hr = converter->Initialize(source.Get(), GUID_WICPixelFormat32bppPBGRA,
                               WICBitmapDitherTypeNone, nullptr, 0.0,
                               WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) return hr;

    return rt->CreateBitmapFromWicBitmap(converter.Get(), nullptr, ppBitmap);
}

// 共通: デコーダからフレーム0を取得
static HRESULT LoadFirstFrame(const std::wstring& path, IWICImagingFactory* factory,
                              IWICBitmapFrameDecode** ppFrame)
{
    ComPtr<IWICBitmapDecoder> decoder;
    HRESULT hr = factory->CreateDecoderFromFilename(
        path.c_str(), nullptr, GENERIC_READ,
        WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf());
    if (FAILED(hr)) return hr;
    return decoder->GetFrame(0, ppFrame);
}

HRESULT DecodeImage(const std::wstring& path, ID2D1RenderTarget* rt, ID2D1Bitmap** ppBitmap)
{
    if (!g_app.viewer.wicFactory || !rt) return E_FAIL;

    ComPtr<IWICBitmapFrameDecode> frame;
    HRESULT hr = LoadFirstFrame(path, g_app.viewer.wicFactory.Get(), frame.GetAddressOf());
    if (FAILED(hr)) return hr;

    // YCbCr最適化 + デコード時リサイズ
    return ConvertAndOptionalResize(frame.Get(), rt, g_app.viewer.wicFactory.Get(), ppBitmap);
}

HRESULT DecodeImageFromMemory(const BYTE* data, size_t size, ID2D1RenderTarget* rt, ID2D1Bitmap** ppBitmap)
{
    if (!g_app.viewer.wicFactory || !rt || !data || size == 0) return E_FAIL;
    if (size > MAXDWORD) return E_INVALIDARG; // 4GB超のデータはWICストリーム非対応

    ComPtr<IWICStream> stream;
    HRESULT hr = g_app.viewer.wicFactory->CreateStream(stream.GetAddressOf());
    if (FAILED(hr)) return hr;

    hr = stream->InitializeFromMemory(const_cast<BYTE*>(data), (DWORD)size);
    if (FAILED(hr)) return hr;

    // マジックバイトでフォーマット判定 → コーデック検索スキップ
    const GUID* vendorGuid = nullptr;
    if (size >= 2)
    {
        if (data[0] == 0xFF && data[1] == 0xD8)
            vendorGuid = &GUID_ContainerFormatJpeg;
        else if (data[0] == 0x89 && data[1] == 0x50)
            vendorGuid = &GUID_ContainerFormatPng;
        else if (size >= 12 && data[0] == 'R' && data[1] == 'I' && data[2] == 'F' && data[3] == 'F'
                 && data[8] == 'W' && data[9] == 'E' && data[10] == 'B' && data[11] == 'P')
            vendorGuid = &GUID_ContainerFormatWebp;
    }

    ComPtr<IWICBitmapDecoder> decoder;
    hr = g_app.viewer.wicFactory->CreateDecoderFromStream(
        stream.Get(), vendorGuid, WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf());
    if (FAILED(hr)) return hr;

    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, frame.GetAddressOf());
    if (FAILED(hr)) return hr;

    hr = ConvertAndOptionalResize(frame.Get(), rt, g_app.viewer.wicFactory.Get(), ppBitmap);
    return hr;
}

// === プリフェッチ用: WICBitmapまでデコード（スレッドセーフ） ===

static HRESULT DecodeFrameToWicBitmap(IWICBitmapSource* frame, IWICImagingFactory* factory, IWICBitmap** ppWicBmp)
{
    // ビューアサイズに基づくダウンスケール（プリフェッチでも表示サイズベースでデコード）
    ComPtr<IWICBitmapSource> source = frame;
    UINT imgW = 0, imgH = 0;
    frame->GetSize(&imgW, &imgH);

    HWND hwndViewer = g_app.wnd.hwndViewer;
    if (hwndViewer && imgW > 0 && imgH > 0)
    {
        RECT rc;
        GetClientRect(hwndViewer, &rc);
        UINT viewW = rc.right - rc.left;
        UINT viewH = rc.bottom - rc.top;
        if (viewW > 0 && viewH > 0 && imgW > viewW * 3 / 2 && imgH > viewH * 3 / 2)
        {
            float scale = std::min((float)viewW / imgW, (float)viewH / imgH);
            UINT dstW = std::max(1U, (UINT)(imgW * scale));
            UINT dstH = std::max(1U, (UINT)(imgH * scale));

            ComPtr<IWICBitmapSource> scaled;
            if (SUCCEEDED(DownscaleSource(frame, factory, dstW, dstH, scaled)))
                source = scaled;
        }
    }

    ComPtr<IWICFormatConverter> converter;
    HRESULT hr = factory->CreateFormatConverter(converter.GetAddressOf());
    if (FAILED(hr)) return hr;

    hr = converter->Initialize(source.Get(), GUID_WICPixelFormat32bppPBGRA,
                               WICBitmapDitherTypeNone, nullptr, 0.0,
                               WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) return hr;

    hr = factory->CreateBitmapFromSource(converter.Get(), WICBitmapCacheOnLoad, ppWicBmp);
    return hr;
}

HRESULT DecodeToWicBitmap(const std::wstring& path, IWICImagingFactory* factory, IWICBitmap** ppWicBmp)
{
    if (!factory) return E_FAIL;

    ComPtr<IWICBitmapFrameDecode> frame;
    HRESULT hr = LoadFirstFrame(path, factory, frame.GetAddressOf());
    if (FAILED(hr)) return hr;

    return DecodeFrameToWicBitmap(frame.Get(), factory, ppWicBmp);
}

HRESULT DecodeMemoryToWicBitmap(const BYTE* data, size_t size, IWICImagingFactory* factory, IWICBitmap** ppWicBmp)
{
    if (!factory || !data || size == 0) return E_FAIL;
    if (size > MAXDWORD) return E_INVALIDARG;

    ComPtr<IWICStream> stream;
    HRESULT hr = factory->CreateStream(stream.GetAddressOf());
    if (FAILED(hr)) return hr;

    hr = stream->InitializeFromMemory(const_cast<BYTE*>(data), (DWORD)size);
    if (FAILED(hr)) return hr;

    // マジックバイトでフォーマット判定 → コーデック検索スキップ
    const GUID* vendorGuid = nullptr;
    if (size >= 2)
    {
        if (data[0] == 0xFF && data[1] == 0xD8)
            vendorGuid = &GUID_ContainerFormatJpeg;
        else if (data[0] == 0x89 && data[1] == 0x50)
            vendorGuid = &GUID_ContainerFormatPng;
        else if (size >= 12 && data[0] == 'R' && data[1] == 'I' && data[2] == 'F' && data[3] == 'F'
                 && data[8] == 'W' && data[9] == 'E' && data[10] == 'B' && data[11] == 'P')
            vendorGuid = &GUID_ContainerFormatWebp;
    }

    ComPtr<IWICBitmapDecoder> decoder;
    hr = factory->CreateDecoderFromStream(
        stream.Get(), vendorGuid, WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf());
    if (FAILED(hr)) return hr;

    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, frame.GetAddressOf());
    if (FAILED(hr)) return hr;

    return DecodeFrameToWicBitmap(frame.Get(), factory, ppWicBmp);
}

HRESULT WicBitmapToD2D(IWICBitmapSource* wicBmp, ID2D1RenderTarget* rt, ID2D1Bitmap** ppBitmap)
{
    if (!wicBmp || !rt) return E_FAIL;
    // D2D 1.1 DeviceContext経由のCreateBitmapFromWicBitmapは
    // 内部的にGPU最適化パスを使用（YCbCr対応GPUでは自動的に最適化）
    return rt->CreateBitmapFromWicBitmap(wicBmp, nullptr, ppBitmap);
}
