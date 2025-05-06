#ifndef BAA_TOKEN_SCANNERS_H
#define BAA_TOKEN_SCANNERS_H

// WARNING: This header declares functions intended for internal use within the lexer module.
// Including it outside the lexer implementation is not recommended.

#include "lexer.h" // For BaaLexer, BaaToken

// Forward declarations of static scanning functions originally in lexer.c
// These are internal to the lexer module but declared here as requested.

// Note: These functions return BaaToken* which must be freed by the caller (usually baa_lexer_next_token)
// or ownership transferred. Error tokens also need freeing.

BaaToken *scan_identifier(BaaLexer *lexer);
BaaToken *scan_number(BaaLexer *lexer);
BaaToken *scan_string(BaaLexer *lexer);
BaaToken *scan_char_literal(BaaLexer *lexer);
BaaToken *scan_multiline_string_literal(BaaLexer *lexer);

#endif // BAA_TOKEN_SCANNERS_H
