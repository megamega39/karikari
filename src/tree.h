#pragma once
#include "app.h"

void InitFolderTree();
void ExpandTreeNode(HTREEITEM hItem);
std::wstring GetTreeItemPath(HTREEITEM hItem);
void SelectTreePath(const std::wstring& path);
void RefreshFavoritesInTree();
void SaveTreeCaches(); // アプリ終了時にアイコン/サブフォルダキャッシュを保存
void ShowBookshelfTree();  // 本棚（お気に入り）のみ表示
void ShowHistoryTree();    // 履歴のみ表示
void ShowNormalTree();     // 通常のフォルダツリーに戻す
void SaveNormalTreeState(); // 通常ツリーの展開状態を保存（モード切替前に呼ぶ）
void RefreshTree();         // ツリーを再構築（展開状態+スクロール位置を保���）
void RemoveTreeItemByPath(const std::wstring& path); // パス一致するツリーノードを削除
int GetTreeMode();         // 0=通常, 1=本棚, 2=履歴
bool IsFavoritesChild(HTREEITEM hItem); // お気に入り直下の子ノードか判定

// ツリーソート
enum TreeSortMode { SortByName, SortByDate, SortBySize, SortByType };
void SetTreeSortMode(TreeSortMode mode, bool descending);
TreeSortMode GetTreeSortMode();
bool GetTreeSortDescending();
