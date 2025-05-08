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
extern BaaExpr *baa_parse_expression(BaaParser *parser);
extern BaaType *baa_parse_type_annotation(BaaParser *parser);
extern void baa_set_parser_error(BaaParser *parser, const wchar_t *message);
extern void baa_unexpected_token_error(BaaParser *parser, const wchar_t *expected);

// Forward declarations for functions defined in this file
// Renamed and modified signatures
BaaStmt *baa_parse_variable_rest(BaaParser *parser, BaaType *type, const wchar_t *name, size_t name_len, bool is_const);
BaaParameter *baa_parse_parameter(BaaParser *parser);
BaaFunction *baa_parse_function_rest(BaaParser *parser, BaaType *return_type, const wchar_t *name, size_t name_len);
BaaStmt *baa_parse_import_directive(BaaParser *parser);
// Removed baa_parse_declaration - logic moved to baa_parse_program

// Function implementations for parameter handling (Definitions moved to src/ast/function.c)

// Function implementations for statement creation
BaaStmt *baa_create_variable_declaration(const wchar_t *name, size_t name_length, BaaType *type, BaaExpr *initializer)
{
    BaaVarDeclStmt *var_decl = baa_malloc(sizeof(BaaVarDeclStmt));
    if (!var_decl)
        return NULL;

    var_decl->name = baa_strndup(name, name_length);
    if (!var_decl->name)
    {
        baa_free(var_decl);
        return NULL;
    }

    var_decl->name_length = name_length;
    var_decl->type = type;
    var_decl->initializer = initializer;

    BaaStmt *stmt = baa_malloc(sizeof(BaaStmt));
    if (!stmt)
    {
        baa_free((void *)var_decl->name);
        baa_free(var_decl);
        return NULL;
    }

    stmt->kind = BAA_STMT_VAR_DECL;
    stmt->data = var_decl;
    return stmt;
}

static BaaFunction *baa_create_function(const wchar_t *name, size_t name_length,
                                        BaaParameter **params, size_t param_count,
                                        BaaType *return_type, BaaBlock *body)
{
    BaaFunction *func = baa_malloc(sizeof(BaaFunction));
    if (!func)
        return NULL;

    func->name = baa_strndup(name, name_length);
    if (!func->name)
    {
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
 * Parse the rest of a variable declaration after type and identifier.
 * Handles optional initializer and expects dot terminator.
 */
BaaStmt *baa_parse_variable_rest(BaaParser *parser, BaaType *type, const wchar_t *name, size_t name_len, bool is_const)
{
    // Type and identifier are already parsed and passed in.
    // 'is_const' flag is also passed in.

    // Initialize expression
    BaaExpr *initializer = NULL;

    // Check for initializer
    if (parser->current_token.type == BAA_TOKEN_ASSIGN) {
        // Consume the '=' token
        baa_token_next(parser);

        // Parse the initializer expression
        initializer = baa_parse_expression(parser);
        if (!initializer) {
            // Type is owned by caller (baa_parse_program), don't free here on error
            // baa_free_type(type);
            return NULL; // Error should be set by baa_parse_expression
        }
    } else if (is_const) {
        // Constants must be initialized
        baa_set_parser_error(parser, L"يجب تهيئة الثوابت");
        // baa_free_type(type); // Caller owns type
        return NULL;
    }

    // Expect dot at the end of the declaration
    if (parser->current_token.type != BAA_TOKEN_DOT)
    {                                             // Changed from SEMICOLON to DOT
        baa_unexpected_token_error(parser, L"."); // Changed expected token in error message
        baa_free_type(type);
        if (initializer)
        {
            baa_free_expr(initializer);
        }
        return NULL;
    }

    // Consume the dot
    baa_token_next(parser);

    // Create the variable declaration statement
    // TODO: Need a way to represent const-ness in the AST statement/node
    BaaStmt *statement = baa_create_variable_declaration(name, name_len, type, initializer);
    if (!statement) {
        baa_set_parser_error(parser, L"فشل في إنشاء تصريح المتغير");
        // baa_free_type(type); // Caller owns type
        if (initializer) {
            baa_free_expr(initializer);
        }
        return NULL;
    }
    // TODO: Set a flag like ((BaaVarDeclStmt*)statement->data)->is_const = is_const;
    // This requires adding is_const to BaaVarDeclStmt in statements.h/c

    return statement;
}


/**
 * Parse a parameter for a function declaration
 */
BaaParameter *baa_parse_parameter(BaaParser *parser)
{
    // Expect a type first
    BaaType *type = baa_parse_type(parser); // Use the function that parses type names directly
    if (!type)
    {
        // baa_parse_type should set the error
        return NULL;
    }

    // Expect an identifier for the parameter name
    if (parser->current_token.type != BAA_TOKEN_IDENTIFIER)
    {
        baa_unexpected_token_error(parser, L"معرف الوسيط");
        baa_free_type(type); // Clean up parsed type
        return NULL;
    }

    // Get the parameter name
    const wchar_t *name = parser->current_token.lexeme;
    size_t name_len = parser->current_token.length;

    // Consume the identifier token
    baa_token_next(parser);

    // Create the parameter (assuming mutable=false by default for params)
    BaaParameter *parameter = baa_create_parameter(name, name_len, type, false);
    if (!parameter)
    {
        baa_set_parser_error(parser, L"فشل في إنشاء وسيط");
        baa_free_type(type);
        return NULL;
    }

    return parameter;
}

/**
 * Parse the rest of a function declaration after return type and identifier.
 * Parses parameters and body.
 */
BaaFunction *baa_parse_function_rest(BaaParser *parser, BaaType *return_type, const wchar_t *name, size_t name_len)
{
    // Return type and identifier are already parsed and passed in.

    // Expect open parenthesis
    if (parser->current_token.type != BAA_TOKEN_LEFT_PAREN)
    {
        baa_unexpected_token_error(parser, L"(");
        return NULL;
    }

    // Consume the open parenthesis
    baa_token_next(parser);

    // Parse parameters
    BaaParameter **parameters = NULL;
    size_t parameter_count = 0;
    size_t parameter_capacity = 0;

    // Parse parameters until we reach the closing parenthesis
    if (parser->current_token.type != BAA_TOKEN_RIGHT_PAREN)
    {
        do
        {
            // Parse a parameter
            BaaParameter *parameter = baa_parse_parameter(parser);
            if (!parameter)
            {
                // Free already parsed parameters
                for (size_t i = 0; i < parameter_count; i++)
                {
                    baa_free_parameter(parameters[i]);
                }
                free(parameters);
                return NULL;
            }

            // Add parameter to the list
            if (parameter_count >= parameter_capacity)
            {
                parameter_capacity = parameter_capacity == 0 ? 4 : parameter_capacity * 2;
                BaaParameter **new_parameters = (BaaParameter **)realloc(parameters, parameter_capacity * sizeof(BaaParameter *));
                if (!new_parameters)
                {
                    baa_set_parser_error(parser, L"فشل في تخصيص الذاكرة للوسائط");
                    baa_free_parameter(parameter);
                    for (size_t i = 0; i < parameter_count; i++)
                    {
                        baa_free_parameter(parameters[i]);
                    }
                    free(parameters);
                    return NULL;
                }
                parameters = new_parameters;
            }

            parameters[parameter_count++] = parameter;

            // Check for comma
            if (parser->current_token.type == BAA_TOKEN_COMMA)
            {
                baa_token_next(parser);
            }
            else
            {
                break;
            }
        } while (parser->current_token.type != BAA_TOKEN_RIGHT_PAREN);
    }

    // Expect closing parenthesis
    if (parser->current_token.type != BAA_TOKEN_RIGHT_PAREN)
    {
        baa_unexpected_token_error(parser, L")");
        for (size_t i = 0; i < parameter_count; i++)
        {
            baa_free_parameter(parameters[i]);
        }
        free(parameters);
        return NULL;
    }

    // Consume the closing parenthesis
    baa_token_next(parser);

    // Return type is passed in, use it directly.
    // If the caller didn't parse a type, it should pass a default void type.
    if (!return_type) {
         // This shouldn't happen if caller handles default correctly, but check defensively.
         baa_set_parser_error(parser, L"نوع الإرجاع مفقود داخليًا");
         for (size_t i = 0; i < parameter_count; i++) {
             baa_free_parameter(parameters[i]);
         }
         free(parameters);
         return NULL;
    }

    // Parse function body - expect a block
    BaaStmt *body_stmt = baa_parse_block(parser);
    if (!body_stmt || body_stmt->kind != BAA_STMT_BLOCK)
    {
        baa_set_parser_error(parser, L"توقع كتلة نصية لجسم الدالة");
        baa_free_type(return_type);
        for (size_t i = 0; i < parameter_count; i++)
        {
            baa_free_parameter(parameters[i]);
        }
        free(parameters);
        if (body_stmt)
            baa_free_stmt(body_stmt); // Free if it was parsed but wrong type
        return NULL;
    }
    BaaBlock *body_block = (BaaBlock *)body_stmt->data;
    body_stmt->data = NULL;   // Avoid double free, ownership transferred to BaaFunction
    baa_free_stmt(body_stmt); // Free the wrapper BaaStmt

    // Create the function structure
    BaaFunction *function = baa_create_function(name, name_len, parameters, parameter_count, return_type, body_block);
    if (!function)
    {
        baa_set_parser_error(parser, L"فشل في إنشاء تصريح الدالة");
        baa_free_type(return_type);
        baa_free_block(body_block); // Use baa_free_block
        for (size_t i = 0; i < parameter_count; i++)
        {
            baa_free_parameter(parameters[i]);
        }
        free(parameters); // Parameters array itself was allocated with realloc/malloc
        return NULL;
    }

    return function;
}

/**
 * Parse a declaration (variable, function, or import) - **REMOVED**
 * Logic moved to baa_parse_program in parser.c
 */
// BaaStmt *baa_parse_declaration(BaaParser *parser) { ... } // Removed

// --- Implementation moved from parser.c ---

/**
 * Parse an import directive (implementation moved from parser.c)
 */
BaaStmt *baa_parse_import_directive(BaaParser *parser) // Made non-static
{
    // Assumes the '#تضمين' token/identifier has already been consumed by the caller
    BaaToken start_token = parser->previous_token; // Location is the '#تضمين' token

    // Lexer should handle whitespace between '#تضمين' and path

    wchar_t *clean_path = NULL;
    wchar_t *alias = NULL; // Alias support not fully implemented in original code
    BaaStmt *import_stmt = NULL;
    BaaNode *node = NULL; // Node creation might be better handled by caller (e.g., baa_parse)

    // Check for system import '<...>' or local import '"..."'
    if (parser->current_token.type == BAA_TOKEN_LESS)
    {
        baa_parser_advance_token(parser); // Consume '<'

        // TODO: Parse the path until '>'
        // This requires careful handling of the token stream or direct lexer interaction
        // For now, assume the path is a single token (e.g., IDENTIFIER or STRING_LIT)
        if (parser->current_token.type != BAA_TOKEN_IDENTIFIER && parser->current_token.type != BAA_TOKEN_STRING_LIT)
        {
            baa_unexpected_token_error(parser, L"مسار النظام");
            return NULL;
        }
        clean_path = baa_strdup(parser->current_token.lexeme);
        baa_parser_advance_token(parser); // Consume path token

        if (parser->current_token.type != BAA_TOKEN_GREATER)
        {
            baa_unexpected_token_error(parser, L">");
            baa_free(clean_path);
            return NULL;
        }
        baa_parser_advance_token(parser); // Consume '>'

        // Mark as system import? (Maybe add a flag to BaaImportStmt)
    }
    else if (parser->current_token.type == BAA_TOKEN_STRING_LIT)
    {
        // Local import "..."
        clean_path = baa_strdup(parser->current_token.lexeme); // Path includes quotes? Needs cleaning.
        // TODO: Remove surrounding quotes from clean_path if present
        baa_parser_advance_token(parser); // Consume string literal token
    }
    else
    {
        baa_unexpected_token_error(parser, L"< أو سلسلة نصية");
        return NULL;
    }

    if (!clean_path)
    {
        baa_set_parser_error(parser, L"فشل في استخراج مسار التضمين");
        return NULL;
    }

    // TODO: Parse optional 'as alias' part

    // Create the import statement
    // Assuming baa_create_import_stmt exists and handles allocation
    import_stmt = baa_create_import_stmt(clean_path, alias);
    baa_free(clean_path); // baa_create_import_stmt should copy the path
    // baa_free(alias); // If alias is implemented and copied

    if (!import_stmt)
    {
        baa_set_parser_error(parser, L"فشل في إنشاء عبارة التضمين");
        return NULL;
    }

    // Link to AST node (optional here, maybe done by caller)
    // node = baa_create_node(BAA_NODE_STMT, import_stmt);
    // if (!node) {
    //     baa_set_parser_error(parser, L"فشل في إنشاء عقدة التضمين");
    //     baa_free_stmt(import_stmt);
    //     return NULL;
    // }
    // import_stmt->ast_node = node;
    // node->location = start_token.location; // Set location if node created here

    // Expect semicolon
    // skip_whitespace(parser); // Lexer handles whitespace
    if (parser->current_token.type != BAA_TOKEN_DOT) // Assuming DOT is semicolon
    {
        baa_unexpected_token_error(parser, L".");
        baa_free_stmt(import_stmt); // Free the created statement
        // baa_free_node(node); // Free node if created
        return NULL;
    }
    baa_parser_advance_token(parser); // Consume the dot

    return import_stmt; // Return the statement itself
}
