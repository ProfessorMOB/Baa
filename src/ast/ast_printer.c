#include "baa/ast/ast_printer.h"
#include "baa/ast/ast.h"
#include "baa/ast/expressions.h"
#include "baa/ast/statements.h"
#include "baa/ast/literals.h"
#include "baa/types/types.h"
#include "baa/operators/operators.h"
#include <stdio.h>
#include <stdlib.h>

// Helper to print indentation
static void print_indent(FILE* stream, int level) {
    for (int i = 0; i < level; ++i) {
        fprintf(stream, "  ");
    }
}

// Forward declarations for recursive printing
static void print_statement(FILE* stream, BaaStmt* stmt, int indent_level);
static void print_expression(FILE* stream, BaaExpr* expr, int indent_level);
static void print_block(FILE* stream, BaaBlock* block, int indent_level);
static void print_type(FILE* stream, BaaType* type);
static const char* operator_to_string(BaaOperatorType op);

// --- Type Printing ---
static void print_type(FILE* stream, BaaType* type) {
    if (!type) {
        fprintf(stream, "<NULL Type>");
        return;
    }
    // Basic type printing - needs types.h details for full implementation
    // Assuming BaaType has a 'kind' field and potentially a 'name' for user types
    switch (type->kind) {
        case BAA_TYPE_INT: fprintf(stream, "عدد_صحيح"); break;
        case BAA_TYPE_FLOAT: fprintf(stream, "عدد_حقيقي"); break;
        case BAA_TYPE_CHAR: fprintf(stream, "حرف"); break;
        case BAA_TYPE_BOOL: fprintf(stream, "منطقي"); break;
        case BAA_TYPE_VOID: fprintf(stream, "فراغ"); break;
        // Add other basic types (e.g., BAA_TYPE_STRING?)
        // case BAA_TYPE_POINTER: fprintf(stream, "Pointer to "); print_type(stream, type->base); break; // Example
        // case BAA_TYPE_ARRAY: fprintf(stream, "Array of "); print_type(stream, type->element_type); break; // Example
        // case BAA_TYPE_FUNCTION: fprintf(stream, "Function(...)"); break; // Example
        // case BAA_TYPE_STRUCT: fprintf(stream, "Struct %ls", type->name); break; // Example
        default: fprintf(stream, "Type(Kind:%d)", type->kind); break;
    }
}

// --- Operator Printing ---
static const char* operator_to_string(BaaOperatorType op) {
    switch(op) {
        case BAA_OP_ADD: return "+";
        case BAA_OP_SUB: return "-";
        case BAA_OP_MUL: return "*";
        case BAA_OP_DIV: return "/";
        case BAA_OP_MOD: return "%";
        case BAA_OP_EQ: return "==";
        case BAA_OP_NE: return "!="; // Corrected from NEQ
        case BAA_OP_LT: return "<";
        case BAA_OP_LE: return "<="; // Corrected from LTE
        case BAA_OP_GT: return ">";
        case BAA_OP_GE: return ">="; // Corrected from GTE
        case BAA_OP_AND: return "&&"; // Corrected from LOGICAL_AND
        case BAA_OP_OR: return "||";  // Corrected from LOGICAL_OR
        case BAA_OP_NOT: return "!";  // Corrected from LOGICAL_NOT
        case BAA_OP_BIT_AND: return "&";
        case BAA_OP_BIT_OR: return "|";
        case BAA_OP_BIT_XOR: return "^";
        case BAA_OP_BIT_NOT: return "~";
        case BAA_OP_SHL: return "<<";
        case BAA_OP_SHR: return ">>";
        case BAA_OP_ASSIGN: return "=";
        case BAA_OP_ADD_ASSIGN: return "+=";
        case BAA_OP_SUB_ASSIGN: return "-=";
        case BAA_OP_MUL_ASSIGN: return "*=";
        case BAA_OP_DIV_ASSIGN: return "/=";
        case BAA_OP_MOD_ASSIGN: return "%=";
        case BAA_OP_DOT: return ".";
        case BAA_OP_SUBSCRIPT: return "[]";
        case BAA_OP_INC: return "++";
        case BAA_OP_DEC: return "--";
        case BAA_OP_NONE: return "NONE";
        default: return "UnknownOp";
    }
}

// --- Expression Printing ---

static void print_literal_expr(FILE* stream, BaaLiteralExpr* data, int indent_level) {
    print_indent(stream, indent_level);
    fprintf(stream, "LiteralExpr: ");
    if (!data || !data->value || !data->value->data) { // Check node and its data pointer
         fprintf(stream, "<Invalid Literal Data>\n");
         return;
    }
    BaaLiteralData* literal_data = (BaaLiteralData*)data->value->data; // Get data from the node
    switch (literal_data->kind) {
        // Use the correct union members from literals.h
        case BAA_LITERAL_INT:    fprintf(stream, "INT(%d)", literal_data->int_value); break; // Corrected member name
        case BAA_LITERAL_FLOAT:  fprintf(stream, "FLOAT(%f)", literal_data->float_value); break; // Corrected member name
        case BAA_LITERAL_CHAR:   fprintf(stream, "CHAR('%lc')", literal_data->char_value); break; // Corrected member name
        case BAA_LITERAL_STRING: fprintf(stream, "STRING(\"%ls\")", literal_data->string_value); break; // Corrected member name
        case BAA_LITERAL_BOOL:   fprintf(stream, "BOOL(%s)", literal_data->bool_value ? "true" : "false"); break; // Corrected member name
        case BAA_LITERAL_NULL:   fprintf(stream, "NULL"); break;
        default:                 fprintf(stream, "UNKNOWN_LITERAL"); break;
    }
    fprintf(stream, " Type: ");
    print_type(stream, data->type);
    fprintf(stream, "\n");
}

static void print_variable_expr(FILE* stream, BaaVariableExpr* data, int indent_level) {
    print_indent(stream, indent_level);
    fprintf(stream, "VariableExpr: Name=%ls\n", data->name);
}

static void print_unary_expr(FILE* stream, BaaUnaryExpr* data, int indent_level) {
    print_indent(stream, indent_level);
    fprintf(stream, "UnaryExpr: Op=%s\n", operator_to_string(data->op));
    print_expression(stream, data->operand, indent_level + 1);
}

static void print_binary_expr(FILE* stream, BaaBinaryExpr* data, int indent_level) {
    print_indent(stream, indent_level);
    fprintf(stream, "BinaryExpr: Op=%s\n", operator_to_string(data->op));
    print_expression(stream, data->left, indent_level + 1);
    print_expression(stream, data->right, indent_level + 1);
}

static void print_assign_expr(FILE* stream, BaaAssignExpr* data, int indent_level) {
    print_indent(stream, indent_level);
    fprintf(stream, "AssignExpr:\n");
    print_indent(stream, indent_level + 1); fprintf(stream, "Target:\n");
    print_expression(stream, data->target, indent_level + 2);
    print_indent(stream, indent_level + 1); fprintf(stream, "Value:\n");
    print_expression(stream, data->value, indent_level + 2);
}

static void print_call_expr(FILE* stream, BaaCallExpr* data, int indent_level) {
    print_indent(stream, indent_level);
    fprintf(stream, "CallExpr: IsMethod=%s\n", data->is_method_call ? "true" : "false");
    print_indent(stream, indent_level + 1); fprintf(stream, "Callee:\n");
    print_expression(stream, data->callee, indent_level + 2);
    print_indent(stream, indent_level + 1); fprintf(stream, "Arguments (%zu):\n", data->argument_count);
    for(size_t i = 0; i < data->argument_count; ++i) {
        print_indent(stream, indent_level + 2);
        if (data->named_args && data->named_args[i]) {
             fprintf(stream, "NamedArg(%ls):\n", data->named_args[i]);
             print_expression(stream, data->arguments[i], indent_level + 3);
        } else {
             fprintf(stream, "Arg %zu:\n", i);
             print_expression(stream, data->arguments[i], indent_level + 3);
        }
    }
}

static void print_cast_expr(FILE* stream, BaaCastExpr* data, int indent_level) {
    print_indent(stream, indent_level);
    fprintf(stream, "CastExpr: TargetType: ");
    print_type(stream, data->target_type);
    fprintf(stream, "\n");
    print_expression(stream, data->operand, indent_level + 1);
}

static void print_array_expr(FILE* stream, BaaArrayExpr* data, int indent_level) {
     print_indent(stream, indent_level);
     fprintf(stream, "ArrayExpr: ElementType: ");
     print_type(stream, data->element_type);
     fprintf(stream, " Count: %zu\n", data->element_count);
     for(size_t i = 0; i < data->element_count; ++i) {
        print_expression(stream, data->elements[i], indent_level + 1);
     }
}

static void print_index_expr(FILE* stream, BaaIndexExpr* data, int indent_level) {
    print_indent(stream, indent_level);
    fprintf(stream, "IndexExpr:\n");
    print_indent(stream, indent_level + 1); fprintf(stream, "Array:\n");
    print_expression(stream, data->array, indent_level + 2);
    print_indent(stream, indent_level + 1); fprintf(stream, "Index:\n");
    print_expression(stream, data->index, indent_level + 2);
}

static void print_compound_assign_expr(FILE* stream, BaaCompoundAssignmentData* data, int indent_level) {
    print_indent(stream, indent_level);
    fprintf(stream, "CompoundAssignExpr: Op=%s\n", operator_to_string(data->operator_type));
    print_indent(stream, indent_level + 1); fprintf(stream, "Target:\n");
    print_expression(stream, data->target, indent_level + 2);
    print_indent(stream, indent_level + 1); fprintf(stream, "Value:\n");
    print_expression(stream, data->value, indent_level + 2);
}

static void print_inc_dec_expr(FILE* stream, BaaIncDecData* data, int indent_level) {
    print_indent(stream, indent_level);
    fprintf(stream, "IncDecExpr: Op=%s, Prefix=%s\n",
            operator_to_string(data->operator_type), data->is_prefix ? "true" : "false");
    print_expression(stream, data->operand, indent_level + 1);
}

static void print_grouping_expr(FILE* stream, BaaGroupingData* data, int indent_level) {
    print_indent(stream, indent_level);
    fprintf(stream, "GroupingExpr:\n");
    print_expression(stream, data->expression, indent_level + 1);
}


static void print_expression(FILE* stream, BaaExpr* expr, int indent_level) {
    if (!expr) {
        print_indent(stream, indent_level);
        fprintf(stream, "<NULL Expression>\n");
        return;
    }
    // Optionally print overall expression type
    // print_indent(stream, indent_level); fprintf(stream, "(Expr Type: "); print_type(stream, expr->type); fprintf(stream, ")\n");

    switch (expr->kind) {
        case BAA_EXPR_LITERAL:        print_literal_expr(stream, (BaaLiteralExpr*)expr->data, indent_level); break;
        case BAA_EXPR_VARIABLE:       print_variable_expr(stream, (BaaVariableExpr*)expr->data, indent_level); break;
        case BAA_EXPR_UNARY:          print_unary_expr(stream, (BaaUnaryExpr*)expr->data, indent_level); break;
        case BAA_EXPR_BINARY:         print_binary_expr(stream, (BaaBinaryExpr*)expr->data, indent_level); break;
        case BAA_EXPR_CALL:           print_call_expr(stream, (BaaCallExpr*)expr->data, indent_level); break;
        case BAA_EXPR_CAST:           print_cast_expr(stream, (BaaCastExpr*)expr->data, indent_level); break;
        case BAA_EXPR_ASSIGN:         print_assign_expr(stream, (BaaAssignExpr*)expr->data, indent_level); break;
        case BAA_EXPR_ARRAY:          print_array_expr(stream, (BaaArrayExpr*)expr->data, indent_level); break;
        case BAA_EXPR_INDEX:          print_index_expr(stream, (BaaIndexExpr*)expr->data, indent_level); break;
        case BAA_EXPR_COMPOUND_ASSIGN:print_compound_assign_expr(stream, (BaaCompoundAssignmentData*)expr->data, indent_level); break;
        case BAA_EXPR_INC_DEC:        print_inc_dec_expr(stream, (BaaIncDecData*)expr->data, indent_level); break;
        case BAA_EXPR_GROUPING:       print_grouping_expr(stream, (BaaGroupingData*)expr->data, indent_level); break;
        default:
            print_indent(stream, indent_level);
            fprintf(stream, "Expression (Kind: %d - Unknown/Not Implemented)\n", expr->kind);
            break;
    }
}

// --- Statement Printing ---

static void print_expr_stmt(FILE* stream, BaaExprStmt* data, int indent_level) {
    print_indent(stream, indent_level);
    fprintf(stream, "ExpressionStatement:\n");
    print_expression(stream, data->expr, indent_level + 1);
}

static void print_var_decl_stmt(FILE* stream, BaaVarDeclStmt* data, int indent_level) {
    print_indent(stream, indent_level);
    fprintf(stream, "VarDeclStatement: Name=%ls, Type=", data->name);
    print_type(stream, data->type);
    // is_const is not part of BaaVarDeclStmt in statements.h
    fprintf(stream, "\n");
    if (data->initializer) {
        print_indent(stream, indent_level + 1); fprintf(stream, "Initializer:\n");
        print_expression(stream, data->initializer, indent_level + 2);
    } else {
        print_indent(stream, indent_level + 1);
        fprintf(stream, "<No Initializer>\n");
    }
}

static void print_block(FILE* stream, BaaBlock* block, int indent_level) {
    print_indent(stream, indent_level);
    if (!block) {
        fprintf(stream, "<NULL Block>\n");
        return;
    }
    fprintf(stream, "BlockStatement (%zu statements):\n", block->count);
    for (size_t i = 0; i < block->count; ++i) {
        print_statement(stream, block->statements[i], indent_level + 1);
    }
}

static void print_if_stmt(FILE* stream, BaaIfStmt* data, int indent_level) {
    print_indent(stream, indent_level);
    fprintf(stream, "IfStatement:\n");
    print_indent(stream, indent_level + 1); fprintf(stream, "Condition:\n");
    print_expression(stream, data->condition, indent_level + 2);
    print_indent(stream, indent_level + 1); fprintf(stream, "IfBody:\n");
    print_block(stream, data->if_body, indent_level + 2);
    if (data->else_body) {
        print_indent(stream, indent_level + 1); fprintf(stream, "ElseBody:\n");
        print_block(stream, data->else_body, indent_level + 2);
    }
}

static void print_while_stmt(FILE* stream, BaaWhileStmt* data, int indent_level) {
    print_indent(stream, indent_level);
    fprintf(stream, "WhileStatement:\n");
    print_indent(stream, indent_level + 1); fprintf(stream, "Condition:\n");
    print_expression(stream, data->condition, indent_level + 2);
    print_indent(stream, indent_level + 1); fprintf(stream, "Body:\n");
    print_block(stream, data->body, indent_level + 2);
}

static void print_for_stmt(FILE* stream, BaaForStmt* data, int indent_level) {
    print_indent(stream, indent_level);
    fprintf(stream, "ForStatement:\n");
    print_indent(stream, indent_level + 1); fprintf(stream, "Initializer:\n");
    if (data->initializer) print_statement(stream, data->initializer, indent_level + 2); else { print_indent(stream, indent_level + 2); fprintf(stream, "<None>\n"); }
    print_indent(stream, indent_level + 1); fprintf(stream, "Condition:\n");
     if (data->condition) print_expression(stream, data->condition, indent_level + 2); else { print_indent(stream, indent_level + 2); fprintf(stream, "<None>\n"); }
    print_indent(stream, indent_level + 1); fprintf(stream, "Increment:\n");
     if (data->increment) print_expression(stream, data->increment, indent_level + 2); else { print_indent(stream, indent_level + 2); fprintf(stream, "<None>\n"); }
    print_indent(stream, indent_level + 1); fprintf(stream, "Body:\n");
    print_block(stream, data->body, indent_level + 2);
}

static void print_return_stmt(FILE* stream, BaaReturnStmt* data, int indent_level) {
    print_indent(stream, indent_level);
    fprintf(stream, "ReturnStatement:\n");
    if (data->value) {
        print_expression(stream, data->value, indent_level + 1);
    } else {
        print_indent(stream, indent_level + 1); fprintf(stream, "<No Value>\n");
    }
}

static void print_case_stmt(FILE* stream, BaaCaseStmt* data, int indent_level) {
    print_indent(stream, indent_level);
    fprintf(stream, "CaseStatement: Fallthrough=%s\n", data->has_fallthrough ? "true" : "false");
    print_indent(stream, indent_level + 1); fprintf(stream, "Value:\n");
    print_expression(stream, data->value, indent_level + 2);
    print_indent(stream, indent_level + 1); fprintf(stream, "Body:\n");
    print_block(stream, data->body, indent_level + 2);
}

static void print_default_stmt(FILE* stream, BaaDefaultStmt* data, int indent_level) {
    print_indent(stream, indent_level);
    fprintf(stream, "DefaultStatement:\n");
    print_block(stream, data->body, indent_level + 1);
}

static void print_switch_stmt(FILE* stream, BaaSwitchStmt* data, int indent_level) {
    print_indent(stream, indent_level);
    fprintf(stream, "SwitchStatement:\n");
    print_indent(stream, indent_level + 1); fprintf(stream, "Expression:\n");
    print_expression(stream, data->expression, indent_level + 2);
    print_indent(stream, indent_level + 1); fprintf(stream, "Cases (%zu):\n", data->case_count);
    for (size_t i = 0; i < data->case_count; ++i) {
        // Assuming cases are BaaStmt* with kind BAA_STMT_CASE
        print_statement(stream, (BaaStmt*)data->cases[i], indent_level + 2); // Need to cast? Check definition
    }
    print_indent(stream, indent_level + 1); fprintf(stream, "Default:\n");
    if (data->default_case) {
        // Assuming default_case is BaaStmt* with kind BAA_STMT_DEFAULT
        print_statement(stream, (BaaStmt*)data->default_case, indent_level + 2); // Need to cast? Check definition
    } else {
         print_indent(stream, indent_level + 2); fprintf(stream, "<None>\n");
    }
}

static void print_break_stmt(FILE* stream, BaaBreakStmt* data, int indent_level) {
     print_indent(stream, indent_level);
     fprintf(stream, "BreakStatement: Depth=%d, IsSwitch=%s\n", data->loop_depth, data->is_switch_break ? "true" : "false");
}

static void print_continue_stmt(FILE* stream, BaaContinueStmt* data, int indent_level) {
     print_indent(stream, indent_level);
     fprintf(stream, "ContinueStatement: Depth=%d\n", data->loop_depth);
}

static void print_import_stmt(FILE* stream, BaaImportStmt* data, int indent_level) {
     print_indent(stream, indent_level);
     fprintf(stream, "ImportStatement: Path=\"%ls\", Alias=%ls\n",
             data->path ? data->path : L"<NULL>",
             data->alias ? data->alias : L"<None>");
}


static void print_statement(FILE* stream, BaaStmt* stmt, int indent_level) {
     if (!stmt) {
        print_indent(stream, indent_level);
        fprintf(stream, "<NULL Statement>\n");
        return;
    }
    switch (stmt->kind) {
        case BAA_STMT_EXPR:       print_expr_stmt(stream, (BaaExprStmt*)stmt->data, indent_level); break;
        case BAA_STMT_VAR_DECL:   print_var_decl_stmt(stream, (BaaVarDeclStmt*)stmt->data, indent_level); break;
        case BAA_STMT_BLOCK:      print_block(stream, (BaaBlock*)stmt->data, indent_level); break; // Block data is directly the BaaBlock*
        case BAA_STMT_IF:         print_if_stmt(stream, (BaaIfStmt*)stmt->data, indent_level); break;
        case BAA_STMT_WHILE:      print_while_stmt(stream, (BaaWhileStmt*)stmt->data, indent_level); break;
        case BAA_STMT_FOR:        print_for_stmt(stream, (BaaForStmt*)stmt->data, indent_level); break;
        case BAA_STMT_RETURN:     print_return_stmt(stream, (BaaReturnStmt*)stmt->data, indent_level); break;
        case BAA_STMT_SWITCH:     print_switch_stmt(stream, (BaaSwitchStmt*)stmt->data, indent_level); break;
        case BAA_STMT_CASE:       print_case_stmt(stream, (BaaCaseStmt*)stmt->data, indent_level); break; // Added Case
        case BAA_STMT_DEFAULT:    print_default_stmt(stream, (BaaDefaultStmt*)stmt->data, indent_level); break; // Added Default
        case BAA_STMT_BREAK:      print_break_stmt(stream, (BaaBreakStmt*)stmt->data, indent_level); break;
        case BAA_STMT_CONTINUE:   print_continue_stmt(stream, (BaaContinueStmt*)stmt->data, indent_level); break;
        case BAA_STMT_IMPORT:     print_import_stmt(stream, (BaaImportStmt*)stmt->data, indent_level); break;
        default:
            print_indent(stream, indent_level);
            fprintf(stream, "Statement (Kind: %d - Unknown/Not Implemented)\n", stmt->kind);
            break;
    }
}

// --- Function Printing ---
static void print_function(FILE* stream, BaaFunction* func, int indent_level) {
    if (!func) {
        print_indent(stream, indent_level); fprintf(stream, "<NULL Function>\n"); return;
    }
    print_indent(stream, indent_level);
    fprintf(stream, "Function: Name=%ls, ReturnType=", func->name ? func->name : L"<Anonymous>");
    print_type(stream, func->return_type);
    // Print flags like extern, method, variadic
    fprintf(stream, " (Extern:%s, Method:%s, Variadic:%s)\n",
            func->is_extern ? "T" : "F", func->is_method ? "T" : "F", func->is_variadic ? "T" : "F");


    print_indent(stream, indent_level + 1);
    fprintf(stream, "Parameters (%zu):\n", func->parameter_count);
    for (size_t i = 0; i < func->parameter_count; ++i) {
        BaaParameter* param = func->parameters[i];
        if (!param) {
             print_indent(stream, indent_level + 2); fprintf(stream, "<NULL Param %zu>\n", i); continue;
        }
        print_indent(stream, indent_level + 2);
        fprintf(stream, "Param %zu: Name=%ls, Type=", i, param->name ? param->name : L"<NoName>");
        print_type(stream, param->type);
        fprintf(stream, " (Mutable:%s, Optional:%s, Rest:%s)\n",
                param->is_mutable ? "T" : "F", param->is_optional ? "T" : "F", param->is_rest ? "T" : "F");
        if(param->default_value) {
            print_indent(stream, indent_level + 3); fprintf(stream, "DefaultValue:\n");
            print_expression(stream, param->default_value, indent_level + 4);
        }
    }

    print_indent(stream, indent_level + 1);
    fprintf(stream, "Body:\n");
    print_block(stream, func->body, indent_level + 2); // Body is BaaBlock*
}


// --- Program Printing ---

void baa_print_ast(FILE* stream, BaaProgram* program) {
    if (!program) {
        fprintf(stream, "AST Program is NULL.\n");
        return;
    }
    fprintf(stream, "--- Abstract Syntax Tree ---\n");
    fprintf(stream, "Program (%zu functions):\n", program->function_count);
    for (size_t i = 0; i < program->function_count; ++i) {
        print_function(stream, program->functions[i], 1); // Start with indent level 1
    }
    fprintf(stream, "--- End Abstract Syntax Tree ---\n");
}
