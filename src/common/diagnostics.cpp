#include "common/diagnostics.h"

#include <utility>

namespace vibe {

void DiagnosticSink::report(Diagnostic d) {
    if (d.severity == Severity::Error) {
        hasErrors_ = true;
    }
    diagnostics_.push_back(std::move(d));
}

bool DiagnosticSink::hasErrors() const {
    return hasErrors_;
}

const std::vector<Diagnostic>& DiagnosticSink::all() const {
    return diagnostics_;
}

std::string_view severityName(Severity severity) {
    switch (severity) {
        case Severity::Error:   return "ошибка";
        case Severity::Warning: return "предупреждение";
        case Severity::Note:    return "примечание";
    }
    return "?";
}

std::string formatDiagnostic(std::string_view fileName, const Diagnostic& d) {
    std::string out;
    out += fileName;
    out += ':';
    out += std::to_string(d.range.begin.line);
    out += ':';
    out += std::to_string(d.range.begin.column);
    out += ": ";
    out += severityName(d.severity);
    out += ' ';
    out += d.code;
    out += ": ";
    out += d.message;
    return out;
}

}  // namespace vibe
