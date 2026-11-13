#include <gtest/gtest.h>

#include "common/diagnostics.h"

namespace vibe {
namespace {

Diagnostic makeDiagnostic(Severity severity) {
    return Diagnostic{
        .severity = severity,
        .stage = Stage::Lexer,
        .code = "LEX001",
        .message = "недопустимый символ '#'",
        .range = {.begin = {.line = 3, .column = 14, .offset = 40},
                  .end = {.line = 3, .column = 15, .offset = 41}},
    };
}

TEST(DiagnosticSink, StartsEmpty) {
    DiagnosticSink sink;
    EXPECT_FALSE(sink.hasErrors());
    EXPECT_TRUE(sink.all().empty());
}

TEST(DiagnosticSink, ErrorSetsHasErrors) {
    DiagnosticSink sink;
    sink.report(makeDiagnostic(Severity::Error));
    EXPECT_TRUE(sink.hasErrors());
    ASSERT_EQ(sink.all().size(), 1u);
    EXPECT_EQ(sink.all()[0].code, "LEX001");
}

TEST(DiagnosticSink, WarningsAndNotesAreNotErrors) {
    DiagnosticSink sink;
    sink.report(makeDiagnostic(Severity::Warning));
    sink.report(makeDiagnostic(Severity::Note));
    EXPECT_FALSE(sink.hasErrors());
    EXPECT_EQ(sink.all().size(), 2u);
}

TEST(DiagnosticSink, KeepsReportOrder) {
    DiagnosticSink sink;
    Diagnostic first = makeDiagnostic(Severity::Error);
    Diagnostic second = makeDiagnostic(Severity::Error);
    second.code = "LEX002";
    sink.report(first);
    sink.report(second);
    ASSERT_EQ(sink.all().size(), 2u);
    EXPECT_EQ(sink.all()[0].code, "LEX001");
    EXPECT_EQ(sink.all()[1].code, "LEX002");
}

// N-6: файл, строка, столбец, код ошибки, текст.
TEST(FormatDiagnostic, UsesCommonFormat) {
    EXPECT_EQ(formatDiagnostic("main.vibe", makeDiagnostic(Severity::Error)),
              "main.vibe:3:14: ошибка LEX001: недопустимый символ '#'");
}

TEST(FormatDiagnostic, NamesSeverity) {
    EXPECT_EQ(formatDiagnostic("a.vibe", makeDiagnostic(Severity::Warning)),
              "a.vibe:3:14: предупреждение LEX001: недопустимый символ '#'");
    EXPECT_EQ(formatDiagnostic("a.vibe", makeDiagnostic(Severity::Note)),
              "a.vibe:3:14: примечание LEX001: недопустимый символ '#'");
}

}  // namespace
}  // namespace vibe
