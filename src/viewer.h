#pragma once
#include "app.h"

bool RegisterViewerClass(HINSTANCE hInst);
HWND CreateViewer(HWND parent, HINSTANCE hInst);
void ViewerLoadImage(const std::wstring& path);
void ViewerShowSpread(const std::wstring& path1, const std::wstring& path2);
void ViewerSetScaleMode(ScaleMode mode);
void ViewerSetViewMode(int mode);  // 0=自動, 1=単独, 2=強制見開き
void ViewerToggleBinding();        // RTL切替
void ViewerZoomIn();       // 25%刻み（ボタン用）
void ViewerZoomOut();      // 25%刻み（ボタン用）
void ViewerZoomSmooth(bool zoomIn); // なめらか×1.1/×0.9（ホイール用）
void ViewerStopAnimation();
void ViewerRotateCW();   // 時計回り90度
void ViewerRotateCCW();  // 反時計回り90度
bool ShouldShowSpread(int index);
void ClearSpreadCache();
int GetPagesPerView();
int ViewerGetEffectiveZoomPercent();
void CacheImageSize(const std::wstring& path, UINT w, UINT h);
bool IsConfirmedAnimation(const std::wstring& path); // サイズキャッシュでフレーム数>1を確認
bool TryLoadAnimation(const std::wstring& path, AnimState& anim, ComPtr<ID2D1Bitmap>& outBmp);
void ViewerStartAnimationIfNeeded(const std::wstring& path1, const std::wstring& path2 = L"");
void ViewerLoadImageAsync(const std::wstring& path); // 非同期版: キャッシュミス時にUIをブロックしない
int GetAsyncDecodeGeneration(); // 現在の非同期デコード世代番号を返す（古い結果の棄却用）
void InvalidateAsyncDecode();   // 保留中の非同期デコード結果を無効化
void ViewerSetBitmap(ComPtr<ID2D1Bitmap> bmp, const std::wstring& path);
void ViewerResetView();
constexpr UINT WM_ASYNC_DECODE_DONE = WM_APP + 9;

struct AsyncDecodeDoneMsg {
    IWICBitmap* wicBmp;
    std::wstring path;
    int generation;
};

struct AsyncSpreadDoneMsg {
    IWICBitmap* wicBmp1;
    IWICBitmap* wicBmp2;
    std::wstring path1;
    std::wstring path2;
    int generation;
};

void ViewerLoadSpreadAsync(const std::wstring& path1, const std::wstring& path2);
void PreloadImageSizes(const std::vector<std::wstring>& paths); // 見開き判定用にサイズを先行取得
