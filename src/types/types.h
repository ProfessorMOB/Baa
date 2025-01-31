#pragma once

#include <stdint.h>
#include <stdbool.h>

// Type kinds
typedef enum {
    TYPE_INT,      // عدد_صحيح
    TYPE_FLOAT,    // عدد_حقيقي
    TYPE_CHAR,     // محرف
    TYPE_VOID,     // فراغ
    TYPE_ERROR     // For error handling
} TypeKind;

// Type structure
typedef struct Type {
    TypeKind kind;
    uint32_t size;     // Size in bytes
    bool is_signed;    // For numeric types
    struct Type* next; // For future use with complex types
} Type;

// Basic type singletons
extern Type* type_int;    // عدد_صحيح
extern Type* type_float;  // عدد_حقيقي
extern Type* type_char;   // محرف
extern Type* type_void;   // فراغ
extern Type* type_error;  // Error type

// Type system initialization
void baa_init_type_system(void);

// Type creation and manipulation
Type* baa_create_type(TypeKind kind, uint32_t size, bool is_signed);

// Type comparison
bool baa_types_equal(Type* a, Type* b);

// Type conversion checking
bool baa_can_convert(Type* from, Type* to);

// Type error handling
const char* baa_type_to_string(Type* type);
