#include "lexer/keyword_trie.h"

#include <stdexcept>
#include <string>

namespace vibe {
namespace {

int alphabetIndex(char c) {
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c >= 'A' && c <= 'Z') return 26 + (c - 'A');
    if (c >= '0' && c <= '9') return 52 + (c - '0');
    if (c == '_') return 62;
    return -1;
}

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
        const int index = alphabetIndex(c);
        if (index < 0) {
            throw std::invalid_argument("keyword has a character outside [A-Za-z0-9_]: " +
                                        std::string(word));
        }
        State child = nodes_[state].children[index];
        if (child == kDead) {
            child = static_cast<State>(nodes_.size());
            nodes_[state].children[index] = child;
            nodes_.emplace_back();
        }
        state = child;
    }
    nodes_[state].isWord = true;
}

KeywordTrie::State KeywordTrie::next(State state, char c) const {
    if (state == kDead) return kDead;
    const int index = alphabetIndex(c);
    if (index < 0) return kDead;
    return nodes_[state].children[index];
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
