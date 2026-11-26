#pragma once

#include <cstdint>
#include <map>
#include <span>
#include <string_view>
#include <vector>

namespace vibe {

// Бор над алфавитом идентификаторов [A-Za-z0-9_] (LEX-3).
// Лексер переходит по нему посимвольно, пока читает идентификатор.
class KeywordTrie {
public:
    using State = std::int32_t;

    static constexpr State kRoot = 0;
    static constexpr State kDead = -1;

    KeywordTrie();
    explicit KeywordTrie(std::span<const std::string_view> words);

    // Слово из символов вне алфавита — ошибка программиста: std::invalid_argument.
    void insert(std::string_view word);

    State next(State state, char c) const;
    bool isWord(State state) const;
    bool contains(std::string_view word) const;

private:
    struct Node {
        std::map<char, State> children;
        bool isWord = false;
    };

    std::vector<Node> nodes_;
};

std::span<const std::string_view> keywords();

const KeywordTrie& keywordTrie();

}  // namespace vibe
