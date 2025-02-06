add_library(test_framework
    test_framework.c
    test_framework.h
)

add_executable(test_parser
    test_parser.c
)

target_link_libraries(test_parser PRIVATE
    test_framework
    baa_parser
    baa_ast
    baa_utils
)

add_test(NAME test_parser COMMAND test_parser)
