#pragma once
#include "app.h"
#include <string>

constexpr UINT WM_PREFETCH_DONE = WM_APP + 3;

struct PrefetchDoneMsg {
    IWICBitmap* wicBmp;
    std::wstring path;
};

void PrefetchShutdown();
void PrefetchStart(int currentIndex, int direction);
void PrefetchIdleStart(int currentIndex, int direction); // アイドル時追加先読み
void PrefetchCancel();
void PrefetchResetSettings(); // 設定変更時に呼ぶ
void PrefetchSetFirstLoad();  // 書庫オープン直後の初回プリフェッチを少量にする
