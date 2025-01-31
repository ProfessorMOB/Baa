#include "../src/operators/operators.h"
#include <assert.h>
#include <stdio.h>

// Test operator initialization and lookup
void test_operator_init(void) {
    printf("Testing operator initialization...\n");
    
    baa_init_operator_system();
    
    Operator* add_op = baa_get_operator("+");
    assert(add_op != NULL);
    assert(add_op->kind == OP_ADD);
    assert(strcmp(add_op->symbol, "+") == 0);
    assert(strcmp(add_op->arabic_name, "جمع") == 0);
    
    Operator* eq_op = baa_get_operator("==");
    assert(eq_op != NULL);
    assert(eq_op->kind == OP_EQ);
    assert(strcmp(eq_op->symbol, "==") == 0);
    assert(strcmp(eq_op->arabic_name, "يساوي") == 0);
    
    // Test invalid operator
    Operator* invalid_op = baa_get_operator("invalid");
    assert(invalid_op != NULL);
    assert(invalid_op->kind == OP_ERROR);
    
    printf("Operator initialization tests passed.\n");
}

// Test arithmetic operator type checking
void test_arithmetic_operators(void) {
    printf("Testing arithmetic operators...\n");
    
    // Integer arithmetic
    Type* result = baa_check_binary_op(type_int, type_int, OP_ADD);
    assert(result == type_int);
    
    result = baa_check_binary_op(type_int, type_int, OP_SUB);
    assert(result == type_int);
    
    result = baa_check_binary_op(type_int, type_int, OP_MUL);
    assert(result == type_int);
    
    result = baa_check_binary_op(type_int, type_int, OP_DIV);
    assert(result == type_int);
    
    // Float arithmetic
    result = baa_check_binary_op(type_float, type_float, OP_ADD);
    assert(result == type_float);
    
    // Mixed arithmetic
    result = baa_check_binary_op(type_int, type_float, OP_MUL);
    assert(result == type_float);
    
    // Invalid arithmetic
    result = baa_check_binary_op(type_void, type_int, OP_ADD);
    assert(result == type_error);
    
    printf("Arithmetic operator tests passed.\n");
}

// Test comparison operator type checking
void test_comparison_operators(void) {
    printf("Testing comparison operators...\n");
    
    // Integer comparison
    Type* result = baa_check_binary_op(type_int, type_int, OP_EQ);
    assert(result == type_int);
    
    result = baa_check_binary_op(type_int, type_int, OP_LT);
    assert(result == type_int);
    
    // Float comparison
    result = baa_check_binary_op(type_float, type_float, OP_GE);
    assert(result == type_int);
    
    // Mixed comparison
    result = baa_check_binary_op(type_int, type_float, OP_NE);
    assert(result == type_int);
    
    // Invalid comparison
    result = baa_check_binary_op(type_void, type_int, OP_EQ);
    assert(result == type_error);
    
    printf("Comparison operator tests passed.\n");
}

// Test assignment operator type checking
void test_assignment_operator(void) {
    printf("Testing assignment operator...\n");
    
    // Valid assignments
    Type* result = baa_check_binary_op(type_int, type_int, OP_ASSIGN);
    assert(result == type_int);
    
    result = baa_check_binary_op(type_float, type_float, OP_ASSIGN);
    assert(result == type_float);
    
    result = baa_check_binary_op(type_float, type_int, OP_ASSIGN);
    assert(result == type_float);
    
    // Invalid assignments
    result = baa_check_binary_op(type_void, type_int, OP_ASSIGN);
    assert(result == type_error);
    
    printf("Assignment operator tests passed.\n");
}

// Test operator to string conversion
void test_operator_to_string(void) {
    printf("Testing operator to string conversion...\n");
    
    assert(strcmp(baa_operator_to_string(OP_ADD), "جمع") == 0);
    assert(strcmp(baa_operator_to_string(OP_SUB), "طرح") == 0);
    assert(strcmp(baa_operator_to_string(OP_MUL), "ضرب") == 0);
    assert(strcmp(baa_operator_to_string(OP_DIV), "قسمة") == 0);
    assert(strcmp(baa_operator_to_string(OP_ERROR), "خطأ") == 0);
    
    printf("Operator to string conversion tests passed.\n");
}

// Test operator validity checking
void test_operator_validity(void) {
    printf("Testing operator validity...\n");
    
    assert(baa_is_valid_operator(type_int, type_int, OP_ADD) == true);
    assert(baa_is_valid_operator(type_float, type_float, OP_MUL) == true);
    assert(baa_is_valid_operator(type_int, type_float, OP_DIV) == true);
    assert(baa_is_valid_operator(type_void, type_int, OP_ADD) == false);
    assert(baa_is_valid_operator(type_char, type_float, OP_MUL) == false);
    
    printf("Operator validity tests passed.\n");
}

int main(void) {
    printf("Running operator system tests...\n\n");
    
    baa_init_type_system(); // Initialize types first
    
    test_operator_init();
    test_arithmetic_operators();
    test_comparison_operators();
    test_assignment_operator();
    test_operator_to_string();
    test_operator_validity();
    
    printf("\nAll operator system tests passed successfully!\n");
    return 0;
}
