add_library(baa_ast
    ast.c
)

target_include_directories(baa_ast
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
