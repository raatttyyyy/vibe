#include <gtest/gtest.h>

#include "lexer/token.h"

namespace vibe {
namespace {

// LEX-4: шесть типов с доски и служебный конец файла.
TEST(TokenType, NamesAllTypes) {
    EXPECT_EQ(toString(TokenType::Keyword), "Keyword");
    EXPECT_EQ(toString(TokenType::Operator), "Operator");
    EXPECT_EQ(toString(TokenType::Literal), "Literal");
    EXPECT_EQ(toString(TokenType::Identifier), "Identifier");
    EXPECT_EQ(toString(TokenType::Delimiter), "Delimiter");
    EXPECT_EQ(toString(TokenType::Unknown), "Unknown");
    EXPECT_EQ(toString(TokenType::EndOfFile), "EndOfFile");
}

TEST(LiteralKind, NamesAllKinds) {
    EXPECT_EQ(toString(LiteralKind::None), "None");
    EXPECT_EQ(toString(LiteralKind::Int), "Int");
    EXPECT_EQ(toString(LiteralKind::Double), "Double");
    EXPECT_EQ(toString(LiteralKind::Char), "Char");
    EXPECT_EQ(toString(LiteralKind::String), "String");
    EXPECT_EQ(toString(LiteralKind::Prompt), "Prompt");
    EXPECT_EQ(toString(LiteralKind::Unit), "Unit");
}

TEST(Token, LiteralKindDefaultsToNone) {
    Token token{.type = TokenType::Identifier, .lexeme = "a"};
    EXPECT_EQ(token.literal, LiteralKind::None);
}

}  // namespace
}  // namespace vibe
