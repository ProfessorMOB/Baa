# Add subdirectories for each component
add_subdirectory(lexer)
add_subdirectory(parser)
add_subdirectory(ast)
add_subdirectory(codegen)
add_subdirectory(utils)
add_subdirectory(types)
add_subdirectory(operators)
add_subdirectory(src/analysis)

# Main executable
add_executable(Baa
    main.c
)

target_link_libraries(Baa
    PRIVATE
    baa_lexer
    baa_parser
    baa_ast
    baa_codegen
    baa_utils
)
