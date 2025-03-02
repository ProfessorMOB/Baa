#include "baa/parser/parser.h"
#include "baa/parser/parser_helper.h"
#include "baa/ast/expressions.h"
#include "baa/ast/statements.h"
#include "baa/types/types.h"
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

// Forward declarations for functions defined in other files
extern BaaExpr* baa_parse_expression(BaaParser* parser);
extern BaaType* baa_parse_type_annotation(BaaParser* parser);
extern void baa_set_parser_error(BaaParser *parser, const wchar_t *message);
extern void baa_unexpected_token_error(BaaParser *parser, const wchar_t *expected);

// Define parameter structure locally if it's not defined elsewhere
typedef struct {
    const wchar_t* name;
    size_t name_length;
    BaaType* type;
} BaaFuncParam;

// Forward declarations for functions defined in this file
BaaStmt* baa_parse_variable_declaration(BaaParser* parser);
BaaFuncParam* baa_parse_parameter(BaaParser* parser);
BaaStmt* baa_parse_function_declaration(BaaParser* parser);
BaaStmt* baa_parse_declaration(BaaParser* parser);

// Function implementations for parameter handling
BaaFuncParam* baa_create_parameter(const wchar_t* name, size_t name_len, BaaType* type) {
    BaaFuncParam* param = (BaaFuncParam*)malloc(sizeof(BaaFuncParam));
    if (!param) return NULL;

    // Copy the name
    wchar_t* name_copy = (wchar_t*)malloc((name_len + 1) * sizeof(wchar_t));
    if (!name_copy) {
        free(param);
        return NULL;
    }

    wcsncpy(name_copy, name, name_len);
    name_copy[name_len] = L'\0';

    param->name = name_copy;
    param->name_length = name_len;
    param->type = type;

    return param;
}

void baa_free_parameter(BaaFuncParam* param) {
    if (param) {
        if (param->name) {
            free((void*)param->name);
        }
        if (param->type) {
            baa_free_type(param->type);
        }
        free(param);
    }
}

// Function implementations for statement creation
BaaStmt* baa_create_variable_declaration(const wchar_t* name, size_t name_len, BaaType* type, BaaExpr* initializer) {
    // Create a new variable declaration statement
    BaaStmt* stmt = (BaaStmt*)malloc(sizeof(BaaStmt));
    if (!stmt) return NULL;

    // Copy the name
    wchar_t* name_copy = (wchar_t*)malloc((name_len + 1) * sizeof(wchar_t));
    if (!name_copy) {
        free(stmt);
        return NULL;
    }

    wcsncpy(name_copy, name, name_len);
    name_copy[name_len] = L'\0';

    // Initialize the statement
    stmt->type = BAA_STMT_VAR_DECL;
    stmt->var_decl.name = name_copy;
    stmt->var_decl.name_length = name_len;
    stmt->var_decl.type = type;
    stmt->var_decl.initializer = initializer;

    // Create an AST node for this statement
    BaaSourceLocation loc = {0};
    loc.line = 0;
    loc.column = 0;
    stmt->ast_node = baa_create_node(BAA_NODE_STATEMENT, loc);
    stmt->var_decl.ast_node = stmt->ast_node;

    return stmt;
}

BaaStmt* baa_create_function_declaration(const wchar_t* name, size_t name_len,
                                        BaaFuncParam** parameters, size_t param_count,
                                        BaaType* return_type, BaaBlock* body) {
    // For now, we'll create a simple block statement as a placeholder
    // In a real implementation, you would create a proper function declaration
    BaaStmt* stmt = baa_create_block_stmt();
    if (!stmt) return NULL;

    // Add the body statements to the block
    if (body && body->statements) {
        for (size_t i = 0; i < body->count; i++) {
            baa_add_stmt_to_block(&stmt->block_stmt, body->statements[i]);
        }
    }

    return stmt;
}

/**
 * Parse a variable declaration
 */
BaaStmt* baa_parse_variable_declaration(BaaParser* parser)
{
    // Consume the 'متغير' token
    baa_token_next(parser);

    // Expect an identifier for the variable name
    if (parser->current_token.type != BAA_TOKEN_IDENTIFIER) {
        baa_unexpected_token_error(parser, L"معرف");
        return NULL;
    }

    // Get the variable name
    const wchar_t* name = parser->current_token.lexeme;
    size_t name_len = parser->current_token.length;

    // Consume the identifier token
    baa_token_next(parser);

    // Parse optional type annotation
    BaaType* type = baa_parse_type_annotation(parser);
    if (!type) {
        return NULL;
    }

    // Initialize expression
    BaaExpr* initializer = NULL;

    // Check for initializer
    if (parser->current_token.type == BAA_TOKEN_ASSIGN) {
        // Consume the '=' token
        baa_token_next(parser);

        // Parse the initializer expression
        initializer = baa_parse_expression(parser);
        if (!initializer) {
            baa_free_type(type);
            return NULL;
        }
    }

    // Expect semicolon at the end of the declaration
    if (parser->current_token.type != BAA_TOKEN_SEMICOLON) {
        baa_unexpected_token_error(parser, L";");
        baa_free_type(type);
        if (initializer) {
            baa_free_expression(initializer);
        }
        return NULL;
    }

    // Consume the semicolon
    baa_token_next(parser);

    // Create the variable declaration statement
    BaaStmt* statement = baa_create_variable_declaration(name, name_len, type, initializer);
    if (!statement) {
        baa_set_parser_error(parser, L"فشل في إنشاء تصريح المتغير");
        baa_free_type(type);
        if (initializer) {
            baa_free_expression(initializer);
        }
        return NULL;
    }

    return statement;
}

/**
 * Parse a parameter for a function declaration
 */
BaaFuncParam* baa_parse_parameter(BaaParser* parser)
{
    // Expect an identifier for the parameter name
    if (parser->current_token.type != BAA_TOKEN_IDENTIFIER) {
        baa_unexpected_token_error(parser, L"معرف");
        return NULL;
    }

    // Get the parameter name
    const wchar_t* name = parser->current_token.lexeme;
    size_t name_len = parser->current_token.length;

    // Consume the identifier token
    baa_token_next(parser);

    // Parse type annotation
    BaaType* type = baa_parse_type_annotation(parser);
    if (!type) {
        return NULL;
    }

    // Create the parameter
    BaaFuncParam* parameter = baa_create_parameter(name, name_len, type);
    if (!parameter) {
        baa_set_parser_error(parser, L"فشل في إنشاء وسيط");
        baa_free_type(type);
        return NULL;
    }

    return parameter;
}

/**
 * Parse a function declaration
 */
BaaStmt* baa_parse_function_declaration(BaaParser* parser)
{
    // Consume the 'دالة' token
    baa_token_next(parser);

    // Expect an identifier for the function name
    if (parser->current_token.type != BAA_TOKEN_IDENTIFIER) {
        baa_unexpected_token_error(parser, L"معرف");
        return NULL;
    }

    // Get the function name
    const wchar_t* name = parser->current_token.lexeme;
    size_t name_len = parser->current_token.length;

    // Consume the identifier token
    baa_token_next(parser);

    // Expect open parenthesis
    if (parser->current_token.type != BAA_TOKEN_LEFT_PAREN) {
        baa_unexpected_token_error(parser, L"(");
        return NULL;
    }

    // Consume the open parenthesis
    baa_token_next(parser);

    // Parse parameters
    BaaFuncParam** parameters = NULL;
    size_t parameter_count = 0;
    size_t parameter_capacity = 0;

    // Parse parameters until we reach the closing parenthesis
    if (parser->current_token.type != BAA_TOKEN_RIGHT_PAREN) {
        do {
            // Parse a parameter
            BaaFuncParam* parameter = baa_parse_parameter(parser);
            if (!parameter) {
                // Free already parsed parameters
                for (size_t i = 0; i < parameter_count; i++) {
                    baa_free_parameter(parameters[i]);
                }
                free(parameters);
                return NULL;
            }

            // Add parameter to the list
            if (parameter_count >= parameter_capacity) {
                parameter_capacity = parameter_capacity == 0 ? 4 : parameter_capacity * 2;
                BaaFuncParam** new_parameters = (BaaFuncParam**)realloc(parameters, parameter_capacity * sizeof(BaaFuncParam*));
                if (!new_parameters) {
                    baa_set_parser_error(parser, L"فشل في تخصيص الذاكرة للوسائط");
                    baa_free_parameter(parameter);
                    for (size_t i = 0; i < parameter_count; i++) {
                        baa_free_parameter(parameters[i]);
                    }
                    free(parameters);
                    return NULL;
                }
                parameters = new_parameters;
            }

            parameters[parameter_count++] = parameter;

            // Check for comma
            if (parser->current_token.type == BAA_TOKEN_COMMA) {
                baa_token_next(parser);
            } else {
                break;
            }
        } while (parser->current_token.type != BAA_TOKEN_RIGHT_PAREN);
    }

    // Expect closing parenthesis
    if (parser->current_token.type != BAA_TOKEN_RIGHT_PAREN) {
        baa_unexpected_token_error(parser, L")");
        for (size_t i = 0; i < parameter_count; i++) {
            baa_free_parameter(parameters[i]);
        }
        free(parameters);
        return NULL;
    }

    // Consume the closing parenthesis
    baa_token_next(parser);

    // Parse return type
    BaaType* return_type = baa_parse_type_annotation(parser);
    if (!return_type) {
        for (size_t i = 0; i < parameter_count; i++) {
            baa_free_parameter(parameters[i]);
        }
        free(parameters);
        return NULL;
    }

    // Parse function body
    BaaBlock* body = baa_parse_block(parser);
    if (!body) {
        baa_free_type(return_type);
        for (size_t i = 0; i < parameter_count; i++) {
            baa_free_parameter(parameters[i]);
        }
        free(parameters);
        return NULL;
    }

    // Create the function declaration statement
    BaaStmt* statement = baa_create_function_declaration(name, name_len, parameters, parameter_count, return_type, body);
    if (!statement) {
        baa_set_parser_error(parser, L"فشل في إنشاء تصريح الدالة");
        baa_free_type(return_type);
        baa_free_block(body);
        for (size_t i = 0; i < parameter_count; i++) {
            baa_free_parameter(parameters[i]);
        }
        free(parameters);
        return NULL;
    }

    return statement;
}

/**
 * Parse a declaration (variable or function)
 */
BaaStmt* baa_parse_declaration(BaaParser* parser)
{
    switch (parser->current_token.type) {
        case BAA_TOKEN_VAR:
            return baa_parse_variable_declaration(parser);
        case BAA_TOKEN_FUNC:
            return baa_parse_function_declaration(parser);
        default:
            baa_set_parser_error(parser, L"توقعت تصريح");
            return NULL;
    }
}
