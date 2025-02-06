#ifndef BAA_PARSER_H
#define BAA_PARSER_H

#include "baa/ast.h"
#include <stdbool.h>

// Parser state
typedef struct {
    const char* source;      // Source code
    size_t position;         // Current position in source
    size_t line;            // Current line number
    size_t column;          // Current column number
    bool had_error;         // Error flag
    const char* error_msg;  // Error message
} Parser;

// Initialize parser with source code
Parser* baa_parser_init(const char* source);

// Free parser resources
void baa_parser_free(Parser* parser);

// Parse the entire program
Node* baa_parse_program(Parser* parser);

// Parse a single function
Node* baa_parse_function(Parser* parser);

// Parse a statement
Node* baa_parse_statement(Parser* parser);

// Parse an expression
Node* baa_parse_expression(Parser* parser);

// Parse a declaration
Node* baa_parse_declaration(Parser* parser);

// Parse a type
Node* baa_parse_type(Parser* parser);

// Error handling
bool baa_parser_had_error(const Parser* parser);
const char* baa_parser_error_message(const Parser* parser);

#endif /* BAA_PARSER_H */
