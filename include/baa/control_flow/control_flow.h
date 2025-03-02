#ifndef BAA_CONTROL_FLOW_H
#define BAA_CONTROL_FLOW_H
#include "baa/ast/statements.h"
#include <stdbool.h>
#include <wchar.h>

// Control flow structure
typedef struct
{
    const wchar_t **break_labels;
    const wchar_t **continue_labels;
    size_t break_count;
    size_t continue_count;
    size_t break_capacity;
    size_t continue_capacity;
} BaaControlFlow;

// Control flow management
BaaControlFlow *baa_create_control_flow(void);
void baa_free_control_flow(BaaControlFlow *cf);

// Label management
bool baa_add_break_label(BaaControlFlow *cf, const wchar_t *label);
bool baa_add_continue_label(BaaControlFlow *cf, const wchar_t *label);
bool baa_can_break(BaaControlFlow *cf, const wchar_t *label);
bool baa_can_continue(BaaControlFlow *cf, const wchar_t *label);

// Block management
bool baa_block_add_statement(BaaBlock *block, BaaStmt *statement);
void baa_free_block(BaaBlock *block);

// Control flow analysis functions
bool baa_analyze_control_flow(BaaBlock *block);
bool baa_validate_function_control_flow(BaaBlock *function_body, BaaType *return_type);

// Path analysis
bool baa_has_return_path(BaaBlock *block);
bool baa_has_unreachable_code(BaaBlock *block);

// Loop analysis
bool baa_validate_loop_condition(BaaExpr *condition);
bool baa_has_infinite_loop(BaaWhileStmt *while_stmt);

// Error reporting
const wchar_t *baa_get_control_flow_error(void);
void baa_clear_control_flow_error(void);

#endif /* BAA_CONTROL_FLOW_H */
