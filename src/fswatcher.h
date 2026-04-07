#pragma once
#include "app.h"

constexpr UINT WM_FOLDER_CHANGED = WM_APP + 7;

void FsWatcherStart(const std::wstring& folderPath);
void FsWatcherStop();
