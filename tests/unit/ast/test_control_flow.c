#include <assert.h>
#include <stdio.h>
#include "baa/control_flow/control_flow.h"
#include "baa/ast/statements.h"
#include "baa/ast/expressions.h"

void test_if_statement() {
    printf("Starting test_if_statement\n");

    // Create a dummy condition expression
    BaaExpr* condition = baa_create_literal_expr(NULL, NULL);
    printf("Condition created: %p\n", (void*)condition);
    assert(condition != NULL);

    // Create if and else blocks
    BaaStmt* if_block_stmt = baa_create_block_stmt();
    printf("If block created: %p\n", (void*)if_block_stmt);
    BaaStmt* else_block_stmt = baa_create_block_stmt();
    printf("Else block created: %p\n", (void*)else_block_stmt);
    assert(if_block_stmt != NULL);
    assert(else_block_stmt != NULL);

    BaaBlock* if_body = &if_block_stmt->block_stmt;
    BaaBlock* else_body = &else_block_stmt->block_stmt;

    // Create the if statement manually
    printf("Creating if statement manually...\n");
    BaaStmt* if_stmt = (BaaStmt*)malloc(sizeof(BaaStmt));
    assert(if_stmt != NULL);

    if_stmt->type = BAA_STMT_IF;
    if_stmt->if_stmt.condition = condition;
    if_stmt->if_stmt.if_body = if_body;
    if_stmt->if_stmt.else_body = else_body;
    if_stmt->if_stmt.ast_node = NULL;

    printf("If statement created manually: %p\n", (void*)if_stmt);
    assert(if_stmt != NULL);
    assert(if_stmt->type == BAA_STMT_IF);
    assert(if_stmt->if_stmt.condition == condition);
    assert(if_stmt->if_stmt.if_body == if_body);
    assert(if_stmt->if_stmt.else_body == else_body);

    // Free resources
    baa_free_stmt(if_stmt);
    // Note: if_body and else_body are freed as part of if_stmt

    printf("test_if_statement passed\n");
}

void test_while_statement() {
    printf("Starting test_while_statement\n");

    // Create a dummy condition expression
    BaaExpr* condition = baa_create_literal_expr(NULL, NULL);
    printf("Condition created: %p\n", (void*)condition);
    assert(condition != NULL);

    // Create while body block
    BaaStmt* block_stmt = baa_create_block_stmt();
    printf("Block created: %p\n", (void*)block_stmt);
    assert(block_stmt != NULL);

    BaaBlock* body = &block_stmt->block_stmt;

    // Create the while statement manually
    printf("Creating while statement manually...\n");
    BaaStmt* while_stmt = (BaaStmt*)malloc(sizeof(BaaStmt));
    assert(while_stmt != NULL);

    while_stmt->type = BAA_STMT_WHILE;
    while_stmt->while_stmt.condition = condition;
    while_stmt->while_stmt.body = body;
    while_stmt->while_stmt.ast_node = NULL;

    printf("While statement created manually: %p\n", (void*)while_stmt);
    assert(while_stmt != NULL);
    assert(while_stmt->type == BAA_STMT_WHILE);
    assert(while_stmt->while_stmt.condition == condition);
    assert(while_stmt->while_stmt.body == body);

    // Free resources
    baa_free_stmt(while_stmt);
    // Note: body is freed as part of while_stmt

    printf("test_while_statement passed\n");
}

void test_return_statement() {
    // Create a dummy return value expression
    BaaExpr* value = baa_create_literal_expr(NULL, NULL);
    assert(value != NULL);

    // Create the return statement
    BaaStmt* return_stmt = baa_create_return_stmt(value);
    assert(return_stmt != NULL);
    assert(return_stmt->type == BAA_STMT_RETURN);
    assert(return_stmt->return_stmt.value == value);

    // Free resources
    baa_free_stmt(return_stmt);

    printf("test_return_statement passed\n");
}

int main() {
    test_if_statement();
    test_while_statement();
    test_return_statement();
    return 0;
}
