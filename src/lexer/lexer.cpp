#include "lexer/lexer.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

#include "lexer/keyword_trie.h"

namespace vibe {
namespace {

constexpr std::string_view kInvalidCharacter = "LEX001";
constexpr std::string_view kIncompleteOperator = "LEX002";
constexpr std::string_view kMalformedNumber = "LEX003";
constexpr std::string_view kUnclosedComment = "LEX008";

constexpr std::string_view kUnits[] = {"usd", "tok", "ktok", "s", "min"};

bool isDigit(char c) { return c >= '0' && c <= '9'; }
bool isLetter(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
bool isIdentifierStart(char c) { return isLetter(c) || c == '_'; }
bool isIdentifierChar(char c) { return isIdentifierStart(c) || isDigit(c); }
bool isLineBreak(char c) { return c == '\n' || c == '\r'; }
bool isWhitespace(char c) { return c == ' ' || c == '\t' || isLineBreak(c); }

bool startsToken(char c) {
    if (isIdentifierChar(c)) return true;
    switch (c) {
        case '(': case ')': case '[': case ']': case '{': case '}':
        case ';': case ':': case '.':
        case '+': case '-': case '*': case '/': case '%': case '=': case '!':
        case '<': case '>': case '&': case '|': case ',':
        case '\'': case '"': case '`':
            return true;
        default:
            return false;
    }
}

// Длина корректной последовательности UTF-8 в text[i], 0 — если она испорчена.
std::size_t utf8SequenceLength(std::string_view text, std::size_t i) {
    const auto byte = [&](std::size_t k) { return static_cast<unsigned char>(text[k]); };
    const unsigned char lead = byte(i);
    if (lead < 0x80) return 1;

    std::size_t length = 0;
    unsigned char low = 0x80;
    unsigned char high = 0xBF;
    if (lead >= 0xC2 && lead <= 0xDF) {
        length = 2;
    } else if (lead == 0xE0) {
        length = 3;
        low = 0xA0;
    } else if ((lead >= 0xE1 && lead <= 0xEC) || lead == 0xEE || lead == 0xEF) {
        length = 3;
    } else if (lead == 0xED) {
        length = 3;
        high = 0x9F;
    } else if (lead == 0xF0) {
        length = 4;
        low = 0x90;
    } else if (lead >= 0xF1 && lead <= 0xF3) {
        length = 4;
    } else if (lead == 0xF4) {
        length = 4;
        high = 0x8F;
    } else {
        return 0;
    }

    if (i + length > text.size()) return 0;
    if (byte(i + 1) < low || byte(i + 1) > high) return 0;
    for (std::size_t k = 2; k < length; ++k) {
        if (byte(i + k) < 0x80 || byte(i + k) > 0xBF) return 0;
    }
    return length;
}

// Текст для сообщения: управляющие символы и испорченный UTF-8 записываются как \xNN.
std::string printable(std::string_view text) {
    static constexpr char kHex[] = "0123456789ABCDEF";
    std::string out;
    std::size_t i = 0;
    while (i < text.size()) {
        const auto c = static_cast<unsigned char>(text[i]);
        const std::size_t length = utf8SequenceLength(text, i);
        if (c == '\n') {
            out += "\\n";
        } else if (c == '\r') {
            out += "\\r";
        } else if (c == '\t') {
            out += "\\t";
        } else if (length == 0 || c < 0x20 || c == 0x7F) {
            out += "\\x";
            out += kHex[c >> 4];
            out += kHex[c & 0xF];
        } else {
            out.append(text.substr(i, length));
            i += length;
            continue;
        }
        ++i;
    }
    return out;
}

std::string quoted(std::string_view text) {
    return "'" + printable(text) + "'";
}

bool isUnit(std::string_view suffix) {
    for (std::string_view unit : kUnits) {
        if (suffix == unit) return true;
    }
    return false;
}

// Конечный автомат, закодированный переходами в коде: начальное состояние —
// scanToken(), который по первому символу выбирает ветку автомата.
// Каждый символ читается один раз, поэтому время линейно (LEX-2).
class Lexer {
public:
    Lexer(std::string_view source, DiagnosticSink& diags) : source_(source), diags_(diags) {}

    std::vector<Token> run() {
        while (true) {
            skipWhitespaceAndComments();
            if (atEnd()) break;
            scanToken();
        }
        tokens_.push_back(Token{.type = TokenType::EndOfFile, .range = {pos_, pos_}});
        return std::move(tokens_);
    }

private:
    bool atEnd() const { return pos_.offset >= source_.size(); }

    char peek(std::size_t ahead = 0) const {
        const std::size_t i = pos_.offset + ahead;
        return i < source_.size() ? source_[i] : '\0';
    }

    // Переходит через один символ: код Unicode, испорченный байт или перевод строки (CRLF — один).
    void advance() {
        const char c = source_[pos_.offset];
        if (isLineBreak(c)) {
            ++pos_.offset;
            if (c == '\r' && !atEnd() && source_[pos_.offset] == '\n') {
                ++pos_.offset;
            }
            ++pos_.line;
            pos_.column = 1;
            return;
        }
        const std::size_t length = utf8SequenceLength(source_, pos_.offset);
        pos_.offset += static_cast<std::uint32_t>(length == 0 ? 1 : length);
        ++pos_.column;
    }

    void beginToken() { start_ = pos_; }

    std::string_view currentText() const {
        return source_.substr(start_.offset, pos_.offset - start_.offset);
    }

    Token& emit(TokenType type, LiteralKind literal = LiteralKind::None) {
        tokens_.push_back(Token{
            .type = type,
            .literal = literal,
            .lexeme = std::string(currentText()),
            .range = {start_, pos_},
        });
        return tokens_.back();
    }

    void emitUnknown(std::string_view code, std::string message) {
        const SourceRange range = emit(TokenType::Unknown).range;
        reportError(code, std::move(message), range);
    }

    void reportError(std::string_view code, std::string message, SourceRange range) {
        diags_.report(Diagnostic{
            .severity = Severity::Error,
            .stage = Stage::Lexer,
            .code = std::string(code),
            .message = std::move(message),
            .range = range,
        });
    }

    void skipWhitespaceAndComments() {
        while (!atEnd()) {
            const char c = peek();
            if (isWhitespace(c)) {
                advance();
            } else if (c == '/' && peek(1) == '/') {
                skipLineComment();
            } else if (c == '/' && peek(1) == '*') {
                skipBlockComment();
            } else {
                return;
            }
        }
    }

    void skipLineComment() {
        while (!atEnd() && !isLineBreak(peek())) {
            advance();
        }
    }

    void skipBlockComment() {
        beginToken();
        advance();
        advance();
        while (!atEnd()) {
            if (peek() == '*' && peek(1) == '/') {
                advance();
                advance();
                return;
            }
            advance();
        }
        emitUnknown(kUnclosedComment,
                    "незакрытый комментарий: ожидалось '*/', встретился конец файла");
    }

    void scanToken() {
        beginToken();
        const char c = peek();
        if (isIdentifierStart(c)) {
            scanWord();
            return;
        }
        if (isDigit(c)) {
            scanNumber();
            return;
        }
        switch (c) {
            case '(': case ')': case '[': case ']': case '{': case '}':
            case ';': case ':': case '.':
                advance();
                emit(TokenType::Delimiter);
                return;
            case '*': case '/': case '%': case ',':
                advance();
                emit(TokenType::Operator);
                return;
            case '+': scanOperator('+'); return;
            case '-': scanOperator('-'); return;
            case '=': scanOperator('='); return;
            case '!': scanOperator('='); return;
            case '<': scanOperator('='); return;
            case '>': scanOperator('='); return;
            case '&': scanDoubledOperator('&'); return;
            case '|': scanDoubledOperator('|'); return;
            default:
                scanInvalidCharacters();
                return;
        }
    }

    // Самая длинная лексема (LEX-6): первый символ, затем second, если он следует.
    void scanOperator(char second) {
        advance();
        if (peek() == second) {
            advance();
        }
        emit(TokenType::Operator);
    }

    // && и ||: одиночный символ операцией не является.
    void scanDoubledOperator(char c) {
        advance();
        if (peek() == c) {
            advance();
            emit(TokenType::Operator);
            return;
        }
        const std::string doubled(2, c);
        emitUnknown(kIncompleteOperator, "неполная операция " + quoted(std::string(1, c)) +
                                             ": ожидалось " + quoted(doubled));
    }

    // Максимальный идентификатор, по бору одновременно решается, ключевое ли это слово (LEX-3).
    void scanWord() {
        const KeywordTrie& trie = keywordTrie();
        KeywordTrie::State state = KeywordTrie::kRoot;
        while (!atEnd() && isIdentifierChar(peek())) {
            state = trie.next(state, peek());
            advance();
        }
        emit(trie.isWord(state) ? TokenType::Keyword : TokenType::Identifier);
    }

    enum class NumberState {
        Integer,       // 12
        Fraction,      // 12.5
        ExponentMark,  // 12e
        ExponentSign,  // 12e-
        Exponent,      // 12e-3
        Suffix,        // 12usd
        Malformed,     // 3.5.7, 35k6: дочитывается весь ошибочный фрагмент
    };

    // INT, DOUBLE и UNIT. Точка без следующей цифры остаётся разделителем.
    void scanNumber() {
        NumberState state = NumberState::Integer;
        LiteralKind kind = LiteralKind::Int;
        std::size_t suffixBegin = 0;
        char exponentMark = 'e';
        std::string reason;

        while (true) {
            const char c = peek();
            const bool dotThenDigit = c == '.' && isDigit(peek(1));
            switch (state) {
                case NumberState::Integer:
                case NumberState::Fraction:
                case NumberState::Exponent:
                    if (isDigit(c)) {
                        advance();
                    } else if (dotThenDigit && state == NumberState::Integer) {
                        advance();
                        kind = LiteralKind::Double;
                        state = NumberState::Fraction;
                    } else if (dotThenDigit) {
                        reason = state == NumberState::Fraction
                                     ? "после дробной части ожидался конец числа, встретилась "
                                       "вторая точка"
                                     : "порядок должен быть целым, встретилась точка";
                        state = NumberState::Malformed;
                    } else if ((c == 'e' || c == 'E') && state != NumberState::Exponent) {
                        exponentMark = c;
                        advance();
                        kind = LiteralKind::Double;
                        state = NumberState::ExponentMark;
                    } else if (isIdentifierStart(c)) {
                        suffixBegin = pos_.offset;
                        state = NumberState::Suffix;
                    } else {
                        emit(TokenType::Literal, kind);
                        return;
                    }
                    break;

                case NumberState::ExponentMark:
                case NumberState::ExponentSign:
                    if (isDigit(c)) {
                        state = NumberState::Exponent;
                    } else if ((c == '+' || c == '-') && state == NumberState::ExponentMark) {
                        advance();
                        state = NumberState::ExponentSign;
                    } else {
                        reason = "после " + quoted(std::string(1, exponentMark)) +
                                 " ожидались цифры порядка";
                        state = NumberState::Malformed;
                    }
                    break;

                case NumberState::Suffix: {
                    while (isIdentifierChar(peek())) {
                        advance();
                    }
                    const std::string_view suffix =
                        source_.substr(suffixBegin, pos_.offset - suffixBegin);
                    if (!isUnit(suffix)) {
                        reason = "ожидалась единица usd, tok, ktok, s или min, встретилось " +
                                 quoted(suffix);
                        state = NumberState::Malformed;
                    } else if (peek() == '.' && isDigit(peek(1))) {
                        reason = "после единицы ожидался конец числа, встретилась точка";
                        state = NumberState::Malformed;
                    } else {
                        emit(TokenType::Literal, LiteralKind::Unit);
                        return;
                    }
                    break;
                }

                case NumberState::Malformed:
                    if (isIdentifierChar(c) || dotThenDigit) {
                        advance();
                        break;
                    }
                    emitUnknown(kMalformedNumber, "неверно записанное число " +
                                                      quoted(currentText()) + ": " + reason);
                    return;
            }
        }
    }

    // Подряд идущие недопустимые символы дают одну лексему Unknown.
    void scanInvalidCharacters() {
        std::size_t count = 0;
        do {
            advance();
            ++count;
        } while (!atEnd() && !isWhitespace(peek()) && !startsToken(peek()));
        const std::string text = quoted(currentText());
        emitUnknown(kInvalidCharacter, count == 1 ? "недопустимый символ " + text
                                                  : "недопустимые символы " + text);
    }

    std::string_view source_;
    DiagnosticSink& diags_;
    SourcePos pos_;
    SourcePos start_;
    std::vector<Token> tokens_;
};

}  // namespace

std::vector<Token> tokenize(std::string_view source, DiagnosticSink& diags) {
    return Lexer(source, diags).run();
}

}  // namespace vibe
