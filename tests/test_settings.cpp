#include <gtest/gtest.h>

// settings.cpp の非static関数をテストするために必要なヘッダ
#include "app.h"
#include "settings.h"

// settings.cpp が参照するグローバル（test_main.cpp で定義済み）
extern AppState g_app;

// === KeyComboToString ===

TEST(KeyComboToString, SimpleArrowKey) {
    KeyCombo kc = { VK_LEFT, false, false, false };
    std::wstring result = KeyComboToString(kc);
    // VK_LEFT → "←"
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.find(L"Ctrl"), std::wstring::npos); // Ctrlなし
}

TEST(KeyComboToString, CtrlPlusC) {
    KeyCombo kc = { 'C', true, false, false };
    std::wstring result = KeyComboToString(kc);
    EXPECT_NE(result.find(L"Ctrl+"), std::wstring::npos);
    EXPECT_NE(result.find(L"C"), std::wstring::npos);
}

TEST(KeyComboToString, CtrlShiftPlusR) {
    KeyCombo kc = { 'R', true, true, false };
    std::wstring result = KeyComboToString(kc);
    EXPECT_NE(result.find(L"Ctrl+"), std::wstring::npos);
    EXPECT_NE(result.find(L"Shift+"), std::wstring::npos);
    EXPECT_NE(result.find(L"R"), std::wstring::npos);
}

TEST(KeyComboToString, AltPlusArrow) {
    KeyCombo kc = { VK_LEFT, false, false, true };
    std::wstring result = KeyComboToString(kc);
    EXPECT_NE(result.find(L"Alt+"), std::wstring::npos);
}

TEST(KeyComboToString, FunctionKey) {
    KeyCombo kc = { VK_F11, false, false, false };
    std::wstring result = KeyComboToString(kc);
    EXPECT_EQ(result, L"F11");
}

TEST(KeyComboToString, F1Key) {
    KeyCombo kc = { VK_F1, false, false, false };
    std::wstring result = KeyComboToString(kc);
    EXPECT_EQ(result, L"F1");
}

TEST(KeyComboToString, F12Key) {
    KeyCombo kc = { VK_F12, false, false, false };
    std::wstring result = KeyComboToString(kc);
    EXPECT_EQ(result, L"F12");
}

TEST(KeyComboToString, SpaceKey) {
    KeyCombo kc = { VK_SPACE, false, false, false };
    EXPECT_EQ(KeyComboToString(kc), L"Space");
}

TEST(KeyComboToString, EnterKey) {
    KeyCombo kc = { VK_RETURN, false, false, false };
    EXPECT_EQ(KeyComboToString(kc), L"Enter");
}

TEST(KeyComboToString, DeleteKey) {
    KeyCombo kc = { VK_DELETE, false, false, false };
    EXPECT_EQ(KeyComboToString(kc), L"Delete");
}

TEST(KeyComboToString, HomeKey) {
    KeyCombo kc = { VK_HOME, false, false, false };
    EXPECT_EQ(KeyComboToString(kc), L"Home");
}

TEST(KeyComboToString, EndKey) {
    KeyCombo kc = { VK_END, false, false, false };
    EXPECT_EQ(KeyComboToString(kc), L"End");
}

TEST(KeyComboToString, AllModifiers) {
    KeyCombo kc = { 'A', true, true, true };
    std::wstring result = KeyComboToString(kc);
    EXPECT_NE(result.find(L"Ctrl+"), std::wstring::npos);
    EXPECT_NE(result.find(L"Shift+"), std::wstring::npos);
    EXPECT_NE(result.find(L"Alt+"), std::wstring::npos);
    EXPECT_NE(result.find(L"A"), std::wstring::npos);
}

// === KeyBindingToString ===

TEST(KeyBindingToString, SingleKey) {
    KeyBinding kb;
    kb.action = L"test";
    kb.label = L"Test";
    kb.keys = { { VK_F5, false, false, false } };
    std::wstring result = KeyBindingToString(kb);
    EXPECT_EQ(result, L"F5");
}

TEST(KeyBindingToString, MultipleKeys) {
    KeyBinding kb;
    kb.action = L"zoom_in";
    kb.label = L"Zoom In";
    kb.keys = { { VK_OEM_PLUS, true, false, false }, { VK_ADD, true, false, false } };
    std::wstring result = KeyBindingToString(kb);
    // "Ctrl++, Ctrl+Num+" のような形式
    EXPECT_NE(result.find(L", "), std::wstring::npos);
}

TEST(KeyBindingToString, EmptyKeys) {
    KeyBinding kb;
    kb.action = L"test";
    kb.label = L"Test";
    kb.keys = {};
    EXPECT_EQ(KeyBindingToString(kb), L"");
}

// === InitDefaultKeyBindings + FindAction ===

TEST(FindAction, DefaultBindings_F11IsFullscreen) {
    InitDefaultKeyBindings();
    EXPECT_EQ(FindAction(VK_F11, false, false, false), L"fullscreen");
}

TEST(FindAction, DefaultBindings_CtrlCIsCopyImage) {
    InitDefaultKeyBindings();
    EXPECT_EQ(FindAction('C', true, false, false), L"copy_image");
}

TEST(FindAction, DefaultBindings_SpaceIsPlayPause) {
    InitDefaultKeyBindings();
    EXPECT_EQ(FindAction(VK_SPACE, false, false, false), L"play_pause");
}

TEST(FindAction, DefaultBindings_LeftArrowIsPrevPage) {
    InitDefaultKeyBindings();
    EXPECT_EQ(FindAction(VK_LEFT, false, false, false), L"prev_page");
}

TEST(FindAction, DefaultBindings_AltLeftIsNavBack) {
    InitDefaultKeyBindings();
    EXPECT_EQ(FindAction(VK_LEFT, false, false, true), L"nav_back");
}

TEST(FindAction, NoMatch) {
    InitDefaultKeyBindings();
    // Ctrl+Shift+Alt+F12 は登録されていない
    EXPECT_EQ(FindAction(VK_F12, true, true, true), L"");
}

TEST(FindAction, ModifierMismatch) {
    InitDefaultKeyBindings();
    // F11 は修飾キーなし。Ctrl+F11 は未登録
    EXPECT_EQ(FindAction(VK_F11, true, false, false), L"");
}

TEST(FindAction, DefaultBindings_F5IsRefresh) {
    InitDefaultKeyBindings();
    EXPECT_EQ(FindAction(VK_F5, false, false, false), L"refresh");
}

TEST(FindAction, DefaultBindings_F1IsHelp) {
    InitDefaultKeyBindings();
    EXPECT_EQ(FindAction(VK_F1, false, false, false), L"help");
}
