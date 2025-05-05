#ifndef BAA_PARSER_H
#define BAA_PARSER_H

#include "baa/ast/ast.h"
#include "baa/ast/expressions.h"
#include "baa/ast/statements.h"
#include "baa/lexer/lexer.h"
#include <stdbool.h>
#include <wchar.h>

// Parser state
typedef struct {
    BaaLexer* lexer;           // Lexer
    BaaToken current_token;    // Current token
    BaaToken previous_token;   // Previous token
    bool had_error;            // Error flag
    bool panic_mode;           // Added panic_mode field
    const wchar_t* error_message;  // Error message if had_error is true
    BaaSourceLocation location;    // Current source location
} BaaParser;

// Parser functions
void baa_init_parser(BaaParser* parser, BaaLexer* lexer);

// Program parsing
BaaProgram* baa_parse_program(BaaParser* parser);
BaaFunction* baa_parse_function(BaaParser* parser);

// Statement parsing
BaaStmt* baa_parse_statement(BaaParser* parser);
BaaStmt* baa_parse_block(BaaParser* parser); // Changed return type to BaaStmt*
BaaStmt* baa_parse_if_statement(BaaParser* parser);
BaaStmt* baa_parse_while_statement(BaaParser* parser);
BaaStmt* baa_parse_for_statement(BaaParser* parser); // Added missing declaration
BaaStmt* baa_parse_return_statement(BaaParser* parser);
BaaStmt* baa_parse_var_declaration(BaaParser* parser);

// Type parsing
BaaType* baa_parse_type(BaaParser* parser);
BaaType* baa_parse_type_annotation(BaaParser* parser);

// Error handling and recovery
void baa_set_parser_error(BaaParser* parser, const wchar_t* message); // Implemented in parser.c
// Removed synchronize declaration (moved to parser_helper.h)
const wchar_t* baa_get_parser_error(BaaParser* parser); // Should likely be baa_parser_error_message (implemented in parser.c)
void baa_clear_parser_error(BaaParser* parser); // Implemented in parser.c

// Source location tracking
BaaSourceLocation baa_get_current_location(BaaParser* parser);
void baa_update_location(BaaParser* parser);

#endif /* BAA_PARSER_H */
