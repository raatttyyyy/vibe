#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "lexer_test_util.h"

namespace vibe {
namespace {

using test::codes;
using test::dump;
using test::lex;
using Codes = std::vector<std::string>;

// --- CHAR: правильные ---

TEST(LexChars, PlainCharacters) {
    const auto result = lex("'a' ':' ' ' '\"' '/'");
    EXPECT_EQ(dump(result.tokens), "CHAR('a') CHAR(':') CHAR(' ') CHAR('\"') CHAR('/') EOF");
    EXPECT_FALSE(result.diags.hasErrors());
    EXPECT_EQ(result.tokens[0].type, TokenType::Literal);
    EXPECT_EQ(result.tokens[0].literal, LiteralKind::Char);
}

TEST(LexChars, Escapes) {
    const auto result = lex(R"('\n' '\r' '\t' '\\' '\'')");
    EXPECT_EQ(dump(result.tokens), R"(CHAR('\n') CHAR('\r') CHAR('\t') CHAR('\\') CHAR('\'') EOF)");
    EXPECT_FALSE(result.diags.hasErrors());
}

TEST(LexChars, OneUnicodeCharacter) {
    // Два и четыре байта UTF-8, но по одному символу.
    const auto result = lex("'я' '😀'");
    EXPECT_EQ(dump(result.tokens), "CHAR('я') CHAR('😀') EOF");
    EXPECT_FALSE(result.diags.hasErrors());
    EXPECT_EQ(result.tokens[1].range.begin.column, 5u);
}

TEST(LexChars, InDeclaration) {
    EXPECT_EQ(dump("char separator = ':';"),
              "KW(char) ID(separator) OP(=) CHAR(':') DELIM(;) EOF");
}

// --- CHAR: неправильные (LEX-8) ---

TEST(LexChars, Empty) {
    const auto result = lex("''");
    EXPECT_EQ(dump(result.tokens), "UNKNOWN('') EOF");
    ASSERT_EQ(codes(result.diags), Codes{"LEX006"});
    EXPECT_EQ(result.diags.all()[0].message,
              "символьный литерал: ожидался ровно один символ, встретилось символов: 0");
}

TEST(LexChars, TooLong) {
    const auto result = lex("'ab' '\\n\\t'");
    EXPECT_EQ(dump(result.tokens), "UNKNOWN('ab') UNKNOWN('\\n\\t') EOF");
    ASSERT_EQ(codes(result.diags), (Codes{"LEX006", "LEX006"}));
    EXPECT_EQ(result.diags.all()[0].message,
              "символьный литерал: ожидался ровно один символ, встретилось символов: 2");
}

TEST(LexChars, InvalidEscape) {
    const auto result = lex(R"('\q' '\"')");
    EXPECT_EQ(dump(result.tokens), R"(UNKNOWN('\q') UNKNOWN('\"') EOF)");
    ASSERT_EQ(codes(result.diags), (Codes{"LEX007", "LEX007"}));
    EXPECT_EQ(result.diags.all()[0].message,
              R"(недопустимая escape-последовательность '\q': ожидалось \n, \r, \t, \\ или \')");
}

TEST(LexChars, UnclosedAtLineBreak) {
    const auto result = lex("'a\nb");
    EXPECT_EQ(dump(result.tokens), "UNKNOWN('a) ID(b) EOF");
    ASSERT_EQ(codes(result.diags), Codes{"LEX005"});
    EXPECT_EQ(result.diags.all()[0].message,
              "незакрытый символьный литерал: ожидалась закрывающая ', встретился перевод строки");
    EXPECT_EQ(result.tokens[1].range.begin.line, 2u);
}

TEST(LexChars, UnclosedAtEndOfFile) {
    const auto result = lex("x = '");
    EXPECT_EQ(dump(result.tokens), "ID(x) OP(=) UNKNOWN(') EOF");
    ASSERT_EQ(codes(result.diags), Codes{"LEX005"});
    EXPECT_EQ(result.diags.all()[0].message,
              "незакрытый символьный литерал: ожидалась закрывающая ', встретился конец файла");
}

TEST(LexChars, EscapedQuoteDoesNotClose) {
    EXPECT_EQ(codes(R"('\')"), Codes{"LEX005"});
}

// --- STRING: правильные ---

TEST(LexStrings, Plain) {
    const auto result = lex(R"("" "abc" "Result" "привет, мир")");
    EXPECT_EQ(dump(result.tokens),
              R"(STRING("") STRING("abc") STRING("Result") STRING("привет, мир") EOF)");
    EXPECT_FALSE(result.diags.hasErrors());
    EXPECT_EQ(result.tokens[0].literal, LiteralKind::String);
}

TEST(LexStrings, Escapes) {
    const auto result = lex(R"("a\"b" "\\" "\n\r\t" "'")");
    EXPECT_EQ(dump(result.tokens), R"(STRING("a\"b") STRING("\\") STRING("\n\r\t") STRING("'") EOF)");
    EXPECT_FALSE(result.diags.hasErrors());
}

TEST(LexStrings, CommentsAndOperatorsInsideAreText) {
    const auto result = lex(R"("// no" "/* no */" "#" "${x}" "`")");
    EXPECT_EQ(dump(result.tokens),
              R"(STRING("// no") STRING("/* no */") STRING("#") STRING("${x}") STRING("`") EOF)");
    EXPECT_FALSE(result.diags.hasErrors());
}

TEST(LexStrings, ColumnsAfterUnicodeText) {
    const auto result = lex("\"яя\" x");
    EXPECT_EQ(result.tokens[1].range.begin, (SourcePos{.line = 1, .column = 6, .offset = 7}));
}

// --- STRING: неправильные (LEX-8) ---

TEST(LexStrings, UnclosedAtLineBreak) {
    const auto result = lex("s = \"abc\nx;");
    EXPECT_EQ(dump(result.tokens), "ID(s) OP(=) UNKNOWN(\"abc) ID(x) DELIM(;) EOF");
    ASSERT_EQ(codes(result.diags), Codes{"LEX004"});
    EXPECT_EQ(result.diags.all()[0].message,
              "незакрытая строка: ожидалась закрывающая \", встретился перевод строки");
}

TEST(LexStrings, UnclosedAtCrLf) {
    EXPECT_EQ(dump("\"abc\r\nx"), "UNKNOWN(\"abc) ID(x) EOF");
}

TEST(LexStrings, UnclosedAtEndOfFile) {
    const auto result = lex("\"abc");
    EXPECT_EQ(dump(result.tokens), "UNKNOWN(\"abc) EOF");
    ASSERT_EQ(codes(result.diags), Codes{"LEX004"});
    EXPECT_EQ(result.diags.all()[0].message,
              "незакрытая строка: ожидалась закрывающая \", встретился конец файла");
}

TEST(LexStrings, BackslashBeforeLineBreak) {
    EXPECT_EQ(codes("\"abc\\\n\""), (Codes{"LEX004", "LEX004"}));
}

TEST(LexStrings, InvalidEscape) {
    const auto result = lex(R"("a\qb" "\'")");
    EXPECT_EQ(dump(result.tokens), R"(UNKNOWN("a\qb") UNKNOWN("\'") EOF)");
    ASSERT_EQ(codes(result.diags), (Codes{"LEX007", "LEX007"}));
    EXPECT_EQ(result.diags.all()[0].message,
              R"(недопустимая escape-последовательность '\q': ожидалось \n, \r, \t, \\ или \")");
}

TEST(LexStrings, OneMessagePerUnknownToken) {
    EXPECT_EQ(codes(R"("\q\w\e")"), Codes{"LEX007"});
    EXPECT_EQ(codes("\"\\q"), Codes{"LEX004"});
}

TEST(LexStrings, ContinuesAfterError) {
    const auto result = lex("print(\"\\x\"); print(\"ok\");");
    EXPECT_EQ(dump(result.tokens),
              "ID(print) DELIM(() UNKNOWN(\"\\x\") DELIM()) DELIM(;) ID(print) DELIM(() "
              "STRING(\"ok\") DELIM()) DELIM(;) EOF");
    EXPECT_EQ(codes(result.diags), Codes{"LEX007"});
}

}  // namespace
}  // namespace vibe
