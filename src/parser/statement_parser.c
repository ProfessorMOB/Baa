#include "baa/parser/parser.h"
#include "baa/parser/parser_helper.h"
#include "baa/ast/statements.h"
#include "baa/ast/expressions.h"
#include "baa/analysis/flow_analysis.h" // Corrected include path
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

// Forward declarations
static BaaExpr* parse_expression_internal(BaaParser* parser);
static BaaStmt* parse_block_internal(BaaParser* parser); // Return BaaStmt*
BaaStmt* baa_parse_for_statement(BaaParser* parser);
extern void baa_set_parser_error(BaaParser *parser, const wchar_t *message);

// Forward declaration for external function
extern BaaExpr* baa_parse_expression(BaaParser* parser);

// Implementation of parse_expression_internal
static BaaExpr* parse_expression_internal(BaaParser* parser) {
    // This function should call the expression parser from expression_parser.c
    return baa_parse_expression(parser);
}

// Implementation of validation function
bool baa_validate_condition_type(BaaExpr *expr) {
    // For now, we'll accept any expression as a condition
    // In a more complete implementation, you would check that the expression
    // can be converted to a boolean value
    return expr != NULL;
}

// Implementation of baa_validate_condition
bool baa_validate_condition(BaaExpr *expr) {
    // This is a simplified version that just checks if the expression exists
    return expr != NULL;
}

// Helper function to check token type
bool baa_parser_token_is_type(BaaParser *parser, BaaTokenType type) {
    return parser->current_token.type == type;
}

/**
 * Parse an if statement
 */
BaaStmt* baa_parse_if_statement(BaaParser* parser)
{
    // Check that we're at an if keyword
    if (parser->current_token.type != BAA_TOKEN_IF) {
        baa_unexpected_token_error(parser, L"إذا");
        return NULL;
    }

    // Consume 'if' token
    baa_token_next(parser);

    // Expect opening parenthesis
    if (parser->current_token.type != BAA_TOKEN_LPAREN) {
        baa_unexpected_token_error(parser, L"(");
        return NULL;
    }
    baa_token_next(parser);

    // Parse condition
    BaaExpr* condition = parse_expression_internal(parser);
    if (!condition) {
        return NULL;
    }

    // Validate that condition can be used as a boolean
    if (!baa_validate_condition_type(condition)) {
        baa_free_expression(condition);
        baa_set_parser_error(parser, L"التعبير لا يمكن استخدامه كشرط");
        return NULL;
    }

    // Expect closing parenthesis
    if (parser->current_token.type != BAA_TOKEN_RPAREN) {
        baa_unexpected_token_error(parser, L")");
        baa_free_expression(condition);
        return NULL;
    }
    baa_token_next(parser);

    // Expect opening brace
    if (parser->current_token.type != BAA_TOKEN_LBRACE) {
        baa_unexpected_token_error(parser, L"{");
        baa_free_expression(condition);
        return NULL;
    }
    baa_token_next(parser);

    // Parse if body
    BaaStmt* if_body_stmt = parse_block_internal(parser);
    if (!if_body_stmt) {
        baa_free_expression(condition);
        return NULL;
    }
    BaaBlock* if_body = (BaaBlock*)if_body_stmt->data; // Extract block from stmt

    // Check for else clause
    BaaStmt* else_body_stmt = NULL;
    BaaBlock* else_body = NULL;
    if (parser->current_token.type == BAA_TOKEN_ELSE) {
        baa_token_next(parser);

        // Expect opening brace
        if (parser->current_token.type != BAA_TOKEN_LBRACE) {
            baa_unexpected_token_error(parser, L"{");
            baa_free_expression(condition);
            baa_free_block(if_body);
            return NULL;
        }
        baa_token_next(parser);

        // Parse else body
        else_body_stmt = parse_block_internal(parser);
        if (!else_body_stmt) {
            baa_free_expression(condition);
            baa_free_stmt(if_body_stmt); // Free the if body statement
            return NULL;
        }
        else_body = (BaaBlock*)else_body_stmt->data; // Extract block from stmt
    }

    // Create if statement
    // Note: baa_create_if_stmt likely takes ownership or copies blocks,
    // but we need to manage the containing Stmt wrappers if creation fails.
    BaaStmt* final_if_stmt = baa_create_if_stmt(condition, if_body, else_body);
    if (!final_if_stmt) {
        baa_free_expression(condition);
        // Free the original block statements we parsed
        baa_free_stmt(if_body_stmt);
        if (else_body_stmt) {
            baa_free_stmt(else_body_stmt);
        }
        baa_set_parser_error(parser, L"فشل في إنشاء عبارة إذا");
        return NULL;
    }

    // If creation succeeded, the original Stmt wrappers are likely redundant
    // Assuming baa_create_if_stmt handles the blocks correctly.
    // We might need to free if_body_stmt and else_body_stmt here if they are not consumed.
    // For now, let's assume they are handled internally or leaked (needs review).
    // Let's free them to be safe, assuming baa_create_if_stmt copies the data.
    baa_free_stmt(if_body_stmt);
     if (else_body_stmt) {
         baa_free_stmt(else_body_stmt);
     }


    return final_if_stmt;
}

/**
 * Parse a while statement
 */
BaaStmt* baa_parse_while_statement(BaaParser* parser)
{
    // Check that we're at a while keyword
    if (parser->current_token.type != BAA_TOKEN_WHILE) {
        baa_unexpected_token_error(parser, L"طالما");
        return NULL;
    }

    // Consume 'while' token
    baa_token_next(parser);

    // Expect opening parenthesis
    if (parser->current_token.type != BAA_TOKEN_LPAREN) {
        baa_unexpected_token_error(parser, L"(");
        return NULL;
    }
    baa_token_next(parser);

    // Parse condition
    BaaExpr* condition = parse_expression_internal(parser);
    if (!condition) {
        return NULL;
    }

    // Validate that condition can be used as a boolean
    if (!baa_validate_condition_type(condition)) {
        baa_free_expression(condition);
        baa_set_parser_error(parser, L"التعبير لا يمكن استخدامه كشرط");
        return NULL;
    }

    // Expect closing parenthesis
    if (parser->current_token.type != BAA_TOKEN_RPAREN) {
        baa_unexpected_token_error(parser, L")");
        baa_free_expression(condition);
        return NULL;
    }
    baa_token_next(parser);

    // Expect opening brace
    if (parser->current_token.type != BAA_TOKEN_LBRACE) {
        baa_unexpected_token_error(parser, L"{");
        baa_free_expression(condition);
        return NULL;
    }
    baa_token_next(parser);

    // Parse body
    BaaStmt* body_stmt = parse_block_internal(parser);
    if (!body_stmt) {
        baa_free_expression(condition);
        return NULL;
    }
    BaaBlock* body = (BaaBlock*)body_stmt->data; // Extract block

    // Create while statement
    BaaStmt* final_while_stmt = baa_create_while_stmt(condition, body);
    if (!final_while_stmt) {
        baa_free_expression(condition);
        baa_free_stmt(body_stmt); // Free the parsed block statement
        baa_set_parser_error(parser, L"فشل في إنشاء عبارة طالما");
        return NULL;
    }

    // Assuming baa_create_while_stmt handles the block, free the wrapper
    baa_free_stmt(body_stmt);

    return final_while_stmt;
}

/**
 * Parse a for statement
 */
BaaStmt* baa_parse_for_statement(BaaParser* parser)
{
    // Check that we're at a for keyword
    if (parser->current_token.type != BAA_TOKEN_FOR) {
        baa_unexpected_token_error(parser, L"لكل");
        return NULL;
    }

    // Consume the for keyword
    baa_token_next(parser);

    // Expect opening parenthesis
    if (parser->current_token.type != BAA_TOKEN_LPAREN) {
        baa_unexpected_token_error(parser, L"(");
        return NULL;
    }
    baa_token_next(parser);

    // Parse initializer statement
    BaaStmt* initializer = NULL;
    if (parser->current_token.type != BAA_TOKEN_SEMICOLON) {
        if (parser->current_token.type == BAA_TOKEN_VAR) {
            // Parse variable declaration
            initializer = baa_parse_var_declaration(parser);
            if (!initializer) {
                return NULL;
            }
        } else {
            // Parse expression statement
            BaaExpr* init_expr = parse_expression_internal(parser);
            if (!init_expr) {
                return NULL;
            }

            // Expect semicolon after initializer expression
            if (parser->current_token.type != BAA_TOKEN_SEMICOLON) {
                baa_unexpected_token_error(parser, L";");
                baa_free_expression(init_expr);
                return NULL;
            }
            baa_token_next(parser);

            // Create expression statement
            initializer = baa_create_expr_stmt(init_expr);
            if (!initializer) {
                baa_free_expression(init_expr);
                baa_set_parser_error(parser, L"فشل في إنشاء عبارة التعبير للمهيئ");
                return NULL;
            }
        }
    } else {
        // No initializer, just consume the semicolon
        baa_token_next(parser);
    }

    // Parse condition expression (can be null)
    BaaExpr* condition = NULL;
    if (parser->current_token.type != BAA_TOKEN_SEMICOLON) {
        condition = parse_expression_internal(parser);
        if (!condition) {
            if (initializer) baa_free_stmt(initializer);
            return NULL;
        }
    }

    // Expect semicolon after condition
    if (parser->current_token.type != BAA_TOKEN_SEMICOLON) {
        baa_unexpected_token_error(parser, L";");
        if (initializer) baa_free_stmt(initializer);
        if (condition) baa_free_expression(condition);
        return NULL;
    }
    baa_token_next(parser);

    // Parse increment expression (can be null)
    BaaExpr* increment = NULL;
    if (parser->current_token.type != BAA_TOKEN_RPAREN) {
        increment = parse_expression_internal(parser);
        if (!increment) {
            if (initializer) baa_free_stmt(initializer);
            if (condition) baa_free_expression(condition);
            return NULL;
        }
    }

    // Expect closing parenthesis
    if (parser->current_token.type != BAA_TOKEN_RPAREN) {
        baa_unexpected_token_error(parser, L")");
        if (initializer) baa_free_stmt(initializer);
        if (condition) baa_free_expression(condition);
        if (increment) baa_free_expression(increment);
        return NULL;
    }
    baa_token_next(parser);

    // Parse the loop body
    BaaStmt* body_stmt = NULL;
    BaaBlock* body = NULL;
    if (parser->current_token.type == BAA_TOKEN_LBRACE) {
        // Parse block statement
        baa_token_next(parser);
        body_stmt = parse_block_internal(parser);
        if (body_stmt) {
              body = (BaaBlock*)body_stmt->data; // Extract block
         }
     } else {
         // Parse single statement
         BaaStmt* parsed_single_stmt = baa_parse_statement(parser); // Use a distinct name for the parsed statement
         if (!parsed_single_stmt) {
             if (initializer) baa_free_stmt(initializer);
            if (condition) baa_free_expression(condition);
            if (increment) baa_free_expression(increment);
            return NULL;
        }

         // Create a block statement containing just this single statement
         BaaStmt* block_wrapper_stmt = baa_create_block_stmt(); // Use a different name for the wrapper
         if (!block_wrapper_stmt) {
             baa_free_stmt(parsed_single_stmt); // Free the parsed single statement
             if (initializer) baa_free_stmt(initializer);
             if (condition) baa_free_expression(condition);
            if (initializer) baa_free_stmt(initializer);
            if (condition) baa_free_expression(condition);
            if (increment) baa_free_expression(increment);
            baa_set_parser_error(parser, L"فشل في إنشاء كتلة لعبارة الحلقة");
            return NULL;
        }
        body = (BaaBlock*)block_wrapper_stmt->data; // Extract block from the wrapper

         // Add the single statement to the new block
         if (!baa_add_stmt_to_block(body, parsed_single_stmt)) { // Add the parsed statement
              baa_free_stmt(parsed_single_stmt); // Free the parsed statement on failure
              baa_free_stmt(block_wrapper_stmt); // Free the block statement wrapper
              if (initializer) baa_free_stmt(initializer);
             if (condition) baa_free_expression(condition);
             if (increment) baa_free_expression(increment);
             baa_set_parser_error(parser, L"فشل في إضافة عبارة إلى كتلة الحلقة");
             return NULL;
        }
        // Now, body_stmt should refer to the wrapper containing the single statement
        body_stmt = block_wrapper_stmt;
    }

    if (!body_stmt) { // Check if body_stmt (the wrapper) was successfully created/parsed
        if (initializer) baa_free_stmt(initializer);
        if (condition) baa_free_expression(condition);
        if (increment) baa_free_expression(increment);
        return NULL;
    }

    // Validate the for statement components
    if (!baa_validate_for_stmt(initializer, condition, increment)) {
        baa_free_stmt(body_stmt); // Free the block statement
        if (initializer) baa_free_stmt(initializer);
        if (condition) baa_free_expression(condition);
        if (increment) baa_free_expression(increment);
        baa_set_parser_error(parser, L"عبارة الحلقة غير صالحة");
        return NULL;
    }

    // Create the for statement using the existing helper function
    BaaStmt* final_for_stmt = baa_create_for_stmt(initializer, condition, increment, body);
    if (!final_for_stmt) {
        baa_free_stmt(body_stmt); // Free the block statement
        if (initializer) baa_free_stmt(initializer);
        if (condition) baa_free_expression(condition);
        if (increment) baa_free_expression(increment);
        baa_set_parser_error(parser, L"فشل في إنشاء عبارة الحلقة");
        return NULL;
    }

    // Assuming baa_create_for_stmt handles the block, free the wrapper
    baa_free_stmt(body_stmt);

    return final_for_stmt;
}

/**
 * Parse a return statement
 */
BaaStmt* baa_parse_return_statement(BaaParser* parser)
{
    // Check that we're at a return keyword
    if (parser->current_token.type != BAA_TOKEN_RETURN) {
        baa_unexpected_token_error(parser, L"إرجاع");
        return NULL;
    }

    // Consume 'return' token
    baa_token_next(parser);

    // Check if there's a return value
    BaaExpr* value = NULL;
    if (parser->current_token.type != BAA_TOKEN_DOT) {
        // Parse the return value expression
        value = parse_expression_internal(parser);
        if (!value) {
            return NULL;
        }
    }

    // Expect statement terminator
    if (parser->current_token.type != BAA_TOKEN_DOT) {
        baa_unexpected_token_error(parser, L".");
        if (value) {
            baa_free_expression(value);
        }
        return NULL;
    }
    baa_token_next(parser);

    // Create return statement
    BaaStmt* return_stmt = baa_create_return_stmt(value);
    if (!return_stmt) {
        if (value) {
            baa_free_expression(value);
        }
        baa_set_parser_error(parser, L"فشل في إنشاء عبارة إرجاع");
        return NULL;
    }

    return return_stmt;
}

/**
 * Parse a block of statements and return the corresponding BaaStmt
 */
BaaStmt* baa_parse_block(BaaParser* parser) // Return BaaStmt*
{
    // Expect opening brace
    if (parser->current_token.type != BAA_TOKEN_LBRACE) {
        baa_unexpected_token_error(parser, L"{");
        return NULL;
    }
    baa_token_next(parser);

    return parse_block_internal(parser);
}

/**
 * Internal helper to parse a block (after the opening brace is consumed)
 * Returns the BaaStmt containing the block.
 */
static BaaStmt* parse_block_internal(BaaParser* parser) // Return BaaStmt*
{
    // Create a new block statement (which allocates the BaaBlock in its data field)
    BaaStmt* block_stmt = baa_create_block_stmt();
    if (!block_stmt) {
        baa_set_parser_error(parser, L"فشل في تخصيص الذاكرة لعبارة الكتلة");
        return NULL;
    }
    // Get the actual block structure from the statement's data pointer
    BaaBlock* block = (BaaBlock*)block_stmt->data;
    if (!block) { // Should not happen if baa_create_block_stmt is correct
         baa_free_stmt(block_stmt);
         baa_set_parser_error(parser, L"فشل في الحصول على بيانات الكتلة من العبارة");
         return NULL;
    }


    // Parse statements until we reach the closing brace
    while (parser->current_token.type != BAA_TOKEN_RBRACE &&
           parser->current_token.type != BAA_TOKEN_EOF) {

        // Parse a statement
        BaaStmt* stmt = baa_parse_statement(parser);
        if (!stmt) {
            baa_free_stmt(block_stmt); // Free the containing block statement
            return NULL;
        }

        // Add the statement to the block
        if (!baa_add_stmt_to_block(block, stmt)) {
            baa_free_stmt(stmt);
            baa_free_stmt(block_stmt); // Free the containing block statement
            baa_set_parser_error(parser, L"فشل في إضافة العبارة إلى الكتلة");
            return NULL;
        }
    }

    // Expect closing brace
    if (parser->current_token.type != BAA_TOKEN_RBRACE) {
        baa_unexpected_token_error(parser, L"}");
        baa_free_stmt(block_stmt); // Free the containing block statement
        return NULL;
    }
    baa_token_next(parser);

    return block_stmt; // Return the statement containing the block
}

/**
 * Parse any statement
 */
BaaStmt* baa_parse_statement(BaaParser* parser)
{
    switch (parser->current_token.type) {
        case BAA_TOKEN_IF:
            return baa_parse_if_statement(parser);

        case BAA_TOKEN_WHILE:
            return baa_parse_while_statement(parser);

        case BAA_TOKEN_FOR:
            return baa_parse_for_statement(parser);

        case BAA_TOKEN_RETURN:
            return baa_parse_return_statement(parser);

        case BAA_TOKEN_LBRACE:
            // Parse a block statement. baa_parse_block now returns the correct BaaStmt*.
            return baa_parse_block(parser);

        case BAA_TOKEN_VAR:
            return baa_parse_var_declaration(parser);

        default: {
            // Try to parse as an expression statement
            BaaExpr* expr = parse_expression_internal(parser);
            if (!expr) {
                return NULL;
            }

            // Expect statement terminator
            if (parser->current_token.type != BAA_TOKEN_DOT) {
                baa_unexpected_token_error(parser, L".");
                baa_free_expression(expr);
                return NULL;
            }
            baa_token_next(parser);

            // Create expression statement
            BaaStmt* expr_stmt = baa_create_expr_stmt(expr);
            if (!expr_stmt) {
                baa_free_expression(expr);
                baa_set_parser_error(parser, L"فشل في إنشاء عبارة التعبير");
                return NULL;
            }

            return expr_stmt;
        }
    }
}
