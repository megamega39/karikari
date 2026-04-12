#include "history.h"
#include "archive.h"
#include "media.h"
#include "utils.h"
#include <shlwapi.h>

static std::vector<HistoryEntry> g_history;
static const int kMaxHistory = 500;

static std::wstring GetHistoryPath()
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    PathRemoveFileSpecW(path);
    PathAppendW(path, L"history.json");
    return path;
}

static FILETIME GetCurrentFileTime()
{
    SYSTEMTIME st;
    GetSystemTime(&st); // UTC
    FILETIME ft;
    SystemTimeToFileTime(&st, &ft);
    return ft;
}

void HistoryAdd(const std::wstring& path)
{
    if (path.empty()) return;

    // 記録対象: 書庫・メディアのみ（フォルダ・画像は除外）
    std::wstring entryType;
    std::wstring arcTmp, entTmp;
    if (SplitArchivePath(path, arcTmp, entTmp) || IsArchiveFile(path))
        entryType = L"archive";
    else if (IsMediaFile(path))
        entryType = L"media";
    else
        return; // フォルダ・画像は記録しない

    // 表示名（パスの最後の部分）
    std::wstring name = path;
    // 書庫内パスの場合は書庫ファイル名を使う
    std::wstring arcPath, entryPath;
    if (SplitArchivePath(path, arcPath, entryPath))
        name = arcPath;
    auto slashPos = name.find_last_of(L'\\');
    if (slashPos != std::wstring::npos) name = name.substr(slashPos + 1);

    // 重複を除去（同じパスなら時刻更新のため先に削除）
    g_history.erase(
        std::remove_if(g_history.begin(), g_history.end(),
            [&](const HistoryEntry& e) { return _wcsicmp(e.path.c_str(), path.c_str()) == 0; }),
        g_history.end());

    // 先頭に追加
    HistoryEntry entry;
    entry.path = path;
    entry.name = name;
    entry.entryType = entryType;
    entry.accessTime = GetCurrentFileTime();
    g_history.insert(g_history.begin(), entry);

    if ((int)g_history.size() > kMaxHistory)
        g_history.resize(kMaxHistory);
}

void HistoryRemoveEntry(const std::wstring& path)
{
    g_history.erase(
        std::remove_if(g_history.begin(), g_history.end(),
            [&](const HistoryEntry& e) { return _wcsicmp(e.path.c_str(), path.c_str()) == 0; }),
        g_history.end());
}

const std::vector<HistoryEntry>& HistoryGetAll() { return g_history; }

void HistoryClear()
{
    g_history.clear();
    HistorySave();
}

// === JSON保存/読込（時刻付き） ===

void HistorySave()
{
    std::wstring json = L"[\n";
    for (size_t i = 0; i < g_history.size(); i++)
    {
        auto& e = g_history[i];
        ULARGE_INTEGER ul;
        ul.LowPart = e.accessTime.dwLowDateTime;
        ul.HighPart = e.accessTime.dwHighDateTime;

        json += L"  { \"path\": \"" + EscapeJsonPath(e.path) + L"\", ";
        json += L"\"name\": \"" + EscapeJsonPath(e.name) + L"\", ";
        json += L"\"type\": \"" + e.entryType + L"\", ";
        json += L"\"time\": " + std::to_wstring(ul.QuadPart) + L" }";
        json += (i + 1 < g_history.size()) ? L",\n" : L"\n";
    }
    json += L"]\n";
    WriteWStringToFile(GetHistoryPath(), json);
}

void HistoryLoad()
{
    g_history.clear();
    std::wstring json = ReadFileToWString(GetHistoryPath());
    if (json.empty()) return;

    // 簡易パース: 各 { } ブロックを読む
    size_t pos = 0;
    while (pos < json.size())
    {
        auto start = json.find(L'{', pos);
        if (start == std::wstring::npos) break;
        auto end = json.find(L'}', start);
        if (end == std::wstring::npos) break;

        std::wstring block = json.substr(start, end - start + 1);
        HistoryEntry entry;

        // path
        auto findStr = [&](const wchar_t* key) -> std::wstring {
            std::wstring search = L"\""; search += key; search += L"\"";
            auto p = block.find(search);
            if (p == std::wstring::npos) return L"";
            p = block.find(L'"', p + search.size() + 1);
            if (p == std::wstring::npos) return L"";
            p++;
            auto e2 = p;
            while (e2 < block.size() && block[e2] != L'"') { if (block[e2] == L'\\') e2++; e2++; }
            return UnescapeJsonPath(block.substr(p, e2 - p));
        };

        entry.path = findStr(L"path");
        entry.name = findStr(L"name");
        entry.entryType = findStr(L"type");

        // time（ULONGLONG）
        auto timeKey = block.find(L"\"time\"");
        if (timeKey != std::wstring::npos)
        {
            auto colon = block.find(L':', timeKey);
            if (colon != std::wstring::npos)
            {
                colon++;
                while (colon < block.size() && block[colon] == L' ') colon++;
                ULARGE_INTEGER ul;
                ul.QuadPart = _wcstoui64(&block[colon], nullptr, 10);
                entry.accessTime.dwLowDateTime = ul.LowPart;
                entry.accessTime.dwHighDateTime = ul.HighPart;
            }
        }

        // name が空なら path から生成
        if (entry.name.empty() && !entry.path.empty())
        {
            std::wstring n = entry.path;
            auto bang = n.find(L'!');
            if (bang != std::wstring::npos) n = n.substr(0, bang);
            auto slash = n.find_last_of(L'\\');
            if (slash != std::wstring::npos) n = n.substr(slash + 1);
            entry.name = n;
        }
        if (entry.entryType.empty()) entry.entryType = L"archive";

        if (!entry.path.empty())
        {
            // フォルダは履歴に含めない
            DWORD attr = GetFileAttributesW(entry.path.c_str());
            if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
                { pos = end + 1; continue; }
            g_history.push_back(std::move(entry));
        }

        pos = end + 1;
    }
}
