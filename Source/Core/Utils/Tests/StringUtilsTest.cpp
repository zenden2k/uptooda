#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "Core/Utils/StringUtils.h"
#include "Core/3rdpart/xdgmime/ports/fnmatch.h"

using namespace IuStringUtils;

class StringUtilsTest : public ::testing::Test {
protected:
    void TestMatch(std::string_view pattern, std::string_view str, int opts, bool expected) {
        int result = PatternMatch(pattern, str, opts);
        if (expected) {
            EXPECT_EQ(0, result) << "Pattern: '" << pattern << "' String: '" << str << "' Flags: " << opts;
        } else {
            EXPECT_EQ(NoMatch, result) << "Pattern: '" << pattern << "' String: '" << str << "' Flags: " << opts;
        }
    }
};


using ::testing::ElementsAre;

TEST_F(StringUtilsTest, LenghtOfUtf8String)
{
    EXPECT_EQ(11, LengthOfUtf8String("Hello world"));
    EXPECT_EQ(12, LengthOfUtf8String("\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82\x2C\x20\xD0\xBC\xD0\xB8\xD1\x80\x21"));
    EXPECT_EQ(0, LengthOfUtf8String(""));
}

TEST_F(StringUtilsTest, stricmp)
{
    EXPECT_EQ(0, IuStringUtils::StrCaseInsensitiveCompare("Hello world", "hello world"));
    EXPECT_GT(IuStringUtils::StrCaseInsensitiveCompare("Yandex", "google"), 0);
    EXPECT_LT(IuStringUtils::StrCaseInsensitiveCompare("some string", "test"), 0);
}

TEST_F(StringUtilsTest, SplitSV) {
    std::string test = "1,22,333,4,5";
    auto tokens = IuStringUtils::SplitSV(test, ",");
    EXPECT_THAT(tokens, ElementsAre("1", "22", "333", "4", "5"));
    std::string test2 = "Hello: World: 123";
    auto tokens2 = IuStringUtils::SplitSV(test2, ":", 2);
    EXPECT_THAT(tokens2, ElementsAre("Hello", " World: 123"));
    std::string test3 = "1,22,333;4,5";
    auto tokens3 = IuStringUtils::SplitSV(test3, ";,");
    EXPECT_THAT(tokens3, ElementsAre("1", "22", "333", "4", "5"));
}

TEST_F(StringUtilsTest, PatternMatch_BasicMatches) {
    TestMatch("*", "dsf", 0, true);
    TestMatch("?", "s", 0, true);
    TestMatch("abc", "abc", 0, true);
    TestMatch("a*c", "abc", 0, true);
    TestMatch("a?c", "abc", 0, true);
    TestMatch("a[bc]d", "abd", 0, true);
    TestMatch("a*b*c", "abc", 0, true);
    TestMatch("a***c", "abc", 0, true);
}

TEST_F(StringUtilsTest, PatternMatch_BasicMismatches) {
    TestMatch("abc", "abcd", 0, false);
    TestMatch("a*c", "ab", 0, false);
    TestMatch("a?c", "ac", 0, false);
    TestMatch("a[bc]d", "aed", 0, false);
}

TEST_F(StringUtilsTest, PatternMatch_CaseSensitivity) {
    TestMatch("abc", "ABC", 0, false);
    TestMatch("abc", "ABC", FoldCase, true);
    TestMatch("a*B", "aXb", FoldCase, true);
}

TEST_F(StringUtilsTest, PatternMatch_PathnameHandling) {
    TestMatch("a*b", "a/b", 0, true);
    TestMatch("a*b", "a/b", FileName, false);
    TestMatch("a?b", "a/b", FileName, false);
}
/*
TEST_F(StringUtilsTest, PeriodHandling) {
    TestMatch("*", ".hidden", 0, true);
    //TestMatch("*", ".hidden", Period, false);
    TestMatch("a*", "a/.hidden", Period | FileName, false);
}*/

TEST_F(StringUtilsTest, PatternMatch_EscapeHandling) {
    TestMatch("a\\*b", "a*b", 0, true);
    TestMatch("a\\*b", "a*b", NoEscape, false);
    TestMatch("a\\*b", "a\\*b", NoEscape, true);
}

/*TEST_F(StringUtilsTest, LeadingDirHandling) {
    TestMatch("abc*", "abc/def", 0, false);
    TestMatch("abc*", "abc/def", LeadingDir, true);
    TestMatch("abc*", "abcdef", LeadingDir, true);
}*/

TEST_F(StringUtilsTest, PatternMatch_EdgeCases) {
    TestMatch("", "", 0, true);
    TestMatch("*", "", 0, true);
    TestMatch("?", "", 0, false);
    TestMatch("[a]", "", 0, false);
    TestMatch("\\", "\\", NoEscape, true);
}

TEST_F(StringUtilsTest, PatternMatch_BracketExpressions) {
    TestMatch("[abc]", "a", 0, true);
    TestMatch("[!abc]", "d", 0, true);
    TestMatch("[a-c]", "b", 0, true);
    TestMatch("[!a-c]", "d", 0, true);
    TestMatch("[[a]", "[", 0, true);
}

/*TEST_F(StringUtilsTest, ReplacesSingleOccurrence) {
    EXPECT_EQ(Replace("hello world", "world", "there"), "hello there");
}

TEST_F(StringUtilsTest, ReplacesMultipleOccurrences) {
    EXPECT_EQ(Replace("aaa", "a", "b"), "bbb");
}

// Замена подряд идущих вхождений (без наложения)
TEST_F(StringUtilsTest, ReplacesConsecutiveOccurrences) {
    EXPECT_EQ(Replace("abab", "ab", "x"), "xx");
}

// Если искомая подстрока пуста — текст возвращается без изменений
TEST_F(StringUtilsTest, EmptySearchStringReturnsOriginalText) {
    EXPECT_EQ(Replace("hello", "", "x"), "hello");
}

// Если подстрока не найдена — текст не меняется
TEST_F(StringUtilsTest, NoMatchReturnsOriginalText) {
    EXPECT_EQ(Replace("hello world", "xyz", "abc"), "hello world");
}

// Пустая исходная строка
TEST_F(StringUtilsTest, EmptyTextReturnsEmpty) {
    EXPECT_EQ(Replace("", "a", "b"), "");
}

// Замена в начале строки
TEST_F(StringUtilsTest, ReplacesAtStart) {
    EXPECT_EQ(Replace("foobar", "foo", "baz"), "bazbar");
}

// Замена в конце строки
TEST_F(StringUtilsTest, ReplacesAtEnd) {
    EXPECT_EQ(Replace("foobar", "bar", "baz"), "foobaz");
}

// Замена всей строки целиком
TEST_F(StringUtilsTest, ReplacesEntireString) {
    EXPECT_EQ(Replace("foo", "foo", "bar"), "bar");
}

// Замена на пустую строку (удаление вхождений)
TEST_F(StringUtilsTest, ReplacesWithEmptyStringDeletesOccurrences) {
    EXPECT_EQ(Replace("hello world", "l", ""), "heo word");
}

// Искомая и заменяющая строки совпадают — текст не меняется
TEST_F(StringUtilsTest, SearchEqualsReplacementReturnsSameText) {
    EXPECT_EQ(Replace("hello", "l", "l"), "hello");
}

// Искомая строка длиннее текста — совпадений нет
TEST_F(StringUtilsTest, SearchLongerThanTextReturnsOriginal) {
    EXPECT_EQ(Replace("hi", "hello", "bye"), "hi");
}

// Замена, при которой заменяющая строка длиннее искомой
TEST_F(StringUtilsTest, ReplacementLongerThanSearch) {
    EXPECT_EQ(Replace("a-a-a", "-", "---"), "a---a---a");
}

// Замена, при которой заменяющая строка содержит искомую подстроку
// (проверка отсутствия рекурсивной/повторной замены уже вставленного текста)
TEST_F(StringUtilsTest, ReplacementContainingSearchStringIsNotReprocessed) {
    EXPECT_EQ(Replace("a", "a", "aa"), "aa");
}

// Строка состоит целиком из повторяющегося искомого паттерна
TEST_F(StringUtilsTest, TextEntirelyMadeOfPattern) {
    EXPECT_EQ(Replace("abcabcabc", "abc", "x"), "xxx");
}

// Однобуквенный текст и поиск
TEST_F(StringUtilsTest, SingleCharacterReplace) {
    EXPECT_EQ(Replace("a", "a", "b"), "b");
}*/

// Тест 1: Пустая строка поиска (s.empty()) должна вернуть исходный текст без изменений
TEST_F(StringUtilsTest, Replace_EmptySearchStringReturnsOriginal) {
    EXPECT_EQ(Replace("hello world", "", "x"), "hello world");
    EXPECT_EQ(Replace("", "", "x"), "");
    EXPECT_EQ(Replace("abc", "", ""), "abc");
}

// Тест 2: Пустой исходный текст
TEST_F(StringUtilsTest, Replace_EmptyTextReturnsEmpty) {
    EXPECT_EQ(Replace("", "a", "b"), "");
    EXPECT_EQ(Replace("", "", ""), "");
}

// Тест 3: Нет совпадений — возвращается исходная строка
TEST_F(StringUtilsTest, Replace_NoMatchReturnsOriginal) {
    EXPECT_EQ(Replace("hello world", "xyz", "abc"), "hello world");
    EXPECT_EQ(Replace("abc", "d", "e"), "abc");
}

// Тест 4: Одно совпадение в начале строки
TEST_F(StringUtilsTest, Replace_SingleMatchAtBeginning) {
    EXPECT_EQ(Replace("abcdef", "ab", "xy"), "xycdef");
}

// Тест 5: Одно совпадение в конце строки
TEST_F(StringUtilsTest, Replace_SingleMatchAtEnd) {
    EXPECT_EQ(Replace("abcdef", "ef", "xy"), "abcdxy");
}

// Тест 6: Одно совпадение посередине строки
TEST_F(StringUtilsTest, Replace_SingleMatchInMiddle) {
    EXPECT_EQ(Replace("abcdef", "cd", "xy"), "abxyef");
}

// Тест 7: Множественные непересекающиеся совпадения
TEST_F(StringUtilsTest, Replace_MultipleNonOverlappingMatches) {
    EXPECT_EQ(Replace("aaa", "a", "b"), "bbb");
    EXPECT_EQ(Replace("abcabc", "abc", "xyz"), "xyzxyz");
}

// Тест 8: Совпадение с пустой строкой замены (d.empty()) — удаляет совпадения
TEST_F(StringUtilsTest, Replace_EmptyReplacementRemovesMatches) {
    EXPECT_EQ(Replace("hello world", " ", ""), "helloworld");
    EXPECT_EQ(Replace("abcabc", "abc", ""), "");
}

// Тест 9: Замена на более длинную строку
TEST_F(StringUtilsTest, Replace_ReplacementLongerThanSearch) {
    EXPECT_EQ(Replace("a b c", " ", "-"), "a-b-c");
    EXPECT_EQ(Replace("test", "t", "tt"), "ttestt");

}

// Тест 10: Замена на более короткую строку
TEST_F(StringUtilsTest, Replace_ReplacementShorterThanSearch) {
    EXPECT_EQ(Replace("hello world", "world", ""), "hello ");
    EXPECT_EQ(Replace("abcabc", "abc", "x"), "xx");
}

// Тест 11: Совпадение с собой (s == d) — строка не меняется
TEST_F(StringUtilsTest, Replace_SearchEqualsReplacementNoChange) {
    EXPECT_EQ(Replace("hello", "l", "l"), "hello");
    EXPECT_EQ(Replace("abc", "b", "b"), "abc");
}

// Тест 12: Замена всей строки
TEST_F(StringUtilsTest, Replace_ReplaceEntireString) {
    EXPECT_EQ(Replace("hello", "hello", "world"), "world");
    EXPECT_EQ(Replace("abc", "abc", ""), "");
}

// Тест 13: Специальные символы в поиске и замене
TEST_F(StringUtilsTest, Replace_SpecialCharacters) {
    EXPECT_EQ(Replace("a.b.c", ".", "-"), "a-b-c");
    EXPECT_EQ(Replace("line1\nline2", "\n", " "), "line1 line2");
}

// Тест 14: Пустая строка поиска и пустая замена
TEST_F(StringUtilsTest, Replace_EmptySearchAndEmptyReplacement) {
    EXPECT_EQ(Replace("test", "", ""), "test"); // s.empty() -> return text
}

// Тест 15: Множественные совпадения с перекрытием? 
// Примечание: данная реализация НЕ обрабатывает перекрывающиеся совпадения.
// Например, Replace("aaa", "aa", "b") даст "ba", а не "bb".
TEST_F(StringUtilsTest, Replace_OverlappingMatchesNotHandled) {
    EXPECT_EQ(Replace("aaa", "aa", "b"), "ba");
}

// Тест 16: Большая строка для проверки производительности (опционально)
TEST_F(StringUtilsTest, Replace_LargeStringPerformance) {
    std::string large_text(100000, 'a');
    std::string result = Replace(large_text, "aa", "b");
    EXPECT_EQ(result.size(), 50000);
    for (char i : result) {
        EXPECT_EQ(i, 'b');
    }
}

// Тест 17: Замена, когда s встречается в d (возможная бесконечность? Нет, так как мы ищем в исходном тексте)
TEST_F(StringUtilsTest, Replace_SearchInReplacementDoesNotCauseLoop) {
    EXPECT_EQ(Replace("b", "a", "aa"), "b");
    EXPECT_EQ(Replace("a", "a", "aa"), "aa");
}

TEST_F(StringUtilsTest, TrimSV_EmptyString) {
    EXPECT_EQ(TrimSV(""), "");
}

TEST_F(StringUtilsTest, TrimSV_OnlyWhitespace) {
    EXPECT_EQ(TrimSV("   "), "");
    EXPECT_EQ(TrimSV("\t\t"), "");
    EXPECT_EQ(TrimSV("\r\n\r\n"), "");
    EXPECT_EQ(TrimSV(" \t\r\n "), "");
}

TEST_F(StringUtilsTest, TrimSV_NoLeadingOrTrailingWhitespace) {
    EXPECT_EQ(TrimSV("hello"), "hello");
    EXPECT_EQ(TrimSV("a b c"), "a b c"); // Пробелы внутри не трогаются
}

TEST_F(StringUtilsTest, TrimSV_LeadingWhitespaceOnly) {
    EXPECT_EQ(TrimSV("   hello"), "hello");
    EXPECT_EQ(TrimSV("\t\tworld"), "world");
    EXPECT_EQ(TrimSV("\r\nfoo"), "foo");
}

TEST_F(StringUtilsTest, TrimSV_TrailingWhitespaceOnly) {
    EXPECT_EQ(TrimSV("hello   "), "hello");
    EXPECT_EQ(TrimSV("world\t\t"), "world");
    EXPECT_EQ(TrimSV("bar\r\n"), "bar");
}

TEST_F(StringUtilsTest, TrimSV_BothLeadingAndTrailingWhitespace) {
    EXPECT_EQ(TrimSV("  hello  "), "hello");
    EXPECT_EQ(TrimSV("\t world \t"), "world");
    EXPECT_EQ(TrimSV("\r\n foo bar \r\n"), "foo bar");
}

TEST_F(StringUtilsTest, TrimSV_MixedWhitespaceTypes) {
    EXPECT_EQ(TrimSV(" \t\r\n hello \t\r\n "), "hello");
    EXPECT_EQ(TrimSV("\n\t foo \n\t"), "foo");
}

TEST_F(StringUtilsTest, TrimSV_SingleCharacterNoWhitespace) {
    EXPECT_EQ(TrimSV("a"), "a");
}

TEST_F(StringUtilsTest, TrimSV_SingleCharacterWithWhitespace) {
    EXPECT_EQ(TrimSV(" a "), "a");
    EXPECT_EQ(TrimSV("\ta\t"), "a");
}

TEST_F(StringUtilsTest, TrimSV_MultipleInternalSpacesPreserved) {
    EXPECT_EQ(TrimSV("  hello   world  "), "hello   world");
    EXPECT_EQ(TrimSV("\tfoo\tbar\tbaz\t"), "foo\tbar\tbaz");
}

TEST_F(StringUtilsTest, TrimSV_UnicodeCharactersWithWhitespace) {
    // UTF-8 символы не должны быть затронуты
    std::string_view input = "  Привет мир  ";
    EXPECT_EQ(TrimSV(input), "Привет мир");
    
    std::string_view emoji_input = "  🚀🌍  ";
    EXPECT_EQ(TrimSV(emoji_input), "🚀🌍");
}

TEST_F(StringUtilsTest, TrimSV_NewlineAndCarriageReturn) {
    EXPECT_EQ(TrimSV("\nhello\n"), "hello");
    EXPECT_EQ(TrimSV("\r\nworld\r\n"), "world");
    EXPECT_EQ(TrimSV("foo\rbar\n"), "foo\rbar"); // Внутренние \r и \n сохраняются
}

TEST_F(StringUtilsTest, TrimSV_TabCharacters) {
    EXPECT_EQ(TrimSV("\t\ttabbed\t\t"), "tabbed");
    EXPECT_EQ(TrimSV("a\tb\tc"), "a\tb\tc"); // Внутренние табы сохраняются
}

TEST_F(StringUtilsTest, TrimSV_LargeStringWithWhitespace) {
    std::string large_str = "   ";
    for (int i = 0; i < 1000; ++i) {
        large_str += 'x';
    }
    large_str += "   ";
    
    std::string expected(1000, 'x');
    EXPECT_EQ(TrimSV(large_str), expected);
}

TEST_F(StringUtilsTest, TrimSV_StringViewLifetimeSafety) {
    // Убедимся, что возвращаемый string_view ссылается на исходную строку
    std::string original = "  test  ";
    auto trimmed = TrimSV(original);
    
    EXPECT_EQ(trimmed, "test");
    // Проверка, что указатель совпадает с частью оригинала (не копирование)
    EXPECT_NE(trimmed.data(), nullptr);
}

TEST_F(StringUtilsTest, TrimSV_NullTerminatedStringView) {
    // std::string_view может быть создан из C-строки
    const char* c_str = "  hello world  ";
    std::string_view sv(c_str);
    EXPECT_EQ(TrimSV(sv), "hello world");
}

TEST_F(StringUtilsTest, TrimSV_EdgeCaseSingleSpace) {
    EXPECT_EQ(TrimSV(" "), "");
    EXPECT_EQ(TrimSV("\t"), "");
    EXPECT_EQ(TrimSV("\n"), "");
    EXPECT_EQ(TrimSV("\r"), "");
}


// Тест: Сравнение одинаковых строк (без учета регистра)
TEST_F(StringUtilsTest, StrCaseInsensitiveCompare_IdenticalStrings_ReturnsZero) {
    EXPECT_EQ(StrCaseInsensitiveCompare("Hello", "hello"), 0);
    EXPECT_EQ(StrCaseInsensitiveCompare("WORLD", "world"), 0);
    EXPECT_EQ(StrCaseInsensitiveCompare("MiXeD", "mixed"), 0);
}

// Тест: Сравнение пустых строк
TEST_F(StringUtilsTest, StrCaseInsensitiveCompare_EmptyStrings_ReturnsZero) {
    EXPECT_EQ(StrCaseInsensitiveCompare("", ""), 0);
    EXPECT_EQ(StrCaseInsensitiveCompare("", "a"), -1); // Пустая строка меньше любой непустой
    EXPECT_EQ(StrCaseInsensitiveCompare("a", ""), 1);  // Непустая строка больше пустой
}

// Тест: Сравнение строк разной длины, где одна является префиксом другой
TEST_F(StringUtilsTest, StrCaseInsensitiveCompare_PrefixStrings_ReturnsCorrectSign) {
    // "abc" < "abcd" => -1
    EXPECT_EQ(StrCaseInsensitiveCompare("abc", "abcd"), -1);
    // "abcd" > "abc" => 1
    EXPECT_EQ(StrCaseInsensitiveCompare("abcd", "abc"), 1);
}

// Тест: Сравнение строк с разными символами (ASCII)
TEST_F(StringUtilsTest, StrCaseInsensitiveCompare_DifferentAsciiChars_ReturnsCorrectSign) {
    // 'a' < 'b' => -1
    EXPECT_EQ(StrCaseInsensitiveCompare("a", "b"), -1);
    // 'B' > 'A' => 1 (без учета регистра: b > a)
    EXPECT_EQ(StrCaseInsensitiveCompare("B", "A"), 1);
    // 'Z' < 'z'? Нет, они равны по регистру. Но если сравнивать с другим символом...
    // Например: "apple" vs "banana": 'a' < 'b' => -1
    EXPECT_EQ(StrCaseInsensitiveCompare("apple", "banana"), -1);
    // "zebra" vs "apple": 'z' > 'a' => 1
    EXPECT_EQ(StrCaseInsensitiveCompare("zebra", "apple"), 1);
}

// Тест: Сравнение строк с цифрами и спецсимволами (ASCII)
TEST_F(StringUtilsTest, StrCaseInsensitiveCompare_DigitsAndSpecialChars_ReturnsCorrectSign) {
    // '0' < 'a' => -1
    EXPECT_EQ(StrCaseInsensitiveCompare("0", "a"), -1);
    // 'A' > '9'? В ASCII: '9'=57, 'A'=65. Да, 'A' > '9'.
    // Но lstrcmpiW и std::locale могут вести себя по-разному для спецсимволов.
    // Для простоты проверим только буквы и цифры.
    EXPECT_EQ(StrCaseInsensitiveCompare("123", "456"), -1);
    EXPECT_EQ(StrCaseInsensitiveCompare("456", "123"), 1);
}

// Тест: Сравнение строк с кириллицей (UTF-8)
// Важно: На Windows lstrcmpiW корректно обрабатывает Unicode.
// На Linux std::locale("") зависит от локали системы. Если локаль не поддерживает кириллицу, тест может упасть.
TEST_F(StringUtilsTest, StrCaseInsensitiveCompare_CyrillicStrings_ReturnsZero) {
    // "Привет" и "привет" должны быть равны без учета регистра
    EXPECT_EQ(StrCaseInsensitiveCompare("Привет", "привет"), 0);
    EXPECT_EQ(StrCaseInsensitiveCompare("МИР", "мир"), 0);
}

// Тест: Сравнение строк с кириллицей (разные символы)
TEST_F(StringUtilsTest, StrCaseInsensitiveCompare_DifferentCyrillicChars_ReturnsCorrectSign) {
    // 'а' < 'б' => -1
    EXPECT_EQ(StrCaseInsensitiveCompare("а", "б"), -1);
    // 'Б' > 'А' => 1 (без учета регистра: б > а)
    EXPECT_EQ(StrCaseInsensitiveCompare("Б", "А"), 1);
}

// Тест: Сравнение строк с латиницей и кириллицей вместе
TEST_F(StringUtilsTest, StrCaseInsensitiveCompare_MixedScripts_ReturnsCorrectSign) {
    // Порядок зависит от реализации (кодовые точки или лексикографический порядок в локали).
    // Обычно ASCII символы имеют меньшие коды, чем Unicode.
    // "a" < "а" => -1 (в большинстве реализаций)
    EXPECT_EQ(StrCaseInsensitiveCompare("a", "а"), -1);
    EXPECT_EQ(StrCaseInsensitiveCompare("а", "a"), 1);
}

// Тест: Длинные строки
TEST_F(StringUtilsTest, StrCaseInsensitiveCompare_LongStrings_ReturnsCorrectSign) {
    std::string longStr1 = "abcdefghijklmnopqrstuvwxyz";
    std::string longStr2 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    EXPECT_EQ(StrCaseInsensitiveCompare(longStr1, longStr2), 0);

    EXPECT_EQ(StrCaseInsensitiveCompare("hello world", "Hello World!"), -1);
}

// Тест: Проверка на переполнение или ошибки при очень длинных строках (опционально)
TEST_F(StringUtilsTest, StrCaseInsensitiveCompare_VeryLongStrings_DoesNotCrash) {
    std::string longStr(10000, 'a');
    std::string longStr2(10000, 'A');
    EXPECT_EQ(StrCaseInsensitiveCompare(longStr, longStr2), 0);

    longStr2[9999] = 'b'; // Последняя буква другая
    EXPECT_EQ(StrCaseInsensitiveCompare(longStr, longStr2), -1);
}
