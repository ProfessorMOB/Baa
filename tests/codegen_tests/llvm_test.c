#include "baa/codegen/llvm_codegen.h"
#include "baa/ast/ast.h"
#include "baa/types/types.h"
#include <stdlib.h>
#include <stdio.h>

int main() {
    // Initialize LLVM codegen
    BaaLLVMCodegen* codegen = baa_init_llvm_codegen();
    if (!codegen) {
        fprintf(stderr, "Failed to initialize LLVM codegen\n");
        return 1;
    }

    // Create a simple program with a function that returns 42
    BaaProgram program = {0};
    program.function_count = 1;
    program.functions = (BaaFunction**)malloc(sizeof(BaaFunction*));
    if (!program.functions) {
        fprintf(stderr, "Failed to allocate functions array\n");
        baa_free_llvm_codegen(codegen);
        return 1;
    }

    // Create main function
    BaaFunction* main_function = baa_create_function_signature(L"main", wcslen(L"main"));
    if (!main_function) {
        fprintf(stderr, "Failed to create main function\n");
        free(program.functions);
        baa_free_llvm_codegen(codegen);
        return 1;
    }
    program.functions[0] = main_function;

    // Create function body
    BaaBlock* body = baa_create_block_stmt();
    if (!body) {
        fprintf(stderr, "Failed to create function body\n");
        baa_free_function(main_function);
        free(program.functions);
        baa_free_llvm_codegen(codegen);
        return 1;
    }

    // Create return statement
    BaaStmt* return_stmt = baa_create_return_stmt();
    if (!return_stmt) {
        fprintf(stderr, "Failed to create return statement\n");
        baa_free_block(body);
        baa_free_function(main_function);
        free(program.functions);
        baa_free_llvm_codegen(codegen);
        return 1;
    }

    // Create integer literal expression
    BaaExpr* int_expr = baa_create_literal_expr();
    if (!int_expr) {
        fprintf(stderr, "Failed to create integer literal\n");
        baa_free_stmt(return_stmt);
        baa_free_block(body);
        baa_free_function(main_function);
        free(program.functions);
        baa_free_llvm_codegen(codegen);
        return 1;
    }

    // Set up the expression
    BaaLiteralData* literal_data = baa_create_int_literal_data(42);
    if (!literal_data) {
        fprintf(stderr, "Failed to create literal data\n");
        baa_free_expr(int_expr);
        baa_free_stmt(return_stmt);
        baa_free_block(body);
        baa_free_function(main_function);
        free(program.functions);
        baa_free_llvm_codegen(codegen);
        return 1;
    }
    int_expr->data.literal = literal_data;

    // Set up the return statement
    return_stmt->data.return_stmt.expr = int_expr;

    // Add return statement to body
    if (!baa_add_stmt_to_block(body, return_stmt)) {
        fprintf(stderr, "Failed to add statement to block\n");
        baa_free_literal_data(literal_data);
        baa_free_expr(int_expr);
        baa_free_stmt(return_stmt);
        baa_free_block(body);
        baa_free_function(main_function);
        free(program.functions);
        baa_free_llvm_codegen(codegen);
        return 1;
    }

    // Set function body
    main_function->body = body;

    // Generate LLVM IR
    if (!baa_generate_llvm_ir(codegen, &program)) {
        fprintf(stderr, "Failed to generate LLVM IR\n");
        baa_free_block(body);
        baa_free_function(main_function);
        free(program.functions);
        baa_free_llvm_codegen(codegen);
        return 1;
    }

    // Clean up
    baa_free_block(body);
    baa_free_function(main_function);
    free(program.functions);
    baa_free_llvm_codegen(codegen);

    return 0;
}
