#ifndef BAA_AST_PRINTER_H
#define BAA_AST_PRINTER_H

#include "ast.h" // Include base AST definitions
#include <stdio.h> // For FILE*

// Function to print the entire AST program structure
void baa_print_ast(FILE* stream, BaaProgram* program);

// You might add more specific print functions later if needed
// void baa_print_statement(FILE* stream, BaaStatement* stmt, int indent_level);
// void baa_print_expression(FILE* stream, BaaExpression* expr, int indent_level);

#endif // BAA_AST_PRINTER_H
