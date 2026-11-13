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
    lexer/lexer_grammar_examples_test.cpp
    lexer/lexer_number_test.cpp
    lexer/lexer_prompt_test.cpp
    lexer/lexer_test.cpp
    lexer/lexer_text_test.cpp
    lexer/token_test.cpp
)
target_link_libraries(vibe_tests PRIVATE vibe_lexer vibe_warnings GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(vibe_tests)

set(VIBE_TEST_DATA ${CMAKE_CURRENT_SOURCE_DIR}/data)

add_test(NAME VibeLex.ClassicProgram COMMAND vibe-lex ${VIBE_TEST_DATA}/classic.vibe)
add_test(NAME VibeLex.AgentProgram COMMAND vibe-lex ${VIBE_TEST_DATA}/agent.vibe)
add_test(NAME VibeLex.PrintsTokens COMMAND vibe-lex ${VIBE_TEST_DATA}/classic.vibe)
set_tests_properties(VibeLex.PrintsTokens PROPERTIES
    PASS_REGULAR_EXPRESSION "4:1 +Keyword +func\r?\n4:6 +Keyword +int\r?\n4:10 +Identifier +factorial\r?\n")

add_test(NAME VibeLex.ErrorsExitCode COMMAND vibe-lex ${VIBE_TEST_DATA}/lexical_errors.vibe)
set_tests_properties(VibeLex.ErrorsExitCode PROPERTIES WILL_FAIL TRUE)
add_test(NAME VibeLex.ReportsInvalidCharacter COMMAND vibe-lex ${VIBE_TEST_DATA}/lexical_errors.vibe)
set_tests_properties(VibeLex.ReportsInvalidCharacter PROPERTIES
    PASS_REGULAR_EXPRESSION "lexical_errors.vibe:1:9: [^ ]+ LEX001: ")
add_test(NAME VibeLex.ReportsMalformedNumber COMMAND vibe-lex ${VIBE_TEST_DATA}/lexical_errors.vibe)
set_tests_properties(VibeLex.ReportsMalformedNumber PROPERTIES
    PASS_REGULAR_EXPRESSION "lexical_errors.vibe:1:17: [^ ]+ LEX003: ")
add_test(NAME VibeLex.ContinuesAfterErrors COMMAND vibe-lex ${VIBE_TEST_DATA}/lexical_errors.vibe)
set_tests_properties(VibeLex.ContinuesAfterErrors PROPERTIES
    PASS_REGULAR_EXPRESSION "3:6 +Operator +\\+\\+\r?\n3:8 +Operator +\\+\r?\n")

add_test(NAME VibeLex.MissingArgument COMMAND vibe-lex)
set_tests_properties(VibeLex.MissingArgument PROPERTIES WILL_FAIL TRUE)
add_test(NAME VibeLex.MissingFile COMMAND vibe-lex ${VIBE_TEST_DATA}/no_such_file.vibe)
set_tests_properties(VibeLex.MissingFile PROPERTIES WILL_FAIL TRUE)
