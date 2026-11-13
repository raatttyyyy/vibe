#pragma once

#include <string>
#include <string_view>

#include "common/source.h"

namespace vibe {

enum class TokenType {
    Keyword,     // ключевое слово, найдено бором
    Operator,    // + - * / % ++ -- = == != < <= > >= && || ! ,
    Literal,     // число, число с единицей, символ, строка, фрагмент промпта
    Identifier,  // id
    Delimiter,   // ( ) { } [ ] ; : .
    Unknown,     // лексическая ошибка
    EndOfFile    // служебная, всегда последняя
};

enum class LiteralKind { None, Int, Double, Char, String, Prompt, Unit };

struct Token {
    TokenType   type;
    LiteralKind literal = LiteralKind::None;  // заполнено только у Literal
    std::string lexeme;                       // точный текст из исходника
    SourceRange range;
};

std::string_view toString(TokenType type);
std::string_view toString(LiteralKind kind);

}  // namespace vibe
