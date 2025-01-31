enable_testing()

add_executable(test_lexer test_lexer.c)
target_link_libraries(test_lexer PRIVATE baa_lexer baa_utils)
add_test(NAME test_lexer COMMAND test_lexer)

add_executable(test_utils test_utils.c)
target_link_libraries(test_utils PRIVATE baa_utils)
add_test(NAME test_utils COMMAND test_utils)
