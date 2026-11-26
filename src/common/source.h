#pragma once

#include <cstdint>

namespace vibe {

struct SourcePos {
    std::uint32_t line   = 1;  // с единицы
    std::uint32_t column = 1;  // с единицы, в символах, а не в байтах
    std::uint32_t offset = 0;  // смещение в байтах от начала файла

    friend bool operator==(const SourcePos&, const SourcePos&) = default;
};

struct SourceRange {
    SourcePos begin;  // первый символ
    SourcePos end;    // позиция сразу за последним символом

    friend bool operator==(const SourceRange&, const SourceRange&) = default;
};

}  // namespace vibe
