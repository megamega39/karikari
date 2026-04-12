#pragma once
#include "app.h"
#include <string>

// コンテキストメニューコマンドID
enum ContextCmd {
    CTX_OPEN_EXPLORER = 1000, CTX_COPY_PATH, CTX_COPY_PARENT_PATH, CTX_COPY_IMAGE, CTX_FULLSCREEN,
    CTX_OPEN_ASSOC, CTX_PROPERTIES,
    CTX_ADD_FAV, CTX_REMOVE_FAV, CTX_REMOVE_SHELF, CTX_SHELF_AS_CAT, CTX_DELETE,
    CTX_SHELF_BASE = 1100, CTX_SHELF_NEW = 1199
};

void ShowFileContextMenu(HWND hwnd, const std::wstring& path, POINT pt);
void ShowInExplorer(const std::wstring& path);
void ShowContextMenu(HWND hwnd, int x, int y, bool isViewer);
void CopyToClipboard(HWND hwnd, const std::wstring& text);
void CopyImageToClipboard(HWND hwnd);
bool ShowInputDialog(HWND hwndParent, const wchar_t* title, const wchar_t* prompt,
                     wchar_t* buf, int bufSize, const wchar_t* defaultText = L"");
