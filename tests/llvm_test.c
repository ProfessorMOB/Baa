#include <stdio.h>
#include "baa/codegen/llvm_codegen.h"
#include "baa/ast/ast.h"
#include "baa/types/types.h"

int main() {
    // Initialize LLVM context
    BaaLLVMContext context;
    if (!baa_init_llvm_context(&context, L"test_module")) {
        printf("Failed to initialize LLVM context: %ls\n", baa_get_llvm_error(&context));
        return 1;
    }

    // Create a simple program
    BaaProgram program;
    program.function_count = 0;
    program.functions = NULL;

    // Create a function
    BaaFunction function;
    function.name = L"main";
    
    // Set return type to int
    BaaType* return_type = baa_create_primitive_type(BAA_TYPE_INT);
    function.return_type = return_type;
    
    // Create a simple function body (return 42)
    BaaBlock* body = (BaaBlock*)malloc(sizeof(BaaBlock));
    body->count = 1;
    body->statements = (BaaStmt**)malloc(sizeof(BaaStmt*));
    
    // Create a return statement
    BaaStmt* return_stmt = (BaaStmt*)malloc(sizeof(BaaStmt));
    return_stmt->type = BAA_STMT_RETURN;
    
    // Create a literal int expression for the return value
    BaaExpr* int_expr = (BaaExpr*)malloc(sizeof(BaaExpr));
    int_expr->type = BAA_EXPR_INT;
    int_expr->int_literal = 42;
    
    // Set the return value
    return_stmt->return_stmt.expr = int_expr;
    
    // Add the return statement to the function body
    body->statements[0] = return_stmt;
    function.body = body;
    
    // Add the function to the program
    program.function_count = 1;
    program.functions = (BaaFunction**)malloc(sizeof(BaaFunction*));
    program.functions[0] = &function;
    
    // Generate LLVM IR for the program
    if (!baa_generate_llvm_ir(&context, &program)) {
        printf("Failed to generate LLVM IR: %ls\n", baa_get_llvm_error(&context));
        baa_cleanup_llvm_context(&context);
        return 1;
    }
    
    // Write LLVM IR to file
    if (!baa_write_llvm_ir_to_file(&context, L"test.ll")) {
        printf("Failed to write LLVM IR to file: %ls\n", baa_get_llvm_error(&context));
        baa_cleanup_llvm_context(&context);
        return 1;
    }
    
    // Compile LLVM IR to object file
    if (!baa_compile_llvm_ir_to_object(&context, L"test.o")) {
        printf("Failed to compile LLVM IR to object file: %ls\n", baa_get_llvm_error(&context));
        baa_cleanup_llvm_context(&context);
        return 1;
    }
    
    printf("Successfully generated LLVM IR and compiled to object file\n");
    
    // Clean up
    baa_cleanup_llvm_context(&context);
    free(body->statements);
    free(body);
    free(program.functions);
    
    return 0;
}
