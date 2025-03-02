#ifndef BAA_SCOPE_H
#define BAA_SCOPE_H

#include "baa/ast/ast.h"
#include "baa/types/types.h"
#include <stdbool.h>
#include <wchar.h>

// Symbol kinds
typedef enum {
    BAA_SYMBOL_VARIABLE,    // Variable declaration
    BAA_SYMBOL_FUNCTION,    // Function declaration
    BAA_SYMBOL_PARAMETER,   // Function parameter
    BAA_SYMBOL_TYPE,       // Type declaration
    BAA_SYMBOL_CONSTANT    // Constant declaration
} BaaSymbolKind;

// Symbol information
typedef struct BaaSymbol {
    BaaSymbolKind kind;           // Kind of symbol
    const wchar_t* name;          // Symbol name
    size_t name_length;           // Name length
    BaaType* type;               // Symbol type
    BaaNode* declaration;        // AST node where symbol is declared
    struct BaaSymbol* next;      // Next symbol in hash chain
    bool is_mutable;             // Whether symbol can be modified
    bool is_initialized;         // Whether symbol has been initialized
} BaaSymbol;

// Scope structure
typedef struct BaaScope {
    struct BaaScope* parent;     // Parent scope
    BaaSymbol** symbols;         // Hash table of symbols
    size_t capacity;            // Hash table capacity
    size_t count;               // Number of symbols in scope
    bool is_function_scope;     // Whether this is a function's scope
    BaaType* return_type;       // Return type for function scopes
} BaaScope;

// Scope management
BaaScope* baa_create_scope(BaaScope* parent);
void baa_enter_scope(BaaScope* scope);
void baa_exit_scope(void);
void baa_free_scope(BaaScope* scope);

// Symbol management
BaaSymbol* baa_declare_symbol(BaaScope* scope, const wchar_t* name, size_t name_length,
                            BaaSymbolKind kind, BaaType* type, BaaNode* declaration);
BaaSymbol* baa_lookup_symbol(BaaScope* scope, const wchar_t* name, size_t name_length);
BaaSymbol* baa_lookup_symbol_in_current_scope(BaaScope* scope, const wchar_t* name, size_t name_length);

// Scope validation
bool baa_validate_symbol_declaration(BaaScope* scope, const wchar_t* name, size_t name_length);
bool baa_validate_symbol_usage(BaaScope* scope, const wchar_t* name, size_t name_length);

// Error handling
const wchar_t* baa_get_scope_error(void);
void baa_clear_scope_error(void);

#endif /* BAA_SCOPE_H */
