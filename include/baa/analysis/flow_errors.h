#ifndef BAA_FLOW_ERRORS_H
#define BAA_FLOW_ERRORS_H

#include <wchar.h>

// --- Moved from include/baa/control_flow_errors.h ---
typedef enum {
    BAA_FLOW_SUCCESS = 0,           // Renamed from BAA_CTRL_SUCCESS
    BAA_FLOW_NULL_CONDITION,        // Renamed from BAA_CTRL_NULL_CONDITION
    BAA_FLOW_NULL_BODY,             // Renamed from BAA_CTRL_NULL_BODY
    BAA_FLOW_INVALID_TYPE,          // Renamed from BAA_CTRL_INVALID_TYPE
    BAA_FLOW_MEMORY_ERROR,          // Renamed from BAA_CTRL_MEMORY_ERROR
    BAA_FLOW_INVALID_RETURN,        // Renamed from BAA_CTRL_INVALID_RETURN
    BAA_FLOW_NESTED_LIMIT,          // Renamed from BAA_CTRL_NESTED_LIMIT
    BAA_FLOW_UNREACHABLE_CODE,      // New potential error
    BAA_FLOW_MISSING_RETURN,        // New potential error
    BAA_FLOW_INVALID_BREAK,         // New potential error
    BAA_FLOW_INVALID_CONTINUE       // New potential error
} BaaFlowAnalysisError;

// Error handling functions
const wchar_t* baa_flow_analysis_error_message(BaaFlowAnalysisError error);
void baa_set_flow_analysis_error(BaaFlowAnalysisError error);
BaaFlowAnalysisError baa_get_last_flow_analysis_error(void);
// --- End of moved code ---

#endif /* BAA_FLOW_ERRORS_H */
