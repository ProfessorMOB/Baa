#include "baa/codegen/llvm_codegen.h"
#include "baa/ast/ast.h"
#include "baa/ast/expressions.h"
#include "baa/ast/statements.h"
#include "baa/ast/literals.h"
#include "baa/types/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int main() {
    // Initialize LLVM context
    BaaLLVMContext context;
    if (!baa_init_llvm_context(&context, L"test_module")) {
        fprintf(stderr, "Failed to initialize LLVM context: %ls\n", baa_get_llvm_error(&context));
        return 1;
    }

    // Create program structure
    BaaProgram program;
    program.functions = NULL;
    program.function_count = 0;
    program.function_capacity = 0;
    program.ast_node = NULL; // Can be set if needed

    // Create main function signature
    BaaFunction* main_function = baa_create_function_signature(L"main", wcslen(L"main"));
    if (!main_function) {
        fprintf(stderr, "Failed to create main function signature\n");
        baa_cleanup_llvm_context(&context);
        return 1;
    }

    // Create function body (a block statement)
    BaaStmt* body_stmt = baa_create_block_stmt(); // Returns BaaStmt*
    if (!body_stmt || body_stmt->kind != BAA_STMT_BLOCK) {
        fprintf(stderr, "Failed to create function body block statement\n");
        baa_free_function(main_function);
        baa_cleanup_llvm_context(&context);
        return 1;
    }
    BaaBlock* body_block = (BaaBlock*)body_stmt->data; // Extract block data
    main_function->body = body_block; // Assign block to function

    // Create return statement
    // 1. Create literal data
    BaaLiteralData* literal_data = baa_create_int_literal_data(42);
    if (!literal_data) {
        fprintf(stderr, "Failed to create literal data\n");
        body_stmt->data = NULL; baa_free_stmt(body_stmt); main_function->body = NULL;
        baa_free_function(main_function);
        baa_cleanup_llvm_context(&context);
        return 1;
    }
    // 2. Create literal expression
    BaaExpr* int_expr = baa_create_literal_expr(literal_data); // Pass literal_data
    if (!int_expr) {
        fprintf(stderr, "Failed to create literal expression\n");
        baa_free_literal_data(literal_data);
        body_stmt->data = NULL; baa_free_stmt(body_stmt); main_function->body = NULL;
        baa_free_function(main_function);
        baa_cleanup_llvm_context(&context);
        return 1;
    }

    // 3. Create return statement
    BaaStmt* return_stmt = baa_create_return_stmt(int_expr); // Pass int_expr
    if (!return_stmt) {
        fprintf(stderr, "Failed to create return statement\n");
        baa_free_expr(int_expr); // Frees literal_data too
        body_stmt->data = NULL; baa_free_stmt(body_stmt); main_function->body = NULL;
        baa_free_function(main_function);
        baa_cleanup_llvm_context(&context);
        return 1;
    }

    // Add return statement to the function body block
    if (!baa_add_stmt_to_block(body_block, return_stmt)) {
         fprintf(stderr, "Failed to add return statement to block\n");
         baa_free_stmt(return_stmt); // Frees expr/literal
         body_stmt->data = NULL; baa_free_stmt(body_stmt); main_function->body = NULL;
         baa_free_function(main_function);
         baa_cleanup_llvm_context(&context);
         return 1;
    }

    // Add function to program
    if (!baa_add_function_to_program(&program, main_function)) {
        fprintf(stderr, "Failed to add function to program\n");
        // Cleanup is complex here
        baa_free_function(main_function);
        baa_cleanup_llvm_context(&context);
        return 1;
    }

    // Generate LLVM IR
    if (!baa_generate_llvm_ir(&context, &program)) { // Pass &context
        fprintf(stderr, "Failed to generate LLVM IR: %ls\n", baa_get_llvm_error(&context));
        baa_free_program(&program);
        baa_cleanup_llvm_context(&context);
        return 1;
    }

    printf("LLVM IR generated successfully.\n");

    // Clean up
    baa_free_program(&program);
    baa_cleanup_llvm_context(&context);

    return 0;
}
