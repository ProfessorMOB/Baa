#include "baa/control_flow.h"
#include <stdlib.h>

BaaStatement* baa_create_if_statement(BaaExpression* condition, BaaBlock* if_body, BaaBlock* else_body) {
    BaaStatement* stmt = (BaaStatement*)malloc(sizeof(BaaStatement));
    if (!stmt) return NULL;

    stmt->type = BAA_STMT_IF;
    stmt->if_stmt.condition = condition;
    stmt->if_stmt.if_body = if_body;
    stmt->if_stmt.else_body = else_body;

    return stmt;
}

BaaStatement* baa_create_while_statement(BaaExpression* condition, BaaBlock* body) {
    BaaStatement* stmt = (BaaStatement*)malloc(sizeof(BaaStatement));
    if (!stmt) return NULL;

    stmt->type = BAA_STMT_WHILE;
    stmt->while_stmt.condition = condition;
    stmt->while_stmt.body = body;

    return stmt;
}

BaaStatement* baa_create_return_statement(BaaExpression* value) {
    BaaStatement* stmt = (BaaStatement*)malloc(sizeof(BaaStatement));
    if (!stmt) return NULL;

    stmt->type = BAA_STMT_RETURN;
    stmt->return_stmt.value = value;

    return stmt;
}

void baa_free_statement(BaaStatement* stmt) {
    if (!stmt) return;

    switch (stmt->type) {
        case BAA_STMT_IF:
            // Free condition and bodies handled by expression/block free functions
            break;
        case BAA_STMT_WHILE:
            // Free condition and body handled by expression/block free functions
            break;
        case BAA_STMT_RETURN:
            // Free value handled by expression free function
            break;
    }

    free(stmt);
}
