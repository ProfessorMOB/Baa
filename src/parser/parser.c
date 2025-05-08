#include "baa/parser/parser.h"
#include "baa/utils/utils.h"
#include "baa/ast/ast.h"
#include "baa/utils/errors.h"
#include "baa/ast/statements.h"
#include "baa/lexer/lexer.h"
#include "baa/parser/parser_helper.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <wchar.h>

// Forward declarations from other parser modules
extern BaaStmt *baa_parse_declaration(BaaParser *parser);              // From declaration_parser.c
extern BaaFunction *baa_parse_function_declaration(BaaParser *parser); // From declaration_parser.c (or function_parser.c)
extern BaaStmt *baa_parse_if(BaaParser *parser);                       // Added
extern BaaStmt *baa_parse_while(BaaParser *parser);                    // From statement_parser.c / control_flow_parser.c
extern BaaStmt *baa_parse_for(BaaParser *parser);                      // Added
extern BaaStmt *baa_parse_return(BaaParser *parser);                   // Added
extern void synchronize(BaaParser *parser);                            // From parser_helper.c

// Forward declarations for functions in this file
// Removed forward declaration for parse_function
static BaaStmt *parse_block(BaaParser *parser);        // Forward declaration (implementation moved)
static BaaStmt *parse_statement(BaaParser *parser);    // Forward declaration (implementation moved)
// Removed forward declaration for parse_import_directive
void baa_set_parser_error(BaaParser *parser, const wchar_t *message);
static void baa_add_node_to_program(BaaProgram *program, BaaNode *node);
// Removed forward declaration for advance (moved)

// External declarations from expression_parser.c
extern BaaExpr *baa_parse_expression(BaaParser *parser);
extern void baa_free_expression(BaaExpr *expr);

// Forward declarations for functions not yet defined in headers
// Removed baa_add_child (using baa_add_child_node from ast.c)

// Note: Most static helper functions previously defined here (is_whitespace, is_digit,
// advance, peek, match_keyword, parse_identifier, expect_char, etc.) have been moved
// to src/parser/parser_helper.c and declared in include/baa/parser/parser_helper.h

// Parser implementation
void baa_init_parser(BaaParser *parser, BaaLexer *lexer)
{
    parser->lexer = lexer; // Stores a pointer to the lexer instance
    parser->had_error = false;
    parser->error_message = NULL;

    // Initialize tokens
    memset(&parser->current_token, 0, sizeof(BaaToken));
    memset(&parser->previous_token, 0, sizeof(BaaToken));

    // Get the first token to prime the pump
    BaaToken *token = baa_lexer_next_token(lexer);
    if (token)
    {
        // Copy token data to the parser's current_token
        parser->current_token.type = token->type;
        // IMPORTANT: Transfer ownership of lexeme pointer
        parser->current_token.lexeme = token->lexeme;
        parser->current_token.length = token->length;
        parser->current_token.line = token->line;
        parser->current_token.column = token->column;

        // Free the container, but NOT the lexeme it points to
        baa_free(token);
    }
    // Note: Should probably handle the case where the *first* token is NULL (e.g., lexer memory error)

    if (!token)
    {
        baa_set_parser_error(parser, L"Lexer returned NULL token");
        return;
    }
}

BaaSourceLocation baa_get_current_location(BaaParser *parser)
{
    BaaSourceLocation location;
    location.line = parser->current_token.line;
    location.column = parser->current_token.column;
    location.file = NULL; // File name not tracked in this implementation
    return location;
}

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
        parser->error_message = _wcsdup(full_msg);
    }
}

const wchar_t *baa_get_parser_error(BaaParser *parser)
{
    return parser->error_message;
}

void baa_clear_parser_error(BaaParser *parser)
{
    parser->had_error = false;
    if (parser->error_message)
    {
        free((void *)parser->error_message);
        parser->error_message = NULL;
    }
}

// Removed static parse_declaration wrapper - no longer needed with new top-level parsing logic

// Function removed: parse_block (moved to statement_parser.c)

// Function removed: parse_statement (moved to statement_parser.c)

// Function removed: parse_function (wrapper was redundant, baa_parse calls baa_parse_function_declaration directly)

// Function removed: baa_create_import_stmt (Placeholder - should be in ast/statements.c)

// Function removed: parse_import_directive (moved to declaration_parser.c)

// Helper function to check if a token type represents a type keyword
static bool is_type_token(BaaTokenType type) {
    return type == BAA_TOKEN_TYPE_INT || type == BAA_TOKEN_TYPE_FLOAT ||
           type == BAA_TOKEN_TYPE_CHAR || type == BAA_TOKEN_TYPE_BOOL ||
           type == BAA_TOKEN_TYPE_VOID; // Add other type tokens if they exist
           // || type == BAA_TOKEN_IDENTIFIER; // Potentially allow identifiers for user types
}

// Main parsing function - parses the entire program from the token stream provided by the parser's lexer.
BaaProgram *baa_parse_program(BaaParser *parser)
{
    if (!parser || !parser->lexer) {
        return NULL;
    }

    BaaProgram *program = baa_create_program();
    if (!program) {
        return NULL;
    }

    // Create the root AST node for the program
    program->ast_node = baa_create_node(BAA_NODE_PROGRAM, program);
    if (!program->ast_node) {
        baa_free_program(program);
        return NULL;
    }

    // Initialize parser state (should be done by caller, but ensure here)
    parser->had_error = false;
    parser->panic_mode = false;

    while (parser->current_token.type != BAA_TOKEN_EOF)
    {
        BaaType* potential_return_type = NULL;
        bool is_const = false;

        // --- Check for Declaration Start ---
        if (parser->current_token.type == BAA_TOKEN_CONST) {
            is_const = true;
            baa_token_next(parser); // Consume 'ثابت'
            // Now expect a type
            if (!is_type_token(parser->current_token.type)) {
                 baa_unexpected_token_error(parser, L"نوع بعد 'ثابت'");
                 synchronize(parser); // Error recovery
                 continue;
            }
            // Fall through to type parsing
        }

        if (is_type_token(parser->current_token.type)) {
            // Could be variable, constant variable (if is_const is true), or function
            BaaType* decl_type = baa_parse_type(parser); // Parse the type
            if (!decl_type) {
                 // Error during type parsing, synchronize
                 synchronize(parser);
                 continue;
            }

            // Now expect an identifier
            if (parser->current_token.type != BAA_TOKEN_IDENTIFIER) {
                baa_unexpected_token_error(parser, L"معرف بعد النوع");
                baa_free_type(decl_type);
                synchronize(parser);
                continue;
            }
            BaaToken identifier_token = parser->current_token; // Save identifier info
            baa_token_next(parser); // Consume identifier

            // Check what follows the identifier
            if (parser->current_token.type == BAA_TOKEN_LPAREN) {
                // --- Function Declaration ---
                if (is_const) {
                    baa_set_parser_error(parser, L"لا يمكن استخدام 'ثابت' مع الدوال");
                    baa_free_type(decl_type);
                    synchronize(parser);
                    continue;
                }
                // Pass the parsed type and identifier to a function parser helper
                BaaFunction* func = baa_parse_function_rest(parser, decl_type, identifier_token.lexeme, identifier_token.length);
                if (func) {
                    baa_add_function_to_program(program, func); // Add function to program list
                    // Optionally link AST nodes if needed
                    // baa_add_child_node(program->ast_node, func->ast_node);
                } else {
                    // Error occurred in function parsing, synchronize
                    // Note: baa_parse_function_rest should free decl_type on error
                    synchronize(parser);
                }
            } else if (parser->current_token.type == BAA_TOKEN_ASSIGN || parser->current_token.type == BAA_TOKEN_DOT) {
                // --- Variable or Constant Variable Declaration ---
                // Pass type, const flag, and identifier to variable parser helper
                BaaStmt* var_stmt = baa_parse_variable_rest(parser, decl_type, identifier_token.lexeme, identifier_token.length, is_const);
                if (var_stmt) {
                    // TODO: Add var_stmt to program->top_level_statements
                    // For now, just print a warning and free it
                    fwprintf(stderr, L"Warning: Top-level variable/const declaration parsed but not stored.\n");
                    baa_free_stmt(var_stmt);
                } else {
                    // Error occurred in variable parsing, synchronize
                    // Note: baa_parse_variable_rest should free decl_type on error
                    synchronize(parser);
                }
            } else {
                // Unexpected token after type identifier
                baa_unexpected_token_error(parser, L"( أو = أو .");
                baa_free_type(decl_type);
                synchronize(parser);
            }
        }
        else if (parser->current_token.type == BAA_TOKEN_IDENTIFIER && wcscmp(parser->current_token.lexeme, L"#تضمين") == 0) {
             // --- Import Directive ---
             baa_token_next(parser); // Consume '#تضمين'
             BaaStmt* import_stmt = baa_parse_import_directive(parser);
             if (import_stmt) {
                 // TODO: Add import_stmt to program->top_level_statements
                 fwprintf(stderr, L"Warning: Import directive parsed but not stored.\n");
                 baa_free_stmt(import_stmt);
             } else {
                 synchronize(parser);
             }
        }
        // Removed check for BAA_TOKEN_FUNC (old syntax)
        else {
            // Unexpected token at top level
            baa_unexpected_token_error(parser, L"تصريح (نوع، ثابت، #تضمين)");
            synchronize(parser);
        }

        // --- Error Recovery Handling ---
        if (parser->had_error)
        {
            if (!parser->panic_mode)
            {
                // Report error details if needed (already done by error setting functions)
                parser->panic_mode = true; // Enter panic mode after first error
            }
            // Attempt to recover or break
            synchronize(parser);
            if (parser->current_token.type == BAA_TOKEN_EOF)
                break; // Stop if EOF reached after error
        }
        else
        {
            parser->panic_mode = false; // Reset panic mode on successful parse
        }
    }

    // Do NOT free the lexer here, it's owned by the caller (compiler.c)

    if (parser->had_error)
    {
        // Error message should already be set in the parser struct
        baa_free_program(program);
        return NULL;
    }

    return program;
}


bool baa_parser_had_error(const BaaParser *parser)
{
    return parser->had_error;
}

const wchar_t *baa_parser_error_message(const BaaParser *parser)
{
    return parser->error_message;
}

// Function removed: synchronize (moved to parser_helper.c)

BaaParser *baa_create_parser(const wchar_t *source, size_t source_len)
{
    if (!source)
    {
        return NULL;
    }

    // Allocate memory for the parser
    BaaParser *parser = (BaaParser *)baa_malloc(sizeof(BaaParser));
    if (!parser)
    {
        return NULL;
    }

    // Create the lexer
    BaaLexer *lexer = baa_create_lexer(source);
    if (!lexer)
    {
        baa_free(parser);
        return NULL;
    }

    // Initialize the parser with the lexer
    baa_init_parser(parser, lexer);

    return parser;
}

// Function removed: baa_add_child (using baa_add_child_node from ast.c)

// Helper to add a node to a program
static void baa_add_node_to_program(BaaProgram *program, BaaNode *node)
{
    if (!program || !node || !program->ast_node) // Added check for program->ast_node
        return;

    // Set parent link using the function from ast.c
    baa_add_child_node(program->ast_node, node);

    // Add function specifically if it's a function node
    if (node->kind == BAA_NODE_FUNCTION && node->data)
    {
        // baa_add_function_to_program already called by baa_parse,
        // this might be redundant or handled differently depending on AST structure.
        // Let's assume baa_add_function_to_program handles adding to the program's list
        // and baa_add_child_node handles the AST parent link.
        // No need to call baa_add_function_to_program again here.
        // baa_add_function_to_program(program, (BaaFunction *)node->data);
    }
    // TODO: Handle top-level statements if allowed (e.g., imports, global vars)
    // else if (node->kind == BAA_NODE_STMT && node->data) {
    //     BaaStmt* stmt = (BaaStmt*)node->data;
    //     if (stmt->kind == BAA_STMT_IMPORT || stmt->kind == BAA_STMT_VAR_DECL) {
    //         // Add to program's list of top-level statements/declarations
    //     }
    // }
}

// Function removed: baa_free_program_wrapper (unused)
