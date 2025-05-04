#include "baa/parser/parser.h"
#include "baa/parser/parser_helper.h"
#include "baa/types/types.h"
#include "baa/parser/tokens.h"
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

// Forward declarations
void baa_set_parser_error(BaaParser *parser, const wchar_t *message);
void baa_unexpected_token_error(BaaParser *parser, const wchar_t *expected);

// Function declarations for type creation
BaaType* baa_create_user_type(const wchar_t* name, size_t name_len);
// Removed forward declaration for baa_create_array_type

/**
 * Create a user-defined type
 */
BaaType* baa_create_user_type(const wchar_t* name, size_t name_len) {
    // Copy the name
    wchar_t* name_copy = (wchar_t*)malloc((name_len + 1) * sizeof(wchar_t));
    if (!name_copy) return NULL;

    wcsncpy(name_copy, name, name_len);
    name_copy[name_len] = L'\0';

    // Create a type with user-defined kind
    // For now, we'll use BAA_TYPE_ERROR as a placeholder for user-defined types
    BaaType* type = baa_create_type(BAA_TYPE_ERROR, name_copy, sizeof(void*), false);

    // Free the name copy since baa_create_type makes its own copy
    free(name_copy);

    return type;
}

// Removed duplicate definition of baa_create_array_type

/**
 * Parse a type specification
 */
BaaType* baa_parse_type(BaaParser* parser)
{
    BaaTypeKind kind;

    // Determine the type kind based on the current token
    switch (parser->current_token.type) {
        case TOKEN_INT:
            kind = BAA_TYPE_INT;
            break;
        case TOKEN_FLOAT:
            kind = BAA_TYPE_FLOAT;
            break;
        case TOKEN_CHAR:
            kind = BAA_TYPE_CHAR;
            break;
        // Instead of BAA_TOKEN_TYPE_BOOL, we need a proper token
        // For now using TOKEN_IDENTIFIER as a workaround
        case TOKEN_IDENTIFIER:
            // Check if it's a boolean type name
            if (parser->current_token.length == 5 &&
                wcsncmp(parser->current_token.lexeme, L"منطقي", 5) == 0) {
                kind = BAA_TYPE_BOOL;
                baa_token_next(parser);
                return baa_create_primitive_type(kind);
            }
            // Check if it's a string type name
            else if (parser->current_token.length == 2 &&
                     wcsncmp(parser->current_token.lexeme, L"نص", 2) == 0) {
                kind = BAA_TYPE_STRING;
                baa_token_next(parser);
                return baa_create_primitive_type(kind);
            } else {
                // Custom/user-defined type
                const wchar_t* name = parser->current_token.lexeme;
                size_t name_len = parser->current_token.length;

                // Consume the identifier token
                baa_token_next(parser);

                // Create a user-defined type
                return baa_create_user_type(name, name_len);
            }
        case TOKEN_VOID:
            kind = BAA_TYPE_VOID;
            break;
        default:
            baa_set_parser_error(parser, L"توقعت نوع");
            return NULL;
    }

    // Consume the type token
    baa_token_next(parser);

    // Check for array type
    bool is_array = false;
    size_t array_size = 0;

    if (parser->current_token.type == (BaaTokenType)TOKEN_LEFT_BRACKET) {
        is_array = true;
        baa_token_next(parser);

        // Check if array size is specified
        if (parser->current_token.type == (BaaTokenType)TOKEN_NUMBER) {
            // Parse the array size
            wchar_t* end_ptr;
            array_size = (size_t)wcstoul(parser->current_token.lexeme, &end_ptr, 10);
            if (*end_ptr != L'\0') {
                baa_set_parser_error(parser, L"حجم المصفوفة غير صالح");
                return NULL;
            }
            baa_token_next(parser);
        }

        // Expect closing bracket
        if (parser->current_token.type != (BaaTokenType)TOKEN_RIGHT_BRACKET) {
            baa_unexpected_token_error(parser, L"]");
            return NULL;
        }
        baa_token_next(parser);
    }

    // Create the type object
    BaaType* type;
    if (is_array) {
        // Create array type
        BaaType* element_type = baa_create_primitive_type(kind);
        if (!element_type) {
            baa_set_parser_error(parser, L"فشل في إنشاء نوع العنصر");
            return NULL;
        }

        type = baa_create_array_type(element_type, array_size);
        if (!type) {
            baa_free_type(element_type);
            baa_set_parser_error(parser, L"فشل في إنشاء نوع المصفوفة");
            return NULL;
        }
    } else {
        // Create primitive type
        type = baa_create_primitive_type(kind);
        if (!type) {
            baa_set_parser_error(parser, L"فشل في إنشاء النوع");
            return NULL;
        }
    }

    return type;
}

/**
 * Parse a type annotation (e.g., in function parameters or variable declarations)
 */
BaaType* baa_parse_type_annotation(BaaParser* parser)
{
    // Check for optional type marker
    if (parser->current_token.type == (BaaTokenType)TOKEN_COLON) {
        baa_token_next(parser);
        return baa_parse_type(parser);
    }

    // No type annotation, return default type (integer)
    return baa_create_primitive_type(BAA_TYPE_INT);
}
