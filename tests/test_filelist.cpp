#include <gtest/gtest.h>
#include "file_utils.h"

// === FormatFileSizeUtil ===

TEST(FormatFileSize, ZeroBytes) {
    EXPECT_EQ(FormatFileSizeUtil(0), L"0 B");
}

TEST(FormatFileSize, OneByte) {
    EXPECT_EQ(FormatFileSizeUtil(1), L"1 B");
}

TEST(FormatFileSize, BytesBoundary) {
    EXPECT_EQ(FormatFileSizeUtil(1023), L"1023 B");
}

TEST(FormatFileSize, ExactlyOneKB) {
    EXPECT_EQ(FormatFileSizeUtil(1024), L"1.0 KB");
}

TEST(FormatFileSize, OneAndHalfKB) {
    EXPECT_EQ(FormatFileSizeUtil(1536), L"1.5 KB");
}

TEST(FormatFileSize, KBBoundary) {
    EXPECT_EQ(FormatFileSizeUtil(1024 * 1024 - 1), L"1024.0 KB");
}

TEST(FormatFileSize, ExactlyOneMB) {
    EXPECT_EQ(FormatFileSizeUtil(1024 * 1024), L"1.0 MB");
}

TEST(FormatFileSize, LargeMB) {
    EXPECT_EQ(FormatFileSizeUtil(500ULL * 1024 * 1024), L"500.0 MB");
}

TEST(FormatFileSize, ExactlyOneGB) {
    EXPECT_EQ(FormatFileSizeUtil(1024ULL * 1024 * 1024), L"1.0 GB");
}

TEST(FormatFileSize, LargeGB) {
    EXPECT_EQ(FormatFileSizeUtil(4ULL * 1024 * 1024 * 1024), L"4.0 GB");
}

// === IsImageFileCheck ===

TEST(IsImageFile, JpgExtension) {
    EXPECT_TRUE(IsImageFileCheck(L"photo.jpg"));
}

TEST(IsImageFile, JpegExtension) {
    EXPECT_TRUE(IsImageFileCheck(L"photo.jpeg"));
}

TEST(IsImageFile, PngExtension) {
    EXPECT_TRUE(IsImageFileCheck(L"image.png"));
}

TEST(IsImageFile, WebpExtension) {
    EXPECT_TRUE(IsImageFileCheck(L"image.webp"));
}

TEST(IsImageFile, AvifExtension) {
    EXPECT_TRUE(IsImageFileCheck(L"image.avif"));
}

TEST(IsImageFile, GifExtension) {
    EXPECT_TRUE(IsImageFileCheck(L"animation.gif"));
}

TEST(IsImageFile, BmpExtension) {
    EXPECT_TRUE(IsImageFileCheck(L"image.bmp"));
}

TEST(IsImageFile, TiffExtension) {
    EXPECT_TRUE(IsImageFileCheck(L"image.tiff"));
}

TEST(IsImageFile, TifExtension) {
    EXPECT_TRUE(IsImageFileCheck(L"image.tif"));
}

TEST(IsImageFile, IcoExtension) {
    EXPECT_TRUE(IsImageFileCheck(L"icon.ico"));
}

TEST(IsImageFile, CaseInsensitive) {
    EXPECT_TRUE(IsImageFileCheck(L"photo.JPG"));
    EXPECT_TRUE(IsImageFileCheck(L"image.PNG"));
    EXPECT_TRUE(IsImageFileCheck(L"file.WebP"));
}

TEST(IsImageFile, NotAnImage) {
    EXPECT_FALSE(IsImageFileCheck(L"document.txt"));
    EXPECT_FALSE(IsImageFileCheck(L"archive.zip"));
    EXPECT_FALSE(IsImageFileCheck(L"video.mp4"));
}

TEST(IsImageFile, NoExtension) {
    EXPECT_FALSE(IsImageFileCheck(L"README"));
}

TEST(IsImageFile, EmptyString) {
    EXPECT_FALSE(IsImageFileCheck(L""));
}

TEST(IsImageFile, FullPath) {
    EXPECT_TRUE(IsImageFileCheck(L"C:\\photos\\vacation\\beach.jpg"));
}

// === IsArchiveFileCheck ===

TEST(IsArchiveFile, ZipExtension) {
    EXPECT_TRUE(IsArchiveFileCheck(L"file.zip"));
}

TEST(IsArchiveFile, SevenZipExtension) {
    EXPECT_TRUE(IsArchiveFileCheck(L"file.7z"));
}

TEST(IsArchiveFile, RarExtension) {
    EXPECT_TRUE(IsArchiveFileCheck(L"file.rar"));
}

TEST(IsArchiveFile, CbzExtension) {
    EXPECT_TRUE(IsArchiveFileCheck(L"comic.cbz"));
}

TEST(IsArchiveFile, CbrExtension) {
    EXPECT_TRUE(IsArchiveFileCheck(L"comic.cbr"));
}

TEST(IsArchiveFile, Cb7Extension) {
    EXPECT_TRUE(IsArchiveFileCheck(L"comic.cb7"));
}

TEST(IsArchiveFile, CaseInsensitive) {
    EXPECT_TRUE(IsArchiveFileCheck(L"file.ZIP"));
    EXPECT_TRUE(IsArchiveFileCheck(L"file.RAR"));
}

TEST(IsArchiveFile, NotAnArchive) {
    EXPECT_FALSE(IsArchiveFileCheck(L"image.jpg"));
    EXPECT_FALSE(IsArchiveFileCheck(L"document.pdf"));
}
