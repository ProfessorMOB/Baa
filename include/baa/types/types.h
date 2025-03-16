#ifndef BAA_TYPES_H
#define BAA_TYPES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <wchar.h>

// First, forward declare the struct
typedef struct BaaType BaaType;

// Type kinds
typedef enum
{
    BAA_TYPE_VOID,   // فراغ
    BAA_TYPE_INT,    // عدد_صحيح
    BAA_TYPE_FLOAT,  // عدد_حقيقي
    BAA_TYPE_CHAR,   // حرف
    BAA_TYPE_STRING, // نص
    BAA_TYPE_BOOL,   // منطقي
    BAA_TYPE_NULL,   // فارغ
    BAA_TYPE_ERROR,  // خطأ
    BAA_TYPE_ARRAY,  // مصفوفة
} BaaTypeKind;

// Type structure
struct BaaType
{
    BaaTypeKind kind;    // Type kind
    const wchar_t *name; // Type name in Arabic
    uint32_t size;       // Size in bytes
    bool is_signed;      // Whether type is signed
    BaaType *next;       // For linked list of types

    // Array type information (only valid if kind == BAA_TYPE_ARRAY)
    struct BaaType *element_type; // Type of array elements
    size_t array_size;            // Number of elements (0 for dynamic arrays)
};

// Global type instances
extern BaaType *baa_type_void;   // فراغ
extern BaaType *baa_type_int;    // عدد_صحيح
extern BaaType *baa_type_float;  // عدد_حقيقي
extern BaaType *baa_type_char;   // حرف
extern BaaType *baa_type_string; // نص
extern BaaType *baa_type_bool;   // منطقي
extern BaaType *baa_type_null;   // فارغ
extern BaaType *baa_type_error;  // خطأ

// Type system functions
void baa_init_type_system(void);
BaaType *baa_create_type(BaaTypeKind kind, const wchar_t *name, uint32_t size, bool is_signed);
void baa_free_type(BaaType *type);

// Type comparison and conversion
bool baa_types_equal(BaaType *a, BaaType *b);
bool baa_can_convert(BaaType *from, BaaType *to);
const wchar_t *baa_type_to_string(BaaType *type);

// Type getter functions
BaaType *baa_get_void_type(void);
BaaType *baa_get_int_type(void);
BaaType *baa_get_float_type(void);
BaaType *baa_get_char_type(void);
BaaType *baa_get_string_type(void);
BaaType *baa_get_bool_type(void);
BaaType *baa_get_null_type(void);
BaaType *baa_get_error_type(void);

// Create a primitive type based on the type kind
BaaType *baa_create_primitive_type(BaaTypeKind kind);

// Array type operations
BaaType *baa_create_array_type(BaaType *element_type, size_t size);
BaaType *baa_get_array_element_type(BaaType *array_type);
size_t baa_get_array_size(BaaType *array_type);
bool baa_is_array_type(BaaType *type);

#endif /* BAA_TYPES_H */
