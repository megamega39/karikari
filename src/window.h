#pragma once
#include "app.h"

bool RegisterMainWindow(HINSTANCE hInst);
HWND CreateMainWindow(HINSTANCE hInst, int nCmdShow);
void LayoutChildren(HWND hwndParent);
void ToggleFullscreen(HWND hwnd);
void SetMainTitle(const std::wstring& path);
void ApplyFontSize(int fontSize);
void RebuildUI();

// グローバルUIフォント（g_uiFontがnullptrでなければこちらを使用）
extern HFONT g_uiFont;
extern HFONT g_uiFontBold;
