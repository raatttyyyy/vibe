cmake_minimum_required(VERSION 3.20)

project(vibe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(VIBE_BUILD_TESTS "Build the Vibe test suite" ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

add_library(vibe_warnings INTERFACE)
if(MSVC)
    target_compile_options(vibe_warnings INTERFACE /W4 /permissive-)
else()
    target_compile_options(vibe_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

add_subdirectory(src)

if(VIBE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
