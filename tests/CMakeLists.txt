enable_testing()

# Add parser tests directory
add_subdirectory(parser_tests)

# Test utilities
add_executable(test_utils test_utils.c)
target_link_libraries(test_utils PRIVATE baa_utils)
add_test(NAME test_utils COMMAND test_utils)

# Test operators
add_executable(test_operators test_operators.c)
target_link_libraries(test_operators PRIVATE
    baa_utils
    baa_types
    baa_operators
)
add_test(NAME test_operators COMMAND test_operators)

# Test types
add_executable(test_types test_types.c)
target_link_libraries(test_types PRIVATE
    baa_utils
    baa_types
)
add_test(NAME test_types COMMAND test_types)

# Test control flow
add_executable(test_control_flow test_control_flow.c)
target_link_libraries(test_control_flow PRIVATE
    baa_utils
    baa_ast
    baa_control_flow
)
add_test(NAME test_control_flow COMMAND test_control_flow)

# Remove duplicate test_parser target since it's defined in parser_tests/CMakeLists.txt
