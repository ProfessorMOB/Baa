#ifndef BAA_EXPRESSION_PARSER_H
#define BAA_EXPRESSION_PARSER_H

#include "baa/ast/expressions.h"
#include <stddef.h>
#include <stdbool.h>
#include <wchar.h>

// Parser state
typedef struct {
    const wchar_t* source;  // Source text
    size_t length;         // Text length
    size_t position;       // Current position
    bool had_error;        // Error flag
    const wchar_t* error_message;  // Error message if had_error is true
} BaaExprParser;

// Parser functions
void baa_init_expr_parser(BaaExprParser* parser, const wchar_t* source, size_t length);
BaaExpr* baa_parse_expression(BaaExprParser* parser);
BaaExpr* baa_parse_literal(BaaExprParser* parser);
BaaExpr* baa_parse_identifier(BaaExprParser* parser);
BaaExpr* baa_parse_unary(BaaExprParser* parser);
BaaExpr* baa_parse_binary(BaaExprParser* parser);
BaaExpr* baa_parse_call(BaaExprParser* parser);
BaaExpr* baa_parse_assignment(BaaExprParser* parser);

// Error handling
const wchar_t* baa_get_parser_error(BaaExprParser* parser);
void baa_clear_parser_error(BaaExprParser* parser);

#endif /* BAA_EXPRESSION_PARSER_H */
