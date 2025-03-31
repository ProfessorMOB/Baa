#include "baa/analysis/flow_errors.h"
#include <wchar.h>

// --- Moved from src/control_flow/control_flow_errors.c ---
static BaaFlowAnalysisError last_error = BAA_FLOW_SUCCESS;

const wchar_t *baa_flow_analysis_error_message(BaaFlowAnalysisError error)
{
    switch (error)
    {
    case BAA_FLOW_SUCCESS:
        return L"نجاح"; // Success
    case BAA_FLOW_NULL_CONDITION:
        return L"شرط فارغ في تحليل التدفق"; // Empty condition in flow analysis
    case BAA_FLOW_NULL_BODY:
        return L"جسم عبارة فارغ في تحليل التدفق"; // Empty statement body in flow analysis
    case BAA_FLOW_INVALID_TYPE:
        return L"نوع تعبير غير صالح للتحكم بالتدفق"; // Invalid expression type for flow control
    case BAA_FLOW_MEMORY_ERROR:
        return L"خطأ في الذاكرة أثناء تحليل التدفق"; // Memory error during flow analysis
    case BAA_FLOW_INVALID_RETURN:
        return L"قيمة إرجاع غير صالحة في هذا السياق"; // Invalid return value in this context
    case BAA_FLOW_NESTED_LIMIT:
        return L"تجاوز الحد الأقصى لتداخل كتل التدفق"; // Exceeded flow block nesting limit
    case BAA_FLOW_UNREACHABLE_CODE:
        return L"تم اكتشاف كود لا يمكن الوصول إليه"; // Unreachable code detected
    case BAA_FLOW_MISSING_RETURN:
        return L"مسار تحكم محتمل بدون عبارة إرجاع"; // Possible control path without return statement
    case BAA_FLOW_INVALID_BREAK:
        return L"عبارة 'توقف' غير صالحة خارج حلقة أو تبديل"; // Invalid 'break' outside loop or switch
    case BAA_FLOW_INVALID_CONTINUE:
        return L"عبارة 'استمر' غير صالحة خارج حلقة"; // Invalid 'continue' outside loop
    default:
        return L"خطأ غير معروف في تحليل التدفق"; // Unknown flow analysis error
    }
}

void baa_set_flow_analysis_error(BaaFlowAnalysisError error)
{
    last_error = error;
}

BaaFlowAnalysisError baa_get_last_flow_analysis_error(void)
{
    return last_error;
}
// --- End of moved code ---
