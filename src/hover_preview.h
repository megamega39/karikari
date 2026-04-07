#pragma once
#include "app.h"
#include <string>
#include <atomic>

// ホバープレビュー定数（サブクラスから参照）
constexpr UINT_PTR kHoverTimerId = 77;
constexpr int kHoverDelayMs = 200;
constexpr UINT WM_HOVER_PREVIEW_DONE = WM_APP + 10;

// ホバープレビュー状態（サブクラスから参照）
extern int g_hoverItemIndex;
extern std::wstring g_hoverPath;
extern std::atomic<int> g_hoverGeneration;

void ShowHoverPreview(const std::wstring& path, POINT screenPos);
void HideHoverPreview();
void HandleHoverPreviewDone(HWND hwnd, WPARAM wParam, LPARAM lParam);
void CleanupHoverPreview();
