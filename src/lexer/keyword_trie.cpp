#include "lexer/keyword_trie.h"

#include <stdexcept>
#include <string>

#include "lexer/identifier_chars.h"

namespace vibe {
namespace {

constexpr std::string_view kKeywords[] = {
    "import", "func", "struct", "enum", "void", "int", "double", "bool", "char", "string",
    "list", "map", "future",
    "if", "else", "while", "do", "for", "break", "continue", "goto", "return", "true", "false",
    "model", "agent", "task", "on", "context", "use", "hide", "attempt", "ensure", "spawn",
    "await", "ask",
    "tools", "budget", "space", "system", "worktree",
};

}  // namespace

KeywordTrie::KeywordTrie() : nodes_(1) {}

KeywordTrie::KeywordTrie(std::span<const std::string_view> words) : KeywordTrie() {
    for (std::string_view word : words) {
        insert(word);
    }
}

void KeywordTrie::insert(std::string_view word) {
    if (word.empty()) {
        throw std::invalid_argument("keyword must not be empty");
    }
    State state = kRoot;
    for (char c : word) {
        if (!isIdentifierChar(c)) {
            throw std::invalid_argument("keyword has a character outside [A-Za-z0-9_]: " +
                                        std::string(word));
        }
        State child = next(state, c);
        if (child == kDead) {
            child = static_cast<State>(nodes_.size());
            nodes_.emplace_back();
            nodes_[state].children.emplace(c, child);
        }
        state = child;
    }
    nodes_[state].isWord = true;
}

KeywordTrie::State KeywordTrie::next(State state, char c) const {
    if (state == kDead) return kDead;
    const auto& children = nodes_[state].children;
    const auto it = children.find(c);
    return it == children.end() ? kDead : it->second;
}

bool KeywordTrie::isWord(State state) const {
    return state != kDead && nodes_[state].isWord;
}

bool KeywordTrie::contains(std::string_view word) const {
    State state = kRoot;
    for (char c : word) {
        state = next(state, c);
    }
    return isWord(state);
}

std::span<const std::string_view> keywords() {
    return kKeywords;
}

const KeywordTrie& keywordTrie() {
    static const KeywordTrie trie(keywords());
    return trie;
}

}  // namespace vibe
