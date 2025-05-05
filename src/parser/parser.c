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

// Function removed: parse_declaration - This seems to be a wrapper, maybe keep it?
// Let's re-evaluate if parse_declaration wrapper is needed.
// It calls baa_parse_declaration and wraps the result in a node.
// This seems specific to how declarations are handled *within* parser.c's main loop
// or other internal functions. If baa_parse_statement now calls baa_parse_declaration directly,
// this wrapper might be redundant *unless* top-level parsing in baa_parse needs it.
// Let's keep parse_declaration for now as it might be used by baa_parse.
static BaaNode *parse_declaration(BaaParser *parser)
{
    BaaStmt *stmt = baa_parse_declaration(parser);
    if (!stmt)
    {
        // baa_parse_declaration handles setting the error or returned NULL for function
        return NULL;
    }

    BaaNode *node = baa_create_node(BAA_NODE_STMT, stmt);
    if (!node)
    {
        baa_set_parser_error(parser, L"فشل في إنشاء عقدة AST للتصريح");
        baa_free_stmt(stmt);
        return NULL;
    }
    // Optionally set location from the start token of the statement if needed
    // node->location = parser->start_token_location; // Example

    return node;
}

// Function removed: parse_block (moved to statement_parser.c)

// Function removed: parse_statement (moved to statement_parser.c)

// Function removed: parse_function (wrapper was redundant, baa_parse calls baa_parse_function_declaration directly)

// Function removed: baa_create_import_stmt (Placeholder - should be in ast/statements.c)

// Function removed: parse_import_directive (moved to declaration_parser.c)

// Main parsing function - parses the entire program from the token stream provided by the parser's lexer.
BaaProgram *baa_parse_program(BaaParser *parser)
{
    if (!parser || !parser->lexer) {
        // Cannot proceed without a valid parser and lexer
        // TODO: How to report this error? Maybe return NULL and expect caller to check?
        return NULL;
    }

    BaaProgram *program = baa_create_program();
    if (!program)
    {
        // Allocation failed
        // TODO: Set parser error?
        return NULL;
    }

    // Create the root AST node for the program
    program->ast_node = baa_create_node(BAA_NODE_PROGRAM, program);
    if (!program->ast_node)
    {
        baa_free_program(program); // Frees the inner program struct too
        // TODO: Set parser error?
        return NULL;
    }

    // Assume parser is already initialized and first token is loaded by the caller (e.g., compiler.c)
    // parser->had_error = false; // Should be initialized by caller
    // parser->panic_mode = false; // Should be initialized by caller

    while (parser->current_token.type != BAA_TOKEN_EOF)
    {
        // Try parsing top-level declarations (functions or global vars)
        // TODO: Add support for top-level variable/import declarations if allowed
        if (parser->current_token.type == BAA_TOKEN_FUNC)
        {
            // Call the function declaration parser directly
            BaaFunction *func = baa_parse_function_declaration(parser);
            if (func)
            {
                // Create the AST node for the function
                BaaNode *func_node = baa_create_node(BAA_NODE_FUNCTION, func);
                if (func_node)
                {
                    func->ast_node = func_node; // Link function back to node
                    baa_add_node_to_program(program, func_node);
                }
                else
                {
                    baa_free_function(func); // Cleanup if node creation fails
                    parser->had_error = true; // Signal error
                    // TODO: Set a specific error message? baa_create_node should ideally do this
                }
            }
            // Error handling or NULL return already managed within baa_parse_function_declaration
        }
        // TODO: Add support for top-level variable declarations if needed
        // else if (parser->current_token.type == BAA_TOKEN_VAR) { ... }
        else
        {
            // If it's not a known top-level declaration, report error or skip
            baa_unexpected_token_error(parser, L"تصريح دالة أو نهاية الملف");
            // Attempt error recovery
            synchronize(parser);
        }

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
