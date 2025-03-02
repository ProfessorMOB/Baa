# Lexer tests will be added here
# ...

add_executable(test_lexer test_lexer.c)
target_link_libraries(test_lexer baa_utils baa_lexer)

add_executable(test_number_parser test_number_parser.c)
target_link_libraries(test_number_parser baa_utils baa_lexer test_framework)

if(WIN32)
    # On Windows, the math functions are in the standard C library
    # No need to link with a separate math library
else()
    # On Unix systems, link with the math library
    target_link_libraries(test_lexer m)
    target_link_libraries(test_number_parser m)
endif()

add_test(NAME test_lexer COMMAND test_lexer)
add_test(NAME test_number_parser COMMAND test_number_parser)
