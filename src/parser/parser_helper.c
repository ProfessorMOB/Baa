#include "baa/parser/parser_helper.h"
#include "baa/parser/parser.h" // Include parser.h for BaaParser definition
#include "baa/lexer/lexer.h"   // Include lexer.h for BaaLexer definition
#include "baa/utils/utils.h"   // For baa_strdup
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <ctype.h> // For isdigit, isspace etc. if needed, though custom ones are defined

// --- Moved Utility Functions from parser.c ---

bool is_whitespace(wchar_t c)
{
    // Using iswspace might be more robust for Unicode whitespace
    return c == L' ' || c == L'\t' || c == L'\n' || c == L'\r';
}

bool is_digit(wchar_t c)
{
    // Using iswdigit might be better
    return c >= L'0' && c <= L'9';
}

bool is_arabic_letter(wchar_t c)
{
    // Check if character is in Arabic Unicode range
    // Note: This range might need refinement depending on exact Arabic script coverage needed.
    // 0x0600–0x06FF is the main Arabic block.
    return (c >= 0x0600 && c <= 0x06FF);
}

bool is_identifier_start(wchar_t c)
{
    // Allow letters (including Arabic) or underscore
    // Using iswalpha might be broader if non-Arabic letters are allowed.
    return is_arabic_letter(c) || (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') || c == L'_';
}

bool is_identifier_part(wchar_t c)
{
    // Allow identifier start characters or digits
    return is_identifier_start(c) || is_digit(c);
}

bool is_operator(wchar_t c)
{
    // Consider adding other operators like ., :, ?, etc. if they are parsed as operators
    return c == L'+' || c == L'-' || c == L'*' || c == L'/' || c == L'%' ||
           c == L'=' || c == L'<' || c == L'>' || c == L'!' || c == L'&' || c == L'|';
}

int get_operator_precedence(const wchar_t *op)
{
    // This function is specific to binary operators used in expression parsing.
    // Ensure it covers all binary operators handled by expression_parser.c
    if (!op) return 0;
    if (wcscmp(op, L"=") == 0) return 1; // Assignment
    if (wcscmp(op, L"||") == 0) return 2; // Logical OR
    if (wcscmp(op, L"&&") == 0) return 3; // Logical AND
    // Bitwise operators could go here
    if (wcscmp(op, L"==") == 0 || wcscmp(op, L"!=") == 0) return 4; // Equality
    if (wcscmp(op, L"<") == 0 || wcscmp(op, L">") == 0 ||
        wcscmp(op, L"<=") == 0 || wcscmp(op, L">=") == 0) return 5; // Comparison
    // Bitwise shift operators could go here
    if (wcscmp(op, L"+") == 0 || wcscmp(op, L"-") == 0) return 6; // Addition/Subtraction
    if (wcscmp(op, L"*") == 0 || wcscmp(op, L"/") == 0 || wcscmp(op, L"%") == 0) return 7; // Multiplication/Division/Modulo
    // Unary operators are handled differently (e.g., in parse_unary)
    return 0; // Not a recognized binary operator or lowest precedence
}

bool is_eof(BaaParser *parser)
{
    return parser->current_token.type == BAA_TOKEN_EOF;
}

wchar_t peek(BaaParser *parser)
{
    // Ensure lexeme is not NULL before dereferencing
    if (parser->current_token.lexeme == NULL || parser->current_token.length == 0) {
        // Return null terminator or a specific value indicating end/error
        return L'\0';
    }
    // Return the first character of the current token's lexeme
    return parser->current_token.lexeme[0];
}

wchar_t peek_next(BaaParser *parser)
{
    // This function peeks into the *source code* string in the lexer,
    // not the next *token*. This might be misleading or incorrect depending
    // on its intended use. If the goal is to look ahead one character in the
    // source, it should probably be part of the lexer, not the parser helper.
    // If the goal is to peek at the *next token*, a different mechanism is needed.
    // Assuming original intent was to peek raw source:
    if (!parser || !parser->lexer || !parser->lexer->source) return L'\0';
    size_t current_pos = parser->lexer->current; // Position *after* the current token
    if (current_pos >= wcslen(parser->lexer->source)) return L'\0';
    return parser->lexer->source[current_pos]; // Character immediately after current token
}


void advance(BaaParser *parser)
{
    // Free the lexeme of the current token before advancing,
    // as it will become the previous token.
    // Ownership of the new token's lexeme is transferred from the lexer.
    if (parser->current_token.type != BAA_TOKEN_ERROR && parser->current_token.lexeme != NULL)
    {
        free((void *)parser->current_token.lexeme); // Cast needed due to const
        parser->current_token.lexeme = NULL;        // Avoid double free
    }

    parser->previous_token = parser->current_token;

    // Get the next token from the lexer
    BaaToken *token = baa_lexer_next_token(parser->lexer);
    if (token)
    {
        // Copy token data to the parser's current_token
        parser->current_token.type = token->type;
        // Transfer ownership of the new lexeme pointer
        parser->current_token.lexeme = token->lexeme;
        parser->current_token.length = token->length;
        parser->current_token.line = token->line;
        parser->current_token.column = token->column;

        // Free the container, but NOT the lexeme it points to
        baa_free(token);
    }
    else
    {
        // Handle case where lexer returns NULL (e.g., internal lexer error or end of input)
        // Set current token to EOF
        parser->current_token.type = BAA_TOKEN_EOF;
        parser->current_token.lexeme = NULL; // Ensure lexeme is NULL for EOF
        parser->current_token.length = 0;
        // Keep line/column from the *last valid* token for EOF location?
        // Or update to the end position? Let's keep previous location for now.
        parser->current_token.line = parser->previous_token.line;
        parser->current_token.column = parser->previous_token.column;
    }

    // Update parser's location based on the new current token
    parser->location = baa_get_current_location(parser); // Assumes baa_get_current_location is available
}

bool match_keyword(BaaParser *parser, const wchar_t *keyword)
{
    // Check if the current token is an identifier and matches the keyword
    if (parser->current_token.type == BAA_TOKEN_IDENTIFIER &&
        parser->current_token.lexeme != NULL && // Ensure lexeme exists
        wcscmp(parser->current_token.lexeme, keyword) == 0)
    {
        advance(parser); // Consume the token if it matches
        return true;
    }
    return false;
}

wchar_t *parse_identifier(BaaParser *parser)
{
    // Expects the current token to be an identifier
    if (parser->current_token.type != BAA_TOKEN_IDENTIFIER)
    {
        // Optionally set an error here if an identifier was strictly required
        // baa_unexpected_token_error(parser, L"identifier");
        return NULL;
    }

    // Duplicate the lexeme to transfer ownership to the caller
    wchar_t *identifier = baa_strdup(parser->current_token.lexeme);
    if (!identifier) {
        // Handle memory allocation failure
        baa_set_parser_error(parser, L"Memory allocation failed for identifier");
        return NULL;
    }

    // Consume the identifier token
    advance(parser);

    return identifier;
}

bool expect_char(BaaParser *parser, wchar_t expected_char)
{
    // This function checks the *first character* of the current token's lexeme.
    // It's generally better to check the *token type*. For example, instead of
    // expect_char(parser, L'('), use expect_token(parser, BAA_TOKEN_LPAREN).
    // Let's implement expect_token instead.

    // Placeholder implementation if expect_char is truly needed (e.g., for single-char tokens):
    if (parser->current_token.length == 1 && peek(parser) == expected_char) {
        advance(parser);
        return true;
    }

    // If checking token type is preferred:
    // BaaTokenType expected_type;
    // // Map expected_char to expected_type (e.g., '(' -> BAA_TOKEN_LPAREN)
    // if (parser->current_token.type == expected_type) {
    //     advance(parser);
    //     return true;
    // }

    // If the expectation fails:
    wchar_t expected_str[2] = {expected_char, L'\0'};
    baa_unexpected_token_error(parser, expected_str); // Report the expected character
    return false;
}

// --- Existing Helper Functions (if any) ---

// Example: Implementation for a function declared in the header
bool baa_parser_token_is_type(BaaParser *parser, BaaTokenType type) {
    if (!parser) return false;
    return parser->current_token.type == type;
}

// Add implementations for other functions declared in parser_helper.h
// like baa_validate_condition_type, baa_validate_condition, etc.
// if they are not defined elsewhere.

// Note: synchronize function is declared in parser.h but defined in parser.c.
// It might be better placed here in parser_helper.c if it's a general utility.
// Let's assume it stays in parser.c for now unless moved later.

// Implementation of baa_token_next (already in parser.c, should be removed from there if advance is moved here)
// void baa_token_next(BaaParser *parser)
// {
//     advance(parser);
// }

// Implementation of baa_unexpected_token_error (already in parser.c, should be removed from there)
// void baa_unexpected_token_error(BaaParser *parser, const wchar_t *expected)
// {
//     wchar_t message[256];
//     swprintf(message, 256, L"توقعت '%ls'، وجدت '%ls'",
//              expected, parser->current_token.lexeme ? parser->current_token.lexeme : L"<NULL>");
//     baa_set_parser_error(parser, message); // Assumes baa_set_parser_error is accessible
// }

// Implementation of baa_free_expression (likely belongs in ast/expressions.c or similar)
// void baa_free_expression(BaaExpr* expr) {
//     // Implementation depends heavily on BaaExpr structure
// }

// Implementation of baa_token_next (alias for advance)
void baa_token_next(BaaParser *parser)
{
    advance(parser);
}

// Implementation of baa_unexpected_token_error
void baa_unexpected_token_error(BaaParser *parser, const wchar_t *expected)
{
    wchar_t message[256];
    // Ensure lexeme is not NULL before using it in the error message
    const wchar_t* current_lexeme = parser->current_token.lexeme ? parser->current_token.lexeme : L"<NULL>";
    swprintf(message, 256, L"توقعت '%ls'، وجدت '%ls'",
             expected, current_lexeme);
    // Call baa_set_parser_error (declared in parser.h, implemented in parser.c)
    baa_set_parser_error(parser, message);
}

// Implementation of synchronize (moved from parser.c)
void synchronize(BaaParser *parser) {
    // Basic error recovery: advance tokens until a likely statement boundary
    // or synchronization point is found.
    parser->panic_mode = false; // Exit panic mode once we start synchronizing

    while (parser->current_token.type != BAA_TOKEN_EOF) {
        // Check if the previous token indicates a statement end
        // Use token types directly from lexer.h
        if (parser->previous_token.type == BAA_TOKEN_SEMICOLON || parser->previous_token.type == BAA_TOKEN_DOT) {
             return; // Likely end of a statement
        }

        // Check if the current token indicates the start of a new statement/declaration
        switch (parser->current_token.type) {
            // Declaration starts
            case BAA_TOKEN_FUNC:
            case BAA_TOKEN_VAR:
            case BAA_TOKEN_CONST:
            // Keywords that start statements (assuming these tokens exist in lexer.h)
            // case BAA_TOKEN_IF:    // 'لو'
            // case BAA_TOKEN_WHILE: // 'طالما'
            // case BAA_TOKEN_FOR:   // 'لكل'
            // case BAA_TOKEN_RETURN:// 'ارجع'
            // case BAA_TOKEN_SWITCH:
            // case BAA_TOKEN_BREAK:
            // case BAA_TOKEN_CONTINUE:
            // Check specific keywords if dedicated tokens don't exist
            // Need access to match_keyword or direct lexeme comparison
                return; // Found a likely start of a new declaration/statement

            default:
                // Keep advancing
                break;
        }

        advance(parser); // Consume the current token
    }
}
