#include "baa/parser/parser.h"
#include "parser_internal.h" // For BaaParser struct definition
#include "baa/utils/utils.h" // For baa_malloc, baa_free
#include "baa/lexer/lexer.h" // For baa_lexer_next_token, baa_free_token
#include <stdio.h>           // For error printing (temporary)
#include <stdlib.h>          // For NULL

// Forward declaration for advance, as it's called by baa_parser_create
static void advance(BaaParser *parser);

BaaParser *baa_parser_create(BaaLexer *lexer, const char *source_filename)
{
    if (!lexer)
    {
        // Consider logging this error more formally later
        fprintf(stderr, "Parser Error: Cannot create parser with NULL lexer.\n");
        return NULL;
    }

    BaaParser *parser = (BaaParser *)baa_malloc(sizeof(BaaParser));
    if (!parser)
    {
        fprintf(stderr, "Parser Error: Failed to allocate memory for parser.\n");
        return NULL;
    }

    parser->lexer = lexer;
    parser->source_filename = source_filename; // Store filename
    parser->had_error = false;
    parser->panic_mode = false;

    // Initialize tokens to a known state (e.g., EOF or an UNKNOWN type)
    // The lexemes should be NULL initially so 'advance' doesn't try to free garbage.
    parser->current_token.type = BAA_TOKEN_UNKNOWN; // Or some initial sentinel
    parser->current_token.lexeme = NULL;
    parser->current_token.length = 0;
    parser->current_token.line = 0;
    parser->current_token.column = 0;

    parser->previous_token.type = BAA_TOKEN_UNKNOWN;
    parser->previous_token.lexeme = NULL;
    parser->previous_token.length = 0;
    parser->previous_token.line = 0;
    parser->previous_token.column = 0;

    // Prime the pump: Fetch the first token to be current_token.
    // previous_token will remain in its initial state after this first advance.
    advance(parser);

    // If the very first token is an error from the lexer, report it.
    // (The advance function itself will loop to get a non-error token or EOF)
    // This initial 'advance' might set parser->had_error if lexer immediately fails hard.

    return parser;
}

/**
 * @brief Consumes the current token and fetches the next one from the lexer.
 *
 * Skips over lexical error tokens, reporting them and continuing to the next
 * valid token or EOF. Manages lexeme ownership for current_token and previous_token.
 */
static void advance(BaaParser *parser)
{
    // 1. Free the lexeme of the *old* previous_token (the one about to be overwritten)
    if (parser->previous_token.lexeme != NULL)
    {
        baa_free((void *)parser->previous_token.lexeme); // Cast to void* for baa_free
        parser->previous_token.lexeme = NULL;
    }

    // 2. Shift current_token to previous_token
    parser->previous_token = parser->current_token; // Struct copy. previous_token now "owns" the lexeme
                                                    // that current_token previously owned.

    // 3. Fetch the next token from the lexer until it's not a BAA_TOKEN_ERROR or we hit EOF.
    for (;;)
    {
        BaaToken *lexer_next_token_ptr = baa_lexer_next_token(parser->lexer);
        if (!lexer_next_token_ptr)
        {
            // This indicates a critical failure in the lexer (e.g., malloc failed for token)
            // Report a parser-level error and set current_token to EOF to stop.
            fprintf(stderr, "Parser Critical Error: Lexer failed to return a token.\n"); // TODO: Use parser_error
            parser->had_error = true;                                                    // Signal a general error
            parser->current_token.type = BAA_TOKEN_EOF;
            parser->current_token.lexeme = NULL; // No lexeme for this synthetic EOF
            parser->current_token.length = 0;
            parser->current_token.line = parser->previous_token.line; // Approximate location
            parser->current_token.column = parser->previous_token.column;
            // No need to free lexer_next_token_ptr as it's NULL
            return;
        }

        // 4. Copy the new token's data to parser->current_token
        parser->current_token = *lexer_next_token_ptr; // Struct copy. current_token now "owns" the lexeme.

        // 5. Sever ownership from the temporary token struct returned by the lexer
        lexer_next_token_ptr->lexeme = NULL;

        // 6. Free the shell of the token returned by the lexer
        baa_free_token(lexer_next_token_ptr);

        if (parser->current_token.type != BAA_TOKEN_ERROR)
        {
            break; // Got a valid token or EOF
        }

        // Lexical error encountered, report it (using a placeholder error function for now)
        // TODO: Replace with proper parser_error_at(&parser->current_token, ...)
        fprintf(stderr, "Lexical Error on line %zu, column %zu: %ls\n",
                parser->current_token.line, parser->current_token.column,
                parser->current_token.lexeme ? parser->current_token.lexeme : L"Unknown lexical error");
        parser->had_error = true;
        // The loop continues to fetch the next token after a lexical error.
        // The erroneous token's lexeme in current_token will be freed on the next advance.
    }
}

// Stub for baa_parse_program - will be implemented later
BaaNode *baa_parse_program(BaaParser *parser)
{
    (void)parser; // Mark as unused for now
    // TODO: Implement parsing logic
    return NULL;
}

// Stub for baa_parser_free - will be implemented properly in the next step
void baa_parser_free(BaaParser *parser)
{
    if (!parser)
    {
        return;
    }
    // Free lexemes owned by current_token and previous_token
    if (parser->current_token.lexeme)
    {
        baa_free((void *)parser->current_token.lexeme);
    }
    if (parser->previous_token.lexeme)
    {
        baa_free((void *)parser->previous_token.lexeme);
    }
    baa_free(parser);
}

// Implementation for baa_parser_had_error
bool baa_parser_had_error(const BaaParser *parser)
{
    if (!parser)
        return true; // Or false, depending on desired behavior for NULL parser
    return parser->had_error;
}
