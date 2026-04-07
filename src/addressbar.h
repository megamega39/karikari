#pragma once
#include "app.h"

void CreateAddressBar(HWND parent, HINSTANCE hInst);
void UpdateAddressBar(const std::wstring& path);
void LayoutAddressBar(int x, int y, int w, int h);
