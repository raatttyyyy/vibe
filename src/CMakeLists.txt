add_library(vibe_common
    common/diagnostics.cpp
)
target_include_directories(vibe_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(vibe_common PRIVATE vibe_warnings)

add_library(vibe_lexer
    lexer/keyword_trie.cpp
    lexer/lexer.cpp
    lexer/token.cpp
)
target_link_libraries(vibe_lexer PUBLIC vibe_common PRIVATE vibe_warnings)

add_executable(vibe-lex tools/vibe_lex.cpp)
target_link_libraries(vibe-lex PRIVATE vibe_lexer vibe_warnings)
