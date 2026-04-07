#pragma once
#include "app.h"

bool RegisterSplitterClass(HINSTANCE hInst);

// vertical=true: 左右分割（垂直バー）、false: 上下分割（水平バー）
HWND CreateSplitter(HWND parent, HINSTANCE hInst, bool vertical, UINT id);
