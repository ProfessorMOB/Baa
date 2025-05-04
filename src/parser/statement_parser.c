#include "baa/parser/parser.h"       // Needs BaaParser, BaaStmt, etc.
#include "baa/parser/parser_helper.h" // Needs advance, match_keyword, etc.
#include "baa/ast/ast.h"           // Needs BaaNode, baa_create_node, etc.
#include "baa/ast/statements.h"    // Needs BaaBlock, baa_create_block_stmt, etc.
#include "baa/ast/expressions.h"   // Needs BaaExpr
#include "baa/utils/utils.h"       // Needs baa_malloc, etc. (implicitly via other headers?)
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

// Forward declarations for functions called by parse_statement/parse_block
// These are implemented in other parser modules (declaration, control_flow, expression)
extern BaaStmt *baa_parse_declaration(BaaParser *parser);
// Renamed functions to match definitions likely in control_flow_parser.c
extern BaaStmt *baa_parse_if_statement(BaaParser *parser);
extern BaaStmt *baa_parse_while_statement(BaaParser *parser);
extern BaaStmt *baa_parse_for_statement(BaaParser *parser);
extern BaaStmt *baa_parse_return_statement(BaaParser *parser); // Assuming this follows the pattern
extern BaaExpr *baa_parse_expression(BaaParser *parser);
extern void baa_free_expr(BaaExpr *expr); // Should be baa_free_expression? Check declaration
extern void baa_free_stmt(BaaStmt *stmt); // Needed for cleanup

// --- Implementations moved from parser.c ---

/**
 * Parse a block of statements (implementation moved from parser.c)
 * Assumes the opening '{' has just been consumed.
 */
BaaStmt *baa_parse_block(BaaParser *parser) // Made non-static
{
    BaaBlock *block = baa_create_block();
    if (!block)
    {
        // Assuming baa_set_parser_error is available via parser.h include
        baa_set_parser_error(parser, L"فشل في إنشاء كتلة نصية");
        return NULL;
    }

    // Consume '{' - NO! The caller (e.g., parse_statement) should consume '{' before calling.
    // Let's adjust the logic slightly. The caller consumes '{', this parses until '}'.

    while (parser->current_token.type != BAA_TOKEN_RIGHT_BRACE &&
           parser->current_token.type != BAA_TOKEN_EOF)
    {
        // Parse a statement using the dispatcher in this file
        BaaStmt *stmt = baa_parse_statement(parser);
        if (parser->had_error) // Check parser error flag
        {
            baa_free_block(block); // Free the block we created
            return NULL;
        }
        if (stmt)
        {
            if (!baa_add_stmt_to_block(block, stmt))
            {
                baa_set_parser_error(parser, L"فشل في إضافة عبارة إلى الكتلة");
                baa_free_stmt(stmt);
                baa_free_block(block);
                return NULL;
            }
        }
        // If stmt is NULL but no error, it might be a construct handled elsewhere (e.g., function in declaration parser)
        // This shouldn't happen if called within a block context.
    }

    if (parser->current_token.type != BAA_TOKEN_RIGHT_BRACE)
    {
        // Assuming baa_unexpected_token_error is available via parser_helper.h
        baa_unexpected_token_error(parser, L"}");
        baa_free_block(block);
        return NULL;
    }

    // Consume '}'
    baa_token_next(parser); // Use helper function

    // Create the wrapping BaaStmt for the block
    BaaStmt *block_stmt = baa_create_block_stmt();
    if (!block_stmt)
    {
        baa_set_parser_error(parser, L"فشل في إنشاء عبارة الكتلة");
        baa_free_block(block); // Free the block content
        return NULL;
    }
    // Assign the parsed block to the statement's data field
    // baa_create_block_stmt should ideally initialize data to an empty block,
    // or we need to transfer ownership carefully. Let's assume baa_create_block_stmt
    // gives us an empty block, and we replace its contents.
    // A safer approach might be:
    // BaaStmt* block_stmt = baa_create_stmt(BAA_STMT_BLOCK);
    // block_stmt->data = block;
    // Let's stick to the original code's apparent intent for now:
    baa_free_block((BaaBlock*)block_stmt->data); // Free the default empty block
    block_stmt->data = block; // Assign our parsed block

    // Create the AST Node for the statement (optional here, could be done by caller)
    // BaaNode *node = baa_create_node(BAA_NODE_STMT, block_stmt);
    // if (!node)
    // {
    //     baa_set_parser_error(parser, L"فشل في إنشاء عقدة الكتلة");
    //     baa_free_stmt(block_stmt); // This will free the block via baa_free_block
    //     return NULL;
    // }
    // block_stmt->ast_node = node; // Link statement back to node

    return block_stmt;
}

/**
 * Parse any statement (implementation moved from parser.c)
 * Acts as a dispatcher based on the current token.
 */
BaaStmt *baa_parse_statement(BaaParser *parser) // Made non-static
{
    // The lexer is assumed to skip whitespace and comments,
    // so we directly parse based on the current significant token.
    // BaaToken start_token = parser->current_token; // Keep track if needed for location

    // Use helper functions (declared in parser_helper.h)
    if (match_keyword(parser, L"لو")) // Assuming 'لو' corresponds to BAA_TOKEN_IF internally or is handled by match_keyword
    {
        // Delegate to the specific if parser function
        return baa_parse_if_statement(parser);
    }
    else if (match_keyword(parser, L"طالما")) // Assuming 'طالما' is 'while'
    {
        // Delegate to a specific while parser function
        return baa_parse_while_statement(parser);
    }
    else if (match_keyword(parser, L"لكل")) // Assuming 'لكل' is 'for'
    {
        // Delegate to a specific for parser function
        return baa_parse_for_statement(parser);
    }
    else if (match_keyword(parser, L"ارجع")) // Assuming 'ارجع' is 'return'
    {
        // Delegate to a specific return parser function
        return baa_parse_return_statement(parser);
    }
    else if (parser->current_token.type == BAA_TOKEN_LBRACE)
    {
        // Parse a block statement
        baa_token_next(parser); // Consume '{' before calling baa_parse_block
        return baa_parse_block(parser);
    }
    else if (parser->current_token.type == BAA_TOKEN_VAR || parser->current_token.type == BAA_TOKEN_CONST)
    {
        // Delegate to declaration parser (assumed external)
        BaaStmt *decl_stmt = baa_parse_declaration(parser);
        // Expect semicolon after declaration
        if (decl_stmt && parser->current_token.type == BAA_TOKEN_DOT) // Assuming DOT is semicolon
        {
            advance(parser); // Use advance from helper
            return decl_stmt;
        }
        else if (decl_stmt)
        {
            baa_unexpected_token_error(parser, L"."); // Use helper
            baa_free_stmt(decl_stmt);
            return NULL;
        }
        else
        {
            // Error already set by baa_parse_declaration
            return NULL;
        }
    }
    // Add other statement types like break, continue, switch here
    else
    {
        // Default to expression statement
        BaaExpr *expr = baa_parse_expression(parser); // Assumed external
        if (!expr)
        {
            // If expression parsing failed, check if it was because no expression was found
            // vs. a syntax error within an attempted expression.
            if (parser->had_error)
            {
                return NULL; // Error already set
            }
            // else: Not an expression, maybe a keyword we didn't handle?
            baa_unexpected_token_error(parser, L"عبارة أو تعبير"); // Use helper
            return NULL;
        }

        // Expect semicolon after expression statement
        if (parser->current_token.type != BAA_TOKEN_DOT) // Assuming DOT is semicolon
        {
            baa_unexpected_token_error(parser, L"."); // Use helper
            baa_free_expr(expr); // Use correct free function
            return NULL;
        }
        advance(parser); // Use helper

        BaaStmt *expr_stmt = baa_create_expr_stmt(expr);
        if (!expr_stmt)
        {
            baa_free_expr(expr); // Use correct free function
            // Set error? baa_create_expr_stmt should ideally handle this
            return NULL;
        }
        return expr_stmt;
    }
}
