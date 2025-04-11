#include "baa/types/types.h"
#include "baa/operators/operators.h"
#include "baa/utils/utils.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

// Test operator initialization and lookup
void test_operator_init(void) {
    printf("Testing operator initialization...\n");

    baa_init_operator_system();

    BaaOperator* add_op = baa_get_operator("+");
    assert(add_op != NULL);
    assert(add_op->type == BAA_OP_ADD);
    assert(wcscmp(add_op->symbol, L"+") == 0);
    assert(wcscmp(add_op->arabic_name, L"جمع") == 0);

    BaaOperator* eq_op = baa_get_operator("==");
    assert(eq_op != NULL);
    assert(eq_op->type == BAA_OP_EQUAL);
    assert(wcscmp(eq_op->symbol, L"==") == 0);
    assert(wcscmp(eq_op->arabic_name, L"يساوي") == 0);

    // Test invalid operator
    BaaOperator* invalid_op = baa_get_operator("invalid");
    assert(invalid_op != NULL);
    assert(invalid_op->type == BAA_OP_ERROR);

    printf("Operator initialization tests passed.\n");
}

// Test arithmetic operator type checking
void test_arithmetic_operators(void) {
    // Initialize type system
    baa_init_type_system();

    // Test integer arithmetic
    BaaType* result = baa_check_binary_op(baa_type_int, baa_type_int, BAA_OP_ADD);
    assert(result == baa_type_int);

    result = baa_check_binary_op(baa_type_int, baa_type_int, BAA_OP_SUBTRACT);
    assert(result == baa_type_int);

    result = baa_check_binary_op(baa_type_int, baa_type_int, BAA_OP_MULTIPLY);
    assert(result == baa_type_int);

    result = baa_check_binary_op(baa_type_int, baa_type_int, BAA_OP_DIVIDE);
    assert(result == baa_type_int);

    // Test float arithmetic
    result = baa_check_binary_op(baa_type_float, baa_type_float, BAA_OP_ADD);
    assert(result == baa_type_float);

    // Test mixed type arithmetic
    result = baa_check_binary_op(baa_type_int, baa_type_float, BAA_OP_MULTIPLY);
    assert(result == baa_type_float);

    // Test invalid operands
    result = baa_check_binary_op(baa_type_void, baa_type_int, BAA_OP_ADD);
    assert(result == baa_type_error);

    printf("Arithmetic operator tests passed.\n");
}

// Test comparison operator type checking
void test_comparison_operators(void) {
    // Test integer comparisons
    BaaType* result = baa_check_binary_op(baa_type_int, baa_type_int, BAA_OP_EQUAL);
    assert(result == baa_type_int);

    result = baa_check_binary_op(baa_type_int, baa_type_int, BAA_OP_LESS);
    assert(result == baa_type_int);

    // Test float comparisons
    result = baa_check_binary_op(baa_type_float, baa_type_float, BAA_OP_GREATER_EQ);
    assert(result == baa_type_int);

    // Test mixed type comparisons
    result = baa_check_binary_op(baa_type_int, baa_type_float, BAA_OP_NOT_EQUAL);
    assert(result == baa_type_int);

    // Test invalid operands
    result = baa_check_binary_op(baa_type_void, baa_type_int, BAA_OP_LESS);
    assert(result == baa_type_error);

    printf("Comparison operator tests passed.\n");
}

// Test assignment operator type checking
void test_assignment_operator(void) {
    printf("Testing assignment operator...\n");

    // Valid assignments
    BaaType* result = baa_check_binary_op(baa_type_int, baa_type_int, BAA_OP_ASSIGN);
    assert(result == baa_type_int);

    result = baa_check_binary_op(baa_type_float, baa_type_float, BAA_OP_ASSIGN);
    assert(result == baa_type_float);

    result = baa_check_binary_op(baa_type_float, baa_type_int, BAA_OP_ASSIGN);
    assert(result == baa_type_float);

    // Invalid assignments
    result = baa_check_binary_op(baa_type_void, baa_type_int, BAA_OP_ASSIGN);
    assert(result == baa_type_error);

    printf("Assignment operator tests passed.\n");
}

// Test operator to string conversion
void test_operator_to_string(void) {
    printf("Testing operator to string conversion...\n");

    assert(strcmp(baa_operator_to_string(BAA_OP_ADD), "جمع") == 0);
    assert(strcmp(baa_operator_to_string(BAA_OP_SUBTRACT), "طرح") == 0);
    assert(strcmp(baa_operator_to_string(BAA_OP_MULTIPLY), "ضرب") == 0);
    assert(strcmp(baa_operator_to_string(BAA_OP_DIVIDE), "قسمة") == 0);
    assert(strcmp(baa_operator_to_string(BAA_OP_ERROR), "خطأ") == 0);

    printf("Operator to string conversion tests passed.\n");
}

// Test operator validity checking
void test_operator_validity(void) {
    printf("Testing operator validity...\n");

    assert(baa_is_valid_operator(baa_type_int, baa_type_int, BAA_OP_ADD) == true);
    assert(baa_is_valid_operator(baa_type_float, baa_type_float, BAA_OP_MULTIPLY) == true);
    assert(baa_is_valid_operator(baa_type_int, baa_type_float, BAA_OP_DIVIDE) == true);
    assert(baa_is_valid_operator(baa_type_void, baa_type_int, BAA_OP_ADD) == false);
    assert(baa_is_valid_operator(baa_type_char, baa_type_float, BAA_OP_MULTIPLY) == false);

    printf("Operator validity tests passed.\n");
}

void test_logical_operators(void) {
    // Test logical AND
    BaaType* result = baa_check_binary_op(baa_type_int, baa_type_int, BAA_OP_AND);
    assert(result == baa_type_int);

    // Test logical OR
    result = baa_check_binary_op(baa_type_int, baa_type_int, BAA_OP_OR);
    assert(result == baa_type_int);

    // Test logical NOT (unary)
    result = baa_check_unary_op(baa_type_int, BAA_OP_NOT);
    assert(result == baa_type_int);

    // Test invalid operands
    result = baa_check_binary_op(baa_type_void, baa_type_int, BAA_OP_AND);
    assert(result == baa_type_error);

    printf("Logical operator tests passed.\n");
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
    test_logical_operators();

    printf("\nAll operator system tests passed successfully!\n");
    return 0;
}
