#ifndef BAA_STATEMENTS_H
#define BAA_STATEMENTS_H

#include "baa/ast/ast.h"
#include "baa/ast/expressions.h"
#include "baa/types/types.h"
#include <stdbool.h>
#include <stddef.h>

// Statement types
typedef enum {
    BAA_STMT_IF,      // If statement
    BAA_STMT_WHILE,   // While loop
    BAA_STMT_FOR,     // For loop
    BAA_STMT_RETURN,  // Return statement
    BAA_STMT_EXPR,    // Expression statement
    BAA_STMT_BLOCK,   // Block statement
    BAA_STMT_VAR_DECL, // Variable declaration
    BAA_STMT_SWITCH,  // Switch statement
    BAA_STMT_CASE,    // Case statement
    BAA_STMT_DEFAULT, // Default case statement
    BAA_STMT_BREAK,   // Break statement
    BAA_STMT_CONTINUE, // Continue statement
    BAA_STMT_IMPORT   // Import statement
} BaaStmtKind;

// Forward declarations
typedef struct BaaStmt BaaStmt;
typedef struct BaaBlock BaaBlock;

// Block structure
struct BaaBlock {
    BaaStmt** statements;
    size_t count;
    size_t capacity;
};

// If statement
typedef struct {
    BaaExpr* condition;
    BaaBlock* if_body;
    BaaBlock* else_body;  // Can be NULL if no else clause
} BaaIfStmt;

// While loop
typedef struct {
    BaaExpr* condition;
    BaaBlock* body;
} BaaWhileStmt;

// For loop
typedef struct {
    BaaStmt* initializer;   // Initialization statement (can be expression or variable declaration)
    BaaExpr* condition;     // Loop condition expression
    BaaExpr* increment;     // Increment expression
    BaaBlock* body;         // Loop body
} BaaForStmt;

// Return statement
typedef struct {
    BaaExpr* value;      // Can be NULL for void returns
} BaaReturnStmt;

// Expression statement
typedef struct {
    BaaExpr* expr;       // The expression to evaluate
} BaaExprStmt;

// Variable declaration
typedef struct {
    const wchar_t* name;  // Variable name
    size_t name_length;   // Name length
    BaaType* type;        // Variable type
    BaaExpr* initializer; // Initial value (can be NULL)
} BaaVarDeclStmt;

// Case statement in a switch
typedef struct {
    BaaExpr* value;        // Case value expression
    BaaBlock* body;        // Case body
    bool has_fallthrough;  // Whether this case falls through to the next one
} BaaCaseStmt;

// Default case statement in a switch
typedef struct {
    BaaBlock* body;        // Default case body
} BaaDefaultStmt;

// Break statement
typedef struct {
    int loop_depth;       // How many levels to break out of (default: 1)
    bool is_switch_break; // Whether this break is for a switch statement
} BaaBreakStmt;

// Continue statement
typedef struct {
    int loop_depth;       // How many levels to continue (default: 1)
} BaaContinueStmt;

// Switch statement
typedef struct {
    BaaExpr* expression;   // Switch expression
    BaaCaseStmt** cases;   // Array of case statements
    size_t case_count;     // Number of cases
    BaaDefaultStmt* default_case; // Default case (can be NULL)
} BaaSwitchStmt;

// Import statement
typedef struct {
    const wchar_t* path;  // Path being imported
    const wchar_t* alias; // Optional alias (can be NULL)
} BaaImportStmt;

// Generic statement structure - consistent with our expression structure
struct BaaStmt {
    BaaStmtKind kind;     // Statement kind
    BaaNode* ast_node;    // Link to AST node
    void* data;           // Points to the specific statement data structure
};

// Statement creation functions
BaaStmt* baa_create_if_stmt(BaaExpr* condition, BaaBlock* if_body, BaaBlock* else_body);
BaaStmt* baa_create_while_stmt(BaaExpr* condition, BaaBlock* body);
BaaStmt* baa_create_for_stmt(BaaStmt* initializer, BaaExpr* condition, BaaExpr* increment, BaaBlock* body);
BaaStmt* baa_create_return_stmt(BaaExpr* value);
BaaStmt* baa_create_expr_stmt(BaaExpr* expr);
BaaStmt* baa_create_block_stmt(void);
BaaStmt* baa_create_var_decl_stmt(const wchar_t* name, size_t name_length, BaaType* type, BaaExpr* initializer);
BaaStmt* baa_create_switch_stmt(BaaExpr* expression);
BaaStmt* baa_create_case_stmt(BaaExpr* value, BaaBlock* body, bool has_fallthrough);
BaaStmt* baa_create_default_stmt(BaaBlock* body);
BaaStmt* baa_create_break_stmt(int loop_depth, bool is_switch_break);
BaaStmt* baa_create_continue_stmt(int loop_depth);
bool baa_add_case_to_switch(BaaStmt* switch_stmt, BaaStmt* case_stmt);
bool baa_set_default_case(BaaStmt* switch_stmt, BaaStmt* default_stmt);
BaaStmt* baa_create_import_stmt(const wchar_t* path, const wchar_t* alias); // Added declaration

// Block management
bool baa_add_stmt_to_block(BaaBlock* block, BaaStmt* stmt);
BaaBlock* baa_create_block(void);
void baa_free_block(BaaBlock* block);

// Statement validation
bool baa_validate_condition(BaaExpr* condition);
bool baa_validate_return_value(BaaExpr* value, BaaType* expected_type);
bool baa_validate_var_decl(const wchar_t* name, size_t name_length, BaaType* type, BaaExpr* initializer);
bool baa_validate_for_stmt(BaaStmt* initializer, BaaExpr* condition, BaaExpr* increment);
bool baa_validate_switch_expr(BaaExpr* expression);
bool baa_validate_case_value(BaaExpr* value, BaaExpr* switch_expr);
bool baa_validate_break_stmt(int loop_depth, bool is_switch_break);
bool baa_validate_continue_stmt(int loop_depth);

// Memory management
void baa_free_stmt(BaaStmt* stmt);

#endif /* BAA_STATEMENTS_H */
