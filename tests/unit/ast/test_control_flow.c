#include <assert.h>
#include <stdio.h>
#include <stdlib.h> // Include for malloc/free if needed, though utils should be used

// Use the canonical headers now
#include "baa/ast/statements.h"
#include "baa/ast/expressions.h"
#include "baa/ast/literals.h"
#include "baa/types/types.h"
#include "baa/utils/utils.h" // For baa_malloc, baa_free

// Assuming these are declared in ast.h or similar (included via statements.h/expressions.h)
// If not, add forward declarations or include the correct header
extern BaaNode *baa_create_node(BaaNodeKind kind, void *data);
extern void baa_free_node(BaaNode *node);

void test_if_statement()
{
    printf("Starting test_if_statement\n");

    // Initialize necessary subsystems (like type system)
    baa_init_type_system();

    // Create a dummy boolean condition expression
    BaaLiteralData *true_data = baa_create_bool_literal_data(true);
    assert(true_data != NULL);
    BaaExpr *condition = baa_create_literal_expr(true_data);
    // Don't free true_data here, ownership transferred to expression if successful
    assert(condition != NULL);
    printf("Condition created: %p\n", (void *)condition);

    // Create if and else blocks using the AST function
    BaaStmt *if_block_stmt_wrapper = baa_create_block_stmt(); // Wrapper statement
    assert(if_block_stmt_wrapper != NULL);
    BaaBlock *if_body = (BaaBlock *)if_block_stmt_wrapper->data; // Get the block data
    printf("If block created: %p\n", (void *)if_body);

    BaaStmt *else_block_stmt_wrapper = baa_create_block_stmt(); // Wrapper statement
    assert(else_block_stmt_wrapper != NULL);
    BaaBlock *else_body = (BaaBlock *)else_block_stmt_wrapper->data; // Get the block data
    printf("Else block created: %p\n", (void *)else_body);

    // Optionally add some dummy statements to the blocks
    BaaLiteralData *zero_data = baa_create_int_literal_data(0);
    BaaExpr *zero_expr = baa_create_literal_expr(zero_data);
    BaaStmt *dummy_stmt1 = baa_create_expr_stmt(zero_expr);
    assert(baa_add_stmt_to_block(if_body, dummy_stmt1));

    BaaLiteralData *one_data = baa_create_int_literal_data(1);
    BaaExpr *one_expr = baa_create_literal_expr(one_data);
    BaaStmt *dummy_stmt2 = baa_create_expr_stmt(one_expr);
    assert(baa_add_stmt_to_block(else_body, dummy_stmt2));

    // Create the if statement using the canonical AST function
    printf("Creating if statement using baa_create_if_stmt...\n");
    BaaStmt *if_stmt = baa_create_if_stmt(condition, if_body, else_body);
    // Note: baa_create_if_stmt takes ownership of condition, if_body, else_body.
    // The wrappers if_block_stmt_wrapper/else_block_stmt_wrapper still exist but their .data points to the same blocks.

    printf("If statement created: %p\n", (void *)if_stmt);
    assert(if_stmt != NULL);
    assert(if_stmt->kind == BAA_STMT_IF);            // Use 'kind'
    BaaIfStmt *if_data = (BaaIfStmt *)if_stmt->data; // Access via 'data' pointer
    assert(if_data != NULL);
    assert(if_data->condition == condition);
    assert(if_data->if_body == if_body);
    assert(if_data->else_body == else_body);

    // Free resources
    // Freeing the main if_stmt should recursively free condition, if_body, else_body, and their contents.
    baa_free_stmt(if_stmt);

    // Free the wrapper statements (their data pointers are now dangling, but that's ok as we free the wrapper)
    baa_free_stmt(if_block_stmt_wrapper);
    baa_free_stmt(else_block_stmt_wrapper);

    // Free original literal data used to create expressions
    baa_free_literal_data(true_data);
    baa_free_literal_data(zero_data);
    baa_free_literal_data(one_data);

    printf("test_if_statement passed\n");
}

void test_while_statement()
{
    printf("Starting test_while_statement\n");

    // Initialize necessary subsystems (like type system)
    baa_init_type_system();

    // Create a dummy condition expression
    BaaLiteralData *true_data = baa_create_bool_literal_data(true);
    assert(true_data != NULL);
    BaaExpr *condition = baa_create_literal_expr(true_data);
    assert(condition != NULL);
    printf("Condition created: %p\n", (void *)condition);

    // Create while body block using the AST function
    BaaStmt *block_stmt_wrapper = baa_create_block_stmt();
    assert(block_stmt_wrapper != NULL);
    BaaBlock *body = (BaaBlock *)block_stmt_wrapper->data; // Get the block data
    printf("Block created: %p\n", (void *)body);

    // Add a dummy statement to the body
    BaaLiteralData *zero_data = baa_create_int_literal_data(0);
    BaaExpr *zero_expr = baa_create_literal_expr(zero_data);
    BaaStmt *dummy_stmt = baa_create_expr_stmt(zero_expr);
    assert(baa_add_stmt_to_block(body, dummy_stmt));

    // Create the while statement using the canonical AST function
    printf("Creating while statement using baa_create_while_stmt...\n");
    BaaStmt *while_stmt = baa_create_while_stmt(condition, body);
    // baa_create_while_stmt takes ownership of condition and body

    printf("While statement created: %p\n", (void *)while_stmt);
    assert(while_stmt != NULL);
    assert(while_stmt->kind == BAA_STMT_WHILE);                  // Use 'kind'
    BaaWhileStmt *while_data = (BaaWhileStmt *)while_stmt->data; // Access via 'data' pointer
    assert(while_data != NULL);
    assert(while_data->condition == condition);
    assert(while_data->body == body);

    // Free resources
    baa_free_stmt(while_stmt); // Should free condition, body, and dummy_stmt

    // Free the wrapper statement
    baa_free_stmt(block_stmt_wrapper);

    // Free original literal data
    baa_free_literal_data(true_data);
    baa_free_literal_data(zero_data);

    printf("test_while_statement passed\n");
}

void test_return_statement()
{
    printf("Starting test_return_statement\n");

    // Initialize necessary subsystems (like type system)
    baa_init_type_system();

    // Create a dummy return value expression
    BaaLiteralData *ret_val_data = baa_create_int_literal_data(5);
    assert(ret_val_data != NULL);
    BaaExpr *value = baa_create_literal_expr(ret_val_data);
    assert(value != NULL);

    // Create the return statement using the canonical AST function
    BaaStmt *return_stmt = baa_create_return_stmt(value);
    // baa_create_return_stmt takes ownership of value
    assert(return_stmt != NULL);
    assert(return_stmt->kind == BAA_STMT_RETURN);                    // Use 'kind'
    BaaReturnStmt *return_data = (BaaReturnStmt *)return_stmt->data; // Access via 'data'
    assert(return_data != NULL);
    assert(return_data->value == value);

    // Free resources
    baa_free_stmt(return_stmt); // Should free the value expression

    // Free original literal data
    baa_free_literal_data(ret_val_data);

    // Test return without value (void return)
    BaaStmt *void_return_stmt = baa_create_return_stmt(NULL);
    assert(void_return_stmt != NULL);
    assert(void_return_stmt->kind == BAA_STMT_RETURN);
    BaaReturnStmt *void_return_data = (BaaReturnStmt *)void_return_stmt->data;
    assert(void_return_data != NULL);
    assert(void_return_data->value == NULL);
    baa_free_stmt(void_return_stmt);

    printf("test_return_statement passed\n");
}

int main()
{
    test_if_statement();
    test_while_statement();
    test_return_statement();
    printf("\nAll control flow AST creation tests passed!\n");
    return 0;
}
