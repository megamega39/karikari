#include <gtest/gtest.h>
#include "utils.h"

// === EscapeJsonPath ===

TEST(EscapeJsonPath, BackslashDoubled) {
    EXPECT_EQ(EscapeJsonPath(L"C:\\Users\\file"), L"C:\\\\Users\\\\file");
}

TEST(EscapeJsonPath, QuoteEscaped) {
    EXPECT_EQ(EscapeJsonPath(L"he said \"hi\""), L"he said \\\"hi\\\"");
}

TEST(EscapeJsonPath, NoSpecialChars) {
    EXPECT_EQ(EscapeJsonPath(L"hello"), L"hello");
}

TEST(EscapeJsonPath, EmptyString) {
    EXPECT_EQ(EscapeJsonPath(L""), L"");
}

TEST(EscapeJsonPath, MixedBackslashAndQuote) {
    EXPECT_EQ(EscapeJsonPath(L"C:\\\"test\""), L"C:\\\\\\\"test\\\"");
}

// === UnescapeJsonPath ===

TEST(UnescapeJsonPath, DoubleBackslash) {
    EXPECT_EQ(UnescapeJsonPath(L"C:\\\\Users"), L"C:\\Users");
}

TEST(UnescapeJsonPath, EscapedQuote) {
    EXPECT_EQ(UnescapeJsonPath(L"he said \\\"hi\\\""), L"he said \"hi\"");
}

TEST(UnescapeJsonPath, NoEscapes) {
    EXPECT_EQ(UnescapeJsonPath(L"hello"), L"hello");
}

TEST(UnescapeJsonPath, EmptyString) {
    EXPECT_EQ(UnescapeJsonPath(L""), L"");
}

TEST(UnescapeJsonPath, UnknownEscape) {
    // \a は認識されないエスケープ → バックスラッシュだけ残る
    EXPECT_EQ(UnescapeJsonPath(L"\\a"), L"\\a");
}

// === ラウンドトリップ ===

TEST(JsonPathRoundTrip, WindowsPath) {
    std::wstring original = L"C:\\Users\\Admin\\Documents\\file.txt";
    EXPECT_EQ(UnescapeJsonPath(EscapeJsonPath(original)), original);
}

TEST(JsonPathRoundTrip, PathWithQuotes) {
    std::wstring original = L"C:\\My \"Files\"\\test";
    EXPECT_EQ(UnescapeJsonPath(EscapeJsonPath(original)), original);
}

TEST(JsonPathRoundTrip, EmptyString) {
    EXPECT_EQ(UnescapeJsonPath(EscapeJsonPath(L"")), L"");
}

TEST(JsonPathRoundTrip, JapanesePath) {
    std::wstring original = L"C:\\\x753B\x50CF\\\x30C6\x30B9\x30C8.jpg"; // C:\画像\テスト.jpg
    EXPECT_EQ(UnescapeJsonPath(EscapeJsonPath(original)), original);
}

// === JsonGetInt ===

TEST(JsonGetInt, BasicValue) {
    int val = 0;
    EXPECT_TRUE(JsonGetInt(L"{\"count\": 42}", L"count", val));
    EXPECT_EQ(val, 42);
}

TEST(JsonGetInt, NegativeValue) {
    int val = 0;
    EXPECT_TRUE(JsonGetInt(L"{\"offset\": -10}", L"offset", val));
    EXPECT_EQ(val, -10);
}

TEST(JsonGetInt, Zero) {
    int val = 999;
    EXPECT_TRUE(JsonGetInt(L"{\"zero\": 0}", L"zero", val));
    EXPECT_EQ(val, 0);
}

TEST(JsonGetInt, KeyNotFound) {
    int val = 123;
    EXPECT_FALSE(JsonGetInt(L"{\"other\": 42}", L"count", val));
    EXPECT_EQ(val, 123); // 変更されていないこと
}

TEST(JsonGetInt, WithWhitespace) {
    int val = 0;
    EXPECT_TRUE(JsonGetInt(L"{\"count\" :   42}", L"count", val));
    EXPECT_EQ(val, 42);
}

TEST(JsonGetInt, InMultiLineJson) {
    std::wstring json = L"{\n  \"a\": 1,\n  \"b\": 2,\n  \"c\": 3\n}";
    int val = 0;
    EXPECT_TRUE(JsonGetInt(json, L"b", val));
    EXPECT_EQ(val, 2);
}

// === JsonGetBool ===

TEST(JsonGetBool, TrueValue) {
    bool val = false;
    EXPECT_TRUE(JsonGetBool(L"{\"enabled\": true}", L"enabled", val));
    EXPECT_TRUE(val);
}

TEST(JsonGetBool, FalseValue) {
    bool val = true;
    EXPECT_TRUE(JsonGetBool(L"{\"enabled\": false}", L"enabled", val));
    EXPECT_FALSE(val);
}

TEST(JsonGetBool, KeyNotFound) {
    bool val = true;
    EXPECT_FALSE(JsonGetBool(L"{\"other\": true}", L"enabled", val));
    EXPECT_TRUE(val); // 変更されていないこと
}

TEST(JsonGetBool, WithWhitespace) {
    bool val = false;
    EXPECT_TRUE(JsonGetBool(L"{\"flag\" : true}", L"flag", val));
    EXPECT_TRUE(val);
}

// === JsonGetString ===

TEST(JsonGetString, BasicValue) {
    std::wstring val;
    EXPECT_TRUE(JsonGetString(L"{\"name\": \"hello\"}", L"name", val));
    EXPECT_EQ(val, L"hello");
}

TEST(JsonGetString, EscapedPath) {
    std::wstring val;
    EXPECT_TRUE(JsonGetString(L"{\"path\": \"C:\\\\Users\\\\file\"}", L"path", val));
    EXPECT_EQ(val, L"C:\\Users\\file");
}

TEST(JsonGetString, EmptyStringValue) {
    std::wstring val = L"default";
    EXPECT_TRUE(JsonGetString(L"{\"name\": \"\"}", L"name", val));
    EXPECT_EQ(val, L"");
}

TEST(JsonGetString, KeyNotFound) {
    std::wstring val = L"default";
    EXPECT_FALSE(JsonGetString(L"{\"other\": \"value\"}", L"name", val));
    EXPECT_EQ(val, L"default");
}

TEST(JsonGetString, WithWhitespace) {
    std::wstring val;
    EXPECT_TRUE(JsonGetString(L"{\"name\" : \"test\"}", L"name", val));
    EXPECT_EQ(val, L"test");
}

// === JsonGetFloat ===

TEST(JsonGetFloat, DecimalValue) {
    float val = 0.0f;
    EXPECT_TRUE(JsonGetFloat(L"{\"ratio\": 1.5}", L"ratio", val));
    EXPECT_FLOAT_EQ(val, 1.5f);
}

TEST(JsonGetFloat, IntegerAsFloat) {
    float val = 0.0f;
    EXPECT_TRUE(JsonGetFloat(L"{\"ratio\": 42}", L"ratio", val));
    EXPECT_FLOAT_EQ(val, 42.0f);
}

TEST(JsonGetFloat, KeyNotFound) {
    float val = 1.0f;
    EXPECT_FALSE(JsonGetFloat(L"{\"other\": 2.0}", L"ratio", val));
    EXPECT_FLOAT_EQ(val, 1.0f);
}

TEST(JsonGetFloat, NegativeFloat) {
    float val = 0.0f;
    EXPECT_TRUE(JsonGetFloat(L"{\"offset\": -0.5}", L"offset", val));
    EXPECT_FLOAT_EQ(val, -0.5f);
}

TEST(JsonGetFloat, ZeroFloat) {
    float val = 1.0f;
    EXPECT_TRUE(JsonGetFloat(L"{\"val\": 0.0}", L"val", val));
    EXPECT_FLOAT_EQ(val, 0.0f);
}

// === JsonGetIntArray ===

TEST(JsonGetIntArray, MultipleElements) {
    std::vector<int> val;
    EXPECT_TRUE(JsonGetIntArray(L"{\"arr\": [1, 2, 3]}", L"arr", val));
    EXPECT_EQ(val.size(), 3u);
    EXPECT_EQ(val[0], 1);
    EXPECT_EQ(val[1], 2);
    EXPECT_EQ(val[2], 3);
}

TEST(JsonGetIntArray, SingleElement) {
    std::vector<int> val;
    EXPECT_TRUE(JsonGetIntArray(L"{\"arr\": [42]}", L"arr", val));
    EXPECT_EQ(val.size(), 1u);
    EXPECT_EQ(val[0], 42);
}

TEST(JsonGetIntArray, EmptyArrayReturnsFalse) {
    std::vector<int> val;
    EXPECT_FALSE(JsonGetIntArray(L"{\"arr\": []}", L"arr", val));
}

TEST(JsonGetIntArray, KeyNotFound) {
    std::vector<int> val;
    EXPECT_FALSE(JsonGetIntArray(L"{\"other\": [1]}", L"arr", val));
}

TEST(JsonGetIntArray, WithWhitespace) {
    std::vector<int> val;
    EXPECT_TRUE(JsonGetIntArray(L"{\"arr\": [ 10 , 20 , 30 ]}", L"arr", val));
    EXPECT_EQ(val.size(), 3u);
    EXPECT_EQ(val[0], 10);
    EXPECT_EQ(val[1], 20);
    EXPECT_EQ(val[2], 30);
}

TEST(JsonGetIntArray, NegativeNumbers) {
    std::vector<int> val;
    EXPECT_TRUE(JsonGetIntArray(L"{\"arr\": [-1, -2, -3]}", L"arr", val));
    EXPECT_EQ(val.size(), 3u);
    EXPECT_EQ(val[0], -1);
    EXPECT_EQ(val[1], -2);
    EXPECT_EQ(val[2], -3);
}
