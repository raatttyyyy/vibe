#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "common/diagnostics.h"
#include "lexer/lexer.h"

namespace vibe::test {

struct LexResult {
    std::vector<Token> tokens;
    DiagnosticSink diags;
};

inline LexResult lex(std::string_view source) {
    LexResult result;
    result.tokens = tokenize(source, result.diags);
    return result;
}

inline std::string tag(const Token& token) {
    switch (token.type) {
        case TokenType::Keyword:    return "KW";
        case TokenType::Operator:   return "OP";
        case TokenType::Identifier: return "ID";
        case TokenType::Delimiter:  return "DELIM";
        case TokenType::Unknown:    return "UNKNOWN";
        case TokenType::EndOfFile:  return "EOF";
        case TokenType::Literal:    break;
    }
    switch (token.literal) {
        case LiteralKind::Int:    return "INT";
        case LiteralKind::Double: return "DOUBLE";
        case LiteralKind::Char:   return "CHAR";
        case LiteralKind::String: return "STRING";
        case LiteralKind::Prompt: return "PROMPT";
        case LiteralKind::Unit:   return "UNIT";
        case LiteralKind::None:   break;
    }
    return "LITERAL?";
}

// "ID(a) OP(++) OP(+) ID(b) EOF" — запись из раздела 8 грамматики.
inline std::string dump(const std::vector<Token>& tokens) {
    std::string out;
    for (const Token& token : tokens) {
        if (!out.empty()) out += ' ';
        out += tag(token);
        if (token.type != TokenType::EndOfFile) {
            out += '(';
            out += token.lexeme;
            out += ')';
        }
    }
    return out;
}

inline std::string dump(std::string_view source) {
    return dump(lex(source).tokens);
}

inline std::vector<std::string> codes(const DiagnosticSink& diags) {
    std::vector<std::string> out;
    for (const Diagnostic& d : diags.all()) {
        out.push_back(d.code);
    }
    return out;
}

inline std::vector<std::string> codes(std::string_view source) {
    return codes(lex(source).diags);
}

}  // namespace vibe::test
