#include "baa/ast.h"
#include "baa/utils.h"
#include <stdlib.h>
#include <string.h>

Node *baa_create_node(NodeType type, const char *value) {
    Node *node = baa_malloc(sizeof(Node));
    if (!node) {
        baa_set_error(BAA_ERROR_MEMORY, L"Failed to allocate memory for AST node");
        return NULL;
    }

    node->type = type;
    node->value = value ? strdup(value) : NULL;
    node->children = NULL;
    node->children_count = 0;

    return node;
}

void baa_add_child(Node *parent, Node *child) {
    if (!parent || !child) {
        return;
    }

    size_t new_size = (parent->children_count + 1) * sizeof(Node*);
    Node **new_children = baa_realloc(parent->children, new_size);
    
    if (!new_children) {
        baa_set_error(BAA_ERROR_MEMORY, L"Failed to allocate memory for AST node children");
        return;
    }

    parent->children = new_children;
    parent->children[parent->children_count++] = child;
}

void baa_free_node(Node *node) {
    if (!node) {
        return;
    }

    if (node->value) {
        baa_free(node->value);
    }

    for (size_t i = 0; i < node->children_count; i++) {
        baa_free_node(node->children[i]);
    }

    if (node->children) {
        baa_free(node->children);
    }

    baa_free(node);
}
