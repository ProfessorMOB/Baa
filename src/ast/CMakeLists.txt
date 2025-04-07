add_library(baa_ast
    ast.c
    expressions.c
    statements.c
    literals.c
)

target_include_directories(baa_ast
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(baa_ast
    PUBLIC
    baa_utils
)
