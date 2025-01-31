#ifndef BAA_CODEGEN_H
#define BAA_CODEGEN_H

#include "baa/ast.h"

// Code generation functions
int baa_generate_code(const Node *ast, const char *output_file);
void baa_set_optimization_level(int level);

#endif /* BAA_CODEGEN_H */
