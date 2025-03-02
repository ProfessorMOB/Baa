#ifndef BAA_EXPRESSIONS_H
#define BAA_EXPRESSIONS_H

#include "baa/ast/ast.h"
#include "baa/ast/literals.h"
#include "baa/operators/operators.h"
#include "baa/types/types.h"
#include <stddef.h>
#include <stdbool.h>
#include <wchar.h>

// Expression node types
typedef enum {
    BAA_EXPR_LITERAL,        // Literal value (number, text, etc.)
    BAA_EXPR_VARIABLE,       // Variable reference
    BAA_EXPR_UNARY,          // Unary operation
    BAA_EXPR_BINARY,         // Binary operation
    BAA_EXPR_CALL,           // Function call
    BAA_EXPR_CAST,           // Type cast expression
    BAA_EXPR_ASSIGN,         // Assignment
    BAA_EXPR_ARRAY,          // Array creation
    BAA_EXPR_INDEX,          // Array indexing
    BAA_EXPR_COMPOUND_ASSIGN, // Compound assignment (+=, -=, etc.)
    BAA_EXPR_INC_DEC,        // Increment/Decrement (++, --)
    BAA_EXPR_GROUPING        // Grouping expression (parentheses)
} BaaExprKind;

// Forward declaration
typedef struct BaaExpr BaaExpr;

// Literal expression
typedef struct {
    BaaNode* value;      // AST node containing the literal value
    BaaType* type;       // Type of the literal
} BaaLiteralExpr;

// Variable expression
typedef struct {
    const wchar_t* name;  // Variable name
    size_t name_length;   // Name length
} BaaVariableExpr;

// Unary expression
typedef struct {
    BaaOperatorType op;   // Operator type
    BaaExpr* operand;     // Operand
} BaaUnaryExpr;

// Binary expression
typedef struct {
    BaaOperatorType op;   // Operator type
    BaaExpr* left;        // Left operand
    BaaExpr* right;       // Right operand
} BaaBinaryExpr;

// Function call expression
typedef struct {
    BaaExpr* callee;      // Function being called
    BaaExpr** arguments;  // Arguments
    size_t argument_count; // Number of arguments
    wchar_t** named_args; // Names for named arguments (NULL for positional)
    bool is_method_call;  // Whether this is a method call (obj.method())
} BaaCallExpr;

// Type cast expression
typedef struct {
    BaaExpr* operand;     // Expression to cast
    BaaType* target_type; // Type to cast to
} BaaCastExpr;

// Assignment expression
typedef struct {
    BaaExpr* target;      // Assignment target
    BaaExpr* value;       // Value being assigned
} BaaAssignExpr;

// Compound assignment expression (+=, -=, etc.)
typedef struct {
    BaaExpr* target;       // Target of the assignment (lvalue)
    BaaExpr* value;        // Value to assign
    BaaOperatorType operator_type; // Type of compound operator
} BaaCompoundAssignmentData;

// Increment/Decrement expression (++, --)
typedef struct {
    BaaExpr* operand;     // Operand
    BaaOperatorType operator_type; // INC or DEC
    bool is_prefix;       // true for prefix (++i), false for postfix (i++)
} BaaIncDecData;

// Grouping expression (parentheses)
typedef struct {
    BaaExpr* expression;  // Contained expression
} BaaGroupingData;

// Array creation expression
typedef struct {
    BaaExpr** elements;   // Array elements
    size_t element_count; // Number of elements
    BaaType* element_type; // Type of array elements
} BaaArrayExpr;

// Array indexing expression
typedef struct {
    BaaExpr* array;       // Array being indexed
    BaaExpr* index;       // Index expression
} BaaIndexExpr;

// Main expression structure
struct BaaExpr {
    BaaExprKind kind;     // Expression kind
    BaaType* type;        // Result type (moved from individual expressions)
    BaaNode* ast_node;    // Link to AST node
    void* data;           // Points to the specific expression data structure
};

// Expression creation functions
BaaExpr* baa_create_literal_expr(BaaLiteralData* literal_data);
BaaExpr* baa_create_variable_expr(const wchar_t* name, size_t name_length);
BaaExpr* baa_create_unary_expr(BaaOperatorType op, BaaExpr* operand);
BaaExpr* baa_create_binary_expr(BaaOperatorType op, BaaExpr* left, BaaExpr* right);
BaaExpr* baa_create_call_expr(BaaExpr* callee, BaaExpr** arguments, size_t argument_count, wchar_t** named_args, bool is_method_call);
BaaExpr* baa_create_cast_expr(BaaExpr* operand, BaaType* target_type);
BaaExpr* baa_create_assign_expr(BaaExpr* target, BaaExpr* value);
BaaExpr* baa_create_array_expr(BaaExpr** elements, size_t element_count, BaaType* element_type);
BaaExpr* baa_create_index_expr(BaaExpr* array, BaaExpr* index);
BaaExpr* baa_create_compound_assignment_expr(BaaExpr* target, BaaExpr* value, BaaOperatorType operator_type);
BaaExpr* baa_create_inc_dec_expr(BaaExpr* operand, BaaOperatorType operator_type, bool is_prefix);
BaaExpr* baa_create_grouping_expr(BaaExpr* expression);

// Expression type checking and validation
bool baa_validate_expr_type(BaaExpr* expr, BaaType* expected_type);
BaaType* baa_get_expr_type(BaaExpr* expr);
bool baa_validate_array_elements(BaaExpr** elements, size_t element_count, BaaType* element_type);
bool baa_validate_index_expr(BaaExpr* array, BaaExpr* index);

// Memory management
void baa_free_expr(BaaExpr* expr);

#endif /* BAA_EXPRESSIONS_H */
