#pragma once
#include "app.h"

struct BookshelfItem {
    std::wstring name;
    std::wstring path;
};

struct BookshelfCategory {
    std::wstring id;
    std::wstring name;
    std::vector<BookshelfItem> items;
};

void BookshelfLoad();
void BookshelfSave();
BookshelfCategory& BookshelfAddCategory(const std::wstring& name);
void BookshelfRemoveCategory(const std::wstring& id);
void BookshelfRenameCategory(const std::wstring& id, const std::wstring& newName);
void BookshelfAddItem(const std::wstring& categoryId, const std::wstring& name, const std::wstring& path);
void BookshelfRemoveItem(const std::wstring& path);
bool BookshelfContains(const std::wstring& path);
const std::vector<BookshelfCategory>& BookshelfGetCategories();
