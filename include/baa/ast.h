#ifndef BAA_AST_H
#define BAA_AST_H

#include <stddef.h>

// Basic AST node types
typedef enum {
    NODE_PROGRAM,
    NODE_FUNCTION,
    NODE_STATEMENT,
    NODE_EXPRESSION,
    NODE_IDENTIFIER,
    NODE_NUMBER,
    NODE_STRING
} NodeType;

// Forward declaration of Node structure
typedef struct Node Node;

// AST node structure
struct Node {
    NodeType type;
    char *value;
    Node **children;
    size_t children_count;
};

// AST functions
Node *baa_create_node(NodeType type, const char *value);
void baa_add_child(Node *parent, Node *child);
void baa_free_node(Node *node);

#endif /* BAA_AST_H */
