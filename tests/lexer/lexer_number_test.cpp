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

// --- Правильные числа: INT и DOUBLE по разделу 2 грамматики ---

TEST(LexNumbers, Integers) {
    const auto result = lex("0 42 007 1234567890");
    EXPECT_EQ(dump(result.tokens), "INT(0) INT(42) INT(007) INT(1234567890) EOF");
    EXPECT_FALSE(result.diags.hasErrors());
    EXPECT_EQ(result.tokens[0].type, TokenType::Literal);
    EXPECT_EQ(result.tokens[0].literal, LiteralKind::Int);
}

TEST(LexNumbers, GrammarExamples) {
    const auto result = lex("0 12.5 12e-3 12.5E+3");
    EXPECT_EQ(dump(result.tokens), "INT(0) DOUBLE(12.5) DOUBLE(12e-3) DOUBLE(12.5E+3) EOF");
    EXPECT_FALSE(result.diags.hasErrors());
}

TEST(LexNumbers, Doubles) {
    EXPECT_EQ(dump("0.0 3.14159 1e10 1E5 2e+0 0.5e-10"),
              "DOUBLE(0.0) DOUBLE(3.14159) DOUBLE(1e10) DOUBLE(1E5) DOUBLE(2e+0) "
              "DOUBLE(0.5e-10) EOF");
}

TEST(LexNumbers, SignIsSeparateOperator) {
    EXPECT_EQ(dump("-5 +2.5 a-1 x=-1e3"),
              "OP(-) INT(5) OP(+) DOUBLE(2.5) ID(a) OP(-) INT(1) ID(x) OP(=) OP(-) "
              "DOUBLE(1e3) EOF");
}

TEST(LexNumbers, DotWithoutDigitStaysDelimiter) {
    EXPECT_EQ(dump("3."), "INT(3) DELIM(.) EOF");
    EXPECT_EQ(dump("3.x"), "INT(3) DELIM(.) ID(x) EOF");
    EXPECT_EQ(dump("1..2"), "INT(1) DELIM(.) DELIM(.) INT(2) EOF");
    EXPECT_EQ(dump(".5"), "DELIM(.) INT(5) EOF");
    EXPECT_EQ(dump("12.5.x"), "DOUBLE(12.5) DELIM(.) ID(x) EOF");
    EXPECT_FALSE(lex("1..2").diags.hasErrors());
}

TEST(LexNumbers, NumbersNextToDelimitersAndOperators) {
    EXPECT_EQ(dump("values[0]=1;f(2,3)"),
              "ID(values) DELIM([) INT(0) DELIM(]) OP(=) INT(1) DELIM(;) ID(f) DELIM(() "
              "INT(2) OP(,) INT(3) DELIM()) EOF");
    EXPECT_EQ(dump("1<2"), "INT(1) OP(<) INT(2) EOF");
}

// --- Правильные числа с единицами: UNIT ---

TEST(LexUnits, AllUnits) {
    const auto result = lex("2usd 20min 8ktok 4tok 1s");
    EXPECT_EQ(dump(result.tokens), "UNIT(2usd) UNIT(20min) UNIT(8ktok) UNIT(4tok) UNIT(1s) EOF");
    EXPECT_FALSE(result.diags.hasErrors());
    EXPECT_EQ(result.tokens[0].type, TokenType::Literal);
    EXPECT_EQ(result.tokens[0].literal, LiteralKind::Unit);
}

TEST(LexUnits, DoubleWithUnit) {
    EXPECT_EQ(dump("1.5usd 3e2s 0.5min"), "UNIT(1.5usd) UNIT(3e2s) UNIT(0.5min) EOF");
}

TEST(LexUnits, BudgetProperty) {
    EXPECT_EQ(dump("budget: 2usd, 20min, 8ktok;"),
              "KW(budget) DELIM(:) UNIT(2usd) OP(,) UNIT(20min) OP(,) UNIT(8ktok) DELIM(;) EOF");
}

TEST(LexUnits, SeparatedSuffixIsIdentifier) {
    const auto result = lex("2 usd");
    EXPECT_EQ(dump(result.tokens), "INT(2) ID(usd) EOF");
    EXPECT_FALSE(result.diags.hasErrors());
}

// --- LEX-8: неверно записанные числа ---

TEST(LexMalformedNumbers, BoardExampleTwoDots) {
    // Пример с доски: один ошибочный фрагмент.
    const auto result = lex("3.5.7");
    EXPECT_EQ(dump(result.tokens), "UNKNOWN(3.5.7) EOF");
    ASSERT_EQ(result.diags.all().size(), 1u);
    const Diagnostic& d = result.diags.all()[0];
    EXPECT_EQ(d.code, "LEX003");
    EXPECT_EQ(d.message,
              "неверно записанное число '3.5.7': после дробной части ожидался конец числа, "
              "встретилась вторая точка");
    EXPECT_EQ(d.range.begin, (SourcePos{.line = 1, .column = 1, .offset = 0}));
    EXPECT_EQ(d.range.end, (SourcePos{.line = 1, .column = 6, .offset = 5}));
}

TEST(LexMalformedNumbers, ManyDotsAreOneFragment) {
    EXPECT_EQ(dump("1.2.3.4;"), "UNKNOWN(1.2.3.4) DELIM(;) EOF");
    EXPECT_EQ(codes("1.2.3.4;"), Codes{"LEX003"});
}

TEST(LexMalformedNumbers, ContinuesAfterError) {
    const auto result = lex("x = 3.5.7 + 1;");
    EXPECT_EQ(dump(result.tokens), "ID(x) OP(=) UNKNOWN(3.5.7) OP(+) INT(1) DELIM(;) EOF");
    EXPECT_EQ(codes(result.diags), Codes{"LEX003"});
}

TEST(LexMalformedNumbers, LettersAfterDigits) {
    const auto result = lex("35k6");
    EXPECT_EQ(dump(result.tokens), "UNKNOWN(35k6) EOF");
    ASSERT_EQ(codes(result.diags), Codes{"LEX003"});
    EXPECT_EQ(result.diags.all()[0].message,
              "неверно записанное число '35k6': ожидалась единица usd, tok, ktok, s или min, "
              "встретилось 'k6'");
}

TEST(LexMalformedNumbers, WrongUnits) {
    EXPECT_EQ(dump("2usdx 2Usd 3ms 1_000 0x1F 5k"),
              "UNKNOWN(2usdx) UNKNOWN(2Usd) UNKNOWN(3ms) UNKNOWN(1_000) UNKNOWN(0x1F) "
              "UNKNOWN(5k) EOF");
    EXPECT_EQ(codes("2usdx 2Usd 3ms 1_000 0x1F 5k"),
              (Codes{"LEX003", "LEX003", "LEX003", "LEX003", "LEX003", "LEX003"}));
}

TEST(LexMalformedNumbers, IncompleteExponent) {
    const auto result = lex("12e; 12E+ 1.5e-x");
    EXPECT_EQ(dump(result.tokens), "UNKNOWN(12e) DELIM(;) UNKNOWN(12E+) UNKNOWN(1.5e-x) EOF");
    ASSERT_EQ(codes(result.diags), (Codes{"LEX003", "LEX003", "LEX003"}));
    EXPECT_EQ(result.diags.all()[0].message,
              "неверно записанное число '12e': после 'e' ожидались цифры порядка");
    EXPECT_EQ(result.diags.all()[1].message,
              "неверно записанное число '12E+': после 'E' ожидались цифры порядка");
}

TEST(LexMalformedNumbers, FractionalExponent) {
    const auto result = lex("1e5.3");
    EXPECT_EQ(dump(result.tokens), "UNKNOWN(1e5.3) EOF");
    ASSERT_EQ(codes(result.diags), Codes{"LEX003"});
    EXPECT_EQ(result.diags.all()[0].message,
              "неверно записанное число '1e5.3': порядок должен быть целым, встретилась точка");
}

TEST(LexMalformedNumbers, SecondExponent) {
    EXPECT_EQ(dump("1e5e3"), "UNKNOWN(1e5e3) EOF");
}

TEST(LexMalformedNumbers, DigitsAfterUnit) {
    EXPECT_EQ(dump("2s5 2s.5"), "UNKNOWN(2s5) UNKNOWN(2s.5) EOF");
    EXPECT_EQ(lex("2s.5").diags.all()[0].message,
              "неверно записанное число '2s.5': после единицы ожидался конец числа, "
              "встретилась точка");
}

}  // namespace
}  // namespace vibe
