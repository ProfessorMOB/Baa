#include "baa/parser/parser.h"
#include "baa/parser/parser_helper.h"
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/**
 * Sets an error message in the parser
 */
void baa_set_parser_error(BaaParser *parser, const wchar_t *message)
{
    if (!parser->had_error)
    {
        parser->had_error = true;

        // Format error message with location context
        wchar_t full_msg[256];
        swprintf(full_msg, 256, L"خطأ في السطر %zu، العمود %zu: %ls",
                 parser->location.line, parser->location.column, message);

        if (parser->error_message)
        {
            free((void *)parser->error_message);
        }
        parser->error_message = wcsdup(full_msg);
    }
}

/**
 * Get the current error message from the parser
 */
const wchar_t* baa_get_parser_error(BaaParser *parser)
{
    return parser->error_message;
}

/**
 * Clear error state and message in the parser
 */
void baa_clear_parser_error(BaaParser *parser)
{
    parser->had_error = false;
    if (parser->error_message)
    {
        free((void *)parser->error_message);
        parser->error_message = NULL;
    }
}

/**
 * Error recovery: Skip tokens until finding a synchronization point
 */
void baa_parser_synchronize(BaaParser *parser)
{
    // Skip to the next statement terminator or block end
    while (!parser->had_error &&
           parser->current_token.type != BAA_TOKEN_EOF &&
           parser->current_token.type != BAA_TOKEN_DOT &&
           parser->current_token.type != BAA_TOKEN_RBRACE)
    {
        baa_token_next(parser);
    }
}

/**
 * Create a formatted error message about an unexpected token
 */
void baa_unexpected_token_error(BaaParser *parser, const wchar_t *expected)
{
    wchar_t message[256];
    swprintf(message, 256, L"توقعت '%ls'، وجدت '%ls'",
             expected,
             parser->current_token.lexeme);
    baa_set_parser_error(parser, message);
}
