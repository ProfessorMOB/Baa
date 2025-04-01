# Test control flow
add_executable(test_control_flow test_control_flow.c)
target_link_libraries(test_control_flow PRIVATE
    baa_utils
    baa_ast
)
add_test(NAME test_control_flow COMMAND test_control_flow)
