#pragma once
#include "app.h"

void FavoritesAdd(const std::wstring& path);
void FavoritesRemove(const std::wstring& path);
bool FavoritesContains(const std::wstring& path);
const std::vector<std::wstring>& FavoritesGetAll();
void FavoritesLoad();
void FavoritesSave();
