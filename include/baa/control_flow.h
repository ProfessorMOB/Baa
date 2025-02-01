#ifndef BAA_CONTROL_FLOW_H
#define BAA_CONTROL_FLOW_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    BAA_STMT_IF,
    BAA_STMT_WHILE,
    BAA_STMT_RETURN
} BaaStatementType;

typedef struct BaaStatement BaaStatement;
typedef struct BaaExpression BaaExpression;
typedef struct BaaBlock BaaBlock;

// If statement structure
typedef struct {
    BaaExpression* condition;
    BaaBlock* if_body;
    BaaBlock* else_body;  // Can be NULL if no else clause
} BaaIfStatement;

// While loop structure
typedef struct {
    BaaExpression* condition;
    BaaBlock* body;
} BaaWhileStatement;

// Return statement structure
typedef struct {
    BaaExpression* value;  // Can be NULL for void returns
} BaaReturnStatement;

// Generic statement structure
struct BaaStatement {
    BaaStatementType type;
    union {
        BaaIfStatement if_stmt;
        BaaWhileStatement while_stmt;
        BaaReturnStatement return_stmt;
    };
};

// Function declarations
BaaStatement* baa_create_if_statement(BaaExpression* condition, BaaBlock* if_body, BaaBlock* else_body);
BaaStatement* baa_create_while_statement(BaaExpression* condition, BaaBlock* body);
BaaStatement* baa_create_return_statement(BaaExpression* value);
void baa_free_statement(BaaStatement* stmt);

#endif /* BAA_CONTROL_FLOW_H */
