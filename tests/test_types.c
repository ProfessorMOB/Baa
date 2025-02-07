#include "../src/types/types.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Test type creation and basic properties
void test_type_creation(void) {
    printf("Testing type creation...\n");

    Type* test_int = baa_create_type(TYPE_INT, sizeof(int32_t), true);
    assert(test_int != NULL);
    assert(test_int->kind == TYPE_INT);
    assert(test_int->size == sizeof(int32_t));
    assert(test_int->is_signed == true);

    Type* test_char = baa_create_type(TYPE_CHAR, sizeof(uint16_t), false);
    assert(test_char != NULL);
    assert(test_char->kind == TYPE_CHAR);
    assert(test_char->size == sizeof(uint16_t));
    assert(test_char->is_signed == false);

    free(test_int);
    free(test_char);
    printf("Type creation tests passed.\n");
}

// Test type system initialization
void test_type_system_init(void) {
    printf("Testing type system initialization...\n");

    baa_init_type_system();

    assert(type_int != NULL);
    assert(type_float != NULL);
    assert(type_char != NULL);
    assert(type_void != NULL);
    assert(type_error != NULL);

    assert(type_int->kind == TYPE_INT);
    assert(type_float->kind == TYPE_FLOAT);
    assert(type_char->kind == TYPE_CHAR);
    assert(type_void->kind == TYPE_VOID);
    assert(type_error->kind == TYPE_ERROR);

    printf("Type system initialization tests passed.\n");
}

// Test type comparison
void test_type_comparison(void) {
    printf("Testing type comparison...\n");

    assert(baa_types_equal(type_int, type_int) == true);
    assert(baa_types_equal(type_int, type_float) == false);
    assert(baa_types_equal(type_char, type_int) == false);
    assert(baa_types_equal(type_void, type_void) == true);
    assert(baa_types_equal(NULL, type_int) == false);
    assert(baa_types_equal(type_int, NULL) == false);

    printf("Type comparison tests passed.\n");
}

// Test type conversion rules
void test_type_conversion(void) {
    printf("Testing type conversion...\n");

    // Valid conversions
    assert(baa_can_convert(type_int, type_float) == true);
    assert(baa_can_convert(type_float, type_int) == true);
    assert(baa_can_convert(type_char, type_int) == true);
    assert(baa_can_convert(type_int, type_int) == true);

    // Invalid conversions
    assert(baa_can_convert(type_void, type_int) == false);
    assert(baa_can_convert(type_int, type_void) == false);
    assert(baa_can_convert(type_error, type_int) == false);
    assert(baa_can_convert(type_int, type_error) == false);

    printf("Type conversion tests passed.\n");
}

// Test type to string conversion
void test_type_to_string(void) {
    printf("Testing type to string conversion...\n");

    assert(strcmp(baa_type_to_string(type_int), "عدد_صحيح") == 0);
    assert(strcmp(baa_type_to_string(type_float), "عدد_حقيقي") == 0);
    assert(strcmp(baa_type_to_string(type_char), "محرف") == 0);
    assert(strcmp(baa_type_to_string(type_void), "فراغ") == 0);
    assert(strcmp(baa_type_to_string(type_error), "خطأ") == 0);
    assert(strcmp(baa_type_to_string(NULL), "NULL") == 0);

    printf("Type to string conversion tests passed.\n");
}

int main(void) {
    printf("Running type system tests...\n\n");

    test_type_creation();
    test_type_system_init();
    test_type_comparison();
    test_type_conversion();
    test_type_to_string();

    printf("\nAll type system tests passed successfully!\n");
    return 0;
}
