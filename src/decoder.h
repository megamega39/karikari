#pragma once
#include "app.h"

HRESULT InitD2D();
HRESULT DecodeImage(const std::wstring& path, ID2D1RenderTarget* rt, ID2D1Bitmap** ppBitmap);
HRESULT DecodeImageFromMemory(const BYTE* data, size_t size, ID2D1RenderTarget* rt, ID2D1Bitmap** ppBitmap);

// プリフェッチ用: WICBitmapまでデコード（スレッドセーフ、WICFactoryはスレッドごとに渡す）
HRESULT DecodeToWicBitmap(const std::wstring& path, IWICImagingFactory* factory, IWICBitmap** ppWicBmp);
HRESULT DecodeMemoryToWicBitmap(const BYTE* data, size_t size, IWICImagingFactory* factory, IWICBitmap** ppWicBmp);
HRESULT WicBitmapToD2D(IWICBitmapSource* wicBmp, ID2D1RenderTarget* rt, ID2D1Bitmap** ppBitmap);
