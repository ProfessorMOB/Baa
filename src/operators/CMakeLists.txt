add_library(baa_operators
    operators.c
)

target_include_directories(baa_operators
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(baa_operators PUBLIC # Or PRIVATE
    UNICODE
    _UNICODE
    _CRT_SECURE_NO_WARNINGS
)
# baa_operators depends on baa_types
target_link_libraries(baa_operators PRIVATE baa_types)
# It does not seem to directly depend on baa_utils, but baa_types does.
