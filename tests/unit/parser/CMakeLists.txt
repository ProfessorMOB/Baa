# سيتم إضافة اختبارات المحلل النحوي لاحقاً
# Parser tests will be added later

# Test parser
add_executable(test_parser test_parser.c)
target_link_libraries(test_parser PRIVATE
    test_framework
    baa_utils
    baa_parser
    baa_ast
    baa_lexer
)
add_test(NAME test_parser COMMAND test_parser)
