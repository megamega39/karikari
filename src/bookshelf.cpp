#include "bookshelf.h"
#include "utils.h"
#include <shlwapi.h>
#include <objbase.h>

static std::vector<BookshelfCategory> g_categories;

static std::wstring GetBookshelfPath()
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    PathRemoveFileSpecW(path);
    PathAppendW(path, L"bookshelf.json");
    return path;
}

// GUID生成
static std::wstring GenerateId()
{
    GUID guid;
    CoCreateGuid(&guid);
    wchar_t buf[64];
    swprintf_s(buf, _countof(buf),
        L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    return buf;
}

// === JSON保存/読込 ===

void BookshelfSave()
{
    std::wstring json = L"{\n  \"categories\": [\n";
    for (size_t ci = 0; ci < g_categories.size(); ci++)
    {
        auto& cat = g_categories[ci];
        json += L"    {\n";
        json += L"      \"id\": \"" + cat.id + L"\",\n";
        json += L"      \"name\": \"" + EscapeJsonPath(cat.name) + L"\",\n";
        json += L"      \"items\": [\n";
        for (size_t ii = 0; ii < cat.items.size(); ii++)
        {
            auto& item = cat.items[ii];
            json += L"        { \"name\": \"" + EscapeJsonPath(item.name) + L"\", ";
            json += L"\"path\": \"" + EscapeJsonPath(item.path) + L"\" }";
            json += (ii + 1 < cat.items.size()) ? L",\n" : L"\n";
        }
        json += L"      ]\n";
        json += L"    }";
        json += (ci + 1 < g_categories.size()) ? L",\n" : L"\n";
    }
    json += L"  ]\n}\n";
    WriteWStringToFile(GetBookshelfPath(), json);
}

// 簡易JSONパーサー（本棚専用）
static bool JsonGetStr(const std::wstring& json, const wchar_t* key, std::wstring& out, size_t startPos = 0)
{
    std::wstring search = L"\""; search += key; search += L"\"";
    auto pos = json.find(search, startPos);
    if (pos == std::wstring::npos) return false;
    pos = json.find(L'"', pos + search.size() + 1); // : の後の "
    if (pos == std::wstring::npos) return false;
    pos++;
    auto end = pos;
    while (end < json.size() && json[end] != L'"')
    {
        if (json[end] == L'\\') end++; // エスケープスキップ
        end++;
    }
    out = UnescapeJsonPath(json.substr(pos, end - pos));
    return true;
}

void BookshelfLoad()
{
    g_categories.clear();
    std::wstring json = ReadFileToWString(GetBookshelfPath());
    if (json.empty()) return;

    // "categories" 配列を探す
    auto catArrStart = json.find(L"\"categories\"");
    if (catArrStart == std::wstring::npos) return;
    auto arrStart = json.find(L'[', catArrStart);
    auto arrEnd = json.rfind(L']');
    if (arrStart == std::wstring::npos || arrEnd == std::wstring::npos) return;

    // 各カテゴリ { ... } を読む
    size_t cur = arrStart + 1;
    while (cur < arrEnd)
    {
        auto catStart = json.find(L'{', cur);
        if (catStart == std::wstring::npos || catStart >= arrEnd) break;

        // items配列の中の { } をスキップするため、対応する } を探す
        int depth = 0;
        size_t catEnd = catStart;
        for (size_t i = catStart; i <= arrEnd; i++)
        {
            if (json[i] == L'{') depth++;
            else if (json[i] == L'}') { depth--; if (depth == 0) { catEnd = i; break; } }
        }

        std::wstring catBlock = json.substr(catStart, catEnd - catStart + 1);
        BookshelfCategory cat;
        JsonGetStr(catBlock, L"id", cat.id);
        JsonGetStr(catBlock, L"name", cat.name);

        // items配列を読む
        auto itemsStart = catBlock.find(L"\"items\"");
        if (itemsStart != std::wstring::npos)
        {
            auto iArrStart = catBlock.find(L'[', itemsStart);
            auto iArrEnd = catBlock.rfind(L']');
            if (iArrStart != std::wstring::npos && iArrEnd != std::wstring::npos)
            {
                size_t ic = iArrStart + 1;
                while (ic < iArrEnd)
                {
                    auto iStart = catBlock.find(L'{', ic);
                    if (iStart == std::wstring::npos || iStart >= iArrEnd) break;
                    auto iEnd = catBlock.find(L'}', iStart);
                    if (iEnd == std::wstring::npos) break;

                    std::wstring itemBlock = catBlock.substr(iStart, iEnd - iStart + 1);
                    BookshelfItem item;
                    JsonGetStr(itemBlock, L"name", item.name);
                    JsonGetStr(itemBlock, L"path", item.path);
                    if (!item.path.empty())
                        cat.items.push_back(std::move(item));
                    ic = iEnd + 1;
                }
            }
        }

        if (!cat.id.empty())
            g_categories.push_back(std::move(cat));
        cur = catEnd + 1;
    }
}

// === CRUD操作 ===

const std::vector<BookshelfCategory>& BookshelfGetCategories() { return g_categories; }

BookshelfCategory& BookshelfAddCategory(const std::wstring& name)
{
    BookshelfCategory cat;
    cat.id = GenerateId();
    cat.name = name;
    g_categories.push_back(std::move(cat));
    BookshelfSave();
    return g_categories.back();
}

void BookshelfRemoveCategory(const std::wstring& id)
{
    g_categories.erase(
        std::remove_if(g_categories.begin(), g_categories.end(),
            [&](const BookshelfCategory& c) { return c.id == id; }),
        g_categories.end());
    BookshelfSave();
}

void BookshelfRenameCategory(const std::wstring& id, const std::wstring& newName)
{
    for (auto& cat : g_categories)
    {
        if (cat.id == id) { cat.name = newName; BookshelfSave(); return; }
    }
}

void BookshelfAddItem(const std::wstring& categoryId, const std::wstring& name, const std::wstring& path)
{
    for (auto& cat : g_categories)
    {
        if (cat.id == categoryId)
        {
            // 重複チェック
            for (auto& item : cat.items)
                if (_wcsicmp(item.path.c_str(), path.c_str()) == 0) return;
            cat.items.push_back({ name, path });
            BookshelfSave();
            return;
        }
    }
}

void BookshelfRemoveItem(const std::wstring& path)
{
    for (auto& cat : g_categories)
    {
        cat.items.erase(
            std::remove_if(cat.items.begin(), cat.items.end(),
                [&](const BookshelfItem& i) { return _wcsicmp(i.path.c_str(), path.c_str()) == 0; }),
            cat.items.end());
    }
    BookshelfSave();
}

bool BookshelfContains(const std::wstring& path)
{
    for (auto& cat : g_categories)
        for (auto& item : cat.items)
            if (_wcsicmp(item.path.c_str(), path.c_str()) == 0) return true;
    return false;
}
