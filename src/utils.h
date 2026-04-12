#pragma once
#include <string>
#include <shlwapi.h>

// === パス正規化 ===
inline std::wstring EnsureTrailingSlash(std::wstring path) {
    if (!path.empty() && path.back() != L'\\') path += L'\\';
    return path;
}

inline std::wstring StripTrailingSlash(std::wstring path) {
    if (!path.empty() && path.back() == L'\\') path.pop_back();
    return path;
}

// === 拡張子チェック汎用テンプレート ===
template<size_t N>
bool HasExtension(const std::wstring& path, const wchar_t* const (&exts)[N]) {
    const wchar_t* ext = PathFindExtensionW(path.c_str());
    if (!ext || !*ext) return false;
    for (auto e : exts)
        if (_wcsicmp(ext, e) == 0) return true;
    return false;
}

// === JSON文字列エスケープ ===
inline std::wstring EscapeJsonPath(const std::wstring& path) {
    std::wstring result;
    for (auto c : path) {
        if (c == L'\\') result += L"\\\\";
        else if (c == L'"') result += L"\\\"";
        else result += c;
    }
    return result;
}

inline std::wstring UnescapeJsonPath(const std::wstring& s) {
    std::wstring result;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == L'\\' && i + 1 < s.size()) {
            if (s[i + 1] == L'\\') { result += L'\\'; i++; }
            else if (s[i + 1] == L'"') { result += L'"'; i++; }
            else result += s[i];
        } else {
            result += s[i];
        }
    }
    return result;
}

// === ファイルI/O ===
inline bool WriteWStringToFile(const std::wstring& path, const std::wstring& content)
{
    int len = (int)content.size();
    int u8len = WideCharToMultiByte(CP_UTF8, 0, content.c_str(), len, nullptr, 0, nullptr, nullptr);
    std::string u8(u8len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, content.c_str(), len, &u8[0], u8len, nullptr, nullptr);

    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    DWORD written;
    BOOL ok = WriteFile(hFile, u8.c_str(), (DWORD)u8.size(), &written, nullptr);
    CloseHandle(hFile);
    return ok && written == (DWORD)u8.size();
}

inline std::wstring ReadFileToWString(const std::wstring& path)
{
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return L"";
    DWORD size = GetFileSize(hFile, nullptr);
    if (size == 0 || size > 1024 * 1024) { CloseHandle(hFile); return L""; }
    std::string buf(size, '\0');
    DWORD read;
    ReadFile(hFile, &buf[0], size, &read, nullptr);
    CloseHandle(hFile);
    int wlen = MultiByteToWideChar(CP_UTF8, 0, buf.c_str(), (int)read, nullptr, 0);
    std::wstring wbuf(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, buf.c_str(), (int)read, &wbuf[0], wlen);
    return wbuf;
}

// === JSON 文字列配列の保存/読み込み ===
inline bool SaveStringArray(const std::wstring& path, const std::vector<std::wstring>& items)
{
    std::wstring json = L"[\n";
    for (size_t i = 0; i < items.size(); i++)
    {
        json += L"  \"" + EscapeJsonPath(items[i]) + L"\"";
        if (i + 1 < items.size()) json += L",";
        json += L"\n";
    }
    json += L"]\n";
    return WriteWStringToFile(path, json);
}

inline std::vector<std::wstring> LoadStringArray(const std::wstring& path)
{
    std::vector<std::wstring> result;
    std::wstring json = ReadFileToWString(path);
    if (json.empty()) return result;
    size_t pos = 0;
    while ((pos = json.find(L'"', pos)) != std::wstring::npos)
    {
        pos++;
        auto end = json.find(L'"', pos);
        if (end == std::wstring::npos) break;
        result.push_back(UnescapeJsonPath(json.substr(pos, end - pos)));
        pos = end + 1;
    }
    return result;
}

// === パス操作 ===
inline std::wstring PathBaseName(const std::wstring& path)
{
    auto pos = path.find_last_of(L'\\');
    return (pos != std::wstring::npos) ? path.substr(pos + 1) : path;
}

// === 小文字変換 ===
inline std::wstring ToLowerW(const std::wstring& s) {
    std::wstring result = s;
    for (auto& c : result) c = towlower(c);
    return result;
}

// === 簡易JSONパーサー（共通） ===
inline bool JsonGetInt(const std::wstring& json, const wchar_t* key, int& out)
{
    std::wstring search = L"\""; search += key; search += L"\"";
    auto pos = json.find(search);
    if (pos == std::wstring::npos) return false;
    pos = json.find(L':', pos);
    if (pos == std::wstring::npos) return false;
    pos++;
    while (pos < json.size() && (json[pos] == L' ' || json[pos] == L'\t')) pos++;
    out = _wtoi(&json[pos]);
    return true;
}

inline bool JsonGetBool(const std::wstring& json, const wchar_t* key, bool& out)
{
    std::wstring search = L"\""; search += key; search += L"\"";
    auto pos = json.find(search);
    if (pos == std::wstring::npos) return false;
    pos = json.find(L':', pos);
    if (pos == std::wstring::npos) return false;
    pos++;
    while (pos < json.size() && json[pos] == L' ') pos++;
    out = (json.substr(pos, 4) == L"true");
    return true;
}

inline bool JsonGetString(const std::wstring& json, const wchar_t* key, std::wstring& out)
{
    std::wstring search = L"\""; search += key; search += L"\"";
    auto pos = json.find(search);
    if (pos == std::wstring::npos) return false;
    pos = json.find(L':', pos);
    if (pos == std::wstring::npos) return false;
    pos = json.find(L'"', pos + 1);
    if (pos == std::wstring::npos) return false;
    pos++;
    auto end = json.find(L'"', pos);
    if (end == std::wstring::npos) return false;
    out = UnescapeJsonPath(json.substr(pos, end - pos));
    return true;
}

inline bool JsonGetFloat(const std::wstring& json, const wchar_t* key, float& out)
{
    std::wstring search = L"\""; search += key; search += L"\"";
    auto pos = json.find(search);
    if (pos == std::wstring::npos) return false;
    pos = json.find(L':', pos);
    if (pos == std::wstring::npos) return false;
    pos++;
    while (pos < json.size() && (json[pos] == L' ' || json[pos] == L'\t')) pos++;
    out = (float)_wtof(&json[pos]);
    return true;
}

inline bool JsonGetIntArray(const std::wstring& json, const wchar_t* key, std::vector<int>& out)
{
    std::wstring search = L"\""; search += key; search += L"\"";
    auto pos = json.find(search);
    if (pos == std::wstring::npos) return false;
    pos = json.find(L'[', pos);
    if (pos == std::wstring::npos) return false;
    auto end = json.find(L']', pos);
    if (end == std::wstring::npos) return false;
    out.clear();
    pos++;
    while (pos < end)
    {
        while (pos < end && (json[pos] == L' ' || json[pos] == L',' || json[pos] == L'\t')) pos++;
        if (pos >= end) break;
        out.push_back(_wtoi(&json[pos]));
        while (pos < end && json[pos] != L',') pos++;
    }
    return !out.empty();
}

// === 定数 ===
constexpr float kZoomMin = 0.25f;  // 25%
constexpr float kZoomMax = 16.0f;  // 1600%
constexpr float kZoomStep = 1.1f;
constexpr size_t kDefaultCacheBytes = 300ULL * 1024 * 1024;
constexpr int kDefaultCacheEntries = 50;
