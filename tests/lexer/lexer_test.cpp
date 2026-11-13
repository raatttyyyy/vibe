#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "lexer/keyword_trie.h"
#include "lexer_test_util.h"

namespace vibe {
namespace {

using test::codes;
using test::dump;
using test::lex;
using Codes = std::vector<std::string>;

// --- LEX-1: выход заканчивается ровно одной лексемой конца файла ---

TEST(LexEndOfFile, EmptySourceGivesOnlyEndOfFile) {
    const auto result = lex("");
    ASSERT_EQ(result.tokens.size(), 1u);
    EXPECT_EQ(result.tokens[0].type, TokenType::EndOfFile);
    EXPECT_EQ(result.tokens[0].lexeme, "");
    EXPECT_FALSE(result.diags.hasErrors());
}

TEST(LexEndOfFile, WhitespaceOnlySourceGivesOnlyEndOfFile) {
    EXPECT_EQ(dump(" \t\r\n  \n"), "EOF");
}

TEST(LexEndOfFile, ExactlyOneAtTheEnd) {
    const auto result = lex("a b");
    ASSERT_EQ(result.tokens.size(), 3u);
    EXPECT_EQ(result.tokens.back().type, TokenType::EndOfFile);
    for (std::size_t i = 0; i + 1 < result.tokens.size(); ++i) {
        EXPECT_NE(result.tokens[i].type, TokenType::EndOfFile);
    }
}

TEST(LexEndOfFile, PositionIsEndOfSource) {
    const auto result = lex("ab\ncd ");
    const SourcePos end{.line = 2, .column = 4, .offset = 6};
    EXPECT_EQ(result.tokens.back().range.begin, end);
    EXPECT_EQ(result.tokens.back().range.end, end);
}

TEST(LexEndOfFile, ProducedAfterErrors) {
    const auto result = lex("# @");
    ASSERT_FALSE(result.tokens.empty());
    EXPECT_EQ(result.tokens.back().type, TokenType::EndOfFile);
}

// --- LEX-3: ключевые слова распознаются бором, остальное — идентификаторы ---

TEST(LexKeywords, EveryKeywordIsKeyword) {
    for (std::string_view word : keywords()) {
        const auto result = lex(word);
        ASSERT_EQ(result.tokens.size(), 2u) << word;
        EXPECT_EQ(result.tokens[0].type, TokenType::Keyword) << word;
        EXPECT_EQ(result.tokens[0].lexeme, word);
    }
}

TEST(LexKeywords, FuncFollowedByFuncName) {
    EXPECT_EQ(dump("func funcName"), "KW(func) ID(funcName) EOF");
}

TEST(LexKeywords, LibraryNamesAreIdentifiers) {
    EXPECT_EQ(dump("main readInt readDouble readBool readChar readLine print"),
              "ID(main) ID(readInt) ID(readDouble) ID(readBool) ID(readChar) ID(readLine) "
              "ID(print) EOF");
}

TEST(LexKeywords, TrueAndFalseAreKeywords) {
    EXPECT_EQ(dump("true false"), "KW(true) KW(false) EOF");
}

TEST(LexKeywords, LongestIdentifierIsLookedUp) {
    EXPECT_EQ(dump("iff if_ _if If dob do"), "ID(iff) ID(if_) ID(_if) ID(If) ID(dob) KW(do) EOF");
}

TEST(LexIdentifiers, LettersDigitsUnderscore) {
    EXPECT_EQ(dump("x _ _x1 camelCase snake_case X9_y"),
              "ID(x) ID(_) ID(_x1) ID(camelCase) ID(snake_case) ID(X9_y) EOF");
}

// --- LEX-4: типы лексем ---

TEST(LexTypes, Delimiters) {
    EXPECT_EQ(dump("( ) [ ] { } ; : ."),
              "DELIM(() DELIM()) DELIM([) DELIM(]) DELIM({) DELIM(}) DELIM(;) DELIM(:) "
              "DELIM(.) EOF");
}

TEST(LexTypes, Operators) {
    EXPECT_EQ(dump("+ - * / % ++ -- = == != < <= > >= && || ! ,"),
              "OP(+) OP(-) OP(*) OP(/) OP(%) OP(++) OP(--) OP(=) OP(==) OP(!=) OP(<) OP(<=) "
              "OP(>) OP(>=) OP(&&) OP(||) OP(!) OP(,) EOF");
}

TEST(LexTypes, CommaIsOperatorInLists) {
    EXPECT_EQ(dump("f(a,b)"), "ID(f) DELIM(() ID(a) OP(,) ID(b) DELIM()) EOF");
}

// --- LEX-5: тип, исходный текст и позиция у каждой лексемы ---

TEST(LexPositions, LineColumnOffset) {
    const auto result = lex("ab\n  cd");
    ASSERT_EQ(result.tokens.size(), 3u);
    EXPECT_EQ(result.tokens[0].range.begin, (SourcePos{.line = 1, .column = 1, .offset = 0}));
    EXPECT_EQ(result.tokens[0].range.end, (SourcePos{.line = 1, .column = 3, .offset = 2}));
    EXPECT_EQ(result.tokens[1].range.begin, (SourcePos{.line = 2, .column = 3, .offset = 5}));
    EXPECT_EQ(result.tokens[1].range.end, (SourcePos{.line = 2, .column = 5, .offset = 7}));
}

TEST(LexPositions, CrLfIsOneLineBreak) {
    const auto result = lex("a\r\nb\r\n\r\nc");
    ASSERT_EQ(result.tokens.size(), 4u);
    EXPECT_EQ(result.tokens[1].range.begin, (SourcePos{.line = 2, .column = 1, .offset = 3}));
    EXPECT_EQ(result.tokens[2].range.begin, (SourcePos{.line = 4, .column = 1, .offset = 8}));
}

TEST(LexPositions, LoneCrIsLineBreak) {
    const auto result = lex("a\rb");
    ASSERT_EQ(result.tokens.size(), 3u);
    EXPECT_EQ(result.tokens[1].range.begin, (SourcePos{.line = 2, .column = 1, .offset = 2}));
}

TEST(LexPositions, TabIsOneColumn) {
    const auto result = lex("\t\tx");
    EXPECT_EQ(result.tokens[0].range.begin, (SourcePos{.line = 1, .column = 3, .offset = 2}));
}

TEST(LexPositions, ColumnsCountCharactersNotBytes) {
    // "я" — два байта, но один символ.
    const auto result = lex("/* я */x");
    ASSERT_EQ(result.tokens.size(), 2u);
    EXPECT_EQ(result.tokens[0].range.begin, (SourcePos{.line = 1, .column = 8, .offset = 8}));
}

TEST(LexPositions, LexemeIsExactSourceText) {
    const auto result = lex("  while\t(x1)");
    ASSERT_EQ(result.tokens.size(), 5u);
    EXPECT_EQ(result.tokens[0].lexeme, "while");
    EXPECT_EQ(result.tokens[0].type, TokenType::Keyword);
    EXPECT_EQ(result.tokens[2].lexeme, "x1");
    EXPECT_EQ(result.tokens[2].type, TokenType::Identifier);
}

// --- LEX-6: самая длинная подходящая лексема ---

TEST(LexLongestMatch, BoardExamplePlusPlusPlus) {
    // Пример с доски.
    const auto result = lex("a+++b");
    EXPECT_EQ(dump(result.tokens), "ID(a) OP(++) OP(+) ID(b) EOF");
    EXPECT_FALSE(result.diags.hasErrors());
}

TEST(LexLongestMatch, MinusMinusMinus) {
    EXPECT_EQ(dump("a---b"), "ID(a) OP(--) OP(-) ID(b) EOF");
    EXPECT_EQ(dump("a----b"), "ID(a) OP(--) OP(--) ID(b) EOF");
}

TEST(LexLongestMatch, TwoCharacterOperators) {
    EXPECT_EQ(dump("a<=b>=c==d!=e"),
              "ID(a) OP(<=) ID(b) OP(>=) ID(c) OP(==) ID(d) OP(!=) ID(e) EOF");
    EXPECT_EQ(dump("a===b"), "ID(a) OP(==) OP(=) ID(b) EOF");
    EXPECT_EQ(dump("!!a"), "OP(!) OP(!) ID(a) EOF");
    EXPECT_EQ(dump("a&&b||c"), "ID(a) OP(&&) ID(b) OP(||) ID(c) EOF");
    EXPECT_EQ(dump("a&&&&b"), "ID(a) OP(&&) OP(&&) ID(b) EOF");
}

TEST(LexLongestMatch, NoShiftOperator) {
    EXPECT_EQ(dump("list<list<int>>"),
              "KW(list) OP(<) KW(list) OP(<) KW(int) OP(>) OP(>) EOF");
    EXPECT_EQ(dump("a>>=b"), "ID(a) OP(>) OP(>=) ID(b) EOF");
}

TEST(LexLongestMatch, WhitespaceSplitsOperators) {
    EXPECT_EQ(dump("a+ +b"), "ID(a) OP(+) OP(+) ID(b) EOF");
    EXPECT_EQ(dump("a = = b"), "ID(a) OP(=) OP(=) ID(b) EOF");
}

// --- LEX-7: лексер не проверяет синтаксис ---

TEST(LexNoSyntax, BoardExampleAssignIf) {
    // Пример с доски: все лексемы известны, ошибку найдёт парсер.
    const auto result = lex("a = if : b");
    EXPECT_EQ(dump(result.tokens), "ID(a) OP(=) KW(if) DELIM(:) ID(b) EOF");
    EXPECT_FALSE(result.diags.hasErrors());
}

TEST(LexNoSyntax, NonsenseOrderIsAccepted) {
    const auto result = lex(") ( func ; else } {");
    EXPECT_EQ(dump(result.tokens),
              "DELIM()) DELIM(() KW(func) DELIM(;) KW(else) DELIM(}) DELIM({) EOF");
    EXPECT_FALSE(result.diags.hasErrors());
}

// --- LEX-8: недопустимые символы ---

TEST(LexInvalidCharacter, BoardExampleHash) {
    // Пример с доски.
    const auto result = lex("if (a > #b)");
    EXPECT_EQ(dump(result.tokens),
              "KW(if) DELIM(() ID(a) OP(>) UNKNOWN(#) ID(b) DELIM()) EOF");
    ASSERT_EQ(result.diags.all().size(), 1u);
    const Diagnostic& d = result.diags.all()[0];
    EXPECT_EQ(d.code, "LEX001");
    EXPECT_EQ(d.severity, Severity::Error);
    EXPECT_EQ(d.stage, Stage::Lexer);
    EXPECT_EQ(d.message, "недопустимый символ '#'");
    EXPECT_EQ(d.range.begin, (SourcePos{.line = 1, .column = 9, .offset = 8}));
    EXPECT_EQ(d.range.end, (SourcePos{.line = 1, .column = 10, .offset = 9}));
    EXPECT_EQ(formatDiagnostic("main.vibe", d), "main.vibe:1:9: ошибка LEX001: недопустимый символ '#'");
}

TEST(LexInvalidCharacter, ValidCounterpartHasNoErrors) {
    const auto result = lex("if (a > b)");
    EXPECT_EQ(dump(result.tokens), "KW(if) DELIM(() ID(a) OP(>) ID(b) DELIM()) EOF");
    EXPECT_FALSE(result.diags.hasErrors());
}

TEST(LexInvalidCharacter, AdjacentInvalidCharactersFormOneToken) {
    const auto result = lex("a @#$ b");
    EXPECT_EQ(dump(result.tokens), "ID(a) UNKNOWN(@#$) ID(b) EOF");
    ASSERT_EQ(result.diags.all().size(), 1u);
    EXPECT_EQ(result.diags.all()[0].message, "недопустимые символы '@#$'");
}

TEST(LexInvalidCharacter, NonLatinIdentifier) {
    const auto result = lex("имя = x");
    EXPECT_EQ(dump(result.tokens), "UNKNOWN(имя) OP(=) ID(x) EOF");
    EXPECT_EQ(codes(result.diags), Codes{"LEX001"});
    EXPECT_EQ(result.tokens[1].range.begin.column, 5u);
}

TEST(LexInvalidCharacter, MalformedUtf8) {
    const auto result = lex("a \xFF b");
    EXPECT_EQ(result.tokens[1].type, TokenType::Unknown);
    EXPECT_EQ(result.tokens[1].lexeme, "\xFF");
    ASSERT_EQ(result.diags.all().size(), 1u);
    EXPECT_EQ(result.diags.all()[0].message, "недопустимый символ '\\xFF'");
    EXPECT_EQ(result.tokens[2].range.begin.column, 5u);
}

TEST(LexInvalidCharacter, OtherWhitespaceIsInvalid) {
    const auto result = lex("a\fb");
    EXPECT_EQ(result.tokens[1].type, TokenType::Unknown);
    EXPECT_EQ(result.diags.all()[0].message, "недопустимый символ '\\x0C'");
}

TEST(LexInvalidCharacter, ContinuesToEndOfFile) {
    const auto result = lex("# a\n? b\n~");
    EXPECT_EQ(dump(result.tokens), "UNKNOWN(#) ID(a) UNKNOWN(?) ID(b) UNKNOWN(~) EOF");
    EXPECT_EQ(codes(result.diags), (Codes{"LEX001", "LEX001", "LEX001"}));
    EXPECT_EQ(result.diags.all()[1].range.begin.line, 2u);
    EXPECT_EQ(result.diags.all()[2].range.begin.line, 3u);
}

TEST(LexIncompleteOperator, SingleAmpersandAndBar) {
    const auto result = lex("a & b | c");
    EXPECT_EQ(dump(result.tokens), "ID(a) UNKNOWN(&) ID(b) UNKNOWN(|) ID(c) EOF");
    EXPECT_EQ(codes(result.diags), (Codes{"LEX002", "LEX002"}));
    EXPECT_EQ(result.diags.all()[0].message, "неполная операция '&': ожидалось '&&'");
    EXPECT_EQ(result.diags.all()[1].message, "неполная операция '|': ожидалось '||'");
}

TEST(LexIncompleteOperator, TripleAmpersand) {
    const auto result = lex("a&&&b");
    EXPECT_EQ(dump(result.tokens), "ID(a) OP(&&) UNKNOWN(&) ID(b) EOF");
    EXPECT_EQ(codes(result.diags), Codes{"LEX002"});
}

// --- LEX-9: пробелы и комментарии пропускаются ---

TEST(LexComments, LineComment) {
    const auto result = lex("a // comment + - #\nb");
    EXPECT_EQ(dump(result.tokens), "ID(a) ID(b) EOF");
    EXPECT_FALSE(result.diags.hasErrors());
    EXPECT_EQ(result.tokens[1].range.begin.line, 2u);
}

TEST(LexComments, LineCommentAtEndOfFile) {
    EXPECT_EQ(dump("a // no line break"), "ID(a) EOF");
    EXPECT_EQ(dump("//"), "EOF");
}

TEST(LexComments, BlockCommentSpansLines) {
    const auto result = lex("a /* x\r\n y # */ b");
    EXPECT_EQ(dump(result.tokens), "ID(a) ID(b) EOF");
    EXPECT_FALSE(result.diags.hasErrors());
    EXPECT_EQ(result.tokens[1].range.begin, (SourcePos{.line = 2, .column = 9, .offset = 16}));
}

TEST(LexComments, BlockCommentsDoNotNest) {
    EXPECT_EQ(dump("/* /* */ */"), "OP(*) OP(/) EOF");
}

TEST(LexComments, CommentSeparatesOperators) {
    EXPECT_EQ(dump("+/**/+"), "OP(+) OP(+) EOF");
    EXPECT_EQ(dump("/***/"), "EOF");
}

TEST(LexComments, SlashIsDivision) {
    EXPECT_EQ(dump("a/b"), "ID(a) OP(/) ID(b) EOF");
}

TEST(LexComments, UnclosedBlockComment) {
    const auto result = lex("a /* never\nclosed");
    EXPECT_EQ(dump(result.tokens), "ID(a) UNKNOWN(/* never\nclosed) EOF");
    ASSERT_EQ(codes(result.diags), Codes{"LEX008"});
    EXPECT_EQ(result.diags.all()[0].message,
              "незакрытый комментарий: ожидалось '*/', встретился конец файла");
    EXPECT_EQ(result.diags.all()[0].range.begin, (SourcePos{.line = 1, .column = 3, .offset = 2}));
}

TEST(LexComments, UnclosedBlockCommentEndingWithStar) {
    EXPECT_EQ(codes("/* *"), Codes{"LEX008"});
    EXPECT_EQ(codes("/*/"), Codes{"LEX008"});
}

}  // namespace
}  // namespace vibe
