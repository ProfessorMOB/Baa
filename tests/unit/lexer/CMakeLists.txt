# Lexer tests will be added here
# ...

add_executable(test_lexer test_lexer.c)
target_link_libraries(test_lexer baa_utils baa_lexer)

add_executable(test_number_parser test_number_parser.c)
target_link_libraries(test_number_parser baa_utils baa_lexer test_framework)

add_executable(test_tokens test_tokens.c)
target_link_libraries(test_tokens baa_utils baa_lexer test_framework)

add_executable(test_program_files test_program_files.c)
target_link_libraries(test_program_files
    baa_utils
    baa_lexer
    test_framework
)

if(WIN32)
    # On Windows, the math functions are in the standard C library
    # No need to link with a separate math library
else()
    # On Unix systems, link with the math library
    target_link_libraries(test_lexer m)
    target_link_libraries(test_number_parser m)
    target_link_libraries(test_tokens m)
    target_link_libraries(test_program_files m)
endif()

add_test(NAME test_lexer COMMAND test_lexer)
add_test(NAME test_number_parser COMMAND test_number_parser)
add_test(NAME test_tokens COMMAND test_tokens)
add_test(NAME test_program_files COMMAND test_program_files)
