 #include "baa/parser/parser.h"
#include "baa/parser/parser_helper.h"
#include "baa/ast/ast.h" // Added for BaaParameter, BaaFunction declarations
#include "baa/ast/expressions.h"
#include "baa/ast/statements.h"
#include "baa/types/types.h"
#include "baa/utils/utils.h"
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

// Forward declarations for functions defined in other files
extern BaaExpr* baa_parse_expression(BaaParser* parser);
extern BaaType* baa_parse_type_annotatiozn(BaaParser* parser);
extern void baa_set_parser_error(BaaParser *parser, const wchar_t *message);
extern void baa_unexpected_token_error(BaaParser *parser, const wchar_t *expected);

// Forward declarations for functions defined in this file
BaaStmt* baa_parse_variable_declaration(BaaParser* parser);
BaaParameter* baa_parse_parameter(BaaParser* parser);
BaaFunction* baa_parse_function_declaration(BaaParser* parser);
BaaStmt* baa_parse_declaration(BaaParser* parser);

// Function implementations for parameter handling (Definitions moved to src/ast/function.c)

// Function implementations for statement creation
BaaStmt* baa_create_variable_declaration(const wchar_t* name, size_t name_length, BaaType* type, BaaExpr* initializer) {
    BaaVarDeclStmt* var_decl = baa_malloc(sizeof(BaaVarDeclStmt));
    if (!var_decl) return NULL;

    var_decl->name = baa_strndup(name, name_length);
    if (!var_decl->name) {
        baa_free(var_decl);
        return NULL;
    }

    var_decl->name_length = name_length;
    var_decl->type = type;
    var_decl->initializer = initializer;

    BaaStmt* stmt = baa_malloc(sizeof(BaaStmt));
    if (!stmt) {
        baa_free((void*)var_decl->name);
        baa_free(var_decl);
        return NULL;
    }

    stmt->kind = BAA_STMT_VAR_DECL;
    stmt->data = var_decl;
    return stmt;
}

static BaaFunction* baa_create_function(const wchar_t* name, size_t name_length,
                                      BaaParameter** params, size_t param_count,
                                      BaaType* return_type, BaaBlock* body) {
    BaaFunction* func = baa_malloc(sizeof(BaaFunction));
    if (!func) return NULL;

    func->name = baa_strndup(name, name_length);
    if (!func->name) {
        baa_free(func);
        return NULL;
    }

    func->name_length = name_length;
    func->parameters = params; // Note: This takes ownership of the params array
    func->parameter_count = param_count;
    func->parameter_capacity = param_count; // Assuming the passed array is exact size
    func->return_type = return_type;
    func->body = body;
    func->is_variadic = false; // Set defaults
    func->is_extern = false;
    func->is_method = false;
    func->module_name = NULL;
    func->ast_node = NULL; // Will be set later if needed
    func->documentation = NULL;

    return func;
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
            baa_free_expr(initializer); // Ensure correct name is used
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
            baa_free_expr(initializer); // Ensure correct name is used
        }
        return NULL;
    }

    return statement;
}

/**
 * Parse a parameter for a function declaration
 */
BaaParameter* baa_parse_parameter(BaaParser* parser)
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
    BaaParameter* parameter = baa_create_parameter(name, name_len, type, false);
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
BaaFunction* baa_parse_function_declaration(BaaParser* parser)
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
    BaaParameter** parameters = NULL;
    size_t parameter_count = 0;
    size_t parameter_capacity = 0;

    // Parse parameters until we reach the closing parenthesis
    if (parser->current_token.type != BAA_TOKEN_RIGHT_PAREN) {
        do {
            // Parse a parameter
            BaaParameter* parameter = baa_parse_parameter(parser);
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
                BaaParameter** new_parameters = (BaaParameter**)realloc(parameters, parameter_capacity * sizeof(BaaParameter*));
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

    // Parse function body - expect a block
    BaaStmt* body_stmt = baa_parse_block(parser);
    if (!body_stmt || body_stmt->kind != BAA_STMT_BLOCK) {
        baa_set_parser_error(parser, L"توقع كتلة نصية لجسم الدالة");
        baa_free_type(return_type);
        for (size_t i = 0; i < parameter_count; i++) {
            baa_free_parameter(parameters[i]);
        }
        free(parameters);
        if (body_stmt) baa_free_stmt(body_stmt); // Free if it was parsed but wrong type
        return NULL;
    }
    BaaBlock* body_block = (BaaBlock*)body_stmt->data;
    body_stmt->data = NULL; // Avoid double free, ownership transferred to BaaFunction
    baa_free_stmt(body_stmt); // Free the wrapper BaaStmt

    // Create the function structure
    BaaFunction* function = baa_create_function(name, name_len, parameters, parameter_count, return_type, body_block);
    if (!function) {
        baa_set_parser_error(parser, L"فشل في إنشاء تصريح الدالة");
        baa_free_type(return_type);
        baa_free_block(body_block); // Use baa_free_block
        for (size_t i = 0; i < parameter_count; i++) {
            baa_free_parameter(parameters[i]);
        }
        free(parameters); // Parameters array itself was allocated with realloc/malloc
        return NULL;
    }

    return function;
}

/**
 * Parse a declaration (variable, function, or import)
 */
BaaStmt* baa_parse_declaration(BaaParser* parser)
{
    // Check for specific declaration keywords/tokens first
    if (parser->current_token.type == BAA_TOKEN_VAR) {
        return baa_parse_variable_declaration(parser);
    } else if (parser->current_token.type == BAA_TOKEN_FUNC) {
        // Function declarations are handled differently (not regular statements)
        BaaFunction* func = baa_parse_function_declaration(parser);
        if (func) {
            // TODO: Handle function storage in Program AST.
            // The main baa_parse loop should handle adding functions.
            // Returning NULL here signals it's not a statement for a block.
            // We should NOT free the function here; the caller (baa_parse) owns it.
             printf("Parsed function (will be handled by caller): %ls\n", func->name); // Debug print
            // baa_free_function(func); // Caller should handle freeing on error or adding to program
            return NULL; // Indicate not a statement
        } else {
            return NULL; // Error during parsing
        }
    } else {
        // Not a recognized declaration start
        baa_unexpected_token_error(parser, L"تصريح (متغير، دالة)");
        return NULL;
    }
}


// --- Implementation moved from parser.c ---

