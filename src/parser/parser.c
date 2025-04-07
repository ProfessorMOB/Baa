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
extern BaaStmt* baa_parse_declaration(BaaParser *parser);       // From declaration_parser.c
extern BaaFunction* baa_parse_function_declaration(BaaParser *parser); // From declaration_parser.c (or function_parser.c)
extern BaaStmt* baa_parse_while(BaaParser* parser);            // From statement_parser.c / control_flow_parser.c
extern void synchronize(BaaParser *parser);                     // From parser_helper.c

// Forward declarations for functions in this file
static BaaFunction* parse_function(BaaParser *parser); // Changed return type
static BaaNode *parse_block(BaaParser *parser);
static BaaStmt* parse_statement(BaaParser *parser);   // Changed return type
static BaaNode *parse_import_directive(BaaParser *parser);
void baa_set_parser_error(BaaParser *parser, const wchar_t *message);
static void baa_add_node_to_program(BaaProgram *program, BaaNode *node);
static void advance(BaaParser *parser);

// External declarations from expression_parser.c
extern BaaExpr *baa_parse_expression(BaaParser *parser);
extern void baa_free_expression(BaaExpr *expr);

// Forward declarations for functions not yet defined in headers
static void baa_add_child(BaaNode *parent, BaaNode *child);
BaaBlock *baa_create_block(void);

// Implementation of baa_token_next for expression_parser.c
void baa_token_next(BaaParser *parser)
{
    advance(parser);
}

// Implementation of baa_unexpected_token_error for parser_helper.h
void baa_unexpected_token_error(BaaParser *parser, const wchar_t *expected)
{
    wchar_t message[256];
    swprintf(message, 256, L"توقعت '%ls'، وجدت '%ls'",
             expected, parser->current_token.lexeme);
    baa_set_parser_error(parser, message);
}

// Helper functions
static bool is_whitespace(wchar_t c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static bool is_digit(wchar_t c)
{
    return c >= '0' && c <= '9';
}

static bool is_arabic_letter(wchar_t c)
{
    // Check if character is in Arabic Unicode range
    return (unsigned char)c >= 0xD8 && (unsigned char)c <= 0xF2;
}

static bool is_identifier_start(wchar_t c)
{
    return is_arabic_letter(c) || c == '_';
}

static bool is_identifier_part(wchar_t c)
{
    return is_arabic_letter(c) || is_digit(c) || c == '_';
}

static bool is_operator(wchar_t c)
{
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
           c == '=' || c == '<' || c == '>' || c == '!' || c == '&' || c == '|';
}

static int get_operator_precedence(const wchar_t *op)
{
    if (wcscmp(op, L"=") == 0)
        return 1;
    if (wcscmp(op, L"||") == 0)
        return 2;
    if (wcscmp(op, L"&&") == 0)
        return 3;
    if (wcscmp(op, L"==") == 0 || wcscmp(op, L"!=") == 0)
        return 4;
    if (wcscmp(op, L"<") == 0 || wcscmp(op, L">") == 0 ||
        wcscmp(op, L"<=") == 0 || wcscmp(op, L">=") == 0)
        return 5;
    if (wcscmp(op, L"+") == 0 || wcscmp(op, L"-") == 0)
        return 6;
    if (wcscmp(op, L"*") == 0 || wcscmp(op, L"/") == 0 || wcscmp(op, L"%") == 0)
        return 7;
    return 0;
}

static bool is_eof(BaaParser *parser)
{
    // Check if we've reached the end of the token stream
    return parser->current_token.type == BAA_TOKEN_EOF;
}

static wchar_t peek(BaaParser *parser)
{
    // Return the current character from the lexer
    return parser->current_token.lexeme[0];
}

static wchar_t peek_next(BaaParser *parser)
{
    size_t current_pos = parser->lexer->current;
    if (current_pos + 1 >= wcslen(parser->lexer->source))
        return L'\0';
    return parser->lexer->source[current_pos + 1];
}

static void advance(BaaParser *parser)
{
    // Store the current token as previous
    // *** POTENTIAL MEMORY LEAK/DANGLING POINTER ***
    // If previous_token.lexeme pointed to dynamically allocated memory
    // from the *previous* advance call, it needs to be freed here *before* overwriting.
    // Let's assume for now that previous_token.lexeme is either NULL or static,
    // or that the ownership model guarantees it's freed elsewhere.
    // A safer approach is needed if lexemes are consistently dynamic.

    // Free the *lexeme* of the *previous* current token, as we're about to replace it.
    // Only free if it's not an ERROR token (which uses static messages)
    // and if the lexeme pointer is not NULL.
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
        // Handle case where lexer returns NULL (e.g., internal lexer error)
        // Set current token to EOF or an error state?
        parser->current_token.type = BAA_TOKEN_EOF; // Or maybe a special parser error token?
        parser->current_token.lexeme = NULL;
        parser->current_token.length = 0;
        // Keep line/column from previous token for error reporting location
    }

    // Update location (likely based on the *new* current_token)
    parser->location = baa_get_current_location(parser);
}

static void skip_whitespace(BaaParser *parser)
{
    while (!is_eof(parser) && is_whitespace(peek(parser)))
    {
        advance(parser);
    }
}

static void skip_comment(BaaParser *parser)
{
    wchar_t c = peek(parser);
    if (c == L'/')
    {
        advance(parser); // Skip first '/'
        c = peek(parser);
        if (c == L'/')
        {
            // Single-line comment
            while (peek(parser) && peek(parser) != L'\n')
            {
                advance(parser);
            }
        }
        else if (c == L'*')
        {
            // Multi-line comment
            advance(parser); // Skip '*'
            while (peek(parser))
            {
                if (peek(parser) == L'*' && peek_next(parser) == L'/')
                {
                    advance(parser); // Skip '*'
                    advance(parser); // Skip '/'
                    break;
                }
                advance(parser);
            }
        }
    }
}

static bool match_keyword(BaaParser *parser, const wchar_t *keyword)
{
    // Check if the current token is the keyword we're looking for
    if (parser->current_token.type == BAA_TOKEN_IDENTIFIER)
    {
        size_t len = wcslen(keyword);
        if (parser->current_token.length == len &&
            wcsncmp(parser->current_token.lexeme, keyword, len) == 0)
        {
            // Consume the token
            advance(parser);
            return true;
        }
    }
    return false;
}

static wchar_t *parse_identifier(BaaParser *parser)
{
    if (parser->current_token.type != BAA_TOKEN_IDENTIFIER)
    {
        return NULL;
    }

    // Duplicate the lexeme
    wchar_t *identifier = baa_strdup(parser->current_token.lexeme);
    if (!identifier)
        return NULL;

    // Consume the identifier token
    advance(parser);

    return identifier;
}

static bool expect_char(BaaParser *parser, wchar_t expected)
{
    if (peek(parser) != expected)
    {
        wchar_t message[100];
        swprintf(message, 100, L"Expected '%lc'", expected);
        baa_set_parser_error(parser, message);
        return false;
    }
    advance(parser);
    return true;
}

// Type parsing
static BaaType *parse_type(BaaParser *parser)
{
    if (match_keyword(parser, L"عدد_صحيح"))
    {
        return baa_get_int_type();
    }
    else if (match_keyword(parser, L"عدد_حقيقي"))
    {
        return baa_get_float_type();
    }
    else if (match_keyword(parser, L"حرف"))
    {
        return baa_get_char_type();
    }
    else if (match_keyword(parser, L"فراغ"))
    {
        return baa_get_void_type();
    }
    return NULL;
}

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
        parser->error_message = wcsdup(full_msg);
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

static BaaNode *parse_declaration(BaaParser *parser)
{
    BaaStmt *stmt = baa_parse_declaration(parser);
    if (!stmt) {
        // baa_parse_declaration handles setting the error or returned NULL for function
        return NULL;
    }

    BaaNode *node = baa_create_node(BAA_NODE_STMT, stmt);
    if (!node) {
        baa_set_parser_error(parser, L"فشل في إنشاء عقدة AST للتصريح");
        baa_free_stmt(stmt);
        return NULL;
    }
    // Optionally set location from the start token of the statement if needed
    // node->location = parser->start_token_location; // Example

    return node;
}

static BaaNode *parse_block(BaaParser *parser)
{
    BaaBlock *block = baa_create_block();
    if (!block) {
        baa_set_parser_error(parser, L"فشل في إنشاء كتلة نصية");
        return NULL;
    }

    // Consume '{'
    baa_token_next(parser);

    while (parser->current_token.type != BAA_TOKEN_RIGHT_BRACE &&
           parser->current_token.type != BAA_TOKEN_EOF)
    {
        BaaStmt* stmt = baa_parse_statement(parser);
        if (parser->had_error) {
            baa_free_block(block);
            return NULL;
        }
        if (stmt) {
             if (!baa_add_stmt_to_block(block, stmt)) {
                baa_set_parser_error(parser, L"فشل في إضافة عبارة إلى الكتلة");
                baa_free_stmt(stmt);
                baa_free_block(block);
                return NULL;
            }
        } // Might be NULL if it was a function declaration, handled in caller
    }

    if (parser->current_token.type != BAA_TOKEN_RIGHT_BRACE) {
        baa_unexpected_token_error(parser, L"}");
        baa_free_block(block);
        return NULL;
    }

    // Consume '}'
    baa_token_next(parser);

    // Create the wrapping BaaStmt for the block
    BaaStmt* block_stmt = baa_create_block_stmt();
    if (!block_stmt) {
        baa_set_parser_error(parser, L"فشل في إنشاء عبارة الكتلة");
        baa_free_block(block);
        return NULL;
    }
    block_stmt->data = block;

    // Create the AST Node for the statement
    BaaNode* node = baa_create_node(BAA_NODE_STMT, block_stmt);
    if (!node) {
         baa_set_parser_error(parser, L"فشل في إنشاء عقدة الكتلة");
         baa_free_stmt(block_stmt); // This will free the block via baa_free_block
         return NULL;
    }
    block_stmt->ast_node = node; // Link statement back to node

    return node;
}

static BaaStmt* parse_statement(BaaParser *parser)
{
    // Skip potential leading whitespace/comments
    skip_whitespace(parser);
    skip_comment(parser);

    BaaToken start_token = parser->current_token;

    // Parse based on the current token
    if (match_keyword(parser, L"لو"))
    {
        return baa_parse_if(parser);
    }
    else if (match_keyword(parser, L"طالما"))
    {
        return baa_parse_while(parser);
    }
    else if (match_keyword(parser, L"لكل"))
    {
        return baa_parse_for(parser);
    }
    else if (match_keyword(parser, L"ارجع"))
    {
        return baa_parse_return(parser);
    }
    else if (parser->current_token.type == BAA_TOKEN_LEFT_BRACE) {
        return parse_block(parser); // parse_block now returns BaaStmt*
    }
    else if (parser->current_token.type == BAA_TOKEN_VAR || parser->current_token.type == BAA_TOKEN_CONST) {
        // Variable declarations are handled by baa_parse_declaration
        // which is called from parse_declaration() wrapper
        BaaStmt* decl_stmt = baa_parse_declaration(parser);
        // Need semicolon check here or within baa_parse_declaration
        if (decl_stmt && parser->current_token.type == BAA_TOKEN_DOT) { // Assuming DOT is semicolon
             advance(parser);
             return decl_stmt;
        } else if (decl_stmt) {
             baa_unexpected_token_error(parser, L".");
             baa_free_stmt(decl_stmt);
             return NULL;
        } else {
            // Error already set by baa_parse_declaration
            return NULL;
        }
    }
    // Add other statement types like break, continue, switch
    else
    {
        // Default to expression statement
        BaaExpr* expr = baa_parse_expression(parser);
        if (!expr) {
             // If expression parsing failed, check if it was because no expression was found
             // vs. a syntax error within an attempted expression.
             // If it was just not an expression, don't set error here.
             if (parser->had_error) {
                 return NULL; // Error already set
             }
             // else: Not an expression, maybe a keyword we didn't handle?
             baa_unexpected_token_error(parser, L"عبارة أو تعبير");
             return NULL;
        }

        // Expect semicolon after expression statement
        if (parser->current_token.type != BAA_TOKEN_DOT) { // Assuming DOT is semicolon
            baa_unexpected_token_error(parser, L".");
            baa_free_expr(expr);
            return NULL;
        }
        advance(parser);

        BaaStmt* expr_stmt = baa_create_expr_stmt(expr);
        if (!expr_stmt) {
            baa_free_expr(expr);
            return NULL;
        }
        return expr_stmt;
    }
}

/**
 * Parse a function definition
 */
static BaaFunction* parse_function(BaaParser *parser)
{
    // Delegate to the function declaration parser
    BaaFunction* func = baa_parse_function_declaration(parser);
    if (!func) {
        // Error handled in baa_parse_function_declaration
        return NULL;
    }

    // The caller (baa_parse) should handle adding the function to the program
    // and creating the BAA_NODE_FUNCTION node.

    return func;
}

// Import directive parsing
static BaaNode *parse_import_directive(BaaParser *parser)
{
    if (!match_keyword(parser, L"#تضمين"))
    {
        return NULL;
    }

    skip_whitespace(parser);

    // Check for < or "
    bool is_system = false;
    if (parser->current_token.type == BAA_TOKEN_LESS)
    {
        is_system = true;
        advance(parser);
    }
    else if (parser->current_token.type == BAA_TOKEN_STRING_LIT)
    {
        // String literal already includes the quotes
        const wchar_t *path = parser->current_token.lexeme;
        size_t path_len = parser->current_token.length;

        // Remove the quotes from the path
        wchar_t *clean_path = NULL;
        if (path_len > 2)
        {
            clean_path = baa_strndup(path + 1, path_len - 2);
        }
        else
        {
            clean_path = baa_strdup(L"");
        }

        if (!clean_path)
        {
            baa_set_parser_error(parser, L"Failed to allocate memory for import path");
            return NULL;
        }

        advance(parser); // Consume the string token

        BaaSourceLocation loc = baa_create_source_location(0, 0, clean_path);
        BaaNode *import = baa_create_node(BAA_NODE_STMT, loc);
        baa_free(clean_path);

        if (!import)
        {
            baa_set_parser_error(parser, L"Failed to create import node");
            return NULL;
        }

        skip_whitespace(parser);
        if (parser->current_token.type != BAA_TOKEN_DOT)
        {
            baa_set_parser_error(parser, L"Expected '.' after import path");
            baa_free_node(import);
            return NULL;
        }
        advance(parser); // Consume the dot

        return import;
    }
    else
    {
        baa_set_parser_error(parser, L"Expected '<' or '\"' after #تضمين");
        return NULL;
    }

    // For system imports with < >
    wchar_t *path = NULL;
    // Read until we find a '>'
    while (parser->current_token.type != BAA_TOKEN_GREATER &&
           parser->current_token.type != BAA_TOKEN_EOF)
    {
        // Append current token to path
        const wchar_t *token_text = parser->current_token.lexeme;
        size_t token_len = parser->current_token.length;

        wchar_t *new_path = NULL;
        if (path)
        {
            size_t path_len = wcslen(path);
            new_path = baa_malloc((path_len + token_len + 1) * sizeof(wchar_t));
            if (new_path)
            {
                wcscpy(new_path, path);
                wcsncat(new_path, token_text, token_len);
                new_path[path_len + token_len] = L'\0';
            }
            baa_free(path);
        }
        else
        {
            new_path = baa_strndup(token_text, token_len);
        }

        path = new_path;
        if (!path)
        {
            baa_set_parser_error(parser, L"Failed to allocate memory for import path");
            return NULL;
        }

        advance(parser);
    }

    if (parser->current_token.type != BAA_TOKEN_GREATER)
    {
        baa_set_parser_error(parser, L"Expected '>' to close system import");
        baa_free(path);
        return NULL;
    }
    advance(parser); // Consume the '>'

    BaaSourceLocation loc = baa_create_source_location(0, 0, path);
    BaaNode *import = baa_create_node(BAA_NODE_STMT, loc);
    baa_free(path);

    if (!import)
    {
        baa_set_parser_error(parser, L"Failed to create import node");
        return NULL;
    }

    // Mark as system import
    // Note: We need to define NODE_FLAG_SYSTEM_IMPORT in the appropriate header
    // import->flags |= NODE_FLAG_SYSTEM_IMPORT;

    skip_whitespace(parser);
    if (parser->current_token.type != BAA_TOKEN_DOT)
    {
        baa_set_parser_error(parser, L"Expected '.' after import path");
        baa_free_node(import);
        return NULL;
    }
    advance(parser); // Consume the dot

    return import;
}

// Main parsing functions
BaaProgram *baa_parse(const wchar_t *input, const wchar_t *filename)
{
    BaaParser parser;
    BaaLexer *lexer = baa_create_lexer(input);
    if (!lexer) return NULL;

    BaaProgram *program = baa_create_program();
    if (!program) {
        baa_free_lexer(lexer);
        return NULL;
    }

    // Create the root AST node for the program
    program->ast_node = baa_create_node(BAA_NODE_PROGRAM, program);
    if (!program->ast_node) {
        baa_free_program(program); // Frees the inner program struct too
        baa_free_lexer(lexer);
        return NULL;
    }

    parser.lexer = lexer;
    parser.had_error = false;
    parser.panic_mode = false;
    baa_token_next(&parser); // Load the first token

    while (parser.current_token.type != BAA_TOKEN_EOF)
    {
        // Try parsing top-level declarations (functions or global vars)
        if (parser.current_token.type == BAA_TOKEN_FUNC) {
            BaaFunction* func = parse_function(&parser);
            if (func) {
                BaaNode* func_node = baa_create_node(BAA_NODE_FUNCTION, func);
                if (func_node) {
                    func->ast_node = func_node; // Link function back to node
                    baa_add_node_to_program(program, func_node);
                } else {
                    baa_free_function(func); // Cleanup if node creation fails
                    parser.had_error = true; // Signal error
                }
            }
            // Error handling or NULL return already managed within parse_function/baa_parse_function_declaration
        }
        // TODO: Add support for top-level variable declarations if needed
        // else if (parser.current_token.type == BAA_TOKEN_VAR) { ... }
        else {
            // If it's not a known top-level declaration, report error or skip
            baa_unexpected_token_error(&parser, L"تصريح دالة أو نهاية الملف");
            // Attempt error recovery
            synchronize(&parser);
        }

        if (parser.had_error)
        {
            if (!parser.panic_mode) {
                // Report error details if needed
                parser.panic_mode = true; // Enter panic mode after first error
            }
            // Attempt to recover or break
             synchronize(&parser);
             if (parser.current_token.type == BAA_TOKEN_EOF) break; // Stop if EOF reached after error
        } else {
             parser.panic_mode = false; // Reset panic mode on successful parse
        }
    }

    baa_free_lexer(lexer);

    if (parser.had_error) {
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

BaaStmt *baa_parse_statement(BaaParser *parser)
{
    skip_whitespace(parser);

    // Parse if statement (إذا)
    if (match_keyword(parser, L"إذا"))
    {
        BaaExpr *condition = NULL;
        BaaBlock *if_body = NULL;
        BaaBlock *else_body = NULL;

        skip_whitespace(parser);
        if (!expect_char(parser, L'('))
        {
            return NULL;
        }

        BaaExpr *condition_expr = baa_parse_expression(parser);
        if (!condition_expr)
        {
            return NULL;
        }

        // Convert node to expression
        condition = condition_expr;

        if (!expect_char(parser, L')'))
        {
            baa_free_expression(condition);
            return NULL;
        }

        if (!expect_char(parser, L'{'))
        {
            baa_free_expression(condition);
            return NULL;
        }

        BaaNode *if_body_node = parse_block(parser);
        if (!if_body_node)
        {
            baa_free_expression(condition);
            return NULL;
        }

        // Convert node to block
        if_body = baa_create_block();
        if (!if_body)
        {
            baa_free_expression(condition);
            baa_free_node(if_body_node);
            return NULL;
        }

        skip_whitespace(parser);

        // Parse else clause (وإلا)
        if (match_keyword(parser, L"وإلا"))
        {
            skip_whitespace(parser);
            if (!expect_char(parser, L'{'))
            {
                baa_free_expression(condition);
                baa_free_block(if_body);
                return NULL;
            }

            BaaNode *else_body_node = parse_block(parser);
            if (!else_body_node)
            {
                baa_free_expression(condition);
                baa_free_block(if_body);
                return NULL;
            }

            // Convert node to block
            else_body = baa_create_block();
            if (!else_body)
            {
                baa_free_expression(condition);
                baa_free_block(if_body);
                baa_free_node(else_body_node);
                return NULL;
            }
        }

        // Create if statement
        BaaStmt *if_stmt = baa_create_if_stmt(condition, if_body, else_body);
        if (!if_stmt)
        {
            baa_free_expression(condition);
            baa_free_block(if_body);
            if (else_body)
                baa_free_block(else_body);
            return NULL;
        }

        return if_stmt;
    }

    // Parse while loop
    if (match_keyword(parser, L"طالما"))
    {
        BaaStmt* while_stmt = baa_parse_while(parser);
        if (!while_stmt) return NULL;

        // Node creation should happen in the main parse loop or caller
        // if this statement can appear top-level, otherwise it's
        // handled when added to a block.
        // For now, just return the created statement.
        return while_stmt;
    }

    // Parse other statement types
    // ...

    return NULL;
}

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

// Implementation of missing functions
static void baa_add_child(BaaNode *parent, BaaNode *child)
{
    if (!parent || !child)
    {
        return;
    }

    // Set the parent of the child node
    child->parent = parent;

    // Note: This is a simplified implementation. In a real AST,
    // you would typically have a children array in the BaaNode structure.
    // For now, we're just setting the parent-child relationship.
}

BaaBlock *baa_create_block(void)
{
    BaaBlock *block = baa_malloc(sizeof(BaaBlock));
    if (!block)
    {
        return NULL;
    }

    // Initialize the block
    block->statements = NULL;
    block->count = 0;
    block->capacity = 0;

    // Remove AST node creation from here
    // The AST node should wrap the BaaStmt of kind BAA_STMT_BLOCK

    return block;
}

// Helper to add a node to a program
static void baa_add_node_to_program(BaaProgram *program, BaaNode *node)
{
    if (!program || !node) return;

    // Set parent link
    node->parent = program->ast_node; // Link to the program's AST node

    // Add function specifically if it's a function node
    if (node->kind == BAA_NODE_FUNCTION && node->data) {
        baa_add_function_to_program(program, (BaaFunction*)node->data);
    }
    // TODO: Handle top-level statements if allowed
}

// Helper function to free a program properly
static void baa_free_program_wrapper(BaaProgram *program)
{
    if (program)
    {
        baa_free_program(program);
    }
}
