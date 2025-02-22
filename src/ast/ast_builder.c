#include "baa/ast.h"
#include "baa/lexer.h"
#include "baa/utils.h"
#include "baa_errors.h"
#include <stdlib.h>
#include <string.h>

// Error recovery state
typedef struct {
    bool in_recovery;
    size_t error_count;
    size_t last_error_line;
    TokenType sync_tokens[16];
    size_t sync_token_count;
} ErrorRecovery;

// AST Builder state
typedef struct {
    Lexer* lexer;
    Token current;
    Token previous;
    ErrorRecovery error_recovery;
    bool had_error;
    bool panic_mode;
} ASTBuilder;

// Initialize AST builder
ASTBuilder* baa_ast_builder_init(Lexer* lexer) {
    ASTBuilder* builder = (ASTBuilder*)malloc(sizeof(ASTBuilder));
    builder->lexer = lexer;
    builder->had_error = false;
    builder->panic_mode = false;
    
    // Initialize error recovery
    builder->error_recovery.in_recovery = false;
    builder->error_recovery.error_count = 0;
    builder->error_recovery.last_error_line = 0;
    builder->error_recovery.sync_token_count = 0;
    
    // Add synchronization tokens
    TokenType sync_tokens[] = {
        TOKEN_FUNCTION,
        TOKEN_IF,
        TOKEN_WHILE,
        TOKEN_FOR,
        TOKEN_RETURN
    };
    
    for (size_t i = 0; i < sizeof(sync_tokens) / sizeof(TokenType); i++) {
        builder->error_recovery.sync_tokens[i] = sync_tokens[i];
        builder->error_recovery.sync_token_count++;
    }
    
    return builder;
}

// Error handling and recovery
static void report_error(ASTBuilder* builder, const wchar_t* message) {
    if (builder->panic_mode) return;
    
    builder->had_error = true;
    builder->panic_mode = true;
    builder->error_recovery.error_count++;
    
    // Print error with Arabic support
    fwprintf(stderr, L"خطأ في السطر %d، العمود %d: %ls\n",
             builder->current.line,
             builder->current.column,
             message);
}

// Synchronize after error
static void synchronize(ASTBuilder* builder) {
    builder->panic_mode = false;
    
    while (builder->current.type != TOKEN_EOF) {
        if (builder->previous.type == TOKEN_BREAK) return;
        
        for (size_t i = 0; i < builder->error_recovery.sync_token_count; i++) {
            if (builder->current.type == builder->error_recovery.sync_tokens[i]) {
                return;
            }
        }
        
        builder->current = baa_lexer_next_token(builder->lexer);
    }
}

// Consume current token and advance
static Token advance(ASTBuilder* builder) {
    builder->previous = builder->current;
    
    for (;;) {
        builder->current = baa_lexer_next_token(builder->lexer);
        if (!builder->had_error) break;
        
        report_error(builder, L"توكن غير صالح");
    }
    
    return builder->previous;
}

// Check if current token matches expected type
static bool check(ASTBuilder* builder, TokenType type) {
    return builder->current.type == type;
}

// Consume token of expected type or report error
static bool consume(ASTBuilder* builder, TokenType type, const wchar_t* message) {
    if (check(builder, type)) {
        advance(builder);
        return true;
    }
    
    report_error(builder, message);
    return false;
}

// Create AST node from current token
static Node* create_node_from_token(ASTBuilder* builder, NodeType node_type) {
    Token token = builder->previous;
    Node* node = baa_create_node(node_type, token.lexeme);
    baa_set_node_location(node, token.line, token.column, NULL);
    return node;
}

// Parse function declaration
static Node* parse_function(ASTBuilder* builder) {
    // Expect function keyword
    if (!consume(builder, TOKEN_FUNCTION, L"توقع 'دالة'")) {
        return NULL;
    }
    
    Node* func = create_node_from_token(builder, NODE_FUNCTION);
    
    // Parse function name
    if (!consume(builder, TOKEN_IDENTIFIER, L"توقع معرف الدالة")) {
        return func;
    }
    
    // Add function implementation here
    // ...
    
    return func;
}

// Free AST builder resources
void baa_ast_builder_free(ASTBuilder* builder) {
    if (builder) {
        free(builder);
    }
}
