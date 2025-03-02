#include "baa/ast/ast.h"
#include "baa/ast/statements.h"
#include "baa/utils/utils.h"
#include <stdlib.h>
#include <string.h>

// Node creation
BaaNode* baa_create_node(BaaNodeKind kind, void* data) {
    BaaNode* node = (BaaNode*)baa_malloc(sizeof(BaaNode));
    if (!node) return NULL;
    
    node->kind = kind;
    node->data = data;
    node->parent = NULL;
    node->line = 0;
    node->column = 0;
    
    return node;
}

void baa_set_node_location(BaaNode* node, size_t line, size_t column) {
    if (!node) return;
    
    node->line = line;
    node->column = column;
}

void baa_add_child_node(BaaNode* parent, BaaNode* child) {
    if (!parent || !child) return;
    
    child->parent = parent;
}

// Program management
BaaProgram* baa_create_program(void) {
    BaaProgram* program = (BaaProgram*)baa_malloc(sizeof(BaaProgram));
    if (!program) return NULL;
    
    program->functions = NULL;
    program->function_count = 0;
    program->function_capacity = 0;
    
    // Create a node for the program
    BaaNode* node = baa_create_node(BAA_NODE_PROGRAM, program);
    if (!node) {
        baa_free(program);
        return NULL;
    }
    
    program->ast_node = node;
    
    return program;
}

bool baa_add_function_to_program(BaaProgram* program, BaaFunction* function) {
    if (!program || !function) return false;
    
    // Resize the functions array if needed
    if (program->function_count >= program->function_capacity) {
        size_t new_capacity = program->function_capacity == 0 ? 8 : program->function_capacity * 2;
        BaaFunction** new_functions = (BaaFunction**)baa_realloc(
            program->functions, 
            new_capacity * sizeof(BaaFunction*)
        );
        
        if (!new_functions) return false;
        
        program->functions = new_functions;
        program->function_capacity = new_capacity;
    }
    
    // Add the function
    program->functions[program->function_count++] = function;
    
    // Link the function's AST node to the program's AST node
    if (function->ast_node && program->ast_node) {
        baa_add_child_node(program->ast_node, function->ast_node);
    }
    
    return true;
}

// Function management
BaaFunction* baa_create_function(const wchar_t* name, size_t name_length) {
    if (!name) return NULL;
    
    BaaFunction* function = (BaaFunction*)baa_malloc(sizeof(BaaFunction));
    if (!function) return NULL;
    
    // Duplicate the name
    wchar_t* name_copy = baa_strndup(name, name_length);
    if (!name_copy) {
        baa_free(function);
        return NULL;
    }
    
    function->name = name_copy;
    function->name_length = name_length;
    function->body = NULL;
    
    // Create a node for the function
    BaaNode* node = baa_create_node(BAA_NODE_FUNCTION, function);
    if (!node) {
        baa_free((void*)name_copy);
        baa_free(function);
        return NULL;
    }
    
    function->ast_node = node;
    
    return function;
}

// Memory management
void baa_free_node(BaaNode* node) {
    if (!node) return;
    
    // Do not free the data here; it's the responsibility of the specific node type's free function
    baa_free(node);
}

void baa_free_function(BaaFunction* function) {
    if (!function) return;
    
    // Free the name
    if (function->name) {
        baa_free((void*)function->name);
    }
    
    // Free the body
    if (function->body) {
        baa_free_block(function->body);
    }
    
    // Free the AST node
    if (function->ast_node) {
        baa_free_node(function->ast_node);
    }
    
    // Free the function itself
    baa_free(function);
}

void baa_free_program(BaaProgram* program) {
    if (!program) return;
    
    // Free all functions
    if (program->functions) {
        for (size_t i = 0; i < program->function_count; i++) {
            baa_free_function(program->functions[i]);
        }
        baa_free(program->functions);
    }
    
    // Free the AST node
    if (program->ast_node) {
        baa_free_node(program->ast_node);
    }
    
    // Free the program itself
    baa_free(program);
}
