#include "baa/ast/literals.h"
#include "baa/utils/utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <wchar.h>

BaaLiteralData* baa_create_bool_literal_data(bool value) {
    BaaLiteralData* data = (BaaLiteralData*)baa_malloc(sizeof(BaaLiteralData));
    if (!data) return NULL;
    
    data->kind = BAA_LITERAL_BOOL;
    data->bool_value = value;
    return data;
}

BaaLiteralData* baa_create_int_literal_data(int value) {
    BaaLiteralData* data = (BaaLiteralData*)baa_malloc(sizeof(BaaLiteralData));
    if (!data) return NULL;
    
    data->kind = BAA_LITERAL_INT;
    data->int_value = value;
    return data;
}

BaaLiteralData* baa_create_float_literal_data(float value) {
    BaaLiteralData* data = (BaaLiteralData*)baa_malloc(sizeof(BaaLiteralData));
    if (!data) return NULL;
    
    data->kind = BAA_LITERAL_FLOAT;
    data->float_value = value;
    return data;
}

BaaLiteralData* baa_create_char_literal_data(wchar_t value) {
    BaaLiteralData* data = (BaaLiteralData*)baa_malloc(sizeof(BaaLiteralData));
    if (!data) return NULL;
    
    data->kind = BAA_LITERAL_CHAR;
    data->char_value = value;
    return data;
}

BaaLiteralData* baa_create_string_literal_data(const wchar_t* value, size_t length) {
    BaaLiteralData* data = (BaaLiteralData*)baa_malloc(sizeof(BaaLiteralData));
    if (!data) return NULL;
    
    data->kind = BAA_LITERAL_STRING;
    
    // Allocate memory for the string
    data->string_value = (wchar_t*)baa_malloc((length + 1) * sizeof(wchar_t));
    if (!data->string_value) {
        free(data);
        return NULL;
    }
    
    // Copy the string
    wcsncpy(data->string_value, value, length);
    data->string_value[length] = L'\0';
    
    return data;
}

BaaLiteralData* baa_create_null_literal_data(void) {
    BaaLiteralData* data = (BaaLiteralData*)baa_malloc(sizeof(BaaLiteralData));
    if (!data) return NULL;
    
    data->kind = BAA_LITERAL_NULL;
    // No value to set for NULL
    
    return data;
}

// Type conversion functions
BaaType* baa_get_type_for_literal(BaaLiteralData* data) {
    if (!data) return NULL;
    
    switch (data->kind) {
        case BAA_LITERAL_BOOL:
            return baa_get_bool_type();
        case BAA_LITERAL_INT:
            return baa_get_int_type();
        case BAA_LITERAL_FLOAT:
            return baa_get_float_type();
        case BAA_LITERAL_CHAR:
            return baa_get_char_type();
        case BAA_LITERAL_STRING:
            return baa_get_string_type();
        case BAA_LITERAL_NULL:
            return baa_get_null_type();
        default:
            return baa_get_error_type();
    }
}

bool baa_literal_can_convert_to(BaaLiteralData* data, BaaType* target_type) {
    if (!data || !target_type) return false;
    
    BaaType* source_type = baa_get_type_for_literal(data);
    if (!source_type) return false;
    
    // Same type is always convertible
    if (baa_types_equal(source_type, target_type)) return true;
    
    switch (data->kind) {
        case BAA_LITERAL_BOOL:
            // Bool can convert to int or float
            return target_type->kind == BAA_TYPE_INT || 
                   target_type->kind == BAA_TYPE_FLOAT;
            
        case BAA_LITERAL_INT:
            // Int can convert to float, bool, or char (if within range)
            return target_type->kind == BAA_TYPE_FLOAT || 
                   target_type->kind == BAA_TYPE_BOOL ||
                   (target_type->kind == BAA_TYPE_CHAR && 
                    data->int_value >= 0 && data->int_value <= 0xFFFF);
            
        case BAA_LITERAL_FLOAT:
            // Float can convert to int (with potential precision loss)
            return target_type->kind == BAA_TYPE_INT;
            
        case BAA_LITERAL_CHAR:
            // Char can convert to int, bool
            return target_type->kind == BAA_TYPE_INT ||
                   target_type->kind == BAA_TYPE_BOOL;
            
        case BAA_LITERAL_STRING:
            // String cannot be implicitly converted to other types
            return false;
            
        case BAA_LITERAL_NULL:
            // Null can be converted to string or any reference type
            // For now, just string
            return target_type->kind == BAA_TYPE_STRING;
            
        default:
            return false;
    }
}

BaaLiteralData* baa_convert_literal(BaaLiteralData* data, BaaType* target_type) {
    if (!data || !target_type) return NULL;
    
    // Check if conversion is allowed
    if (!baa_literal_can_convert_to(data, target_type)) return NULL;
    
    // If already the correct type, return a copy
    BaaType* source_type = baa_get_type_for_literal(data);
    if (baa_types_equal(source_type, target_type)) {
        return baa_copy_literal_data(data);
    }
    
    // Perform the conversion
    switch (target_type->kind) {
        case BAA_TYPE_BOOL:
            // Convert to boolean
            switch (data->kind) {
                case BAA_LITERAL_INT:
                    return baa_create_bool_literal_data(data->int_value != 0);
                case BAA_LITERAL_CHAR:
                    return baa_create_bool_literal_data(data->char_value != L'\0');
                default:
                    return NULL; // Should not happen if can_convert_to returned true
            }
            
        case BAA_TYPE_INT:
            // Convert to integer
            switch (data->kind) {
                case BAA_LITERAL_BOOL:
                    return baa_create_int_literal_data(data->bool_value ? 1 : 0);
                case BAA_LITERAL_FLOAT:
                    return baa_create_int_literal_data((int)data->float_value);
                case BAA_LITERAL_CHAR:
                    return baa_create_int_literal_data((int)data->char_value);
                default:
                    return NULL; // Should not happen if can_convert_to returned true
            }
            
        case BAA_TYPE_FLOAT:
            // Convert to float
            switch (data->kind) {
                case BAA_LITERAL_BOOL:
                    return baa_create_float_literal_data(data->bool_value ? 1.0f : 0.0f);
                case BAA_LITERAL_INT:
                    return baa_create_float_literal_data((float)data->int_value);
                default:
                    return NULL; // Should not happen if can_convert_to returned true
            }
            
        case BAA_TYPE_CHAR:
            // Convert to char
            switch (data->kind) {
                case BAA_LITERAL_INT:
                    if (data->int_value >= 0 && data->int_value <= 0xFFFF) {
                        return baa_create_char_literal_data((wchar_t)data->int_value);
                    }
                    return NULL;
                default:
                    return NULL; // Should not happen if can_convert_to returned true
            }
            
        case BAA_TYPE_STRING:
            // Convert to string
            switch (data->kind) {
                case BAA_LITERAL_NULL:
                    return baa_create_string_literal_data(L"", 0);
                default:
                    return NULL; // Should not happen if can_convert_to returned true
            }
            
        default:
            return NULL;
    }
}

// Value accessors with type checking
bool baa_literal_get_bool_value(BaaLiteralData* data, bool* success) {
    if (!data) {
        if (success) *success = false;
        return false;
    }
    
    if (data->kind == BAA_LITERAL_BOOL) {
        if (success) *success = true;
        return data->bool_value;
    }
    
    // Try conversion for compatible types
    bool result = false;
    
    switch (data->kind) {
        case BAA_LITERAL_INT:
            result = data->int_value != 0;
            if (success) *success = true;
            break;
            
        case BAA_LITERAL_CHAR:
            result = data->char_value != L'\0';
            if (success) *success = true;
            break;
            
        default:
            if (success) *success = false;
            break;
    }
    
    return result;
}

int baa_literal_get_int_value(BaaLiteralData* data, bool* success) {
    if (!data) {
        if (success) *success = false;
        return 0;
    }
    
    if (data->kind == BAA_LITERAL_INT) {
        if (success) *success = true;
        return data->int_value;
    }
    
    // Try conversion for compatible types
    int result = 0;
    
    switch (data->kind) {
        case BAA_LITERAL_BOOL:
            result = data->bool_value ? 1 : 0;
            if (success) *success = true;
            break;
            
        case BAA_LITERAL_FLOAT:
            result = (int)data->float_value;
            if (success) *success = true;
            break;
            
        case BAA_LITERAL_CHAR:
            result = (int)data->char_value;
            if (success) *success = true;
            break;
            
        default:
            if (success) *success = false;
            break;
    }
    
    return result;
}

float baa_literal_get_float_value(BaaLiteralData* data, bool* success) {
    if (!data) {
        if (success) *success = false;
        return 0.0f;
    }
    
    if (data->kind == BAA_LITERAL_FLOAT) {
        if (success) *success = true;
        return data->float_value;
    }
    
    // Try conversion for compatible types
    float result = 0.0f;
    
    switch (data->kind) {
        case BAA_LITERAL_BOOL:
            result = data->bool_value ? 1.0f : 0.0f;
            if (success) *success = true;
            break;
            
        case BAA_LITERAL_INT:
            result = (float)data->int_value;
            if (success) *success = true;
            break;
            
        default:
            if (success) *success = false;
            break;
    }
    
    return result;
}

wchar_t baa_literal_get_char_value(BaaLiteralData* data, bool* success) {
    if (!data) {
        if (success) *success = false;
        return L'\0';
    }
    
    if (data->kind == BAA_LITERAL_CHAR) {
        if (success) *success = true;
        return data->char_value;
    }
    
    // Try conversion for compatible types
    wchar_t result = L'\0';
    
    switch (data->kind) {
        case BAA_LITERAL_INT:
            if (data->int_value >= 0 && data->int_value <= 0xFFFF) {
                result = (wchar_t)data->int_value;
                if (success) *success = true;
            } else {
                if (success) *success = false;
            }
            break;
            
        default:
            if (success) *success = false;
            break;
    }
    
    return result;
}

const wchar_t* baa_literal_get_string_value(BaaLiteralData* data, bool* success) {
    if (!data) {
        if (success) *success = false;
        return NULL;
    }
    
    if (data->kind == BAA_LITERAL_STRING) {
        if (success) *success = true;
        return data->string_value;
    }
    
    // String type doesn't support conversions from other types in this implementation
    if (success) *success = false;
    return NULL;
}

bool baa_literal_is_null(BaaLiteralData* data) {
    return data && data->kind == BAA_LITERAL_NULL;
}

// Serialization
wchar_t* baa_literal_to_string(BaaLiteralData* data) {
    if (!data) return NULL;
    
    wchar_t buffer[128]; // For numeric conversions
    wchar_t* result = NULL;
    
    switch (data->kind) {
        case BAA_LITERAL_BOOL:
            // Convert boolean to string
            if (data->bool_value) {
                result = baa_wcsdup(L"صحيح"); // true
            } else {
                result = baa_wcsdup(L"خطأ");  // false
            }
            break;
            
        case BAA_LITERAL_INT:
            // Convert integer to string
            swprintf(buffer, 128, L"%d", data->int_value);
            result = baa_wcsdup(buffer);
            break;
            
        case BAA_LITERAL_FLOAT:
            // Convert float to string
            swprintf(buffer, 128, L"%f", data->float_value);
            result = baa_wcsdup(buffer);
            break;
            
        case BAA_LITERAL_CHAR:
            // Convert char to string with quotes
            swprintf(buffer, 128, L"'%lc'", data->char_value);
            result = baa_wcsdup(buffer);
            break;
            
        case BAA_LITERAL_STRING:
            // Return a copy of the string with quotes
            if (data->string_value) {
                size_t len = wcslen(data->string_value);
                result = (wchar_t*)baa_malloc((len + 3) * sizeof(wchar_t)); // +2 for quotes, +1 for null
                if (result) {
                    result[0] = L'"';
                    wcscpy(result + 1, data->string_value);
                    result[len + 1] = L'"';
                    result[len + 2] = L'\0';
                }
            } else {
                result = baa_wcsdup(L"\"\"");
            }
            break;
            
        case BAA_LITERAL_NULL:
            // Null literal
            result = baa_wcsdup(L"فارغ"); // null
            break;
            
        default:
            // Unknown type
            result = baa_wcsdup(L"<غير معروف>"); // unknown
            break;
    }
    
    return result;
}

// Create a new literal from a string representation
BaaLiteralData* baa_literal_from_string(const wchar_t* str, BaaType* type) {
    if (!str || !type) return NULL;
    
    switch (type->kind) {
        case BAA_TYPE_BOOL:
            // Parse boolean
            if (wcscmp(str, L"صحيح") == 0 || wcscmp(str, L"true") == 0) {
                return baa_create_bool_literal_data(true);
            } else if (wcscmp(str, L"خطأ") == 0 || wcscmp(str, L"false") == 0) {
                return baa_create_bool_literal_data(false);
            }
            return NULL;
            
        case BAA_TYPE_INT:
            // Parse integer
            return baa_create_int_literal_data((int)wcstol(str, NULL, 10));
            
        case BAA_TYPE_FLOAT:
            // Parse float
            return baa_create_float_literal_data((float)wcstod(str, NULL));
            
        case BAA_TYPE_CHAR:
            // Parse character (assuming format 'c')
            if (wcslen(str) >= 3 && str[0] == L'\'' && str[wcslen(str) - 1] == L'\'') {
                return baa_create_char_literal_data(str[1]);
            }
            return NULL;
            
        case BAA_TYPE_STRING:
            // Parse string (assuming format "str")
            if (wcslen(str) >= 2 && str[0] == L'"' && str[wcslen(str) - 1] == L'"') {
                size_t len = wcslen(str) - 2;
                wchar_t* content = (wchar_t*)baa_malloc((len + 1) * sizeof(wchar_t));
                if (content) {
                    wcsncpy(content, str + 1, len);
                    content[len] = L'\0';
                    BaaLiteralData* result = baa_create_string_literal_data(content, len);
                    free(content);
                    return result;
                }
            }
            return NULL;
            
        case BAA_TYPE_NULL:
            // Parse null
            if (wcscmp(str, L"فارغ") == 0 || wcscmp(str, L"null") == 0) {
                return baa_create_null_literal_data();
            }
            return NULL;
            
        default:
            return NULL;
    }
}

// Memory management
BaaLiteralData* baa_copy_literal_data(BaaLiteralData* data) {
    if (!data) return NULL;
    
    switch (data->kind) {
        case BAA_LITERAL_BOOL:
            return baa_create_bool_literal_data(data->bool_value);
            
        case BAA_LITERAL_INT:
            return baa_create_int_literal_data(data->int_value);
            
        case BAA_LITERAL_FLOAT:
            return baa_create_float_literal_data(data->float_value);
            
        case BAA_LITERAL_CHAR:
            return baa_create_char_literal_data(data->char_value);
            
        case BAA_LITERAL_STRING:
            if (data->string_value) {
                return baa_create_string_literal_data(data->string_value, wcslen(data->string_value));
            }
            return baa_create_string_literal_data(L"", 0);
            
        case BAA_LITERAL_NULL:
            return baa_create_null_literal_data();
            
        default:
            return NULL;
    }
}

void baa_free_literal_data(BaaLiteralData* data) {
    if (!data) return;
    
    // Free string value if it's a string literal
    if (data->kind == BAA_LITERAL_STRING && data->string_value) {
        free(data->string_value);
    }
    
    free(data);
}
