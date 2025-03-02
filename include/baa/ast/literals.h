#ifndef BAA_LITERALS_H
#define BAA_LITERALS_H

#include <stdbool.h>
#include <wchar.h>
#include <stddef.h>
#include "baa/types/types.h"

// Literal value types
typedef enum {
    BAA_LITERAL_BOOL,    // Boolean literal
    BAA_LITERAL_INT,     // Integer literal
    BAA_LITERAL_FLOAT,   // Float literal
    BAA_LITERAL_CHAR,    // Character literal
    BAA_LITERAL_STRING,  // String literal
    BAA_LITERAL_NULL     // Null literal
} BaaLiteralKind;

// Structure to hold literal data based on type
typedef struct {
    BaaLiteralKind kind;   // Type of literal value stored
    union {
        bool bool_value;        // For boolean literals
        int int_value;          // For integer literals
        float float_value;      // For float literals
        wchar_t char_value;     // For character literals
        wchar_t* string_value;  // For string literals
    };
} BaaLiteralData;

// Helper functions for creating literal data
BaaLiteralData* baa_create_bool_literal_data(bool value);
BaaLiteralData* baa_create_int_literal_data(int value);
BaaLiteralData* baa_create_float_literal_data(float value);
BaaLiteralData* baa_create_char_literal_data(wchar_t value);
BaaLiteralData* baa_create_string_literal_data(const wchar_t* value, size_t length);
BaaLiteralData* baa_create_null_literal_data(void);

// Type conversion functions
BaaType* baa_get_type_for_literal(BaaLiteralData* data);
bool baa_literal_can_convert_to(BaaLiteralData* data, BaaType* target_type);
BaaLiteralData* baa_convert_literal(BaaLiteralData* data, BaaType* target_type);

// Value accessors (with type checking)
bool baa_literal_get_bool_value(BaaLiteralData* data, bool* success);
int baa_literal_get_int_value(BaaLiteralData* data, bool* success);
float baa_literal_get_float_value(BaaLiteralData* data, bool* success);
wchar_t baa_literal_get_char_value(BaaLiteralData* data, bool* success);
const wchar_t* baa_literal_get_string_value(BaaLiteralData* data, bool* success);
bool baa_literal_is_null(BaaLiteralData* data);

// Serialization/deserialization
wchar_t* baa_literal_to_string(BaaLiteralData* data);
BaaLiteralData* baa_literal_from_string(const wchar_t* str, BaaType* type);

// Memory management
BaaLiteralData* baa_copy_literal_data(BaaLiteralData* data);
void baa_free_literal_data(BaaLiteralData* data);

#endif /* BAA_LITERALS_H */
