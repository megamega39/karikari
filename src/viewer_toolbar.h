#pragma once
#include "app.h"

void CreateViewerToolbars(HWND parent, HINSTANCE hInst);
void UpdateViewerToolbarState();
void LayoutViewerToolbarLabels();
void UpdatePageCounter(int current, int total);
void UpdateZoomLabel();
