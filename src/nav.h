#pragma once
#include "app.h"

void NavigateTo(const std::wstring& path, NavigateOptions opts = {});
void NavigateBack();
void NavigateForward();
void NavigateUp();
void GoToFile(int index);
void NavigateToSiblingArchive(int direction); // direction: -1=前, +1=次
void LoadArchiveToList(const std::wstring& archivePath);
void NavResetSettings(); // 設定変更時に呼ぶ
void NavHistorySave();   // 戻る/進む履歴を永続化
void NavHistoryLoad();   // 戻る/進む履歴を復元

struct ArchiveLoadResult {
    int generation;
    std::wstring archivePath;
    std::vector<FileItem> fileItems;
    std::vector<std::wstring> viewableFiles;
    std::unordered_map<std::wstring, int> viewableFileIndex;
    std::unordered_map<std::wstring, int> fileItemIndex;
    NavigateOptions opts;
};

int GetArchiveLoadGeneration();
void ApplyArchiveLoadResult(ArchiveLoadResult* result);
