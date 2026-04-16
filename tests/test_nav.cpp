#include <gtest/gtest.h>
#include "nav_utils.h"

// === ResolveNavIndex: ラップ有効 ===

TEST(ResolveNavIndex, WrapEnabled_NegativeWrapsToEnd) {
    EXPECT_EQ(ResolveNavIndex(-1, 10, true), 9);
}

TEST(ResolveNavIndex, WrapEnabled_OverMaxWrapsToStart) {
    EXPECT_EQ(ResolveNavIndex(10, 10, true), 0);
}

TEST(ResolveNavIndex, WrapEnabled_ValidIndexUnchanged) {
    EXPECT_EQ(ResolveNavIndex(5, 10, true), 5);
}

TEST(ResolveNavIndex, WrapEnabled_FirstIndex) {
    EXPECT_EQ(ResolveNavIndex(0, 10, true), 0);
}

TEST(ResolveNavIndex, WrapEnabled_LastIndex) {
    EXPECT_EQ(ResolveNavIndex(9, 10, true), 9);
}

// === ResolveNavIndex: ラップ無効 ===

TEST(ResolveNavIndex, WrapDisabled_NegativeReturnsInvalid) {
    EXPECT_EQ(ResolveNavIndex(-1, 10, false), -1);
}

TEST(ResolveNavIndex, WrapDisabled_OverMaxReturnsInvalid) {
    EXPECT_EQ(ResolveNavIndex(10, 10, false), -1);
}

TEST(ResolveNavIndex, WrapDisabled_ValidIndexUnchanged) {
    EXPECT_EQ(ResolveNavIndex(5, 10, false), 5);
}

TEST(ResolveNavIndex, WrapDisabled_FirstIndex) {
    EXPECT_EQ(ResolveNavIndex(0, 10, false), 0);
}

TEST(ResolveNavIndex, WrapDisabled_LastIndex) {
    EXPECT_EQ(ResolveNavIndex(9, 10, false), 9);
}

// === エッジケース ===

TEST(ResolveNavIndex, TotalZero) {
    EXPECT_EQ(ResolveNavIndex(0, 0, true), -1);
    EXPECT_EQ(ResolveNavIndex(0, 0, false), -1);
}

TEST(ResolveNavIndex, TotalOne_WrapEnabled) {
    EXPECT_EQ(ResolveNavIndex(0, 1, true), 0);
    EXPECT_EQ(ResolveNavIndex(-1, 1, true), 0);
    EXPECT_EQ(ResolveNavIndex(1, 1, true), 0);
}

TEST(ResolveNavIndex, TotalOne_WrapDisabled) {
    EXPECT_EQ(ResolveNavIndex(0, 1, false), 0);
    EXPECT_EQ(ResolveNavIndex(-1, 1, false), -1);
    EXPECT_EQ(ResolveNavIndex(1, 1, false), -1);
}

TEST(ResolveNavIndex, NegativeTotal) {
    EXPECT_EQ(ResolveNavIndex(0, -1, true), -1);
    EXPECT_EQ(ResolveNavIndex(0, -1, false), -1);
}
