#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "common/source.h"

namespace vibe {

enum class Severity { Error, Warning, Note };
enum class Stage    { Lexer, Parser, Sema, Codegen };

struct Diagnostic {
    Severity    severity;
    Stage       stage;
    std::string code;     // "LEX001", "SYN014", "SEM003"
    std::string message;
    SourceRange range;
};

class DiagnosticSink {
public:
    void report(Diagnostic d);
    bool hasErrors() const;
    const std::vector<Diagnostic>& all() const;

private:
    std::vector<Diagnostic> diagnostics_;
    bool hasErrors_ = false;
};

std::string_view severityName(Severity severity);

// "main.vibe:3:14: ошибка LEX001: недопустимый символ '#'" (N-6)
std::string formatDiagnostic(std::string_view fileName, const Diagnostic& d);

}  // namespace vibe
