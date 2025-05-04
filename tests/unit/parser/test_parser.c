#include "test_framework.h"
#include "baa/parser/tokens.h" // Corrected include path
#include "baa/ast/ast.h"       // Corrected include path
#include "baa/parser/parser.h" // Corrected include path
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
void test_token_creation(void) { // Changed to standard function definition
    BaaToken token = {
        .type = TOKEN_FUNCTION, // Note: TOKEN_FUNCTION might be undefined if tokens.h isn't included correctly
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
void test_ast_creation(void) { // Changed to standard function definition
    // Create a simple expression: 1 + 2
    BaaNode* num1 = baa_create_node(NODE_NUMBER, L"1"); // Note: Node, baa_create_node, NODE_NUMBER might be undefined
    BaaNode* plus = baa_create_node(NODE_BINARY_OP, L"+");
    BaaNode* num2 = baa_create_node(NODE_NUMBER, L"2");

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
void test_function_declaration(void) { // Changed to standard function definition
    // Create a simple function: دالة مرحبا() { إرجع 0. }
    BaaNode* func = baa_create_node(BAA_NODE_FUNCTION, L"مرحبا"); // Note: Node, baa_create_node, BAA_NODE_FUNCTION might be undefined
    BaaNode* return_stmt = baa_create_node(BAA_NODE_RETURN, NULL);
    BaaNode* return_val = baa_create_node(BAA_NODE_NUMBER, L"0");

    baa_add_child(return_stmt, return_val);
    baa_add_child(func, return_stmt);

    ASSERT_EQ((size_t)1, func->children_count);
    ASSERT_STR_EQ(L"مرحبا", func->value);
    ASSERT_EQ(NODE_RETURN, func->children[0]->type);
    ASSERT_STR_EQ(L"0", func->children[0]->children[0]->value);

    baa_free_node(func);
}

// Test parsing a simple program
void test_parse_simple_program(void) { // Changed to standard function definition
    wchar_t* source = read_file(L"simple.txt"); // Note: read_file might need adjustment if not finding file
    ASSERT(source != NULL);

    BaaParser* parser = baa_parser_init(source);
    ASSERT(parser != NULL);

    BaaNode* program = baa_parse_program(parser);
    ASSERT(program != NULL);
    ASSERT_EQ(false, baa_parser_had_error(parser));

    // Program should have 2 children: square function and main function
    ASSERT_EQ((size_t)2, program->children_count);

    // Check square function
    BaaNode* square_func = program->children[0];
    ASSERT_EQ(NODE_FUNCTION, square_func->type);
    ASSERT_STR_EQ(L"مربع", square_func->value);

    // Check main function
    BaaNode* main_func = program->children[1];
    ASSERT_EQ(NODE_FUNCTION, main_func->type);
    ASSERT_STR_EQ(L"رئيسية", main_func->value);

    baa_free_node(program);
    baa_parser_free(parser);
    free(source);
}

// Test parsing Arabic program
void test_parse_arabic_program(void) { // Changed to standard function definition
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
    BaaParser* parser = baa_parser_init(source);
    ASSERT(parser != NULL);

    wprintf(L"Parsing program...\n");
    BaaNode* program = baa_parse_program(parser);
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
    BaaNode* import1 = program->children[0];
    BaaNode* import2 = program->children[1];
    ASSERT_EQ(NODE_IMPORT, import1->type);
    ASSERT_EQ(NODE_IMPORT, import2->type);
    ASSERT_STR_EQ(L"نظام/طباعة", import1->value);
    ASSERT_STR_EQ(L"نظام/رياضيات", import2->value);

    // Check constant declaration
    BaaNode* const_decl = program->children[2];
    ASSERT_EQ(NODE_VAR_DECL, const_decl->type);
    ASSERT_STR_EQ(L"حجم_المصفوفة", const_decl->value);

    // Check array declaration
    BaaNode* array_decl = program->children[3];
    ASSERT_EQ(NODE_VAR_DECL, array_decl->type);
    ASSERT_STR_EQ(L"مصفوفة", array_decl->value);

    // Check array sum function
    BaaNode* sum_func = program->children[4];
    ASSERT_EQ(NODE_FUNCTION, sum_func->type);
    ASSERT_STR_EQ(L"مجموع_المصفوفة", sum_func->value);

    // Check main function
    BaaNode* main_func = program->children[5];
    ASSERT_EQ(NODE_FUNCTION, main_func->type);
    ASSERT_STR_EQ(L"رئيسية", main_func->value);

    wprintf(L"Cleaning up...\n");
    baa_free_node(program);
    baa_parser_free(parser);
    free(source);
    wprintf(L"Test completed successfully\n");
}

TEST_SUITE_BEGIN()
    // Note: These test cases might fail if the types/functions
    // from included headers are not resolved correctly by the build system.
    TEST_CASE(test_token_creation);
    TEST_CASE(test_ast_creation);
    TEST_CASE(test_function_declaration);
    TEST_CASE(test_parse_simple_program);
    TEST_CASE(test_parse_arabic_program);
TEST_SUITE_END()
