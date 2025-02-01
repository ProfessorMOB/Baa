#include "baa/control_flow.h"
#include <assert.h>
#include <stdio.h>

void test_if_statement() {
    // Create a dummy condition and blocks for testing
    BaaExpression* condition = NULL;  // In real usage, this would be a valid expression
    BaaBlock* if_body = NULL;        // In real usage, this would be a valid block
    BaaBlock* else_body = NULL;      // In real usage, this would be a valid block

    BaaStatement* if_stmt = baa_create_if_statement(condition, if_body, else_body);
    assert(if_stmt != NULL);
    assert(if_stmt->type == BAA_STMT_IF);
    assert(if_stmt->if_stmt.condition == condition);
    assert(if_stmt->if_stmt.if_body == if_body);
    assert(if_stmt->if_stmt.else_body == else_body);

    baa_free_statement(if_stmt);
    printf("If statement test passed\n");
}

void test_while_statement() {
    BaaExpression* condition = NULL;  // In real usage, this would be a valid expression
    BaaBlock* body = NULL;           // In real usage, this would be a valid block

    BaaStatement* while_stmt = baa_create_while_statement(condition, body);
    assert(while_stmt != NULL);
    assert(while_stmt->type == BAA_STMT_WHILE);
    assert(while_stmt->while_stmt.condition == condition);
    assert(while_stmt->while_stmt.body == body);

    baa_free_statement(while_stmt);
    printf("While statement test passed\n");
}

void test_return_statement() {
    BaaExpression* value = NULL;     // In real usage, this could be NULL or a valid expression

    BaaStatement* return_stmt = baa_create_return_statement(value);
    assert(return_stmt != NULL);
    assert(return_stmt->type == BAA_STMT_RETURN);
    assert(return_stmt->return_stmt.value == value);

    baa_free_statement(return_stmt);
    printf("Return statement test passed\n");
}

int main() {
    test_if_statement();
    test_while_statement();
    test_return_statement();
    printf("All control flow tests passed!\n");
    return 0;
}
