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

// --- LEX-10: правильные промпты ---

TEST(LexPrompts, FullPrompt) {
    const auto result = lex("`Оцени патч.`");
    EXPECT_EQ(dump(result.tokens), "PROMPT(`Оцени патч.`) EOF");
    EXPECT_FALSE(result.diags.hasErrors());
    EXPECT_EQ(result.tokens[0].type, TokenType::Literal);
    EXPECT_EQ(result.tokens[0].literal, LiteralKind::Prompt);
}

TEST(LexPrompts, EmptyPrompt) {
    EXPECT_EQ(dump("``"), "PROMPT(``) EOF");
}

TEST(LexPrompts, LineBreaksAreText) {
    const auto result = lex("`a\nb` x");
    EXPECT_EQ(dump(result.tokens), "PROMPT(`a\nb`) ID(x) EOF");
    EXPECT_FALSE(result.diags.hasErrors());
    EXPECT_EQ(result.tokens[1].range.begin, (SourcePos{.line = 2, .column = 4, .offset = 6}));
}

TEST(LexPrompts, HeadAndTail) {
    const auto result = lex("`Исправь сбой на шаге ${run.step}.`");
    EXPECT_EQ(dump(result.tokens),
              "PROMPT(`Исправь сбой на шаге ${) ID(run) DELIM(.) ID(step) PROMPT(}.`) EOF");
    EXPECT_FALSE(result.diags.hasErrors());
    EXPECT_EQ(result.tokens[0].literal, LiteralKind::Prompt);
    EXPECT_EQ(result.tokens[4].literal, LiteralKind::Prompt);
}

TEST(LexPrompts, HeadMidTail) {
    EXPECT_EQ(dump("`a ${x} b ${y + 1} c`"),
              "PROMPT(`a ${) ID(x) PROMPT(} b ${) ID(y) OP(+) INT(1) PROMPT(} c`) EOF");
}

TEST(LexPrompts, AdjacentSubstitutions) {
    EXPECT_EQ(dump("`${x}${y}`"), "PROMPT(`${) ID(x) PROMPT(}${) ID(y) PROMPT(}`) EOF");
}

TEST(LexPrompts, PositionsOfParts) {
    const auto result = lex("`ab${x}c`");
    ASSERT_EQ(result.tokens.size(), 4u);
    EXPECT_EQ(result.tokens[0].range.end, (SourcePos{.line = 1, .column = 6, .offset = 5}));
    EXPECT_EQ(result.tokens[1].range.begin, (SourcePos{.line = 1, .column = 6, .offset = 5}));
    EXPECT_EQ(result.tokens[2].range.begin, (SourcePos{.line = 1, .column = 7, .offset = 6}));
    EXPECT_EQ(result.tokens[2].range.end, (SourcePos{.line = 1, .column = 10, .offset = 9}));
}

TEST(LexPrompts, BracesInsideSubstitution) {
    EXPECT_EQ(dump("`${ Point { x: 1 } }`"),
              "PROMPT(`${) ID(Point) DELIM({) ID(x) DELIM(:) INT(1) DELIM(}) PROMPT(}`) EOF");
    EXPECT_EQ(dump("`${ map { 1: map {} } }!`"),
              "PROMPT(`${) KW(map) DELIM({) INT(1) DELIM(:) KW(map) DELIM({) DELIM(}) DELIM(}) "
              "PROMPT(}!`) EOF");
}

TEST(LexPrompts, NestedPrompt) {
    const auto result = lex("`a ${ ask<T>(J, `b ${c}`) } d`");
    EXPECT_EQ(dump(result.tokens),
              "PROMPT(`a ${) KW(ask) OP(<) ID(T) OP(>) DELIM(() ID(J) OP(,) PROMPT(`b ${) ID(c) "
              "PROMPT(}`) DELIM()) PROMPT(} d`) EOF");
    EXPECT_FALSE(result.diags.hasErrors());
}

TEST(LexPrompts, StringsAndCommentsInsideSubstitution) {
    EXPECT_EQ(dump("`${ f(\"}`\") }`"),
              "PROMPT(`${) ID(f) DELIM(() STRING(\"}`\") DELIM()) PROMPT(}`) EOF");
    EXPECT_EQ(dump("`${ x /* } ` */ // }\n }`"), "PROMPT(`${) ID(x) PROMPT(}`) EOF");
}

TEST(LexPrompts, Escapes) {
    const auto result = lex(R"(`a \` b \\ c \${x} d`)");
    EXPECT_EQ(dump(result.tokens), R"(PROMPT(`a \` b \\ c \${x} d`) EOF)");
    EXPECT_FALSE(result.diags.hasErrors());
}

TEST(LexPrompts, DollarAndBracesAloneAreText) {
    const auto result = lex("`cost $5 {x} } $`");
    EXPECT_EQ(dump(result.tokens), "PROMPT(`cost $5 {x} } $`) EOF");
    EXPECT_FALSE(result.diags.hasErrors());
}

TEST(LexPrompts, BracesOutsidePromptsAreDelimiters) {
    EXPECT_EQ(dump("} { }"), "DELIM(}) DELIM({) DELIM(}) EOF");
    EXPECT_EQ(dump("{ `${x}` }"), "DELIM({) PROMPT(`${) ID(x) PROMPT(}`) DELIM(}) EOF");
}

TEST(LexPrompts, AgentPropertySystem) {
    EXPECT_EQ(dump("system: `Ты судья.`;"), "KW(system) DELIM(:) PROMPT(`Ты судья.`) DELIM(;) EOF");
}

// LEX-2: подстановки хранятся в стеке, а не в рекурсии; глубокая вложенность не страшна.
TEST(LexPrompts, DeepNesting) {
    constexpr int kDepth = 20000;
    std::string source;
    for (int i = 0; i < kDepth; ++i) source += "`${";
    source += "x";
    for (int i = 0; i < kDepth; ++i) source += "}`";
    const auto result = lex(source);
    EXPECT_FALSE(result.diags.hasErrors());
    ASSERT_EQ(result.tokens.size(), 2u * kDepth + 2);
    EXPECT_EQ(result.tokens[kDepth].lexeme, "x");
    EXPECT_EQ(result.tokens[kDepth + 1].lexeme, "}`");
}

// --- LEX-10 и LEX-8: неправильные промпты ---

TEST(LexPrompts, UnclosedPrompt) {
    const auto result = lex("x = `abc\ndef");
    EXPECT_EQ(dump(result.tokens), "ID(x) OP(=) UNKNOWN(`abc\ndef) EOF");
    ASSERT_EQ(codes(result.diags), Codes{"LEX009"});
    EXPECT_EQ(result.diags.all()[0].message,
              "незакрытый промпт: ожидалась закрывающая `, встретился конец файла");
}

TEST(LexPrompts, UnclosedAfterSubstitution) {
    const auto result = lex("`a ${x} b");
    EXPECT_EQ(dump(result.tokens), "PROMPT(`a ${) ID(x) UNKNOWN(} b) EOF");
    EXPECT_EQ(codes(result.diags), Codes{"LEX009"});
}

TEST(LexPrompts, UnclosedSubstitution) {
    const auto result = lex("`a ${x");
    EXPECT_EQ(dump(result.tokens), "UNKNOWN(`a ${) ID(x) EOF");
    ASSERT_EQ(codes(result.diags), Codes{"LEX010"});
    const Diagnostic& d = result.diags.all()[0];
    EXPECT_EQ(d.message, "незакрытая подстановка в промпте: ожидалась '}', встретился конец файла");
    EXPECT_EQ(d.range.begin, (SourcePos{.line = 1, .column = 1, .offset = 0}));
    EXPECT_EQ(d.range.end, (SourcePos{.line = 1, .column = 6, .offset = 5}));
}

TEST(LexPrompts, UnclosedSubstitutionAfterInnerBraces) {
    const auto result = lex("`${ Point { x: 1 }");
    EXPECT_EQ(dump(result.tokens),
              "UNKNOWN(`${) ID(Point) DELIM({) ID(x) DELIM(:) INT(1) DELIM(}) EOF");
    EXPECT_EQ(codes(result.diags), Codes{"LEX010"});
}

TEST(LexPrompts, NestedUnclosedSubstitutions) {
    const auto result = lex("`a ${ `b ${ c");
    EXPECT_EQ(dump(result.tokens), "UNKNOWN(`a ${) UNKNOWN(`b ${) ID(c) EOF");
    EXPECT_EQ(codes(result.diags), (Codes{"LEX010", "LEX010"}));
}

TEST(LexPrompts, InvalidEscape) {
    const auto result = lex(R"(`line\n` `${x}\q` `a\$b`)");
    EXPECT_EQ(dump(result.tokens),
              R"(UNKNOWN(`line\n`) PROMPT(`${) ID(x) UNKNOWN(}\q`) UNKNOWN(`a\$b`) EOF)");
    ASSERT_EQ(codes(result.diags), (Codes{"LEX007", "LEX007", "LEX007"}));
    EXPECT_EQ(result.diags.all()[0].message,
              R"(недопустимая escape-последовательность '\n': ожидалось \`, \\ или \${)");
}

TEST(LexPrompts, ErrorsInsideSubstitutionKeepStructure) {
    const auto result = lex("`a ${ # } b`");
    EXPECT_EQ(dump(result.tokens), "PROMPT(`a ${) UNKNOWN(#) PROMPT(} b`) EOF");
    EXPECT_EQ(codes(result.diags), Codes{"LEX001"});
}

}  // namespace
}  // namespace vibe
