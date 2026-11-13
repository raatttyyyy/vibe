#include <gtest/gtest.h>

#include <stdexcept>
#include <string_view>

#include "lexer/keyword_trie.h"

namespace vibe {
namespace {

// LEX-3: список ключевых слов из раздела 2 грамматики.
TEST(Keywords, MatchGrammarList) {
    const std::string_view expected[] = {
        "import", "func", "struct", "enum", "void", "int", "double", "bool", "char", "string",
        "list", "map", "future",
        "if", "else", "while", "do", "for", "break", "continue", "goto", "return", "true",
        "false",
        "model", "agent", "task", "on", "context", "use", "hide", "attempt", "ensure", "spawn",
        "await", "ask",
        "tools", "budget", "space", "system", "worktree",
    };
    const auto actual = keywords();
    ASSERT_EQ(actual.size(), std::size(expected));
    for (std::size_t i = 0; i < actual.size(); ++i) {
        EXPECT_EQ(actual[i], expected[i]);
    }
}

// LEX-3: бор находит каждое ключевое слово, в том числе func.
TEST(KeywordTrie, ContainsEveryKeyword) {
    for (std::string_view word : keywords()) {
        EXPECT_TRUE(keywordTrie().contains(word)) << word;
    }
    EXPECT_TRUE(keywordTrie().contains("func"));
}

// LEX-3: префиксы, продолжения и другой регистр — не ключевые слова.
TEST(KeywordTrie, RejectsNonKeywords) {
    for (std::string_view word : {"", "f", "fun", "funcs", "funcName", "i", "in", "integer",
                                  "_if", "If", "IF", "ifelse", "do_", "main", "print",
                                  "readInt", "tok", "usd", "worktrees", "a"}) {
        EXPECT_FALSE(keywordTrie().contains(word)) << word;
    }
}

TEST(KeywordTrie, StepsCharacterByCharacter) {
    const KeywordTrie& trie = keywordTrie();
    KeywordTrie::State state = KeywordTrie::kRoot;
    state = trie.next(state, 'd');
    EXPECT_FALSE(trie.isWord(state));
    state = trie.next(state, 'o');
    EXPECT_TRUE(trie.isWord(state));  // do
    state = trie.next(state, 'u');
    EXPECT_FALSE(trie.isWord(state));
    state = trie.next(state, 'b');
    state = trie.next(state, 'l');
    state = trie.next(state, 'e');
    EXPECT_TRUE(trie.isWord(state));  // double
}

TEST(KeywordTrie, DeadStateIsAbsorbing) {
    const KeywordTrie& trie = keywordTrie();
    KeywordTrie::State state = trie.next(KeywordTrie::kRoot, 'x');
    EXPECT_EQ(state, KeywordTrie::kDead);
    EXPECT_EQ(trie.next(state, 'i'), KeywordTrie::kDead);
    EXPECT_FALSE(trie.isWord(state));
    EXPECT_EQ(trie.next(KeywordTrie::kRoot, '#'), KeywordTrie::kDead);
}

TEST(KeywordTrie, CustomWords) {
    const std::string_view words[] = {"ab", "abc"};
    KeywordTrie trie(words);
    EXPECT_TRUE(trie.contains("ab"));
    EXPECT_TRUE(trie.contains("abc"));
    EXPECT_FALSE(trie.contains("a"));
    EXPECT_FALSE(trie.contains("abcd"));
}

TEST(KeywordTrie, RejectsInvalidWords) {
    KeywordTrie trie;
    EXPECT_THROW(trie.insert(""), std::invalid_argument);
    EXPECT_THROW(trie.insert("a-b"), std::invalid_argument);
}

}  // namespace
}  // namespace vibe
