#ifndef BAA_CONTROL_FLOW_ERRORS_H
#define BAA_CONTROL_FLOW_ERRORS_H

#include <wchar.h>

typedef enum {
    BAA_CTRL_SUCCESS = 0,
    BAA_CTRL_NULL_CONDITION,     // Condition expression is NULL
    BAA_CTRL_NULL_BODY,          // Statement body is NULL
    BAA_CTRL_INVALID_TYPE,       // Invalid expression type in condition
    BAA_CTRL_MEMORY_ERROR,       // Memory allocation failed
    BAA_CTRL_INVALID_RETURN,     // Invalid return type
    BAA_CTRL_NESTED_LIMIT        // Exceeded maximum nesting level
} BaaControlFlowError;

// Error handling functions
const wchar_t* baa_control_flow_error_message(BaaControlFlowError error);
void baa_set_control_flow_error(BaaControlFlowError error);
BaaControlFlowError baa_get_last_control_flow_error(void);

#endif /* BAA_CONTROL_FLOW_ERRORS_H */
