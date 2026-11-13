add_library(vibe_common
    common/diagnostics.cpp
)
target_include_directories(vibe_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(vibe_common PRIVATE vibe_warnings)
