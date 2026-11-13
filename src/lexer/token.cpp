#include "lexer/token.h"

namespace vibe {

std::string_view toString(TokenType type) {
    switch (type) {
        case TokenType::Keyword:    return "Keyword";
        case TokenType::Operator:   return "Operator";
        case TokenType::Literal:    return "Literal";
        case TokenType::Identifier: return "Identifier";
        case TokenType::Delimiter:  return "Delimiter";
        case TokenType::Unknown:    return "Unknown";
        case TokenType::EndOfFile:  return "EndOfFile";
    }
    return "?";
}

std::string_view toString(LiteralKind kind) {
    switch (kind) {
        case LiteralKind::None:   return "None";
        case LiteralKind::Int:    return "Int";
        case LiteralKind::Double: return "Double";
        case LiteralKind::Char:   return "Char";
        case LiteralKind::String: return "String";
        case LiteralKind::Prompt: return "Prompt";
        case LiteralKind::Unit:   return "Unit";
    }
    return "?";
}

}  // namespace vibe
