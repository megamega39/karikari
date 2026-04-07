#pragma once
#include "app.h"

HWND CreateStatusBar(HWND parent, HINSTANCE hInst);
void UpdateStatusBar(const std::wstring& path, int imgW, int imgH, const std::wstring& msg,
                     ULONGLONG fileSize = 0);
