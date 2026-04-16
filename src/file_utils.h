#pragma once
#include <string>
#include <shlwapi.h>

// 画像拡張子リスト
inline const wchar_t* const kImageExts[] = {
    L".jpg", L".jpeg", L".png", L".bmp", L".gif",
    L".webp", L".avif", L".tiff", L".tif", L".ico"
};

// 書庫拡張子リスト
inline const wchar_t* const kArchiveExts[] = {
    L".zip", L".7z", L".rar", L".cbz", L".cbr", L".cb7",
};

// 拡張子チェック汎用テンプレート（utils.h と同一）
template<size_t N>
bool HasExtensionCheck(const std::wstring& path, const wchar_t* const (&exts)[N]) {
    const wchar_t* ext = PathFindExtensionW(path.c_str());
    if (!ext || !*ext) return false;
    for (auto e : exts)
        if (_wcsicmp(ext, e) == 0) return true;
    return false;
}

inline bool IsImageFileCheck(const std::wstring& path)
{
    return HasExtensionCheck(path, kImageExts);
}

inline bool IsArchiveFileCheck(const std::wstring& path)
{
    return HasExtensionCheck(path, kArchiveExts);
}

// ファイルサイズフォーマット
inline std::wstring FormatFileSizeUtil(ULONGLONG size)
{
    wchar_t buf[64];
    if (size < 1024)
        swprintf_s(buf, 64, L"%llu B", size);
    else if (size < 1024 * 1024)
        swprintf_s(buf, 64, L"%.1f KB", size / 1024.0);
    else if (size < 1024ULL * 1024 * 1024)
        swprintf_s(buf, 64, L"%.1f MB", size / (1024.0 * 1024.0));
    else
        swprintf_s(buf, 64, L"%.1f GB", size / (1024.0 * 1024.0 * 1024.0));
    return buf;
}
