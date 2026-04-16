#include <gtest/gtest.h>
#include "archive_utils.h"

// === IsValidEntryPath: セキュリティ検証 ===

// 正常パス
TEST(IsValidEntryPath, NormalPath) {
    EXPECT_TRUE(IsValidEntryPath(L"images\\page1.jpg"));
}

TEST(IsValidEntryPath, NestedPath) {
    EXPECT_TRUE(IsValidEntryPath(L"folder1\\folder2\\page.jpg"));
}

TEST(IsValidEntryPath, ForwardSlashPath) {
    EXPECT_TRUE(IsValidEntryPath(L"images/page1.jpg"));
}

TEST(IsValidEntryPath, SimpleFilename) {
    EXPECT_TRUE(IsValidEntryPath(L"page1.jpg"));
}

TEST(IsValidEntryPath, DoubleDotInFilename) {
    // ファイル名に".."を含むが、パストラバーサルではない
    EXPECT_TRUE(IsValidEntryPath(L"file..name.txt"));
}

TEST(IsValidEntryPath, TripleDotInPath) {
    // "...\file.txt" は "..\\" を部分文字列として含むため拒否される
    EXPECT_FALSE(IsValidEntryPath(L"...\\file.txt"));
}

TEST(IsValidEntryPath, TripleDotForwardSlash) {
    // ".../file.txt" も "../" を含むため拒否される
    EXPECT_FALSE(IsValidEntryPath(L".../file.txt"));
}

TEST(IsValidEntryPath, DotsInMiddle) {
    // ファイル名にドットが多いがトラバーサルではない
    EXPECT_TRUE(IsValidEntryPath(L"folder\\file...name.txt"));
}

// パストラバーサル攻撃
TEST(IsValidEntryPath, BackslashDotDot) {
    EXPECT_FALSE(IsValidEntryPath(L"..\\secret.txt"));
}

TEST(IsValidEntryPath, ForwardSlashDotDot) {
    EXPECT_FALSE(IsValidEntryPath(L"../secret.txt"));
}

TEST(IsValidEntryPath, BareDotDot) {
    EXPECT_FALSE(IsValidEntryPath(L".."));
}

TEST(IsValidEntryPath, TrailingBackslashDotDot) {
    EXPECT_FALSE(IsValidEntryPath(L"folder\\.."));
}

TEST(IsValidEntryPath, TrailingForwardSlashDotDot) {
    EXPECT_FALSE(IsValidEntryPath(L"folder/.."));
}

TEST(IsValidEntryPath, MiddleDotDotTraversal) {
    EXPECT_FALSE(IsValidEntryPath(L"images\\..\\secret.txt"));
}

TEST(IsValidEntryPath, MiddleForwardSlashTraversal) {
    EXPECT_FALSE(IsValidEntryPath(L"images/../secret.txt"));
}

// 絶対パス
TEST(IsValidEntryPath, AbsolutePathWithDrive) {
    EXPECT_FALSE(IsValidEntryPath(L"C:\\windows\\system32"));
}

TEST(IsValidEntryPath, LeadingBackslash) {
    EXPECT_FALSE(IsValidEntryPath(L"\\secret.txt"));
}

TEST(IsValidEntryPath, LeadingForwardSlash) {
    EXPECT_FALSE(IsValidEntryPath(L"/secret.txt"));
}

TEST(IsValidEntryPath, DriveLetter) {
    EXPECT_FALSE(IsValidEntryPath(L"D:\\file"));
}

// === NormalizeArchivePath ===

TEST(NormalizeArchivePath, ForwardToBackslash) {
    EXPECT_EQ(NormalizeArchivePath(L"a/b/c"), L"a\\b\\c");
}

TEST(NormalizeArchivePath, UpperToLower) {
    EXPECT_EQ(NormalizeArchivePath(L"ABC"), L"abc");
}

TEST(NormalizeArchivePath, MixedSlashAndCase) {
    EXPECT_EQ(NormalizeArchivePath(L"Folder/SubDir\\FILE.TXT"), L"folder\\subdir\\file.txt");
}

TEST(NormalizeArchivePath, EmptyString) {
    EXPECT_EQ(NormalizeArchivePath(L""), L"");
}

TEST(NormalizeArchivePath, AlreadyNormalized) {
    EXPECT_EQ(NormalizeArchivePath(L"folder\\file.txt"), L"folder\\file.txt");
}

TEST(NormalizeArchivePath, Idempotent) {
    std::wstring path = L"Folder/FILE.TXT";
    std::wstring normalized = NormalizeArchivePath(path);
    EXPECT_EQ(NormalizeArchivePath(normalized), normalized);
}
