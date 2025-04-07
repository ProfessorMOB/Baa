#ifndef BAA_COMPILER_H
#define BAA_COMPILER_H

/**
 * @brief Compiles a Baa source file.
 *
 * Reads the source file, runs the preprocessor, lexer, parser,
 * and code generator to produce LLVM IR.
 *
 * @param filename The path to the main Baa source file (UTF-8 encoded char*).
 * @return 0 on success, non-zero on failure.
 */
int compile_baa_file(const char* filename);

#endif // BAA_COMPILER_H
