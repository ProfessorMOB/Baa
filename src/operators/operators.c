#include "operators.h"
#include <string.h>
#include <stdlib.h>

// Operator table
static Operator operators[] = {
    {OP_ADD,    "+",  "جمع"},
    {OP_SUB,    "-",  "طرح"},
    {OP_MUL,    "*",  "ضرب"},
    {OP_DIV,    "/",  "قسمة"},
    {OP_MOD,    "%",  "باقي"},
    {OP_EQ,     "==", "يساوي"},
    {OP_NE,     "!=", "لا_يساوي"},
    {OP_LT,     "<",  "أصغر_من"},
    {OP_GT,     ">",  "أكبر_من"},
    {OP_LE,     "<=", "أصغر_أو_يساوي"},
    {OP_GE,     ">=", "أكبر_أو_يساوي"},
    {OP_ASSIGN, "=",  "تعيين"},
    {OP_ERROR,  "?",  "خطأ"}
};

void baa_init_operator_system(void) {
    // Nothing to initialize for now
}

Operator* baa_get_operator(const char* symbol) {
    for (size_t i = 0; i < sizeof(operators) / sizeof(operators[0]); i++) {
        if (strcmp(operators[i].symbol, symbol) == 0) {
            return &operators[i];
        }
    }
    return &operators[sizeof(operators) / sizeof(operators[0]) - 1]; // Return error operator
}

Type* baa_check_binary_op(Type* left, Type* right, OperatorKind op) {
    if (left == NULL || right == NULL) {
        return type_error;
    }
    
    // Handle arithmetic operators
    if (op == OP_ADD || op == OP_SUB || op == OP_MUL || op == OP_DIV) {
        // Both operands must be numeric
        if ((left->kind == TYPE_INT || left->kind == TYPE_FLOAT) &&
            (right->kind == TYPE_INT || right->kind == TYPE_FLOAT)) {
            // If either operand is float, result is float
            return (left->kind == TYPE_FLOAT || right->kind == TYPE_FLOAT) ? type_float : type_int;
        }
        return type_error;
    }
    
    // Handle modulo operator
    if (op == OP_MOD) {
        // Both operands must be integers
        if (left->kind == TYPE_INT && right->kind == TYPE_INT) {
            return type_int;
        }
        return type_error;
    }
    
    // Handle comparison operators
    if (op == OP_EQ || op == OP_NE || op == OP_LT || op == OP_GT || op == OP_LE || op == OP_GE) {
        // Both operands must be of the same type
        if (baa_types_equal(left, right)) {
            return type_int; // Comparison returns an integer (0 or 1)
        }
        // Allow comparison between numeric types
        if ((left->kind == TYPE_INT || left->kind == TYPE_FLOAT) &&
            (right->kind == TYPE_INT || right->kind == TYPE_FLOAT)) {
            return type_int;
        }
        return type_error;
    }
    
    // Handle assignment operator
    if (op == OP_ASSIGN) {
        // Must be able to convert right operand to left operand's type
        if (baa_can_convert(right, left)) {
            return left;
        }
        return type_error;
    }
    
    return type_error;
}

const char* baa_operator_to_string(OperatorKind op) {
    for (size_t i = 0; i < sizeof(operators) / sizeof(operators[0]); i++) {
        if (operators[i].kind == op) {
            return operators[i].arabic_name;
        }
    }
    return "غير_معروف";
}

bool baa_is_valid_operator(Type* left, Type* right, OperatorKind op) {
    Type* result = baa_check_binary_op(left, right, op);
    return result != type_error;
}
