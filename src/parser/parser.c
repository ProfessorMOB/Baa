#include "baa/parser.h"
#include "baa/utils.h"
#include "baa/ast.h"
#include "../include/baa_errors.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

// Forward declarations
static Node* parse_expression(Parser* parser);
static Node* parse_function(Parser* parser);
static void set_error(Parser* parser, const char* message);

// Helper functions
static bool is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_arabic_letter(char c) {
    // Check if character is in Arabic Unicode range
    return (unsigned char)c >= 0xD8 && (unsigned char)c <= 0xF2;
}

static bool is_identifier_start(char c) {
    return is_arabic_letter(c) || c == '_';
}

static bool is_identifier_part(char c) {
    return is_arabic_letter(c) || is_digit(c) || c == '_';
}

static bool is_operator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
           c == '=' || c == '<' || c == '>' || c == '!' || c == '&' || c == '|';
}

static int get_operator_precedence(const char* op) {
    if (strcmp(op, "=") == 0) return 1;
    if (strcmp(op, "||") == 0) return 2;
    if (strcmp(op, "&&") == 0) return 3;
    if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0) return 4;
    if (strcmp(op, "<") == 0 || strcmp(op, ">") == 0 || 
        strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0) return 5;
    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0) return 6;
    if (strcmp(op, "*") == 0 || strcmp(op, "/") == 0 || strcmp(op, "%") == 0) return 7;
    return 0;
}

static bool is_eof(Parser* parser) {
    return parser->position >= strlen(parser->source);
}

static char peek(Parser* parser) {
    if (is_eof(parser)) return '\0';
    return parser->source[parser->position];
}

static char peek_next(Parser* parser) {
    if (parser->position + 1 >= strlen(parser->source)) return '\0';
    return parser->source[parser->position + 1];
}

static void advance(Parser* parser) {
    if (!is_eof(parser)) {
        if (peek(parser) == '\n') {
            parser->line++;
            parser->column = 0;
        } else {
            parser->column++;
        }
        parser->position++;
    }
}

static void skip_whitespace(Parser* parser) {
    while (!is_eof(parser) && is_whitespace(peek(parser))) {
        advance(parser);
    }
}

static void skip_comment(Parser* parser) {
    char c = peek(parser);
    if (c == '/') {
        advance(parser); // Skip first '/'
        c = peek(parser);
        if (c == '/') {
            // Single-line comment
            while (peek(parser) && peek(parser) != '\n') {
                advance(parser);
            }
        } else if (c == '*') {
            // Multi-line comment
            advance(parser); // Skip '*'
            while (peek(parser)) {
                if (peek(parser) == '*' && parser->source[parser->position + 1] == '/') {
                    advance(parser); // Skip '*'
                    advance(parser); // Skip '/'
                    break;
                }
                advance(parser);
            }
        }
    }
}

static bool match_keyword(Parser* parser, const char* keyword) {
    size_t len = strlen(keyword);
    if (parser->position + len > strlen(parser->source)) return false;
    
    for (size_t i = 0; i < len; i++) {
        if (parser->source[parser->position + i] != keyword[i]) {
            return false;
        }
    }
    
    // Make sure the next character is not part of an identifier
    if (parser->position + len < strlen(parser->source)) {
        char next = parser->source[parser->position + len];
        if (is_identifier_part(next)) {
            return false;
        }
    }
    
    parser->position += len;
    return true;
}

static char* parse_identifier(Parser* parser) {
    if (!is_identifier_start(peek(parser))) {
        return NULL;
    }
    
    size_t start = parser->position;
    size_t len = 0;
    
    while (!is_eof(parser) && is_identifier_part(peek(parser))) {
        advance(parser);
        len++;
    }
    
    char* identifier = baa_malloc(len + 1);
    if (!identifier) return NULL;
    
    strncpy(identifier, parser->source + start, len);
    identifier[len] = '\0';
    return identifier;
}

static bool expect_char(Parser* parser, char expected) {
    if (peek(parser) != expected) {
        char message[100];
        sprintf(message, "Expected '%c'", expected);
        set_error(parser, message);
        return false;
    }
    advance(parser);
    return true;
}

// Type parsing
static NodeType parse_type(Parser* parser) {
    if (match_keyword(parser, "عدد_صحيح")) {
        return NODE_TYPE_INT;
    } else if (match_keyword(parser, "عدد_حقيقي")) {
        return NODE_TYPE_FLOAT;
    } else if (match_keyword(parser, "حرف")) {
        return NODE_TYPE_CHAR;
    } else if (match_keyword(parser, "فراغ")) {
        return NODE_TYPE_VOID;
    }
    return (NodeType)-1;
}

// Parser implementation
Parser* baa_parser_init(const char* source) {
    Parser* parser = baa_malloc(sizeof(Parser));
    if (!parser) {
        return NULL;
    }

    parser->source = source;
    parser->position = 0;
    parser->line = 1;
    parser->column = 1;
    parser->had_error = false;
    parser->error_msg = NULL;

    return parser;
}

void baa_parser_free(Parser* parser) {
    if (parser) {
        if (parser->error_msg) {
            baa_free((void*)parser->error_msg);
        }
        baa_free(parser);
    }
}

static void set_error(Parser* parser, const char* message) {
    if (!parser->had_error) {
        parser->had_error = true;
        if (parser->error_msg) {
            baa_free((void*)parser->error_msg);
        }
        parser->error_msg = strdup(message);
    }
}

static Node* parse_declaration(Parser* parser) {
    printf("Attempting to parse declaration\n");
    skip_whitespace(parser);
    
    // Check for function declaration
    if (match_keyword(parser, "دالة")) {
        printf("Found function declaration\n");
        return parse_function(parser);
    }
    
    // Parse type
    NodeType type = parse_type(parser);
    if (type == (NodeType)-1) {
        printf("Failed to parse type\n");
        if (!parser->had_error) {
            set_error(parser, "Expected type");
        }
        return NULL;
    }
    printf("Found type: %d\n", type);
    
    skip_whitespace(parser);
    
    // Check for const declaration
    bool is_const = match_keyword(parser, "ثابت");
    if (is_const) {
        printf("Found const declaration\n");
        skip_whitespace(parser);
    }
    
    // Parse identifier
    char* name = parse_identifier(parser);
    if (!name) {
        printf("Failed to parse identifier\n");
        if (!parser->had_error) {
            set_error(parser, "Expected identifier");
        }
        return NULL;
    }
    printf("Found identifier: %s\n", name);
    
    // Create declaration node
    Node* decl = baa_create_node(NODE_VAR_DECL, name);
    baa_free(name);
    
    if (!decl) {
        printf("Failed to create declaration node\n");
        return NULL;
    }
    
    // Set const flag if needed
    if (is_const) {
        decl->flags |= NODE_FLAG_CONST;
    }
    
    skip_whitespace(parser);
    
    // Check for array declaration
    if (peek(parser) == '[') {
        printf("Found array declaration\n");
        advance(parser);
        skip_whitespace(parser);
        
        // Parse array size
        Node* size = parse_expression(parser);
        if (!size) {
            printf("Failed to parse array size\n");
            baa_free_node(decl);
            return NULL;
        }
        
        skip_whitespace(parser);
        if (!expect_char(parser, ']')) {
            printf("Expected closing bracket\n");
            baa_free_node(size);
            baa_free_node(decl);
            return NULL;
        }
        
        baa_add_child(decl, size);
    }
    
    skip_whitespace(parser);
    
    // Check for initialization
    if (peek(parser) == '=') {
        printf("Found initialization\n");
        advance(parser);
        skip_whitespace(parser);
        
        Node* init = parse_expression(parser);
        if (!init) {
            printf("Failed to parse initializer\n");
            baa_free_node(decl);
            return NULL;
        }
        
        baa_add_child(decl, init);
    }
    
    skip_whitespace(parser);
    if (!expect_char(parser, '.')) {
        printf("Expected statement terminator\n");
        baa_free_node(decl);
        return NULL;
    }
    
    printf("Successfully parsed declaration\n");
    return decl;
}

static Node* parse_expression(Parser* parser) {
    skip_whitespace(parser);
    
    // Parse primary expression
    Node* expr = NULL;
    
    // Try to parse number
    if (is_digit(peek(parser))) {
        size_t start = parser->position;
        size_t len = 0;
        
        while (peek(parser) && is_digit(peek(parser))) {
            advance(parser);
            len++;
        }
        
        char* number = baa_malloc(len + 1);
        if (!number) {
            set_error(parser, "Failed to allocate memory for number");
            return NULL;
        }
        
        strncpy(number, parser->source + start, len);
        number[len] = '\0';
        
        expr = baa_create_node(NODE_NUMBER, number);
        baa_free(number);
        
        if (!expr) {
            set_error(parser, "Failed to create number node");
            return NULL;
        }
        
        return expr;
    }
    
    // Try to parse string
    if (peek(parser) == '"') {
        advance(parser); // Skip opening quote
        
        size_t start = parser->position;
        size_t len = 0;
        
        while (peek(parser) && peek(parser) != '"') {
            advance(parser);
            len++;
        }
        
        if (!peek(parser)) {
            set_error(parser, "Unterminated string literal");
            return NULL;
        }
        
        char* string = baa_malloc(len + 1);
        if (!string) {
            set_error(parser, "Failed to allocate memory for string");
            return NULL;
        }
        
        strncpy(string, parser->source + start, len);
        string[len] = '\0';
        
        advance(parser); // Skip closing quote
        
        expr = baa_create_node(NODE_STRING, string);
        baa_free(string);
        
        if (!expr) {
            set_error(parser, "Failed to create string node");
            return NULL;
        }
        
        return expr;
    }
    
    // Try to parse identifier
    char* identifier = parse_identifier(parser);
    if (identifier) {
        expr = baa_create_node(NODE_IDENTIFIER, identifier);
        baa_free(identifier);
        
        if (!expr) {
            set_error(parser, "Failed to create identifier node");
            return NULL;
        }
        
        return expr;
    }
    
    return NULL;
}

static Node* parse_function(Parser* parser) {
    skip_whitespace(parser);
    
    // Parse function name
    char* name = parse_identifier(parser);
    if (!name) {
        set_error(parser, "Expected function name");
        return NULL;
    }
    
    Node* func = baa_create_node(NODE_FUNCTION, name);
    baa_free(name);
    
    if (!func) {
        set_error(parser, "Failed to create function node");
        return NULL;
    }
    
    skip_whitespace(parser);
    
    // Parse parameter list
    if (!expect_char(parser, '(')) {
        baa_free_node(func);
        return NULL;
    }
    
    skip_whitespace(parser);
    
    // Parse parameters
    while (!is_eof(parser) && peek(parser) != ')') {
        Node* param = parse_declaration(parser);
        if (!param) {
            baa_free_node(func);
            return NULL;
        }
        
        baa_add_child(func, param);
        
        skip_whitespace(parser);
        
        if (peek(parser) == ',') {
            advance(parser);
            skip_whitespace(parser);
        }
    }
    
    if (!expect_char(parser, ')')) {
        baa_free_node(func);
        return NULL;
    }
    
    skip_whitespace(parser);
    
    // Parse function body
    if (!expect_char(parser, '{')) {
        baa_free_node(func);
        return NULL;
    }
    
    // For now, just skip the function body
    int brace_count = 1;
    while (!is_eof(parser) && brace_count > 0) {
        char c = peek(parser);
        if (c == '{') brace_count++;
        else if (c == '}') brace_count--;
        advance(parser);
    }
    
    if (brace_count > 0) {
        set_error(parser, "Unterminated function body");
        baa_free_node(func);
        return NULL;
    }
    
    return func;
}

// Import directive parsing
static Node* parse_import_directive(Parser* parser) {
    if (!match_keyword(parser, "#تضمين")) {
        return NULL;
    }
    
    skip_whitespace(parser);
    
    // Check for < or "
    bool is_system = false;
    char quote = peek(parser);
    if (quote == '<') {
        is_system = true;
    } else if (quote != '"') {
        set_error(parser, "Expected '<' or '\"' after #تضمين");
        return NULL;
    }
    advance(parser);
    
    // Parse path
    size_t start = parser->position;
    size_t len = 0;
    char end_quote = is_system ? '>' : '"';
    
    while (peek(parser) && peek(parser) != end_quote) {
        advance(parser);
        len++;
    }
    
    if (!expect_char(parser, end_quote)) {
        return NULL;
    }
    
    char* path = baa_malloc(len + 1);
    if (!path) {
        set_error(parser, "Failed to allocate memory for import path");
        return NULL;
    }
    
    strncpy(path, parser->source + start, len);
    path[len] = '\0';
    
    Node* import = baa_create_node(NODE_IMPORT, path);
    baa_free(path);
    
    if (!import) {
        set_error(parser, "Failed to create import node");
        return NULL;
    }
    
    // Add system flag if needed
    if (is_system) {
        import->flags |= NODE_FLAG_SYSTEM_IMPORT;
    }
    
    skip_whitespace(parser);
    if (!expect_char(parser, '.')) {
        baa_free_node(import);
        return NULL;
    }
    
    return import;
}

// Main parsing functions
Node* baa_parse_program(Parser* parser) {
    printf("Starting program parse\n");
    Node* program = baa_create_node(NODE_PROGRAM, "program");
    if (!program) return NULL;

    while (!is_eof(parser)) {
        skip_whitespace(parser);
        if (is_eof(parser)) break;

        printf("Current character: %c (0x%02x)\n", peek(parser), (unsigned char)peek(parser));
        
        // Skip comments
        if (peek(parser) == '#') {
            printf("Found comment\n");
            if (peek_next(parser) == 'ت') {
                printf("Found import directive\n");
                Node* import = parse_import_directive(parser);
                if (!import) {
                    baa_free_node(program);
                    return NULL;
                }
                baa_add_child(program, import);
                continue;
            }
            
            // Skip regular comments
            while (!is_eof(parser) && peek(parser) != '\n') {
                advance(parser);
            }
            continue;
        }

        // Parse declarations and functions
        Node* node = parse_declaration(parser);
        if (!node) {
            printf("Failed to parse declaration\n");
            baa_free_node(program);
            return NULL;
        }
        baa_add_child(program, node);
    }

    return program;
}

bool baa_parser_had_error(const Parser* parser) {
    return parser->had_error;
}

const char* baa_parser_error_message(const Parser* parser) {
    return parser->error_msg;
}

Node* baa_parse_statement(Parser* parser) {
    skip_whitespace(parser);
    
    // Return statement
    if (match_keyword(parser, "إرجع")) {
        Node* stmt = baa_create_node(NODE_RETURN, NULL);
        if (!stmt) {
            set_error(parser, "Failed to create return statement node");
            return NULL;
        }
        
        skip_whitespace(parser);
        
        // Parse return value if present
        if (peek(parser) != '.') {
            Node* value = parse_expression(parser);
            if (!value) {
                if (!parser->had_error) {
                    set_error(parser, "Expected return value");
                }
                baa_free_node(stmt);
                return NULL;
            }
            baa_add_child(stmt, value);
        }
        
        skip_whitespace(parser);
        if (!expect_char(parser, '.')) {
            baa_free_node(stmt);
            return NULL;
        }
        
        return stmt;
    }
    
    // If statement
    if (match_keyword(parser, "إذا")) {
        Node* stmt = baa_create_node(NODE_IF, NULL);
        if (!stmt) {
            set_error(parser, "Failed to create if statement node");
            return NULL;
        }
        
        skip_whitespace(parser);
        
        // Parse condition
        if (!expect_char(parser, '(')) {
            baa_free_node(stmt);
            return NULL;
        }
        
        Node* condition = parse_expression(parser);
        if (!condition) {
            if (!parser->had_error) {
                set_error(parser, "Expected condition");
            }
            baa_free_node(stmt);
            return NULL;
        }
        baa_add_child(stmt, condition);
        
        skip_whitespace(parser);
        if (!expect_char(parser, ')')) {
            baa_free_node(stmt);
            return NULL;
        }
        
        skip_whitespace(parser);
        
        // Parse if body
        if (!expect_char(parser, '{')) {
            baa_free_node(stmt);
            return NULL;
        }
        
        Node* body = baa_create_node(NODE_BLOCK, NULL);
        if (!body) {
            set_error(parser, "Failed to create block node");
            baa_free_node(stmt);
            return NULL;
        }
        baa_add_child(stmt, body);
        
        // Parse statements in body
        while (peek(parser) && peek(parser) != '}') {
            skip_whitespace(parser);
            skip_comment(parser);
            
            Node* body_stmt = baa_parse_statement(parser);
            if (!body_stmt) {
                if (!parser->had_error) {
                    set_error(parser, "Expected statement");
                }
                baa_free_node(stmt);
                return NULL;
            }
            baa_add_child(body, body_stmt);
            
            skip_whitespace(parser);
            skip_comment(parser);
        }
        
        if (!expect_char(parser, '}')) {
            baa_free_node(stmt);
            return NULL;
        }
        
        skip_whitespace(parser);
        
        // Parse else clause if present
        if (match_keyword(parser, "وإلا")) {
            skip_whitespace(parser);
            
            if (!expect_char(parser, '{')) {
                baa_free_node(stmt);
                return NULL;
            }
            
            Node* else_body = baa_create_node(NODE_BLOCK, NULL);
            if (!else_body) {
                set_error(parser, "Failed to create else block node");
                baa_free_node(stmt);
                return NULL;
            }
            baa_add_child(stmt, else_body);
            
            // Parse statements in else body
            while (peek(parser) && peek(parser) != '}') {
                skip_whitespace(parser);
                skip_comment(parser);
                
                Node* else_stmt = baa_parse_statement(parser);
                if (!else_stmt) {
                    if (!parser->had_error) {
                        set_error(parser, "Expected statement");
                    }
                    baa_free_node(stmt);
                    return NULL;
                }
                baa_add_child(else_body, else_stmt);
                
                skip_whitespace(parser);
                skip_comment(parser);
            }
            
            if (!expect_char(parser, '}')) {
                baa_free_node(stmt);
                return NULL;
            }
        }
        
        return stmt;
    }
    
    // While loop
    if (match_keyword(parser, "طالما")) {
        Node* stmt = baa_create_node(NODE_WHILE, NULL);
        if (!stmt) {
            set_error(parser, "Failed to create while statement node");
            return NULL;
        }
        
        skip_whitespace(parser);
        
        // Parse condition
        if (!expect_char(parser, '(')) {
            baa_free_node(stmt);
            return NULL;
        }
        
        Node* condition = parse_expression(parser);
        if (!condition) {
            if (!parser->had_error) {
                set_error(parser, "Expected condition");
            }
            baa_free_node(stmt);
            return NULL;
        }
        baa_add_child(stmt, condition);
        
        skip_whitespace(parser);
        if (!expect_char(parser, ')')) {
            baa_free_node(stmt);
            return NULL;
        }
        
        skip_whitespace(parser);
        
        // Parse body
        if (!expect_char(parser, '{')) {
            baa_free_node(stmt);
            return NULL;
        }
        
        Node* body = baa_create_node(NODE_BLOCK, NULL);
        if (!body) {
            set_error(parser, "Failed to create block node");
            baa_free_node(stmt);
            return NULL;
        }
        baa_add_child(stmt, body);
        
        // Parse statements in body
        while (peek(parser) && peek(parser) != '}') {
            skip_whitespace(parser);
            skip_comment(parser);
            
            Node* body_stmt = baa_parse_statement(parser);
            if (!body_stmt) {
                if (!parser->had_error) {
                    set_error(parser, "Expected statement");
                }
                baa_free_node(stmt);
                return NULL;
            }
            baa_add_child(body, body_stmt);
            
            skip_whitespace(parser);
            skip_comment(parser);
        }
        
        if (!expect_char(parser, '}')) {
            baa_free_node(stmt);
            return NULL;
        }
        
        return stmt;
    }
    
    // Declaration statement
    Node* decl = parse_declaration(parser);
    if (decl) {
        return decl;
    }
    
    // Expression statement
    Node* expr = parse_expression(parser);
    if (expr) {
        Node* stmt = baa_create_node(NODE_EXPR_STMT, NULL);
        if (!stmt) {
            set_error(parser, "Failed to create expression statement node");
            baa_free_node(expr);
            return NULL;
        }
        baa_add_child(stmt, expr);
        
        skip_whitespace(parser);
        if (!expect_char(parser, '.')) {
            baa_free_node(stmt);
            return NULL;
        }
        
        return stmt;
    }
    
    if (!parser->had_error) {
        set_error(parser, "Expected statement");
    }
    return NULL;
}

// Expression parsing
static Node* parse_primary_expression(Parser* parser) {
    skip_whitespace(parser);
    
    // Parse number
    if (is_digit(peek(parser))) {
        size_t start = parser->position;
        size_t len = 0;
        
        while (peek(parser) && is_digit(peek(parser))) {
            advance(parser);
            len++;
        }
        
        char* number = baa_malloc(len + 1);
        if (!number) {
            set_error(parser, "Failed to allocate memory for number");
            return NULL;
        }
        
        strncpy(number, parser->source + start, len);
        number[len] = '\0';
        
        Node* node = baa_create_node(NODE_NUMBER, number);
        baa_free(number);
        
        if (!node) {
            set_error(parser, "Failed to create number node");
            return NULL;
        }
        
        return node;
    }
    
    // Parse string literal
    if (peek(parser) == '"') {
        advance(parser); // Skip opening quote
        
        size_t start = parser->position;
        size_t len = 0;
        
        while (peek(parser) && peek(parser) != '"') {
            advance(parser);
            len++;
        }
        
        if (!peek(parser)) {
            set_error(parser, "Unterminated string literal");
            return NULL;
        }
        
        char* string = baa_malloc(len + 1);
        if (!string) {
            set_error(parser, "Failed to allocate memory for string");
            return NULL;
        }
        
        strncpy(string, parser->source + start, len);
        string[len] = '\0';
        
        advance(parser); // Skip closing quote
        
        Node* node = baa_create_node(NODE_STRING, string);
        baa_free(string);
        
        if (!node) {
            set_error(parser, "Failed to create string node");
            return NULL;
        }
        
        return node;
    }
    
    // Parse identifier
    char* identifier = parse_identifier(parser);
    if (identifier) {
        Node* node = baa_create_node(NODE_IDENTIFIER, identifier);
        baa_free(identifier);
        
        if (!node) {
            set_error(parser, "Failed to create identifier node");
            return NULL;
        }
        
        return node;
    }
    
    return NULL;
}

static Node* parse_unary_expression(Parser* parser) {
    skip_whitespace(parser);
    
    // Unary operators
    if (peek(parser) == '-' || peek(parser) == '!' || peek(parser) == '~') {
        char op[2] = {peek(parser), '\0'};
        advance(parser);
        
        Node* operand = parse_unary_expression(parser);
        if (!operand) {
            return NULL;
        }
        
        Node* unary = baa_create_node(NODE_UNARY_OP, op);
        if (!unary) {
            set_error(parser, "Failed to create unary operator node");
            baa_free_node(operand);
            return NULL;
        }
        
        baa_add_child(unary, operand);
        return unary;
    }
    
    return parse_primary_expression(parser);
}

static Node* parse_binary_expression(Parser* parser, int min_precedence) {
    Node* left = parse_unary_expression(parser);
    if (!left) {
        return NULL;
    }
    
    while (true) {
        skip_whitespace(parser);
        
        if (!is_operator(peek(parser))) {
            break;
        }
        
        // Parse operator
        size_t start = parser->position;
        size_t len = 0;
        
        while (is_operator(peek(parser))) {
            advance(parser);
            len++;
        }
        
        char* op = baa_malloc(len + 1);
        if (!op) {
            set_error(parser, "Failed to allocate memory for operator");
            baa_free_node(left);
            return NULL;
        }
        
        strncpy(op, parser->source + start, len);
        op[len] = '\0';
        
        int precedence = get_operator_precedence(op);
        if (precedence == 0 || precedence < min_precedence) {
            parser->position = start; // Backtrack
            baa_free(op);
            break;
        }
        
        skip_whitespace(parser);
        
        Node* right = parse_binary_expression(parser, precedence + 1);
        if (!right) {
            baa_free_node(left);
            baa_free(op);
            return NULL;
        }
        
        Node* binary = baa_create_node(NODE_BINARY_OP, op);
        baa_free(op);
        
        if (!binary) {
            set_error(parser, "Failed to create binary operator node");
            baa_free_node(left);
            baa_free_node(right);
            return NULL;
        }
        
        baa_add_child(binary, left);
        baa_add_child(binary, right);
        left = binary;
    }
    
    return left;
}

Node* baa_parse_expression(Parser* parser) {
    return parse_binary_expression(parser, 1);
}
