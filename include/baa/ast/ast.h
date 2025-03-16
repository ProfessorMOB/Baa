#ifndef BAA_AST_H
#define BAA_AST_H

#include <stddef.h>
#include <stdbool.h>
#include <wchar.h>

// Forward declarations
typedef struct BaaProgram BaaProgram;
typedef struct BaaFunction BaaFunction;
typedef struct BaaNode BaaNode;
typedef struct BaaParameter BaaParameter;
typedef struct BaaType BaaType;
typedef struct BaaExpr BaaExpr;
typedef struct BaaBlock BaaBlock;

// Node types
typedef enum
{
    BAA_NODE_PROGRAM,  // Program node
    BAA_NODE_FUNCTION, // Function node
    BAA_NODE_STMT,     // Statement node
    BAA_NODE_EXPR      // Expression node
} BaaNodeKind;

// Node structure
struct BaaNode
{
    BaaNodeKind kind; // Type of node
    void *data;       // Points to the struct for this specific node type
    BaaNode *parent;  // Parent node, can be NULL for root nodes
    size_t line;      // Line where this node appears in source
    size_t column;    // Column where this node appears in source
};

// Function parameter structure
struct BaaParameter
{
    wchar_t *name;          // Parameter name
    size_t name_length;     // Parameter name length
    BaaType *type;          // Parameter type
    bool is_mutable;        // Whether parameter can be modified
    bool is_optional;       // Whether parameter is optional
    BaaExpr *default_value; // Optional default value (NULL if none)
    bool is_rest;           // Whether this is a rest parameter (...args)
};

// Function structure
struct BaaFunction
{
    wchar_t *name;             // Function name
    size_t name_length;        // Function name length
    BaaType *return_type;      // Return type
    BaaParameter *parameters;  // Array of parameters
    size_t parameter_count;    // Number of parameters
    size_t parameter_capacity; // Capacity of parameters array
    BaaBlock *body;            // Function body
    bool is_variadic;          // Whether function accepts variable arguments
    bool is_extern;            // Whether function is externally defined
    bool is_method;            // Whether function is a method on a type
    wchar_t *module_name;      // Module the function belongs to (NULL for current module)
    BaaNode *ast_node;         // Reference to AST node, if any
    wchar_t *documentation;    // Documentation comment for the function
};

// Program structure - the root of the AST
struct BaaProgram
{
    BaaFunction **functions;  // Array of functions
    size_t function_count;    // Number of functions
    size_t function_capacity; // Capacity of functions array
    BaaNode *ast_node;        // Reference to AST node, if any
};

// Node creation
BaaNode *baa_create_node(BaaNodeKind kind, void *data);
void baa_set_node_location(BaaNode *node, size_t line, size_t column);
void baa_add_child_node(BaaNode *parent, BaaNode *child);

// Program management
BaaProgram *baa_create_program(void);
bool baa_add_function_to_program(BaaProgram *program, BaaFunction *function);

// Function management
BaaFunction *baa_create_function(const wchar_t *name, size_t name_length);
bool baa_add_parameter_to_function(BaaFunction *function, BaaParameter *parameter);
BaaParameter *baa_create_parameter(const wchar_t *name, size_t name_length, BaaType *type, bool is_mutable);
BaaParameter *baa_create_optional_parameter(const wchar_t *name, size_t name_length, BaaType *type, bool is_mutable, BaaExpr *default_value);
BaaParameter *baa_create_rest_parameter(const wchar_t *name, size_t name_length, BaaType *element_type, bool is_mutable);
bool baa_validate_function_signature(BaaFunction *function);

// Memory management
void baa_free_program(BaaProgram *program);
void baa_free_function(BaaFunction *function);
void baa_free_node(BaaNode *node);

#endif /* BAA_AST_H */
