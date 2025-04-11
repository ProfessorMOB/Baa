#include "baa/types/types.h"
#include "baa/utils/utils.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// Test type creation and basic properties
void test_type_creation(void)
{
    printf("Testing type creation...\n");

    // Test creating an integer type
    BaaType *test_int = baa_create_type(BAA_TYPE_INT, L"عدد_صحيح", sizeof(int32_t), true);
    assert(test_int != NULL);
    assert(test_int->kind == BAA_TYPE_INT);
    assert(test_int->size == sizeof(int32_t));
    assert(test_int->is_signed == true);

    // Test creating a character type
    BaaType *test_char = baa_create_type(BAA_TYPE_CHAR, L"حرف", sizeof(uint16_t), false);
    assert(test_char != NULL);
    assert(test_char->kind == BAA_TYPE_CHAR);
    assert(test_char->size == sizeof(uint16_t));
    assert(test_char->is_signed == false);

    baa_free_type(test_int);
    baa_free_type(test_char);
    printf("Type creation tests passed.\n");
}

// Test type system initialization
void test_type_system_init(void)
{
    printf("Testing type system initialization...\n");

    baa_init_type_system();

    assert(baa_type_int != NULL);
    assert(baa_type_float != NULL);
    assert(baa_type_char != NULL);
    assert(baa_type_void != NULL);
    assert(baa_type_error != NULL);

    assert(baa_type_int->kind == BAA_TYPE_INT);
    assert(baa_type_float->kind == BAA_TYPE_FLOAT);
    assert(baa_type_char->kind == BAA_TYPE_CHAR);
    assert(baa_type_void->kind == BAA_TYPE_VOID);
    assert(baa_type_error->kind == BAA_TYPE_ERROR);

    printf("Type system initialization tests passed.\n");
}

// Test type comparison
void test_type_comparison(void)
{
    printf("Testing type comparison...\n");

    assert(baa_types_equal(baa_type_int, baa_type_int) == true);
    assert(baa_types_equal(baa_type_int, baa_type_float) == false);
    assert(baa_types_equal(baa_type_char, baa_type_int) == false);
    assert(baa_types_equal(baa_type_void, baa_type_void) == true);
    assert(baa_types_equal(NULL, baa_type_int) == false);
    assert(baa_types_equal(baa_type_int, NULL) == false);

    printf("Type comparison tests passed.\n");
}

// Test type conversion rules
void test_type_conversion(void)
{
    printf("Testing type conversion...\n");

    // Test valid conversions
    assert(baa_can_convert(baa_type_int, baa_type_float) == true);
    assert(baa_can_convert(baa_type_float, baa_type_int) == true);
    assert(baa_can_convert(baa_type_char, baa_type_int) == true);
    assert(baa_can_convert(baa_type_int, baa_type_int) == true);

    // Test invalid conversions
    assert(baa_can_convert(baa_type_void, baa_type_int) == false);
    assert(baa_can_convert(baa_type_int, baa_type_void) == false);
    assert(baa_can_convert(baa_type_error, baa_type_int) == false);
    assert(baa_can_convert(baa_type_int, baa_type_error) == false);

    printf("Type conversion tests passed.\n");
}

// Test type to string conversion
void test_type_to_string(void)
{
    printf("Testing type to string conversion...\n");

    assert(wcscmp(baa_type_to_string(baa_type_int), L"عدد_صحيح") == 0);
    assert(wcscmp(baa_type_to_string(baa_type_float), L"عدد_حقيقي") == 0);
    assert(wcscmp(baa_type_to_string(baa_type_char), L"حرف") == 0);
    assert(wcscmp(baa_type_to_string(baa_type_void), L"فراغ") == 0);
    assert(wcscmp(baa_type_to_string(baa_type_error), L"خطأ") == 0);
    assert(wcscmp(baa_type_to_string(NULL), L"NULL") == 0);

    printf("Type to string conversion tests passed.\n");
}

int main(void)
{
    printf("Running type system tests...\n\n");

    test_type_creation();
    test_type_system_init();
    test_type_comparison();
    test_type_conversion();
    test_type_to_string();

    printf("\nAll type system tests passed successfully!\n");
    return 0;
}
