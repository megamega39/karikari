#pragma once
#include "app.h"

bool RegisterMainWindow(HINSTANCE hInst);
HWND CreateMainWindow(HINSTANCE hInst, int nCmdShow);
void LayoutChildren(HWND hwndParent);
void ToggleFullscreen(HWND hwnd);
void SetMainTitle(const std::wstring& path);
