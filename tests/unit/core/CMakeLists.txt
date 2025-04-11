# Test types
add_executable(test_types test_types.c)
target_link_libraries(test_types PRIVATE
    baa_utils
    baa_types
)
add_test(NAME test_types COMMAND test_types)

# Test operators
add_executable(test_operators test_operators.c)
target_link_libraries(test_operators PRIVATE
    baa_utils
    baa_types
    baa_operators
    baa_ast
    baa_parser
)
add_test(NAME test_operators COMMAND test_operators)
