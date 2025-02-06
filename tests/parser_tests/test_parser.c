#include "test_framework.h"
#include "../../src/parser/tokens.h"
#include "../../include/baa/ast.h"
#include "../../include/baa/parser.h"
#include <direct.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper function to read file content
static char* read_file(const char* path) {
    printf("Attempting to read file: %s\n", path);
    FILE* file = fopen(path, "rb");
    if (!file) {
        printf("Failed to open file: %s\n", path);
        perror("Error");
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    printf("File size: %ld bytes\n", length);
    
    char* buffer = malloc(length + 1);
    if (!buffer) {
        printf("Failed to allocate memory for file content\n");
        fclose(file);
        return NULL;
    }
    
    size_t read = fread(buffer, 1, length, file);
    printf("Read %zu bytes\n", read);
    
    if (read != (size_t)length) {
        printf("Failed to read entire file. Expected %ld bytes, got %zu bytes\n", length, read);
        free(buffer);
        fclose(file);
        return NULL;
    }
    
    buffer[length] = '\0';
    fclose(file);
    
    // Print first few bytes to check encoding
    printf("First few bytes: ");
    for (int i = 0; i < 10 && i < length; i++) {
        printf("%02x ", (unsigned char)buffer[i]);
    }
    printf("\n");
    
    return buffer;
}

// Test basic token creation
TEST(token_creation) {
    Token token = {
        .type = TOKEN_FUNCTION,
        .start = "دالة",
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
    Node* num1 = baa_create_node(NODE_NUMBER, "1");
    Node* plus = baa_create_node(NODE_BINARY_OP, "+");
    Node* num2 = baa_create_node(NODE_NUMBER, "2");
    
    ASSERT(num1 != NULL);
    ASSERT(plus != NULL);
    ASSERT(num2 != NULL);
    
    baa_add_child(plus, num1);
    baa_add_child(plus, num2);
    
    ASSERT_EQ((size_t)2, plus->children_count);
    ASSERT_STR_EQ("+", plus->value);
    ASSERT_STR_EQ("1", plus->children[0]->value);
    ASSERT_STR_EQ("2", plus->children[1]->value);
    
    baa_free_node(plus); // This will free the entire tree
}

// Test function declaration parsing
TEST(function_declaration) {
    // Create a simple function: دالة مرحبا() { إرجع 0. }
    Node* func = baa_create_node(NODE_FUNCTION, "مرحبا");
    Node* return_stmt = baa_create_node(NODE_RETURN, NULL);
    Node* return_val = baa_create_node(NODE_NUMBER, "0");
    
    baa_add_child(return_stmt, return_val);
    baa_add_child(func, return_stmt);
    
    ASSERT_EQ((size_t)1, func->children_count);
    ASSERT_STR_EQ("مرحبا", func->value);
    ASSERT_EQ(NODE_RETURN, func->children[0]->type);
    ASSERT_STR_EQ("0", func->children[0]->children[0]->value);
    
    baa_free_node(func);
}

// Test parsing a simple program
TEST(parse_simple_program) {
    char* source = read_file("test_files/simple.txt");
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
    ASSERT_STR_EQ("مربع", square_func->value);
    
    // Check main function
    Node* main_func = program->children[1];
    ASSERT_EQ(NODE_FUNCTION, main_func->type);
    ASSERT_STR_EQ("رئيسية", main_func->value);
    
    baa_free_node(program);
    baa_parser_free(parser);
    free(source);
}

// Test parsing Arabic program
TEST(parse_arabic_program) {
    char cwd[1024];
    if (_getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working directory: %s\n", cwd);
    }
    
    printf("Reading test file...\n");
    char* source = read_file("../tests/parser_tests/test_files/program_test.txt");
    if (!source) {
        printf("Failed to read test file\n");
        ASSERT(false);
        return;
    }
    
    printf("Initializing parser...\n");
    Parser* parser = baa_parser_init(source);
    ASSERT(parser != NULL);
    
    printf("Parsing program...\n");
    Node* program = baa_parse_program(parser);
    if (!program) {
        printf("Failed to parse program: %s\n", baa_parser_error_message(parser));
        ASSERT(false);
        baa_parser_free(parser);
        free(source);
        return;
    }
    
    printf("Checking parser errors...\n");
    ASSERT_EQ(false, baa_parser_had_error(parser));
    
    printf("Checking program structure...\n");
    // Program should have imports, constant declaration, array declaration, and three functions
    ASSERT_EQ((size_t)6, program->children_count);
    
    // Check imports
    Node* import1 = program->children[0];
    Node* import2 = program->children[1];
    ASSERT_EQ(NODE_IMPORT, import1->type);
    ASSERT_EQ(NODE_IMPORT, import2->type);
    ASSERT_STR_EQ("نظام/طباعة", import1->value);
    ASSERT_STR_EQ("نظام/رياضيات", import2->value);
    
    // Check constant declaration
    Node* const_decl = program->children[2];
    ASSERT_EQ(NODE_VAR_DECL, const_decl->type);
    ASSERT_STR_EQ("حجم_المصفوفة", const_decl->value);
    
    // Check array declaration
    Node* array_decl = program->children[3];
    ASSERT_EQ(NODE_VAR_DECL, array_decl->type);
    ASSERT_STR_EQ("مصفوفة", array_decl->value);
    
    // Check array sum function
    Node* sum_func = program->children[4];
    ASSERT_EQ(NODE_FUNCTION, sum_func->type);
    ASSERT_STR_EQ("مجموع_المصفوفة", sum_func->value);
    
    // Check main function
    Node* main_func = program->children[5];
    ASSERT_EQ(NODE_FUNCTION, main_func->type);
    ASSERT_STR_EQ("رئيسية", main_func->value);
    
    printf("Cleaning up...\n");
    baa_free_node(program);
    baa_parser_free(parser);
    free(source);
    printf("Test completed successfully\n");
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
