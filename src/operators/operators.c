#include "baa/operators/operators.h"
#include "baa/types/types.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <wchar.h>

// Operator table with precedence and associativity information
static const BaaOperatorInfo operator_table[] = {
    // Unary operators (highest precedence)
    {BAA_OP_NOT,        L"!",  L"نفي",          true,  false, BAA_PREC_UNARY, true},
    {BAA_OP_SUB,        L"-",  L"سالب",         true,  true,  BAA_PREC_UNARY, true},
    {BAA_OP_ADD,        L"+",  L"موجب",         true,  true,  BAA_PREC_UNARY, true},

    // Multiplicative operators
    {BAA_OP_MUL,        L"*",  L"ضرب",          false, true,  BAA_PREC_FACTOR, false},
    {BAA_OP_DIV,        L"/",  L"قسمة",         false, true,  BAA_PREC_FACTOR, false},
    {BAA_OP_MOD,        L"%",  L"باقي",         false, true,  BAA_PREC_FACTOR, false},

    // Additive operators
    {BAA_OP_ADD,        L"+",  L"جمع",          false, true,  BAA_PREC_TERM, false},
    {BAA_OP_SUB,        L"-",  L"طرح",          false, true,  BAA_PREC_TERM, false},

    // Relational operators
    {BAA_OP_LT,         L"<",  L"أصغر_من",      false, true,  BAA_PREC_COMPARISON, false},
    {BAA_OP_GT,         L">",  L"أكبر_من",      false, true,  BAA_PREC_COMPARISON, false},
    {BAA_OP_LE,         L"<=", L"أصغر_أو_يساوي", false, true,  BAA_PREC_COMPARISON, false},
    {BAA_OP_GE,         L">=", L"أكبر_أو_يساوي", false, true,  BAA_PREC_COMPARISON, false},

    // Equality operators
    {BAA_OP_EQ,         L"==", L"يساوي",        false, true,  BAA_PREC_EQUALITY, false},
    {BAA_OP_NE,         L"!=", L"لا_يساوي",     false, true,  BAA_PREC_EQUALITY, false},

    // Logical AND
    {BAA_OP_AND,        L"&&", L"و",            false, true,  BAA_PREC_LOGICAL_AND, false},

    // Logical OR
    {BAA_OP_OR,         L"||", L"أو",           false, true,  BAA_PREC_LOGICAL_OR, false},

    // Assignment operators (lowest precedence)
    {BAA_OP_ASSIGN,     L"=",  L"تعيين",        false, true,  BAA_PREC_ASSIGNMENT, true},
    {BAA_OP_ADD_ASSIGN, L"+=", L"جمع_تعيين",    false, true,  BAA_PREC_ASSIGNMENT, true},
    {BAA_OP_SUB_ASSIGN, L"-=", L"طرح_تعيين",    false, true,  BAA_PREC_ASSIGNMENT, true},
    {BAA_OP_MUL_ASSIGN, L"*=", L"ضرب_تعيين",    false, true,  BAA_PREC_ASSIGNMENT, true},
    {BAA_OP_DIV_ASSIGN, L"/=", L"قسمة_تعيين",   false, true,  BAA_PREC_ASSIGNMENT, true},
    {BAA_OP_MOD_ASSIGN, L"%=", L"باقي_تعيين",   false, true,  BAA_PREC_ASSIGNMENT, true}
};

static const size_t operator_count = sizeof(operator_table) / sizeof(operator_table[0]);

void baa_init_operators(void) {
    // Nothing to initialize for now
}

void baa_init_operator_system(void) {
    // Initialize the type system first
    baa_init_type_system();
}

const BaaOperatorInfo* baa_get_operator_info(BaaOperatorType type) {
    for (size_t i = 0; i < operator_count; i++) {
        if (operator_table[i].type == type) {
            return &operator_table[i];
        }
    }
    return NULL;
}

BaaOperatorType baa_get_operator_from_symbol(const wchar_t* symbol) {
    for (size_t i = 0; i < operator_count; i++) {
        if (wcscmp(operator_table[i].symbol, symbol) == 0) {
            return operator_table[i].type;
        }
    }
    return -1; // Invalid operator
}

BaaOperatorType baa_get_operator_from_name(const wchar_t* arabic_name) {
    for (size_t i = 0; i < operator_count; i++) {
        if (wcscmp(operator_table[i].arabic_name, arabic_name) == 0) {
            return operator_table[i].type;
        }
    }
    return -1; // Invalid operator
}

BaaOperatorPrecedence baa_get_operator_precedence(BaaOperatorType type) {
    const BaaOperatorInfo* op = baa_get_operator_info(type);
    return op ? op->precedence : BAA_PREC_NONE;
}

bool baa_is_right_associative(BaaOperatorType type) {
    const BaaOperatorInfo* op = baa_get_operator_info(type);
    return op ? op->right_associative : false;
}

bool baa_is_unary_operator(BaaOperatorType type) {
    const BaaOperatorInfo* op = baa_get_operator_info(type);
    return op ? op->is_unary : false;
}

bool baa_is_binary_operator(BaaOperatorType type) {
    const BaaOperatorInfo* op = baa_get_operator_info(type);
    return op ? op->is_binary : false;
}

const wchar_t* baa_get_operator_symbol(BaaOperatorType type) {
    const BaaOperatorInfo* op = baa_get_operator_info(type);
    return op ? op->symbol : L"?";
}

const wchar_t* baa_get_operator_arabic_name(BaaOperatorType type) {
    const BaaOperatorInfo* op = baa_get_operator_info(type);
    return op ? op->arabic_name : L"غير_معروف";
}

const char* baa_operator_to_string(BaaOperatorType type) {
    const BaaOperatorInfo* op = baa_get_operator_info(type);
    if (!op) return "خطأ";

    // Convert wchar_t* to char* for the test
    const wchar_t* wstr = op->arabic_name;
    size_t len = wcslen(wstr) + 1;
    char* str = malloc(len * sizeof(char));
    wcstombs(str, wstr, len);
    return str;
}

bool baa_validate_binary_op(BaaOperatorType op, BaaType* left_type, BaaType* right_type, BaaType** result_type) {
    if (!left_type || !right_type) {
        *result_type = baa_type_error;
        return false;
    }

    // Handle arithmetic operators
    if (op == BAA_OP_ADD || op == BAA_OP_SUB || op == BAA_OP_MUL || op == BAA_OP_DIV) {
        if (left_type->kind == BAA_TYPE_INT && right_type->kind == BAA_TYPE_INT) {
            *result_type = baa_type_int;
            return true;
        }
        if (left_type->kind == BAA_TYPE_FLOAT || right_type->kind == BAA_TYPE_FLOAT) {
            *result_type = baa_type_float;
            return true;
        }
    }

    // Handle comparison operators
    if (op == BAA_OP_EQ || op == BAA_OP_NE ||
        op == BAA_OP_LT || op == BAA_OP_GT ||
        op == BAA_OP_LE || op == BAA_OP_GE) {
        if ((left_type->kind == BAA_TYPE_INT || left_type->kind == BAA_TYPE_FLOAT) &&
            (right_type->kind == BAA_TYPE_INT || right_type->kind == BAA_TYPE_FLOAT)) {
            *result_type = baa_type_int; // Comparison operators return int (boolean)
            return true;
        }
    }

    // Handle assignment
    if (op == BAA_OP_ASSIGN) {
        if (left_type->kind == right_type->kind) {
            *result_type = left_type;
            return true;
        }
        if (left_type->kind == BAA_TYPE_FLOAT && right_type->kind == BAA_TYPE_INT) {
            *result_type = left_type;
            return true;
        }
    }

    *result_type = baa_type_error;
    return false;
}

bool baa_validate_unary_op(BaaOperatorType op, BaaType* operand_type, BaaType** result_type) {
    if (!operand_type) {
        *result_type = baa_type_error;
        return false;
    }

    // Handle numeric unary operators
    if (op == BAA_OP_ADD || op == BAA_OP_SUB) {
        if (operand_type->kind == BAA_TYPE_INT || operand_type->kind == BAA_TYPE_FLOAT) {
            *result_type = operand_type;
            return true;
        }
    }

    // Handle logical not
    if (op == BAA_OP_NOT) {
        if (operand_type->kind == BAA_TYPE_INT) {
            *result_type = baa_type_int;
            return true;
        }
    }

    *result_type = baa_type_error;
    return false;
}

// Error handling
static wchar_t error_message[256] = {0};

const wchar_t* baa_get_operator_error(void) {
    return error_message[0] ? error_message : NULL;
}

void baa_clear_operator_error(void) {
    error_message[0] = L'\0';
}
