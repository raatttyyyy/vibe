#pragma once

namespace vibe {

constexpr bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

constexpr bool isIdentifierStart(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

constexpr bool isIdentifierChar(char c) {
    return isIdentifierStart(c) || isDigit(c);
}

}  // namespace vibe
