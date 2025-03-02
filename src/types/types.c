#include "baa/types/types.h"
#include "baa/utils/utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <wchar.h>

// Global type instances
BaaType* baa_type_void = NULL;
BaaType* baa_type_int = NULL;
BaaType* baa_type_float = NULL;
BaaType* baa_type_char = NULL;
BaaType* baa_type_string = NULL;
BaaType* baa_type_bool = NULL;
BaaType* baa_type_null = NULL;
BaaType* baa_type_error = NULL;

void baa_init_type_system(void) {
    if (baa_type_void == NULL) {
        baa_type_void = baa_create_type(BAA_TYPE_VOID, L"فراغ", sizeof(void), false);
        baa_type_int = baa_create_type(BAA_TYPE_INT, L"عدد_صحيح", sizeof(int), true);
        baa_type_float = baa_create_type(BAA_TYPE_FLOAT, L"عدد_حقيقي", sizeof(double), true);
        baa_type_char = baa_create_type(BAA_TYPE_CHAR, L"حرف", sizeof(wchar_t), true);
        baa_type_string = baa_create_type(BAA_TYPE_STRING, L"نص", sizeof(wchar_t*), false);
        baa_type_bool = baa_create_type(BAA_TYPE_BOOL, L"منطقي", sizeof(bool), false);
        baa_type_null = baa_create_type(BAA_TYPE_NULL, L"فارغ", 0, false);
        baa_type_error = baa_create_type(BAA_TYPE_ERROR, L"خطأ", 0, false);
    }
}

BaaType* baa_create_type(BaaTypeKind kind, const wchar_t* name, uint32_t size, bool is_signed) {
    BaaType* type = baa_malloc(sizeof(BaaType));
    if (type) {
        type->kind = kind;
        type->name = baa_strdup(name);
        type->size = size;
        type->is_signed = is_signed;
        type->next = NULL;
    }
    return type;
}

void baa_free_type(BaaType* type) {
    if (type) {
        if (type->name) {
            baa_free((void*)type->name);
        }
        
        // If it's an array type, we don't free the element_type
        // since it might be a shared type that's used elsewhere
        // The element_type is expected to be freed separately
        
        baa_free(type);
    }
}

bool baa_types_equal(BaaType* a, BaaType* b) {
    if (a == NULL || b == NULL) {
        return false;
    }

    return a->kind == b->kind &&
           a->size == b->size &&
           a->is_signed == b->is_signed;
}

bool baa_can_convert(BaaType* from, BaaType* to) {
    if (from == NULL || to == NULL) {
        return false;
    }

    // Same types can always convert
    if (baa_types_equal(from, to)) {
        return true;
    }

    // Error type can't convert to anything
    if (from->kind == BAA_TYPE_ERROR || to->kind == BAA_TYPE_ERROR) {
        return false;
    }

    // Void type can't convert to anything
    if (from->kind == BAA_TYPE_VOID || to->kind == BAA_TYPE_VOID) {
        return false;
    }

    // Allow numeric conversions
    if ((from->kind == BAA_TYPE_INT || from->kind == BAA_TYPE_FLOAT) &&
        (to->kind == BAA_TYPE_INT || to->kind == BAA_TYPE_FLOAT)) {
        return true;
    }

    // Allow char to int conversion
    if (from->kind == BAA_TYPE_CHAR && to->kind == BAA_TYPE_INT) {
        return true;
    }

    return false;
}

const wchar_t* baa_type_to_string(BaaType* type) {
    if (type == NULL) {
        return L"NULL";
    }
    return type->name;
}

// Type getter functions
BaaType* baa_get_void_type(void) {
    if (baa_type_void == NULL) {
        baa_init_type_system();
    }
    return baa_type_void;
}

BaaType* baa_get_int_type(void) {
    if (baa_type_int == NULL) {
        baa_init_type_system();
    }
    return baa_type_int;
}

BaaType* baa_get_float_type(void) {
    if (baa_type_float == NULL) {
        baa_init_type_system();
    }
    return baa_type_float;
}

BaaType* baa_get_char_type(void) {
    if (baa_type_char == NULL) {
        baa_init_type_system();
    }
    return baa_type_char;
}

BaaType* baa_get_string_type(void) {
    if (baa_type_string == NULL) {
        baa_init_type_system();
    }
    return baa_type_string;
}

BaaType* baa_get_bool_type(void) {
    if (baa_type_bool == NULL) {
        baa_init_type_system();
    }
    return baa_type_bool;
}

BaaType* baa_get_null_type(void) {
    if (baa_type_null == NULL) {
        baa_init_type_system();
    }
    return baa_type_null;
}

BaaType* baa_get_error_type(void) {
    if (baa_type_error == NULL) {
        baa_init_type_system();
    }
    return baa_type_error;
}

// Create a primitive type based on the type kind
BaaType* baa_create_primitive_type(BaaTypeKind kind) {
    switch (kind) {
        case BAA_TYPE_VOID:
            return baa_get_void_type();
        case BAA_TYPE_INT:
            return baa_get_int_type();
        case BAA_TYPE_FLOAT:
            return baa_get_float_type();
        case BAA_TYPE_CHAR:
            return baa_get_char_type();
        case BAA_TYPE_STRING:
            return baa_get_string_type();
        case BAA_TYPE_BOOL:
            return baa_get_bool_type();
        case BAA_TYPE_NULL:
            return baa_get_null_type();
        case BAA_TYPE_ERROR:
            return baa_get_error_type();
        default:
            return NULL;
    }
}

// Array type operations
BaaType* baa_create_array_type(BaaType* element_type, size_t size) {
    if (!element_type) return NULL;
    
    // Create a new array type
    BaaType* array_type = baa_malloc(sizeof(BaaType));
    if (!array_type) return NULL;
    
    // Initialize the array type
    array_type->kind = BAA_TYPE_ARRAY;
    array_type->name = baa_strdup(L"مصفوفة");
    array_type->size = element_type->size * (size > 0 ? size : 1);
    array_type->is_signed = false;
    array_type->next = NULL;
    
    // Set array-specific fields
    array_type->element_type = element_type;
    array_type->array_size = size;
    
    return array_type;
}

BaaType* baa_get_array_element_type(BaaType* array_type) {
    if (!array_type || array_type->kind != BAA_TYPE_ARRAY) {
        return NULL;
    }
    
    return array_type->element_type;
}

size_t baa_get_array_size(BaaType* array_type) {
    if (!array_type || array_type->kind != BAA_TYPE_ARRAY) {
        return 0;
    }
    
    return array_type->array_size;
}

bool baa_is_array_type(BaaType* type) {
    return type != NULL && type->kind == BAA_TYPE_ARRAY;
}
