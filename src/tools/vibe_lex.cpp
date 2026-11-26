// vibe-lex <файл.vibe> — печатает лексемы с позициями и типами, ошибки пишет в stderr.
// Код возврата: 0 — без ошибок, 1 — есть лексические ошибки, 2 — неверный вызов или файл.

#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#include "common/diagnostics.h"
#include "lexer/lexer.h"

namespace {

// Лексема печатается в одну строку: управляющие символы заменяются escape-записью.
std::string singleLine(std::string_view text) {
    static constexpr char kHex[] = "0123456789ABCDEF";
    std::string out;
    for (char c : text) {
        const auto byte = static_cast<unsigned char>(c);
        if (c == '\n') {
            out += "\\n";
        } else if (c == '\r') {
            out += "\\r";
        } else if (c == '\t') {
            out += "\\t";
        } else if (byte < 0x20 || byte == 0x7F) {
            out += "\\x";
            out += kHex[byte >> 4];
            out += kHex[byte & 0xF];
        } else {
            out += c;
        }
    }
    return out;
}

std::string typeName(const vibe::Token& token) {
    std::string name(vibe::toString(token.type));
    if (token.type == vibe::TokenType::Literal) {
        name += '(';
        name += vibe::toString(token.literal);
        name += ')';
    }
    return name;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "использование: vibe-lex <файл.vibe>\n";
        return 2;
    }
    const std::string path = argv[1];
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "vibe-lex: не удалось открыть файл '" << path << "'\n";
        return 2;
    }
    const std::string source{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};

    vibe::DiagnosticSink diags;
    const std::vector<vibe::Token> tokens = vibe::tokenize(source, diags);

    for (const vibe::Token& token : tokens) {
        const vibe::SourcePos& begin = token.range.begin;
        const std::string position = std::to_string(begin.line) + ":" + std::to_string(begin.column);
        std::cout << std::left << std::setw(10) << position << std::setw(17) << typeName(token)
                  << singleLine(token.lexeme) << '\n';
    }
    for (const vibe::Diagnostic& d : diags.all()) {
        std::cerr << vibe::formatDiagnostic(path, d) << '\n';
    }
    return diags.hasErrors() ? 1 : 0;
}
