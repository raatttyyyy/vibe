include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(vibe_tests
    common/diagnostics_test.cpp
    lexer/keyword_trie_test.cpp
    lexer/lexer_number_test.cpp
    lexer/lexer_prompt_test.cpp
    lexer/lexer_test.cpp
    lexer/lexer_text_test.cpp
    lexer/token_test.cpp
)
target_link_libraries(vibe_tests PRIVATE vibe_lexer vibe_warnings GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(vibe_tests)
