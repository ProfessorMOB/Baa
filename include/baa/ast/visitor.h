#ifndef BAA_VISITOR_H
#define BAA_VISITOR_H

#include "baa/ast/ast.h"
#include "baa/ast/expressions.h"
#include "baa/ast/statements.h"
#include <stdbool.h>

// Forward declarations
typedef struct BaaVisitor BaaVisitor;

// Visit result
typedef struct {
    bool success;
    void* data;
    const wchar_t* error_message;
} BaaVisitResult;

// Visitor callbacks for expressions
typedef BaaVisitResult (*BaaVisitLiteralExpr)(BaaVisitor* visitor, BaaLiteralExpr* expr);
typedef BaaVisitResult (*BaaVisitIdentifierExpr)(BaaVisitor* visitor, BaaIdentifierExpr* expr);
typedef BaaVisitResult (*BaaVisitUnaryExpr)(BaaVisitor* visitor, BaaUnaryExpr* expr);
typedef BaaVisitResult (*BaaVisitBinaryExpr)(BaaVisitor* visitor, BaaBinaryExpr* expr);
typedef BaaVisitResult (*BaaVisitCallExpr)(BaaVisitor* visitor, BaaCallExpr* expr);
typedef BaaVisitResult (*BaaVisitAssignExpr)(BaaVisitor* visitor, BaaAssignExpr* expr);

// Visitor callbacks for statements
typedef BaaVisitResult (*BaaVisitIfStmt)(BaaVisitor* visitor, BaaIfStmt* stmt);
typedef BaaVisitResult (*BaaVisitWhileStmt)(BaaVisitor* visitor, BaaWhileStmt* stmt);
typedef BaaVisitResult (*BaaVisitReturnStmt)(BaaVisitor* visitor, BaaReturnStmt* stmt);
typedef BaaVisitResult (*BaaVisitExprStmt)(BaaVisitor* visitor, BaaExprStmt* stmt);
typedef BaaVisitResult (*BaaVisitBlockStmt)(BaaVisitor* visitor, BaaBlock* stmt);
typedef BaaVisitResult (*BaaVisitVarDeclStmt)(BaaVisitor* visitor, BaaVarDeclStmt* stmt);

// Visitor callbacks for program elements
typedef BaaVisitResult (*BaaVisitProgram)(BaaVisitor* visitor, BaaProgram* program);
typedef BaaVisitResult (*BaaVisitFunction)(BaaVisitor* visitor, BaaFunction* function);

// Visitor structure
struct BaaVisitor {
    // Expression visitors
    BaaVisitLiteralExpr visit_literal;
    BaaVisitIdentifierExpr visit_identifier;
    BaaVisitUnaryExpr visit_unary;
    BaaVisitBinaryExpr visit_binary;
    BaaVisitCallExpr visit_call;
    BaaVisitAssignExpr visit_assign;

    // Statement visitors
    BaaVisitIfStmt visit_if;
    BaaVisitWhileStmt visit_while;
    BaaVisitReturnStmt visit_return;
    BaaVisitExprStmt visit_expr;
    BaaVisitBlockStmt visit_block;
    BaaVisitVarDeclStmt visit_var_decl;

    // Program element visitors
    BaaVisitProgram visit_program;
    BaaVisitFunction visit_function;

    // Visitor context data
    void* context;
};

// Visitor creation and management
BaaVisitor* baa_create_visitor(void);
void baa_free_visitor(BaaVisitor* visitor);

// Visit functions
BaaVisitResult baa_visit_expr(BaaVisitor* visitor, BaaExpr* expr);
BaaVisitResult baa_visit_stmt(BaaVisitor* visitor, BaaStmt* stmt);
BaaVisitResult baa_visit_program(BaaVisitor* visitor, BaaProgram* program);

// Utility functions
BaaVisitResult baa_visit_success(void* data);
BaaVisitResult baa_visit_error(const wchar_t* message);

#endif /* BAA_VISITOR_H */
