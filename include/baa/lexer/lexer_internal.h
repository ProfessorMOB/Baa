#ifndef BAA_LEXER_INTERNAL_H
#define BAA_LEXER_INTERNAL_H

#include "baa/lexer/lexer.h"      // For BaaLexer, BaaToken, BaaTokenType
#include "lexer_char_utils.h" // For character classification

// --- Forward declarations of core lexer helper functions (from lexer.c) ---
// These are made available to other .c files within the lexer module.

// Navigation and peeking
bool is_at_end(BaaLexer *lexer);
wchar_t peek(BaaLexer *lexer);
wchar_t peek_next(BaaLexer *lexer);
wchar_t advance(BaaLexer *lexer);
bool match(BaaLexer *lexer, wchar_t expected);

// Token creation
BaaToken *make_token(BaaLexer *lexer, BaaTokenType type);
BaaToken *make_error_token(BaaLexer *lexer, const wchar_t *format, ...);

// Error recovery
void synchronize(BaaLexer *lexer);

// String/Char literal helpers
void append_char_to_buffer(wchar_t **buffer, size_t *len, size_t *capacity, wchar_t c);
int scan_hex_escape(BaaLexer *lexer, int length);

// Keyword lookup
// Structure for keyword mapping (defined in lexer.c)
struct KeywordMapping {
    const wchar_t *keyword;
    BaaTokenType token;
};
extern struct KeywordMapping keywords[];
extern const size_t NUM_KEYWORDS; // Defined in lexer.c

#endif // BAA_LEXER_INTERNAL_H
