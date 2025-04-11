#ifndef BAA_OPERATORS_H
#define BAA_OPERATORS_H

#include "baa/types/types.h"
#include <stdbool.h>
#include <wchar.h>

// Forward declarations
typedef struct BaaExpr BaaExpr;

// Operator types
typedef enum {
    // Arithmetic operators
    BAA_OP_ADD,      // +  (جمع)
    BAA_OP_SUB,      // -  (طرح)
    BAA_OP_MUL,      // *  (ضرب)
    BAA_OP_DIV,      // /  (قسمة)
    BAA_OP_MOD,      // %  (باقي)

    // Comparison operators
    BAA_OP_EQ,       // == (يساوي)
    BAA_OP_NE,       // != (لا_يساوي)
    BAA_OP_LT,       // <  (أصغر_من)
    BAA_OP_LE,       // <= (أصغر_أو_يساوي)
    BAA_OP_GT,       // >  (أكبر_من)
    BAA_OP_GE,       // >= (أكبر_أو_يساوي)

    // Logical operators
    BAA_OP_AND,      // && (و)
    BAA_OP_OR,       // || (أو)
    BAA_OP_NOT,      // !  (ليس)

    // Bitwise operators
    BAA_OP_BIT_AND,  // &  (و_ثنائي)
    BAA_OP_BIT_OR,   // |  (أو_ثنائي)
    BAA_OP_BIT_XOR,  // ^  (أو_حصري)
    BAA_OP_BIT_NOT,  // ~  (عكس_ثنائي)
    BAA_OP_SHL,      // << (إزاحة_يسار)
    BAA_OP_SHR,      // >> (إزاحة_يمين)

    // Assignment operators
    BAA_OP_ASSIGN,   // =  (تعيين)
    BAA_OP_ADD_ASSIGN, // += (جمع_وتعيين)
    BAA_OP_SUB_ASSIGN, // -= (طرح_وتعيين)
    BAA_OP_MUL_ASSIGN, // *= (ضرب_وتعيين)
    BAA_OP_DIV_ASSIGN, // /= (قسمة_وتعيين)
    BAA_OP_MOD_ASSIGN,  // %= (باقي_وتعيين)

    // Member access and array subscript operators
    BAA_OP_DOT,       // .  (نقطة)
    BAA_OP_SUBSCRIPT, // [] (فهرس)

    // Increment/Decrement operators
    BAA_OP_INC,       // ++ (زيادة)
    BAA_OP_DEC,       // -- (نقصان)

    // Special value for no operator or default
    BAA_OP_NONE
} BaaOperatorType;

// Operator precedence levels
typedef enum {
    BAA_PREC_NONE,
    BAA_PREC_ASSIGNMENT,  // = += -= *= /= %=
    BAA_PREC_LOGICAL_OR,  // ||
    BAA_PREC_LOGICAL_AND, // &&
    BAA_PREC_BIT_OR,     // |
    BAA_PREC_BIT_XOR,    // ^
    BAA_PREC_BIT_AND,    // &
    BAA_PREC_EQUALITY,   // == !=
    BAA_PREC_COMPARISON, // < > <= >=
    BAA_PREC_SHIFT,     // << >>
    BAA_PREC_TERM,      // + -
    BAA_PREC_FACTOR,    // * / %
    BAA_PREC_UNARY,     // ! ~ - +
    BAA_PREC_CALL,      // . [] ()
    BAA_PREC_PRIMARY
} BaaOperatorPrecedence;

// Operator information
typedef struct {
    BaaOperatorType type;
    const wchar_t* symbol;      // Operator symbol
    const wchar_t* arabic_name; // Arabic name
    bool is_unary;             // Whether it can be used as unary operator
    bool is_binary;            // Whether it can be used as binary operator
    BaaOperatorPrecedence precedence;
    bool right_associative;
} BaaOperatorInfo;

// Operator functions
const BaaOperatorInfo* baa_get_operator_info(BaaOperatorType op);
BaaOperatorType baa_get_operator_from_symbol(const wchar_t* symbol);
BaaOperatorType baa_get_operator_from_name(const wchar_t* arabic_name);

// Type checking for operators
bool baa_validate_unary_op(BaaOperatorType op, BaaType* operand_type, BaaType** result_type);
bool baa_validate_binary_op(BaaOperatorType op, BaaType* left_type, BaaType* right_type, BaaType** result_type);

// Error reporting
const wchar_t* baa_get_operator_error(void);
void baa_clear_operator_error(void);

#endif /* BAA_OPERATORS_H */
