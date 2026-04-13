#pragma once
#include "app.h"

bool IsImageFile(const std::wstring& path);
bool IsViewableFile(const std::wstring& name);
void InitListView();
void LoadFolder(const std::wstring& folderPath);
void PopulateListView();
void SortFileList(int column);
void RebuildViewableFiles();
void ListViewSelectItem(int index, int count = 1); // count=2で見開き時2アイテム選択
void SetFileListFilter(const std::wstring& filter);
void RemoveFileItemByPath(const std::wstring& path);
void StartInlineRename(int index); // ファイルリストのインラインリネーム開始
void CancelInlineRename();         // インラインリネームキャンセル

// 仮想リスト用: アイテムデータ取得ヘルパー
std::wstring FormatFileSize(ULONGLONG size);
std::wstring GetFileType(const std::wstring& name, bool isDir);
std::wstring FormatFileTime(const FILETIME& ft);
int GetFileIconIndex(const std::wstring& path, bool isDir);

// サムネイルグリッド表示
void SwitchToGridView();
void SwitchToListView();
void ApplyThumbnailSize(int size);
int GetThumbnailIndex(int fileItemIndex);  // グリッド用サムネイルインデックス
void OnThumbnailDone(WPARAM wParam, LPARAM lParam); // WM_THUMB_DONE ハンドラ
