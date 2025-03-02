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

// Forward declarations
static BaaNode *parse_function(BaaParser *parser);
static BaaNode *parse_declaration(BaaParser *parser);
static BaaNode *parse_block(BaaParser *parser);
static BaaNode *parse_statement(BaaParser *parser);
static BaaNode *parse_import_directive(BaaParser *parser);
void baa_set_parser_error(BaaParser *parser, const wchar_t *message);
static void baa_add_node_to_program(BaaProgram* program, BaaNode* node);
static void advance(BaaParser *parser);

// External declarations from expression_parser.c
extern BaaExpr* baa_parse_expression(BaaParser* parser);
extern void baa_free_expression(BaaExpr* expr);

// Forward declarations for functions not yet defined in headers
static void baa_add_child(BaaNode* parent, BaaNode* child);
static BaaBlock* baa_create_block(void);

// Implementation of baa_token_next for expression_parser.c
void baa_token_next(BaaParser *parser) {
    advance(parser);
}

// Implementation of baa_unexpected_token_error for parser_helper.h
void baa_unexpected_token_error(BaaParser *parser, const wchar_t *expected) {
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
    parser->previous_token = parser->current_token;

    // Get the next token from the lexer
    BaaToken* token = baa_lexer_next_token(parser->lexer);
    if (token) {
        // Copy token data to the parser's current_token
        parser->current_token.type = token->type;
        parser->current_token.lexeme = token->lexeme;
        parser->current_token.length = token->length;
        parser->current_token.line = token->line;
        parser->current_token.column = token->column;

        // We don't free token->lexeme as it's now owned by parser->current_token
        baa_free(token);
    }

    // Update location
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
    if (parser->current_token.type == BAA_TOKEN_IDENTIFIER) {
        size_t len = wcslen(keyword);
        if (parser->current_token.length == len &&
            wcsncmp(parser->current_token.lexeme, keyword, len) == 0) {
            // Consume the token
            advance(parser);
            return true;
        }
    }
    return false;
}

static wchar_t *parse_identifier(BaaParser *parser)
{
    if (parser->current_token.type != BAA_TOKEN_IDENTIFIER) {
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
static BaaType* parse_type(BaaParser *parser)
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
void baa_init_parser(BaaParser *parser, BaaLexer* lexer)
{
    parser->lexer = lexer;
    parser->had_error = false;
    parser->error_message = NULL;

    // Initialize tokens
    memset(&parser->current_token, 0, sizeof(BaaToken));
    memset(&parser->previous_token, 0, sizeof(BaaToken));

    // Get the first token
    BaaToken* token = baa_lexer_next_token(lexer);
    if (token) {
        // Copy token data to the parser's current_token
        parser->current_token.type = token->type;
        parser->current_token.lexeme = token->lexeme;
        parser->current_token.length = token->length;
        parser->current_token.line = token->line;
        parser->current_token.column = token->column;

        // We don't free token->lexeme as it's now owned by parser->current_token
        baa_free(token);
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

const wchar_t* baa_get_parser_error(BaaParser *parser)
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
    printf("Attempting to parse declaration\n");
    skip_whitespace(parser);

    // Check for function declaration
    if (match_keyword(parser, L"دالة"))
    {
        // Parse function declaration syntax: دالة <name>(<params>){<body>}
        printf("Parsing function declaration\n");
        return parse_function(parser);
    }

    // Parse type
    BaaType *type = parse_type(parser);
    if (!type)
    {
        printf("Failed to parse type\n");
        if (!parser->had_error)
        {
            baa_set_parser_error(parser, L"Expected type");
        }
        return NULL;
    }
    printf("Found type: %d\n", type->kind);

    skip_whitespace(parser);

    // Check for const declaration
    bool is_const = match_keyword(parser, L"ثابت");
    if (is_const)
    {
        printf("Found const declaration\n");
        skip_whitespace(parser);
    }

    // Parse identifier
    wchar_t *name = parse_identifier(parser);
    if (!name)
    {
        printf("Failed to parse identifier\n");
        if (!parser->had_error)
        {
            baa_set_parser_error(parser, L"Expected identifier");
        }
        return NULL;
    }
    printf("Found identifier: %s\n", name);

    // Create declaration node
    BaaSourceLocation loc = baa_create_source_location(0, 0, name);
    BaaNode *decl = baa_create_node(BAA_NODE_STATEMENT, loc);
    baa_free(name);

    if (!decl)
    {
        printf("Failed to create declaration node\n");
        return NULL;
    }

    // Set const flag if needed
    // Note: We need to define proper flags in the AST header
    // if (is_const)
    // {
    //     decl->flags |= NODE_FLAG_CONST;
    // }

    skip_whitespace(parser);

    // Check for array declaration
    if (peek(parser) == L'[')
    {
        printf("Found array declaration\n");
        advance(parser);
        skip_whitespace(parser);

        // Parse array size
        BaaExpr *size = baa_parse_expression(parser);
        if (!size)
        {
            printf("Failed to parse array size\n");
            baa_free_node(decl);
            return NULL;
        }

        skip_whitespace(parser);
        if (!expect_char(parser, L']'))
        {
            printf("Expected closing bracket\n");
            baa_free_expression(size);
            baa_free_node(decl);
            return NULL;
        }

        baa_add_child(decl, (BaaNode*)size);
    }

    skip_whitespace(parser);

    // Check for initialization
    if (peek(parser) == L'=')
    {
        printf("Found initialization\n");
        advance(parser);
        skip_whitespace(parser);

        BaaExpr *init = baa_parse_expression(parser);
        if (!init)
        {
            printf("Failed to parse initializer\n");
            baa_free_node(decl);
            return NULL;
        }

        baa_add_child(decl, (BaaNode*)init);
    }

    skip_whitespace(parser);
    if (!expect_char(parser, L'.'))
    {
        printf("Expected statement terminator\n");
        baa_free_node(decl);
        return NULL;
    }

    printf("Successfully parsed declaration\n");
    return decl;
}

static BaaNode *parse_block(BaaParser *parser)
{
    BaaSourceLocation loc = baa_get_current_location(parser);
    
    // Create a block statement with the standardized structure
    BaaStmt* block_stmt = baa_create_stmt(BAA_STMT_BLOCK, loc);
    if (!block_stmt) {
        return NULL;
    }
    
    // Create the actual block data
    BaaBlock* block = baa_create_block();
    if (!block) {
        baa_free_stmt(block_stmt);
        return NULL;
    }
    
    // Connect the block data to the statement
    block_stmt->data = block;
    
    // Create a node to hold this statement
    BaaNode* node = baa_create_node(BAA_NODE_STATEMENT, loc);
    if (!node) {
        baa_free_stmt(block_stmt); // This will also free the block
        return NULL;
    }
    
    // Connect the statement to the node
    node->data = block_stmt;
    
    while (peek(parser) && peek(parser) != L'}')
    {
        skip_whitespace(parser);
        skip_comment(parser);

        BaaNode *stmt_node = parse_statement(parser);
        if (!stmt_node)
        {
            baa_free_node(node);
            return NULL;
        }
        
        // Extract the statement from the node
        BaaStmt* stmt = (BaaStmt*)stmt_node->data;
        
        // Add the statement to the block
        if (!baa_block_add_statement(block, stmt)) {
            baa_free_node(stmt_node);
            baa_free_node(node);
            return NULL;
        }
    }

    if (!expect_char(parser, L'}'))
    {
        baa_free_node(node);
        return NULL;
    }

    return node;
}

static BaaNode *parse_statement(BaaParser *parser)
{
    skip_whitespace(parser);

    // Parse if statement
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

        // Parse else clause
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

        return if_stmt->ast_node;
    }

    // Parse while loop
    if (match_keyword(parser, L"طالما"))
    {
        if (!expect_char(parser, L'('))
            return NULL;

        BaaExpr *condition = baa_parse_expression(parser);
        if (!condition)
            return NULL;

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

        BaaNode *body = parse_block(parser);
        if (!body)
        {
            baa_free_expression(condition);
            return NULL;
        }

        BaaSourceLocation loc = baa_create_source_location(0, 0, L"while");
        BaaNode *while_stmt = baa_create_node(BAA_NODE_STATEMENT, loc);
        if (!while_stmt)
        {
            baa_free_expression(condition);
            baa_free_node(body);
            return NULL;
        }

        baa_add_child(while_stmt, (BaaNode*)condition);
        baa_add_child(while_stmt, body);

        if (!expect_char(parser, L'.'))
        {
            baa_free_node(while_stmt);
            return NULL;
        }

        return while_stmt;
    }

    // Parse other statements
    BaaExpr *expr = baa_parse_expression(parser);
    if (expr)
    {
        if (!expect_char(parser, L'.'))
        {
            baa_free_expression(expr);
            return NULL;
        }
        return (BaaNode*)expr;
    }

    return NULL;
}

static BaaNode *parse_function(BaaParser *parser)
{
    skip_whitespace(parser);

    // Parse function name
    wchar_t *name = parse_identifier(parser);
    if (!name)
    {
        baa_set_parser_error(parser, L"Expected function name");
        return NULL;
    }

    BaaSourceLocation loc = baa_create_source_location(0, 0, name);
    BaaNode *func = baa_create_node(BAA_NODE_FUNCTION, loc);
    baa_free(name);

    if (!func)
    {
        baa_set_parser_error(parser, L"Failed to create function node");
        return NULL;
    }

    skip_whitespace(parser);

    // Parse parameter list
    if (!expect_char(parser, L'('))
    {
        baa_free_node(func);
        return NULL;
    }

    skip_whitespace(parser);

    // Parse parameters
    while (!is_eof(parser) && peek(parser) != ')')
    {
        BaaNode *param = parse_declaration(parser);
        if (!param)
        {
            baa_free_node(func);
            return NULL;
        }

        baa_add_child(func, param);

        skip_whitespace(parser);

        if (peek(parser) == ',')
        {
            advance(parser);
            skip_whitespace(parser);
        }
    }

    if (!expect_char(parser, ')'))
    {
        baa_free_node(func);
        return NULL;
    }

    skip_whitespace(parser);

    // Parse function body
    if (!expect_char(parser, '{'))
    {
        baa_free_node(func);
        return NULL;
    }

    // Parse function body statements
    BaaNode *body = parse_block(parser);
    if (!body)
    {
        baa_free_node(func);
        return NULL;
    }
    baa_add_child(func, body);
    int brace_count = 1;
    while (!is_eof(parser) && brace_count > 0)
    {
        char c = peek(parser);
        if (c == '{')
            brace_count++;
        else if (c == '}')
            brace_count--;
        advance(parser);
    }

    if (brace_count > 0)
    {
        baa_set_parser_error(parser, L"Unterminated function body");
        baa_free_node(func);
        return NULL;
    }

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
    if (parser->current_token.type == BAA_TOKEN_LESS) {
        is_system = true;
        advance(parser);
    }
    else if (parser->current_token.type == BAA_TOKEN_STRING_LIT) {
        // String literal already includes the quotes
        const wchar_t* path = parser->current_token.lexeme;
        size_t path_len = parser->current_token.length;

        // Remove the quotes from the path
        wchar_t* clean_path = NULL;
        if (path_len > 2) {
            clean_path = baa_strndup(path + 1, path_len - 2);
        } else {
            clean_path = baa_strdup(L"");
        }

        if (!clean_path) {
            baa_set_parser_error(parser, L"Failed to allocate memory for import path");
            return NULL;
        }

        advance(parser); // Consume the string token

        BaaSourceLocation loc = baa_create_source_location(0, 0, clean_path);
        BaaNode *import = baa_create_node(BAA_NODE_STATEMENT, loc);
        baa_free(clean_path);

        if (!import) {
            baa_set_parser_error(parser, L"Failed to create import node");
            return NULL;
        }

        skip_whitespace(parser);
        if (parser->current_token.type != BAA_TOKEN_DOT) {
            baa_set_parser_error(parser, L"Expected '.' after import path");
            baa_free_node(import);
            return NULL;
        }
        advance(parser); // Consume the dot

        return import;
    }
    else {
        baa_set_parser_error(parser, L"Expected '<' or '\"' after #تضمين");
        return NULL;
    }

    // For system imports with < >
    wchar_t* path = NULL;
    // Read until we find a '>'
    while (parser->current_token.type != BAA_TOKEN_GREATER &&
           parser->current_token.type != BAA_TOKEN_EOF) {
        // Append current token to path
        const wchar_t* token_text = parser->current_token.lexeme;
        size_t token_len = parser->current_token.length;

        wchar_t* new_path = NULL;
        if (path) {
            size_t path_len = wcslen(path);
            new_path = baa_malloc((path_len + token_len + 1) * sizeof(wchar_t));
            if (new_path) {
                wcscpy(new_path, path);
                wcsncat(new_path, token_text, token_len);
                new_path[path_len + token_len] = L'\0';
            }
            baa_free(path);
        } else {
            new_path = baa_strndup(token_text, token_len);
        }

        path = new_path;
        if (!path) {
            baa_set_parser_error(parser, L"Failed to allocate memory for import path");
            return NULL;
        }

        advance(parser);
    }

    if (parser->current_token.type != BAA_TOKEN_GREATER) {
        baa_set_parser_error(parser, L"Expected '>' to close system import");
        baa_free(path);
        return NULL;
    }
    advance(parser); // Consume the '>'

    BaaSourceLocation loc = baa_create_source_location(0, 0, path);
    BaaNode *import = baa_create_node(BAA_NODE_STATEMENT, loc);
    baa_free(path);

    if (!import) {
        baa_set_parser_error(parser, L"Failed to create import node");
        return NULL;
    }

    // Mark as system import
    // Note: We need to define NODE_FLAG_SYSTEM_IMPORT in the appropriate header
    // import->flags |= NODE_FLAG_SYSTEM_IMPORT;

    skip_whitespace(parser);
    if (parser->current_token.type != BAA_TOKEN_DOT) {
        baa_set_parser_error(parser, L"Expected '.' after import path");
        baa_free_node(import);
        return NULL;
    }
    advance(parser); // Consume the dot

    return import;
}

// Main parsing functions
BaaProgram *baa_parse_program(BaaParser *parser)
{
    printf("Starting program parse\n");
    BaaProgram *program = baa_create_program();
    if (!program)
        return NULL;

    while (!is_eof(parser))
    {
        skip_whitespace(parser);
        if (is_eof(parser))
            break;

        printf("Current character: %lc (0x%02x)\n", peek(parser), (unsigned char)peek(parser));

        // Skip comments
        if (peek(parser) == L'#')
        {
            printf("Found comment\n");
            if (peek_next(parser) == L'ت')
            {
                printf("Found import directive\n");
                BaaNode *import = parse_import_directive(parser);
                if (!import)
                {
                    baa_free_program(program);
                    return NULL;
                }
                baa_add_node_to_program(program, import);
                continue;
            }

            // Skip regular comments
            while (!is_eof(parser) && peek(parser) != '\n')
            {
                advance(parser);
            }
            continue;
        }

        // Parse declarations and functions
        BaaNode *node = parse_declaration(parser);
        if (!node)
        {
            printf("Failed to parse declaration\n");
            baa_free_program(program);
            return NULL;
        }
        baa_add_node_to_program(program, node);
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

BaaStmt* baa_parse_statement(BaaParser *parser)
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

    // Parse other statement types
    // ...

    return NULL;
}

BaaParser* baa_create_parser(const wchar_t* source, size_t source_len)
{
    if (!source) {
        return NULL;
    }

    // Allocate memory for the parser
    BaaParser* parser = (BaaParser*)baa_malloc(sizeof(BaaParser));
    if (!parser) {
        return NULL;
    }

    // Create the lexer
    BaaLexer* lexer = baa_create_lexer(source);
    if (!lexer) {
        baa_free(parser);
        return NULL;
    }

    // Initialize the parser with the lexer
    baa_init_parser(parser, lexer);

    return parser;
}

// Implementation of missing functions
static void baa_add_child(BaaNode* parent, BaaNode* child) {
    if (!parent || !child) {
        return;
    }

    // Set the parent of the child node
    child->parent = parent;

    // Note: This is a simplified implementation. In a real AST,
    // you would typically have a children array in the BaaNode structure.
    // For now, we're just setting the parent-child relationship.
}

static BaaBlock* baa_create_block(void) {
    BaaBlock* block = (BaaBlock*)baa_malloc(sizeof(BaaBlock));
    if (!block) {
        return NULL;
    }

    // Initialize the block
    block->statements = NULL;
    block->count = 0;
    block->capacity = 0;

    // Create an AST node for this block
    BaaSourceLocation loc = baa_create_source_location(0, 0, L"block");
    block->ast_node = baa_create_node(BAA_NODE_STATEMENT, loc);

    if (!block->ast_node) {
        baa_free(block);
        return NULL;
    }

    return block;
}

// Helper to add a node to a program
static void baa_add_node_to_program(BaaProgram* program, BaaNode* node) {
    if (!program || !node) {
        return;
    }

    // For now, we'll just set the parent relationship
    node->parent = &program->node;

    // In a more complete implementation, you would add the node to a children array
    // or handle different node types appropriately
}

// Helper function to free a program properly
static void baa_free_program_wrapper(BaaProgram* program) {
    if (program) {
        baa_free_program(program);
    }
}
