#include "baa/ast.h"
#include "baa/lexer.h"
#include "baa/utils.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

// Test helper functions
static void assert_token(Token token, TokenType expected_type, const wchar_t* expected_lexeme) {
    assert(token.type == expected_type);
    assert(wcscmp(token.lexeme, expected_lexeme) == 0);
}

static void assert_node(Node* node, NodeType expected_type, const wchar_t* expected_value) {
    assert(node != NULL);
    assert(node->type == expected_type);
    if (expected_value) {
        assert(wcscmp(node->value, expected_value) == 0);
    }
}

// Test basic lexer-AST integration
void test_basic_integration() {
    const wchar_t* source = L"دالة مرحبا() { إرجع 42; }";
    Lexer* lexer = baa_lexer_init(source);
    ASTBuilder* builder = baa_ast_builder_init(lexer);
    
    // Test function parsing
    Node* func = parse_function(builder);
    assert_node(func, NODE_FUNCTION, L"مرحبا");
    
    baa_ast_builder_free(builder);
    baa_lexer_free(lexer);
}

// Test error recovery
void test_error_recovery() {
    const wchar_t* source = L"دالة { إرجع 42; }";
    Lexer* lexer = baa_lexer_init(source);
    ASTBuilder* builder = baa_ast_builder_init(lexer);
    
    // Test missing function name
    Node* func = parse_function(builder);
    assert(builder->had_error);
    assert(func != NULL);
    
    baa_ast_builder_free(builder);
    baa_lexer_free(lexer);
}

// Test Arabic identifier validation
void test_arabic_identifiers() {
    const wchar_t* source = L"دالة متغير_عربي() { }";
    Lexer* lexer = baa_lexer_init(source);
    ASTBuilder* builder = baa_ast_builder_init(lexer);
    
    Node* func = parse_function(builder);
    assert_node(func, NODE_FUNCTION, L"متغير_عربي");
    assert(!builder->had_error);
    
    baa_ast_builder_free(builder);
    baa_lexer_free(lexer);
}

// Test complex Arabic expressions
void test_complex_arabic() {
    const wchar_t* source = L"دالة حساب(عدد_أ، عدد_ب) { إذا (عدد_أ > عدد_ب) { إرجع عدد_أ; } إلا { إرجع عدد_ب; } }";
    Lexer* lexer = baa_lexer_init(source);
    ASTBuilder* builder = baa_ast_builder_init(lexer);
    
    Node* func = parse_function(builder);
    assert_node(func, NODE_FUNCTION, L"حساب");
    assert(!builder->had_error);
    
    baa_ast_builder_free(builder);
    baa_lexer_free(lexer);
}

// Main test runner
int main() {
    printf("Running AST Builder tests...\n");
    
    test_basic_integration();
    test_error_recovery();
    test_arabic_identifiers();
    test_complex_arabic();
    
    printf("All AST Builder tests passed!\n");
    return 0;
}
