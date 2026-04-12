#pragma once
#include "app.h"

HWND CreateNavBar(HWND parent, HINSTANCE hInst);
HWND CreateNavBarRight(HWND parent, HINSTANCE hInst);
void NavUpdateButtons(); // 戻る/進むボタンの有効/無効を更新
void UpdateNavbarTooltips(); // ツールチップを現在の言語で更新
