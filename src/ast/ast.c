#include "baa/ast.h"
#include "baa/utils.h"
#include "baa_errors.h"
#include <stdlib.h>
#include <string.h>

// Source location tracking
typedef struct {
    size_t line;
    size_t column;
    const wchar_t* file;
} SourceLocation;

// Node attributes
typedef struct {
    SourceLocation location;
    bool is_constant;
    bool is_static;
    bool is_extern;
} NodeAttributes;

// Tree traversal utilities
void baa_visit_node(Node *node, void (*visitor)(Node*, void*), void *data) {
    if (!node || !visitor) {
        return;
    }

    visitor(node, data);
    for (size_t i = 0; i < node->children_count; i++) {
        baa_visit_node(node->children[i], visitor, data);
    }
}

// Source location management
void baa_set_node_location(Node *node, size_t line, size_t column, const wchar_t *file) {
    if (!node || !node->attributes) {
        return;
    }
    
    node->attributes->location.line = line;
    node->attributes->location.column = column;
    node->attributes->location.file = file;
}

// Node attribute management
void baa_set_node_constant(Node *node, bool is_constant) {
    if (!node || !node->attributes) {
        return;
    }
    node->attributes->is_constant = is_constant;
}

void baa_set_node_static(Node *node, bool is_static) {
    if (!node || !node->attributes) {
        return;
    }
    node->attributes->is_static = is_static;
}

void baa_set_node_extern(Node *node, bool is_extern) {
    if (!node || !node->attributes) {
        return;
    }
    node->attributes->is_extern = is_extern;
}

Node *baa_create_node(NodeType type, const wchar_t *value) {
    Node *node = baa_malloc(sizeof(Node));
    if (!node) {
        baa_set_error(BAA_ERROR_MEMORY, L"Failed to allocate memory for AST node");
        return NULL;
    }

    node->type = type;
    node->value = value ? wcsdup(value) : NULL;
    node->children = NULL;
    node->children_count = 0;
    node->children_capacity = 0;
    
    // Initialize attributes
    node->attributes = baa_malloc(sizeof(NodeAttributes));
    if (!node->attributes) {
        baa_free(node);
        baa_set_error(BAA_ERROR_MEMORY, L"Failed to allocate memory for node attributes");
        return NULL;
    }
    memset(node->attributes, 0, sizeof(NodeAttributes));

    return node;
}

void baa_add_child(Node *parent, Node *child) {
    if (!parent || !child) {
        return;
    }

    if (parent->children_count >= parent->children_capacity) {
        size_t new_capacity = parent->children_capacity == 0 ? 4 : parent->children_capacity * 2;
        Node **new_children = baa_realloc(parent->children, new_capacity * sizeof(Node*));

        if (!new_children) {
            baa_set_error(BAA_ERROR_MEMORY, L"Failed to allocate memory for AST node children");
            return;
        }

        parent->children = new_children;
        parent->children_capacity = new_capacity;
    }

    parent->children[parent->children_count++] = child;
}

void baa_free_node(Node *node) {
    if (!node) {
        return;
    }

    if (node->value) {
        free(node->value);
    }

    if (node->attributes) {
        baa_free(node->attributes);
    }

    for (size_t i = 0; i < node->children_count; i++) {
        baa_free_node(node->children[i]);
    }

    if (node->children) {
        baa_free(node->children);
    }

    baa_free(node);
}