#include <gtest/gtest.h>
#include "utils.h"

// === EnsureTrailingSlash ===

TEST(EnsureTrailingSlash, AddsSlashWhenMissing) {
    EXPECT_EQ(EnsureTrailingSlash(L"C:\\folder"), L"C:\\folder\\");
}

TEST(EnsureTrailingSlash, NoChangeWhenPresent) {
    EXPECT_EQ(EnsureTrailingSlash(L"C:\\folder\\"), L"C:\\folder\\");
}

TEST(EnsureTrailingSlash, EmptyString) {
    EXPECT_EQ(EnsureTrailingSlash(L""), L"");
}

TEST(EnsureTrailingSlash, RootPath) {
    EXPECT_EQ(EnsureTrailingSlash(L"C:\\"), L"C:\\");
}

TEST(EnsureTrailingSlash, DriveOnly) {
    EXPECT_EQ(EnsureTrailingSlash(L"C:"), L"C:\\");
}

// === StripTrailingSlash ===

TEST(StripTrailingSlash, RemovesSlash) {
    EXPECT_EQ(StripTrailingSlash(L"C:\\folder\\"), L"C:\\folder");
}

TEST(StripTrailingSlash, NoChangeWithoutSlash) {
    EXPECT_EQ(StripTrailingSlash(L"C:\\folder"), L"C:\\folder");
}

TEST(StripTrailingSlash, EmptyString) {
    EXPECT_EQ(StripTrailingSlash(L""), L"");
}

TEST(StripTrailingSlash, RootPathStripsToC) {
    EXPECT_EQ(StripTrailingSlash(L"C:\\"), L"C:");
}

// === PathBaseName ===

TEST(PathBaseName, NormalPath) {
    EXPECT_EQ(PathBaseName(L"C:\\folder\\file.jpg"), L"file.jpg");
}

TEST(PathBaseName, NoBackslash) {
    EXPECT_EQ(PathBaseName(L"file.jpg"), L"file.jpg");
}

TEST(PathBaseName, TrailingBackslash) {
    EXPECT_EQ(PathBaseName(L"C:\\folder\\"), L"");
}

TEST(PathBaseName, EmptyString) {
    EXPECT_EQ(PathBaseName(L""), L"");
}

TEST(PathBaseName, DeepPath) {
    EXPECT_EQ(PathBaseName(L"C:\\a\\b\\c\\d\\file.txt"), L"file.txt");
}

// === ToLowerW ===

TEST(ToLowerW, UppercaseAscii) {
    EXPECT_EQ(ToLowerW(L"HELLO"), L"hello");
}

TEST(ToLowerW, AlreadyLowercase) {
    EXPECT_EQ(ToLowerW(L"hello"), L"hello");
}

TEST(ToLowerW, MixedCase) {
    EXPECT_EQ(ToLowerW(L"HeLLo WoRLD"), L"hello world");
}

TEST(ToLowerW, EmptyString) {
    EXPECT_EQ(ToLowerW(L""), L"");
}

TEST(ToLowerW, NumbersAndSymbols) {
    EXPECT_EQ(ToLowerW(L"FILE123.TXT"), L"file123.txt");
}

TEST(ToLowerW, JapanesePassThrough) {
    EXPECT_EQ(ToLowerW(L"\x3042\x3044\x3046"), L"\x3042\x3044\x3046"); // あいう
}

// === HasExtension ===

TEST(HasExtension, MatchingExtension) {
    const wchar_t* exts[] = { L".jpg", L".png", L".gif" };
    EXPECT_TRUE(HasExtension(L"image.jpg", exts));
}

TEST(HasExtension, CaseInsensitive) {
    const wchar_t* exts[] = { L".jpg", L".png" };
    EXPECT_TRUE(HasExtension(L"image.JPG", exts));
    EXPECT_TRUE(HasExtension(L"image.Jpg", exts));
}

TEST(HasExtension, NoMatch) {
    const wchar_t* exts[] = { L".jpg", L".png" };
    EXPECT_FALSE(HasExtension(L"document.txt", exts));
}

TEST(HasExtension, NoExtension) {
    const wchar_t* exts[] = { L".jpg" };
    EXPECT_FALSE(HasExtension(L"README", exts));
}

TEST(HasExtension, EmptyPath) {
    const wchar_t* exts[] = { L".jpg" };
    EXPECT_FALSE(HasExtension(L"", exts));
}

TEST(HasExtension, FullPathWithExtension) {
    const wchar_t* exts[] = { L".jpg", L".png" };
    EXPECT_TRUE(HasExtension(L"C:\\photos\\vacation\\beach.png", exts));
}

TEST(HasExtension, DotOnly) {
    const wchar_t* exts[] = { L".jpg" };
    EXPECT_FALSE(HasExtension(L"file.", exts));
}
