#pragma once
#include <string>

void I18nInit(const std::wstring& lang = L"ja");
void I18nSetLang(const std::wstring& lang);
const std::wstring& I18nGet(const std::wstring& key);
const std::wstring& I18nGetLang();
