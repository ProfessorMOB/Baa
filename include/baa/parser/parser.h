#ifndef BAA_PARSER_H
#define BAA_PARSER_H

#include "baa/lexer/lexer.h" // For BaaLexer
#include "baa/ast/ast.h"     // For BaaNode (program AST root)
#include <stdbool.h>

// Opaque structure for the parser. Definition is internal.
typedef struct BaaParser BaaParser;

/**
 * @brief Creates and initializes a new parser instance.
 *
 * The parser will take ownership of consuming tokens from the provided lexer.
 * The caller should not use the lexer directly after passing it to the parser.
 *
 * @param lexer A pointer to an initialized BaaLexer.
 * @param source_filename The name of the source file being parsed (for error reporting).
 * @return A pointer to the newly created BaaParser, or NULL on failure.
 */
BaaParser* baa_parser_create(BaaLexer* lexer, const char* source_filename);

/**
 * @brief Frees the resources associated with the parser.
 * This includes freeing any internally held tokens.
 * It does NOT free the lexer passed during creation.
 *
 * @param parser A pointer to the BaaParser to be freed.
 */
void baa_parser_free(BaaParser* parser);

/**
 * @brief Parses the entire token stream from the lexer.
 *
 * This is the main entry point for parsing. It attempts to parse a complete
 * Baa program and constructs an Abstract Syntax Tree (AST).
 *
 * @param parser A pointer to an initialized BaaParser.
 * @return A pointer to the root BaaNode of the AST (typically a BAA_NODE_KIND_PROGRAM node),
 *         or NULL if parsing fails and an AST cannot be constructed.
 *         The caller is responsible for freeing the returned AST using baa_ast_free_node().
 */
BaaNode* baa_parse_program(BaaParser* parser);

/**
 * @brief Checks if the parser encountered any errors during its operation.
 *
 * @param parser A pointer to the BaaParser.
 * @return True if errors were encountered, false otherwise.
 */
bool baa_parser_had_error(const BaaParser* parser);

// Future: Function to retrieve a list of diagnostics/errors
// const BaaDiagnosticList* baa_parser_get_diagnostics(const BaaParser* parser);

#endif // BAA_PARSER_H
