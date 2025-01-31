#pragma once

#include "../types/types.h"

// Operator kinds
typedef enum {
    // Arithmetic operators
    OP_ADD,      // +
    OP_SUB,      // -
    OP_MUL,      // *
    OP_DIV,      // /
    OP_MOD,      // %
    
    // Comparison operators
    OP_EQ,       // ==
    OP_NE,       // !=
    OP_LT,       // <
    OP_GT,       // >
    OP_LE,       // <=
    OP_GE,       // >=
    
    // Assignment operator
    OP_ASSIGN,   // =
    
    // Error
    OP_ERROR
} OperatorKind;

// Operator structure
typedef struct {
    OperatorKind kind;
    const char* symbol;
    const char* arabic_name;
} Operator;

// Initialize operator system
void baa_init_operator_system(void);

// Get operator by symbol
Operator* baa_get_operator(const char* symbol);

// Type checking for operators
Type* baa_check_binary_op(Type* left, Type* right, OperatorKind op);

// Get operator string representation
const char* baa_operator_to_string(OperatorKind op);

// Check if operator is valid for given types
bool baa_is_valid_operator(Type* left, Type* right, OperatorKind op);
