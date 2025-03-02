#include "baa/control_flow/control_flow_errors.h"
#include <wchar.h>

static BaaControlFlowError last_error = BAA_CTRL_SUCCESS;

const wchar_t* baa_control_flow_error_message(BaaControlFlowError error) {
    switch (error) {
        case BAA_CTRL_SUCCESS:
            return L"نجاح"; // Success
        case BAA_CTRL_NULL_CONDITION:
            return L"شرط فارغ"; // Empty condition
        case BAA_CTRL_NULL_BODY:
            return L"جسم العبارة فارغ"; // Empty statement body
        case BAA_CTRL_INVALID_TYPE:
            return L"نوع تعبير غير صالح"; // Invalid expression type
        case BAA_CTRL_MEMORY_ERROR:
            return L"خطأ في الذاكرة"; // Memory error
        case BAA_CTRL_INVALID_RETURN:
            return L"قيمة إرجاع غير صالحة"; // Invalid return value
        case BAA_CTRL_NESTED_LIMIT:
            return L"تجاوز الحد الأقصى للتداخل"; // Exceeded nesting limit
        default:
            return L"خطأ غير معروف"; // Unknown error
    }
}

void baa_set_control_flow_error(BaaControlFlowError error) {
    last_error = error;
}

BaaControlFlowError baa_get_last_control_flow_error(void) {
    return last_error;
}
