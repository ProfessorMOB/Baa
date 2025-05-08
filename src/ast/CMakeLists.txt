add_library(baa_ast
    ast.c
    ast_printer.c # Add the new printer source file
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

# Dependencies are linked by the final executable, not the static library itself.
