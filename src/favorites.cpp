#include "favorites.h"
#include "utils.h"
#include <shlwapi.h>

static std::vector<std::wstring> g_favorites;

static std::wstring GetFavoritesPath()
{
    return GetDataDir() + L"\\favorites.json";
}

void FavoritesAdd(const std::wstring& path)
{
    if (path.empty() || FavoritesContains(path)) return;
    g_favorites.push_back(path);
}

void FavoritesRemove(const std::wstring& path)
{
    g_favorites.erase(
        std::remove_if(g_favorites.begin(), g_favorites.end(),
            [&](const std::wstring& p) { return _wcsicmp(p.c_str(), path.c_str()) == 0; }),
        g_favorites.end());
}

bool FavoritesContains(const std::wstring& path)
{
    for (auto& f : g_favorites)
        if (_wcsicmp(f.c_str(), path.c_str()) == 0) return true;
    return false;
}

const std::vector<std::wstring>& FavoritesGetAll() { return g_favorites; }

void FavoritesSave() { SaveStringArray(GetFavoritesPath(), g_favorites); }
void FavoritesLoad() { g_favorites = LoadStringArray(GetFavoritesPath()); }
