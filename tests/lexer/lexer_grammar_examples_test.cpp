#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <vector>

#include "lexer_test_util.h"

namespace vibe {
namespace {

using test::codes;
using test::dump;
using test::lex;
using Codes = std::vector<std::string>;

// Раздел 8 грамматики, «Классическая программа».
constexpr std::string_view kClassicProgram = R"vibe(struct Point { double x; double y; }
enum State { Ready, Done }

func int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

func void main() {
    int a = readInt();
    int b = 3;
    double ratio = 2.5;
    bool ready = true;
    char separator = ':';
    string title = "Result";
    map<int, int> values = map { 0: 1, 1: 2, 2: 3 };
    map<string, int> counts = map { "ready": 1 };
    Point p = Point { x: 0.5, y: ratio };
    State s = State.Ready;
    if (a > b) a = a - b;
    else b = a;
    a = b = 3;
    for (int i = 0; i < 3; i++) {
        if (i == 1) continue;
        values[i] = factorial(i + 1);
    }
    while (a > 0) { a--; if (a == 1) break; }
    do { ++a; } while (a < 2);
    if (!ready) goto finish;
    print((counts["ready"], values[2]));
    print(p.x);
    finish: print(title);
}
)vibe";

// Раздел 8 грамматики, «Агентная программа».
constexpr std::string_view kAgentProgram = R"vibe(import fs;
import sh;
import ci;
import gh;

struct Verdict { bool ok; string reason; }
model fast { id: "small-model"; }
model strong { id: "large-model"; }

agent Fixer {
    model: fast else strong;
    tools: (fs.read("AGENTS.md"), fs.write("src/**"), sh.command("npm test"));
    budget: 2usd, 20min, 8ktok;
    space: worktree;
}
agent Judge {
    model: strong;
    tools: (fs.read("AGENTS.md"), fs.read("src/**"));
    budget: 1usd, 2min, 4ktok;
}

task Patch fix(CiRun run) {
    context {
        use fs.file("AGENTS.md");
        use run.log;
        attempt (3) {
            Patch patch = ask<Patch>(Fixer, `Исправь сбой на шаге ${run.step}.`);
            ensure sh.run("npm test").ok : "Тесты не прошли";
            context {
                hide run.log;
                Verdict result = ask<Verdict>(Judge, `Оцени патч ${patch.diff}.`);
                ensure result.ok : result.reason;
            }
            return patch;
        }
    }
}

on ci.failed(CiRun run) {
    future<Patch> pending = spawn fix(run);
    Patch patch = await pending;
    gh.open_pr(patch);
}
)vibe";

std::size_t countType(const std::vector<Token>& tokens, TokenType type) {
    std::size_t count = 0;
    for (const Token& token : tokens) {
        if (token.type == type) ++count;
    }
    return count;
}

std::string dumpLine(std::string_view program, std::uint32_t line) {
    std::vector<Token> selected;
    for (const Token& token : lex(program).tokens) {
        if (token.range.begin.line == line && token.type != TokenType::EndOfFile) {
            selected.push_back(token);
        }
    }
    return dump(selected);
}

// G-7: правильная классическая программа лексически корректна.
TEST(LexGrammarExamples, ClassicProgramHasNoErrors) {
    const auto result = lex(kClassicProgram);
    EXPECT_FALSE(result.diags.hasErrors());
    EXPECT_EQ(countType(result.tokens, TokenType::Unknown), 0u);
    EXPECT_EQ(countType(result.tokens, TokenType::EndOfFile), 1u);
    EXPECT_EQ(result.tokens.back().type, TokenType::EndOfFile);
}

TEST(LexGrammarExamples, ClassicProgramLines) {
    EXPECT_EQ(dumpLine(kClassicProgram, 4),
              "KW(func) KW(int) ID(factorial) DELIM(() KW(int) ID(n) DELIM()) DELIM({)");
    EXPECT_EQ(dumpLine(kClassicProgram, 14), "KW(char) ID(separator) OP(=) CHAR(':') DELIM(;)");
    EXPECT_EQ(dumpLine(kClassicProgram, 16),
              "KW(map) OP(<) KW(int) OP(,) KW(int) OP(>) ID(values) OP(=) KW(map) DELIM({) "
              "INT(0) DELIM(:) INT(1) OP(,) INT(1) DELIM(:) INT(2) OP(,) INT(2) DELIM(:) INT(3) "
              "DELIM(}) DELIM(;)");
    EXPECT_EQ(dumpLine(kClassicProgram, 23),
              "KW(for) DELIM(() KW(int) ID(i) OP(=) INT(0) DELIM(;) ID(i) OP(<) INT(3) DELIM(;) "
              "ID(i) OP(++) DELIM()) DELIM({)");
    EXPECT_EQ(dumpLine(kClassicProgram, 28),
              "KW(do) DELIM({) OP(++) ID(a) DELIM(;) DELIM(}) KW(while) DELIM(() ID(a) OP(<) "
              "INT(2) DELIM()) DELIM(;)");
    EXPECT_EQ(dumpLine(kClassicProgram, 32), "ID(finish) DELIM(:) ID(print) DELIM(() ID(title) "
                                             "DELIM()) DELIM(;)");
}

// G-7: правильная агентная программа лексически корректна.
TEST(LexGrammarExamples, AgentProgramHasNoErrors) {
    const auto result = lex(kAgentProgram);
    EXPECT_FALSE(result.diags.hasErrors());
    EXPECT_EQ(countType(result.tokens, TokenType::Unknown), 0u);
    EXPECT_EQ(result.tokens.back().type, TokenType::EndOfFile);
}

TEST(LexGrammarExamples, AgentProgramLines) {
    EXPECT_EQ(dumpLine(kAgentProgram, 1), "KW(import) ID(fs) DELIM(;)");
    EXPECT_EQ(dumpLine(kAgentProgram, 11),
              "KW(model) DELIM(:) ID(fast) KW(else) ID(strong) DELIM(;)");
    EXPECT_EQ(dumpLine(kAgentProgram, 13),
              "KW(budget) DELIM(:) UNIT(2usd) OP(,) UNIT(20min) OP(,) UNIT(8ktok) DELIM(;)");
    EXPECT_EQ(dumpLine(kAgentProgram, 14), "KW(space) DELIM(:) KW(worktree) DELIM(;)");
    EXPECT_EQ(dumpLine(kAgentProgram, 26),
              "KW(attempt) DELIM(() INT(3) DELIM()) DELIM({)");
    EXPECT_EQ(dumpLine(kAgentProgram, 27),
              "ID(Patch) ID(patch) OP(=) KW(ask) OP(<) ID(Patch) OP(>) DELIM(() ID(Fixer) OP(,) "
              "PROMPT(`Исправь сбой на шаге ${) ID(run) DELIM(.) ID(step) PROMPT(}.`) DELIM()) "
              "DELIM(;)");
    EXPECT_EQ(dumpLine(kAgentProgram, 28),
              "KW(ensure) ID(sh) DELIM(.) ID(run) DELIM(() STRING(\"npm test\") DELIM()) DELIM(.) "
              "ID(ok) DELIM(:) STRING(\"Тесты не прошли\") DELIM(;)");
    EXPECT_EQ(dumpLine(kAgentProgram, 40),
              "KW(future) OP(<) ID(Patch) OP(>) ID(pending) OP(=) KW(spawn) ID(fix) DELIM(() "
              "ID(run) DELIM()) DELIM(;)");
    EXPECT_EQ(dumpLine(kAgentProgram, 41),
              "ID(Patch) ID(patch) OP(=) KW(await) ID(pending) DELIM(;)");
}

TEST(LexGrammarExamples, ParallelTasksExample) {
    const auto result = lex("future<int> first = spawn count(\"a\"); print(await first + await second);");
    EXPECT_FALSE(result.diags.hasErrors());
}

// Раздел 8 грамматики, «Контрольные фрагменты» с точными последовательностями лексем.
TEST(LexGrammarExamples, ControlFragmentsTokens) {
    EXPECT_EQ(dump("a+++b"), "ID(a) OP(++) OP(+) ID(b) EOF");
    EXPECT_EQ(dump("func funcName"), "KW(func) ID(funcName) EOF");
    EXPECT_EQ(dump("3.5.7"), "UNKNOWN(3.5.7) EOF");
    EXPECT_EQ(codes("if (a > #b)"), Codes{"LEX001"});
    EXPECT_EQ(lex("if (a > #b)").tokens.back().type, TokenType::EndOfFile);
}

// LEX-7: фрагменты, которые отклоняют парсер и семантика, лексически допустимы.
TEST(LexGrammarExamples, FragmentsRejectedLaterAreLexicallyValid) {
    for (std::string_view fragment : {
             "a = if : b",
             "if (a) if (b) x = 1; else x = 2;",
             "int f() { return 1; }",
             "[1, 2]", "[]", "values[]",
             "int x = ;", "void x;",
             "struct Point { double x }",
             "do x++; while (ok)", "for (i=0 i<3; i++) {}",
             "ask<Verdict>(Judge, )", "spawn fix;",
             "(a + b) = 1;", "if (42) return;",
             "break;", "goto unknown;",
             "use file;", "spawn ordinary_function();",
             "agent A { budget: 1s, 2min; }",
         }) {
        const auto result = lex(fragment);
        EXPECT_FALSE(result.diags.hasErrors()) << fragment;
        EXPECT_EQ(countType(result.tokens, TokenType::Unknown), 0u) << fragment;
    }
}

// LEX-2: один проход по большому входу; ошибочные фрагменты не порождают повторного чтения.
TEST(LexLinear, LargeInput) {
    constexpr int kCopies = 500;
    std::string source;
    for (int i = 0; i < kCopies; ++i) source += kClassicProgram;
    const auto single = lex(kClassicProgram);
    const auto result = lex(source);
    EXPECT_FALSE(result.diags.hasErrors());
    EXPECT_EQ(result.tokens.size(), (single.tokens.size() - 1) * kCopies + 1);
    EXPECT_EQ(result.tokens.back().range.begin.line, 33u * kCopies + 1);
}

TEST(LexLinear, LongErroneousFragments) {
    std::string number = "1";
    for (int i = 0; i < 100000; ++i) number += ".1";
    const std::string invalid(100000, '#');
    const auto result = lex(number + " " + invalid + " /*" + std::string(100000, '*'));
    EXPECT_EQ(result.tokens.size(), 4u);
    EXPECT_EQ(codes(result.diags), (Codes{"LEX003", "LEX001", "LEX008"}));
}

}  // namespace
}  // namespace vibe
