#ifndef BAA_PARSER_INTERNAL_H
#define BAA_PARSER_INTERNAL_H

#include "baa/lexer/lexer.h" // For BaaLexer, BaaToken
#include "baa/ast/ast.h"     // For BaaNode (forward declaration is fine, but full include helps for ast.h API)
#include <stdbool.h>         // For bool

// Forward declaration for BaaParser for use in helper function signatures if needed early
typedef struct BaaParser BaaParser;

/**
 * @brief Structure to hold the parser's state.
 */
struct BaaParser
{
    BaaLexer *lexer;         // Pointer to the lexer instance providing tokens
    BaaToken current_token;  // The current token being processed
    BaaToken previous_token; // The most recently consumed token

    bool had_error;  // Flag: true if any syntax error has been encountered
    bool panic_mode; // Flag: true if the parser is currently recovering from an error

    const wchar_t *source_filename; // Name of the source file being parsed (for error messages)

    // DiagnosticContext* diagnostics; // Future: For collecting multiple parse errors
};

// Internal helper function prototypes will go here later, e.g.:
// void parser_error_at_current(BaaParser* parser, const wchar_t* message_format, ...);
// void advance(BaaParser* parser);
// ... etc.

#endif // BAA_PARSER_INTERNAL_H
