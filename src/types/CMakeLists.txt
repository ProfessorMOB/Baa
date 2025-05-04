add_library(baa_types
    types.c
)

target_include_directories(baa_types
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Dependencies are linked by the final executable, not the static library itself.
