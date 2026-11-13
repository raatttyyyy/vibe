#pragma once

#include <string_view>
#include <vector>

#include "common/diagnostics.h"
#include "lexer/token.h"

namespace vibe {

// Последовательность всегда заканчивается ровно одной лексемой EndOfFile (LEX-1).
// На каждую лексему Unknown в diags приходит сообщение с кодом LEXnnn (LEX-8).
std::vector<Token> tokenize(std::string_view source, DiagnosticSink& diags);

}  // namespace vibe
