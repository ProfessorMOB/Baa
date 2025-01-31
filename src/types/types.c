#include "types.h"
#include <stdlib.h>
#include <string.h>

// Basic type singletons
Type* type_int = NULL;
Type* type_float = NULL;
Type* type_char = NULL;
Type* type_void = NULL;
Type* type_error = NULL;

void baa_init_type_system(void) {
    // Initialize basic types if not already initialized
    if (type_int == NULL) {
        type_int = baa_create_type(TYPE_INT, sizeof(int32_t), true);
        type_float = baa_create_type(TYPE_FLOAT, sizeof(float), true);
        type_char = baa_create_type(TYPE_CHAR, sizeof(uint16_t), false); // UTF-16
        type_void = baa_create_type(TYPE_VOID, 0, false);
        type_error = baa_create_type(TYPE_ERROR, 0, false);
    }
}

Type* baa_create_type(TypeKind kind, uint32_t size, bool is_signed) {
    Type* type = (Type*)malloc(sizeof(Type));
    if (type == NULL) {
        return type_error;
    }
    
    type->kind = kind;
    type->size = size;
    type->is_signed = is_signed;
    type->next = NULL;
    
    return type;
}

bool baa_types_equal(Type* a, Type* b) {
    if (a == NULL || b == NULL) {
        return false;
    }
    
    return a->kind == b->kind &&
           a->size == b->size &&
           a->is_signed == b->is_signed;
}

bool baa_can_convert(Type* from, Type* to) {
    if (from == NULL || to == NULL) {
        return false;
    }
    
    // Same types can always convert
    if (baa_types_equal(from, to)) {
        return true;
    }
    
    // Error type can't convert to anything
    if (from->kind == TYPE_ERROR || to->kind == TYPE_ERROR) {
        return false;
    }
    
    // Void type can't convert to anything
    if (from->kind == TYPE_VOID || to->kind == TYPE_VOID) {
        return false;
    }
    
    // Allow numeric conversions
    if ((from->kind == TYPE_INT || from->kind == TYPE_FLOAT) &&
        (to->kind == TYPE_INT || to->kind == TYPE_FLOAT)) {
        return true;
    }
    
    // Allow char to int conversion
    if (from->kind == TYPE_CHAR && to->kind == TYPE_INT) {
        return true;
    }
    
    return false;
}

const char* baa_type_to_string(Type* type) {
    if (type == NULL) {
        return "NULL";
    }
    
    switch (type->kind) {
        case TYPE_INT:
            return "عدد_صحيح";
        case TYPE_FLOAT:
            return "عدد_حقيقي";
        case TYPE_CHAR:
            return "محرف";
        case TYPE_VOID:
            return "فراغ";
        case TYPE_ERROR:
            return "خطأ";
        default:
            return "غير_معروف";
    }
}
