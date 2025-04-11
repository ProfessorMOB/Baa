#ifndef BAA_FLOW_ANALYSIS_H
#define BAA_FLOW_ANALYSIS_H

#include "baa/ast/statements.h"       // Include AST statement definitions
#include "baa/ast/expressions.h"      // Include AST expression definitions
#include "baa/types/types.h"          // Include type definitions
#include "baa/analysis/flow_errors.h" // Include specific error types
#include <stdbool.h>
#include <wchar.h>
#include <stddef.h>

// --- Moved from include/baa/control_flow.h (Analysis Parts) ---

// Structure to manage break/continue context during analysis
typedef struct BaaFlowContext
{
    struct BaaFlowContext *parent; // Parent context (for nesting)
    bool can_break;                // Is break allowed in this context?
    bool can_continue;             // Is continue allowed in this context?
    // Potentially add target labels if needed for labeled break/continue
} BaaFlowContext;

// Analysis functions (These were just examples, implement as needed)
bool baa_analyze_control_flow(BaaBlock *block, BaaFlowContext *context, BaaType *expected_return_type);
bool baa_validate_function_control_flow(BaaBlock *function_body, BaaType *return_type);

// Path analysis (Example function declarations)
bool baa_block_has_return_path(BaaBlock *block);
bool baa_block_has_unreachable_code(BaaBlock *block);

// Loop analysis (Example function declarations)
bool baa_loop_condition_is_valid(BaaExpr *condition);
bool baa_while_is_infinite_loop(BaaWhileStmt *while_stmt);

// --- End of moved code ---

#endif /* BAA_FLOW_ANALYSIS_H */
