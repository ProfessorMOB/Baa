#include "baa/ast/ast.h"
#include "baa/ast/literals.h"
#include "baa/ast/expressions.h"
#include "baa/ast/statements.h"
#include "baa/codegen/llvm_codegen.h"
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>

// Helper function to free AST memory
void cleanup_ast(BaaProgram* program) {
    if (program && program->functions) {
        for (size_t i = 0; i < program->function_count; i++) {
            BaaFunction* func = &program->functions[i];
            
            // Free function body
            if (func->body) {
                // Recursive cleanup would be implemented here
                free(func->body);
            }
            
            // Free return type
            if (func->return_type) {
                free(func->return_type);
            }
        }
        
        free(program->functions);
    }
}

// Test program to verify LLVM code generation with updated literal system
int main() {
    // Initialize LLVM context
    BaaLLVMContext context;
    if (!baa_init_llvm_context(&context, L"test_module")) {
        fprintf(stderr, "Failed to initialize LLVM context\n");
        return 1;
    }

    // Create a simple program
    BaaProgram program;
    program.functions = NULL;
    program.function_count = 0;

    // Create a function
    program.functions = (BaaFunction*)malloc(sizeof(BaaFunction));
    program.function_count = 1;
    
    BaaFunction* main_function = &program.functions[0];
    main_function->name = L"main";
    main_function->parameters = NULL;
    main_function->parameter_count = 0;
    
    // Create return type (int)
    main_function->return_type = (BaaType*)malloc(sizeof(BaaType));
    main_function->return_type->kind = BAA_TYPE_INT;

    // 1. Create a literal expression for returning 42
    BaaExpr* literal_expr = (BaaExpr*)malloc(sizeof(BaaExpr));
    literal_expr->kind = BAA_EXPR_LITERAL;
    
    // Create literal data
    BaaLiteralData* literal_data = (BaaLiteralData*)malloc(sizeof(BaaLiteralData));
    literal_data->kind = BAA_LITERAL_INT;
    literal_data->int_value = 42;
    literal_expr->data = literal_data;
    
    // 2. Create a return statement
    BaaStmt* return_stmt = (BaaStmt*)malloc(sizeof(BaaStmt));
    return_stmt->kind = BAA_STMT_RETURN;
    
    BaaReturnStmt* return_stmt_data = (BaaReturnStmt*)malloc(sizeof(BaaReturnStmt));
    return_stmt_data->value = literal_expr;
    return_stmt->data = return_stmt_data;
    
    // 3. Create a block statement as the function body
    BaaStmt* block_stmt = (BaaStmt*)malloc(sizeof(BaaStmt));
    block_stmt->kind = BAA_STMT_BLOCK;
    
    BaaBlockStmt* block_stmt_data = (BaaBlockStmt*)malloc(sizeof(BaaBlockStmt));
    block_stmt_data->statements = (BaaStmt*)malloc(sizeof(BaaStmt));
    block_stmt_data->statements[0] = *return_stmt;
    block_stmt_data->statement_count = 1;
    block_stmt->data = block_stmt_data;
    
    main_function->body = block_stmt;
    
    // Generate LLVM IR
    if (!baa_generate_llvm_ir(&context, &program)) {
        const wchar_t* error = context.error_message;
        wprintf(L"Failed to generate LLVM IR: %ls\n", error ? error : L"Unknown error");
        baa_cleanup_llvm_context(&context);
        cleanup_ast(&program);
        return 1;
    }
    
    // Write LLVM IR to file
    if (!baa_write_llvm_ir_to_file(&context, L"test_output.ll")) {
        const wchar_t* error = context.error_message;
        wprintf(L"Failed to write LLVM IR to file: %ls\n", error ? error : L"Unknown error");
        baa_cleanup_llvm_context(&context);
        cleanup_ast(&program);
        return 1;
    }
    
    printf("Successfully generated LLVM IR. Written to test_output.ll\n");
    
    // Clean up
    baa_cleanup_llvm_context(&context);
    cleanup_ast(&program);
    
    return 0;
}
