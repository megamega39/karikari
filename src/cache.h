#pragma once
#include "app.h"
#include <list>
#include <unordered_map>

void CacheInit(size_t maxBytes = 0, int maxEntries = 0); // maxBytes=0: 物理メモリベースの動的上限
ComPtr<ID2D1Bitmap> CacheGet(const std::wstring& key);
bool CacheContains(const std::wstring& key); // LRU更新なし、存在チェックのみ
void CachePut(const std::wstring& key, ComPtr<ID2D1Bitmap> bmp);
void CacheClear();
