#ifndef BAA_PARSER_HELPER_H
#define BAA_PARSER_HELPER_H

#include "baa/lexer/lexer.h"
#include "baa/parser/parser.h"
#include "baa/ast/expressions.h"

// Map BAA_TOKEN_ names to the correct lexer tokens
// These are used in expression_parser.c for compatibility
#define BAA_TOKEN_NUMBER        BAA_TOKEN_INT_LIT
#define BAA_TOKEN_STRING        BAA_TOKEN_STRING_LIT
#define BAA_TOKEN_TRUE          BAA_TOKEN_INT_LIT   // Temporarily map to INT_LIT
#define BAA_TOKEN_FALSE         BAA_TOKEN_INT_LIT   // Temporarily map to INT_LIT
#define BAA_TOKEN_NULL          BAA_TOKEN_INT_LIT   // Temporarily map to INT_LIT
#define BAA_TOKEN_LEFT_PAREN    BAA_TOKEN_LPAREN
#define BAA_TOKEN_RIGHT_PAREN   BAA_TOKEN_RPAREN
#define BAA_TOKEN_LEFT_BRACKET  BAA_TOKEN_LBRACKET
#define BAA_TOKEN_RIGHT_BRACKET BAA_TOKEN_RBRACKET
#define BAA_TOKEN_LEFT_BRACE    BAA_TOKEN_LBRACE
#define BAA_TOKEN_RIGHT_BRACE   BAA_TOKEN_RBRACE
#define BAA_TOKEN_DOT           BAA_TOKEN_DOT
#define BAA_TOKEN_COMMA         BAA_TOKEN_COMMA
#define BAA_TOKEN_ASSIGN        BAA_TOKEN_EQUAL
#define BAA_TOKEN_NOT           BAA_TOKEN_BANG
#define BAA_TOKEN_SEMICOLON     BAA_TOKEN_SEMICOLON

// These should map directly (already the same in lexer.h)
// BAA_TOKEN_IDENTIFIER
// BAA_TOKEN_PLUS
// BAA_TOKEN_MINUS
// BAA_TOKEN_STAR
// BAA_TOKEN_SLASH
// BAA_TOKEN_PERCENT
// BAA_TOKEN_EQUAL_EQUAL
// BAA_TOKEN_BANG_EQUAL
// BAA_TOKEN_LESS
// BAA_TOKEN_LESS_EQUAL
// BAA_TOKEN_GREATER
// BAA_TOKEN_GREATER_EQUAL
// BAA_TOKEN_AND
// BAA_TOKEN_OR
// BAA_TOKEN_VAR
// BAA_TOKEN_FUNC

#ifdef __cplusplus
extern "C" {
#endif

// --- Core Parser Operations (Moved from parser.c) ---
void advance(BaaParser *parser);
bool is_eof(BaaParser *parser);
wchar_t peek(BaaParser *parser);
wchar_t peek_next(BaaParser *parser); // Note: Peeks source char, not next token
bool match_keyword(BaaParser *parser, const wchar_t *keyword);
wchar_t *parse_identifier(BaaParser *parser);
bool expect_char(BaaParser *parser, wchar_t expected_char); // Note: Checks first char of token, consider expect_token_type
// bool expect_token_type(BaaParser *parser, BaaTokenType type); // Example of a better approach

// --- Token Management & Helpers ---
void baa_token_next(BaaParser *parser); // Alias for advance
bool baa_parser_token_is_type(BaaParser *parser, BaaTokenType type);

// --- Expression Validation ---
// bool baa_validate_condition_type(BaaExpr *expr); // Declaration seems missing implementation
// bool baa_validate_condition(BaaExpr *expr);      // Declaration seems missing implementation

// --- Error Handling ---
void baa_unexpected_token_error(BaaParser *parser, const wchar_t *expected);
void synchronize(BaaParser *parser); // Declaration added
// Note: baa_set_parser_error is declared in parser.h

// --- Memory Management ---
// void baa_free_expression(BaaExpr* expr); // Belongs in AST module (e.g., ast/expressions.h/c)

#ifdef __cplusplus
}
#endif

#endif /* BAA_PARSER_HELPER_H */
