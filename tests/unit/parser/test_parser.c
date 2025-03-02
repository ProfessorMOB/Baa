#include "test_framework.h"
#include "../../src/parser/tokens.h"
#include "../../include/baa/ast.h"
#include "../../include/baa/parser.h"
#include <direct.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

// Helper function to read file content
static wchar_t* read_file(const wchar_t* path) {
    wprintf(L"Attempting to read file: %s\n", path);
    FILE* file = _wfopen(path, L"rb");
    if (!file) {
        wprintf(L"Failed to open file: %s\n", path);
        perror("Error");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    wprintf(L"File size: %ld bytes\n", length);

    wchar_t* buffer = malloc(length + 1);
    if (!buffer) {
        wprintf(L"Failed to allocate memory for file content\n");
        fclose(file);
        return NULL;
    }

    size_t read = fread(buffer, 1, length, file);
    wprintf(L"Read %zu bytes\n", read);

    if (read != (size_t)length) {
        wprintf(L"Failed to read entire file. Expected %ld bytes, got %zu bytes\n", length, read);
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[length] = L'\0';
    fclose(file);

    // Print first few bytes to check encoding
    wprintf(L"First few bytes: ");
    for (int i = 0; i < 10 && i < length; i++) {
        wprintf(L"%02x ", (unsigned char)buffer[i]);
    }
    wprintf(L"\n");

    return buffer;
}

// Test basic token creation
TEST(token_creation) {
    Token token = {
        .type = TOKEN_FUNCTION,
        .start = L"دالة",
        .length = 4,
        .line = 1,
        .column = 1
    };

    ASSERT_EQ(TOKEN_FUNCTION, token.type);
    ASSERT_EQ(4, token.length);
    ASSERT_EQ(1, token.line);
    ASSERT_EQ(1, token.column);
}

// Test basic AST node creation
TEST(ast_creation) {
    // Create a simple expression: 1 + 2
    Node* num1 = baa_create_node(NODE_NUMBER, L"1");
    Node* plus = baa_create_node(NODE_BINARY_OP, L"+");
    Node* num2 = baa_create_node(NODE_NUMBER, L"2");

    ASSERT(num1 != NULL);
    ASSERT(plus != NULL);
    ASSERT(num2 != NULL);

    baa_add_child(plus, num1);
    baa_add_child(plus, num2);

    ASSERT_EQ((size_t)2, plus->children_count);
    ASSERT_STR_EQ(L"+", plus->value);
    ASSERT_STR_EQ(L"1", plus->children[0]->value);
    ASSERT_STR_EQ(L"2", plus->children[1]->value);

    baa_free_node(plus); // This will free the entire tree
}

// Test function declaration parsing
TEST(function_declaration) {
    // Create a simple function: دالة مرحبا() { إرجع 0. }
    Node* func = baa_create_node(NODE_FUNCTION, L"مرحبا");
    Node* return_stmt = baa_create_node(NODE_RETURN, NULL);
    Node* return_val = baa_create_node(NODE_NUMBER, L"0");

    baa_add_child(return_stmt, return_val);
    baa_add_child(func, return_stmt);

    ASSERT_EQ((size_t)1, func->children_count);
    ASSERT_STR_EQ(L"مرحبا", func->value);
    ASSERT_EQ(NODE_RETURN, func->children[0]->type);
    ASSERT_STR_EQ(L"0", func->children[0]->children[0]->value);

    baa_free_node(func);
}

// Test parsing a simple program
TEST(parse_simple_program) {
    wchar_t* source = read_file(L"simple.txt");
    ASSERT(source != NULL);

    Parser* parser = baa_parser_init(source);
    ASSERT(parser != NULL);

    Node* program = baa_parse_program(parser);
    ASSERT(program != NULL);
    ASSERT_EQ(false, baa_parser_had_error(parser));

    // Program should have 2 children: square function and main function
    ASSERT_EQ((size_t)2, program->children_count);

    // Check square function
    Node* square_func = program->children[0];
    ASSERT_EQ(NODE_FUNCTION, square_func->type);
    ASSERT_STR_EQ(L"مربع", square_func->value);

    // Check main function
    Node* main_func = program->children[1];
    ASSERT_EQ(NODE_FUNCTION, main_func->type);
    ASSERT_STR_EQ(L"رئيسية", main_func->value);

    baa_free_node(program);
    baa_parser_free(parser);
    free(source);
}

// Test parsing Arabic program
TEST(parse_arabic_program) {
    wchar_t cwd[1024];
    if (_wgetcwd(cwd, sizeof(cwd)) != NULL) {
        wprintf(L"Current working directory: %s\n", cwd);
    }

    wprintf(L"Reading test file...\n");
    wchar_t* source = read_file(L"../tests/parser_tests/test_files/program_test.txt");
    if (!source) {
        wprintf(L"Failed to read test file\n");
        ASSERT(false);
        return;
    }

    wprintf(L"Initializing parser...\n");
    Parser* parser = baa_parser_init(source);
    ASSERT(parser != NULL);

    wprintf(L"Parsing program...\n");
    Node* program = baa_parse_program(parser);
    if (!program) {
        wprintf(L"Failed to parse program: %s\n", baa_parser_error_message(parser));
        ASSERT(false);
        baa_parser_free(parser);
        free(source);
        return;
    }

    wprintf(L"Checking parser errors...\n");
    ASSERT_EQ(false, baa_parser_had_error(parser));

    wprintf(L"Checking program structure...\n");
    // Program should have imports, constant declaration, array declaration, and three functions
    ASSERT_EQ((size_t)6, program->children_count);

    // Check imports
    Node* import1 = program->children[0];
    Node* import2 = program->children[1];
    ASSERT_EQ(NODE_IMPORT, import1->type);
    ASSERT_EQ(NODE_IMPORT, import2->type);
    ASSERT_STR_EQ(L"نظام/طباعة", import1->value);
    ASSERT_STR_EQ(L"نظام/رياضيات", import2->value);

    // Check constant declaration
    Node* const_decl = program->children[2];
    ASSERT_EQ(NODE_VAR_DECL, const_decl->type);
    ASSERT_STR_EQ(L"حجم_المصفوفة", const_decl->value);

    // Check array declaration
    Node* array_decl = program->children[3];
    ASSERT_EQ(NODE_VAR_DECL, array_decl->type);
    ASSERT_STR_EQ(L"مصفوفة", array_decl->value);

    // Check array sum function
    Node* sum_func = program->children[4];
    ASSERT_EQ(NODE_FUNCTION, sum_func->type);
    ASSERT_STR_EQ(L"مجموع_المصفوفة", sum_func->value);

    // Check main function
    Node* main_func = program->children[5];
    ASSERT_EQ(NODE_FUNCTION, main_func->type);
    ASSERT_STR_EQ(L"رئيسية", main_func->value);

    wprintf(L"Cleaning up...\n");
    baa_free_node(program);
    baa_parser_free(parser);
    free(source);
    wprintf(L"Test completed successfully\n");
}

int main() {
    init_test_framework();

    RUN_TEST(token_creation);
    RUN_TEST(ast_creation);
    RUN_TEST(function_declaration);
    RUN_TEST(parse_simple_program);
    RUN_TEST(parse_arabic_program);

    print_test_results();
    return failed_tests > 0 ? 1 : 0;
}
