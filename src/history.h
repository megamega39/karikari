#pragma once
#include "app.h"

struct HistoryEntry {
    std::wstring path;
    std::wstring name;        // 表示名
    std::wstring entryType;   // "archive" | "media" | "folder"
    FILETIME accessTime;
};

void HistoryAdd(const std::wstring& path);
void HistoryRemoveEntry(const std::wstring& path);
const std::vector<HistoryEntry>& HistoryGetAll();
void HistoryClear();
void HistoryLoad();
void HistorySave();
