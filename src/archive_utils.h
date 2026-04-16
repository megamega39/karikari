#pragma once
#include <string>
#include <cwctype>

// パストラバーサル検証（書庫エントリのセキュリティチェック）
inline bool IsValidEntryPath(const std::wstring& path)
{
    if (path.find(L"..\\") != std::wstring::npos) return false;
    if (path.find(L"../") != std::wstring::npos) return false;
    if (path == L"..") return false;
    // 末尾が /.. or \.. で終わるケース
    if (path.size() >= 3) {
        auto tail = path.substr(path.size() - 3);
        if (tail == L"\\.." || tail == L"/..") return false;
    }
    if (path.size() >= 2 && path[1] == L':') return false; // 絶対パス
    if (!path.empty() && (path[0] == L'\\' || path[0] == L'/')) return false;
    return true;
}

// パス正規化: / → \、小文字化
inline void NormalizeArchivePathInPlace(std::wstring& p)
{
    for (auto& c : p) { if (c == L'/') c = L'\\'; c = towlower(c); }
}

inline std::wstring NormalizeArchivePath(const std::wstring& p)
{
    std::wstring r = p;
    NormalizeArchivePathInPlace(r);
    return r;
}
