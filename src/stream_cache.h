#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <memory>

void StreamCacheInit(size_t maxBytes = 100 * 1024 * 1024);
std::shared_ptr<const std::vector<BYTE>> StreamCacheGet(const std::wstring& key);
void StreamCachePut(const std::wstring& key, std::vector<BYTE>&& data);
void StreamCacheClear();
