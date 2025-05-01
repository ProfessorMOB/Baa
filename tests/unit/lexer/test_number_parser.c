#include "test_framework.h"
#include "baa/lexer/lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>

// Test fixture
typedef struct {
    BaaNumberError error;
} NumberParserFixture;

// Setup function for tests
static void setup(NumberParserFixture* fixture) {
    // Set locale for wide character support
    setlocale(LC_ALL, "");

    // Initialize error
    fixture->error = BAA_NUM_SUCCESS;
}

// Test parsing standard integers
static void test_standard_integer_parsing(void) {
    NumberParserFixture fixture;
    setup(&fixture);

    // Test simple integer
    const wchar_t* num1 = L"123";
    BaaNumber* number1 = baa_parse_number(num1, wcslen(num1), &fixture.error);

    ASSERT_NOT_NULL(number1, "Failed to parse standard integer");
    ASSERT_EQUAL(fixture.error, BAA_NUM_SUCCESS, "Unexpected error parsing standard integer");
    ASSERT_EQUAL(number1->type, BAA_NUM_INTEGER, "Wrong number type for standard integer");
    ASSERT_EQUAL(number1->int_value, 123, "Wrong value for standard integer");

    baa_free_number(number1);

    // Test large integer
    const wchar_t* num2 = L"9876543210";
    BaaNumber* number2 = baa_parse_number(num2, wcslen(num2), &fixture.error);

    ASSERT_NOT_NULL(number2, "Failed to parse large integer");
    ASSERT_EQUAL(fixture.error, BAA_NUM_SUCCESS, "Unexpected error parsing large integer");
    ASSERT_EQUAL(number2->type, BAA_NUM_INTEGER, "Wrong number type for large integer");
    ASSERT_EQUAL(number2->int_value, 9876543210LL, "Wrong value for large integer");

    baa_free_number(number2);
}

// Test parsing Arabic-Indic integers
static void test_arabic_integer_parsing(void) {
    NumberParserFixture fixture;
    setup(&fixture);

    // Test simple Arabic integer (١٢٣)
    const wchar_t* num1 = L"\u0661\u0662\u0663"; // Arabic-Indic digits 1-2-3
    BaaNumber* number1 = baa_parse_number(num1, wcslen(num1), &fixture.error);

    ASSERT_NOT_NULL(number1, "Failed to parse Arabic integer");
    ASSERT_EQUAL(fixture.error, BAA_NUM_SUCCESS, "Unexpected error parsing Arabic integer");
    ASSERT_EQUAL(number1->type, BAA_NUM_INTEGER, "Wrong number type for Arabic integer");
    ASSERT_EQUAL(number1->int_value, 123, "Wrong value for Arabic integer");

    baa_free_number(number1);

    // Test large Arabic integer (٩٨٧٦٥٤٣٢١٠)
    const wchar_t* num2 = L"\u0669\u0668\u0667\u0666\u0665\u0664\u0663\u0662\u0661\u0660";
    BaaNumber* number2 = baa_parse_number(num2, wcslen(num2), &fixture.error);

    ASSERT_NOT_NULL(number2, "Failed to parse large Arabic integer");
    ASSERT_EQUAL(fixture.error, BAA_NUM_SUCCESS, "Unexpected error parsing large Arabic integer");
    ASSERT_EQUAL(number2->type, BAA_NUM_INTEGER, "Wrong number type for large Arabic integer");
    ASSERT_EQUAL(number2->int_value, 9876543210LL, "Wrong value for large Arabic integer");

    baa_free_number(number2);
}

// Test parsing standard decimal numbers
static void test_standard_decimal_parsing(void) {
    NumberParserFixture fixture;
    setup(&fixture);

    // Test simple decimal
    const wchar_t* num1 = L"123.45";
    BaaNumber* number1 = baa_parse_number(num1, wcslen(num1), &fixture.error);

    ASSERT_NOT_NULL(number1, "Failed to parse standard decimal");
    ASSERT_EQUAL(fixture.error, BAA_NUM_SUCCESS, "Unexpected error parsing standard decimal");
    ASSERT_EQUAL(number1->type, BAA_NUM_DECIMAL, "Wrong number type for standard decimal");
    ASSERT_DOUBLE_EQUAL(number1->decimal_value, 123.45, 0.0001, "Wrong value for standard decimal");

    baa_free_number(number1);

    // Test decimal with zero integer part
    const wchar_t* num2 = L"0.789";
    BaaNumber* number2 = baa_parse_number(num2, wcslen(num2), &fixture.error);

    ASSERT_NOT_NULL(number2, "Failed to parse decimal with zero integer part");
    ASSERT_EQUAL(fixture.error, BAA_NUM_SUCCESS, "Unexpected error parsing decimal with zero integer part");
    ASSERT_EQUAL(number2->type, BAA_NUM_DECIMAL, "Wrong number type for decimal with zero integer part");
    ASSERT_DOUBLE_EQUAL(number2->decimal_value, 0.789, 0.0001, "Wrong value for decimal with zero integer part");

    baa_free_number(number2);
}

// Test parsing Arabic decimal numbers
static void test_arabic_decimal_parsing(void) {
    NumberParserFixture fixture;
    setup(&fixture);

    // Test Arabic decimal with Arabic decimal separator (١٢٣٫٤٥)
    const wchar_t* num1 = L"\u0661\u0662\u0663\u066B\u0664\u0665"; // Arabic digits with Arabic decimal point
    BaaNumber* number1 = baa_parse_number(num1, wcslen(num1), &fixture.error);

    ASSERT_NOT_NULL(number1, "Failed to parse Arabic decimal");
    ASSERT_EQUAL(fixture.error, BAA_NUM_SUCCESS, "Unexpected error parsing Arabic decimal");
    ASSERT_EQUAL(number1->type, BAA_NUM_DECIMAL, "Wrong number type for Arabic decimal");
    ASSERT_DOUBLE_EQUAL(number1->decimal_value, 123.45, 0.0001, "Wrong value for Arabic decimal");

    baa_free_number(number1);

    // Test Arabic decimal with standard decimal separator (١٢٣.٤٥)
    const wchar_t* num2 = L"\u0661\u0662\u0663.\u0664\u0665";
    BaaNumber* number2 = baa_parse_number(num2, wcslen(num2), &fixture.error);

    ASSERT_NOT_NULL(number2, "Failed to parse Arabic decimal with standard separator");
    ASSERT_EQUAL(fixture.error, BAA_NUM_SUCCESS, "Unexpected error parsing Arabic decimal with standard separator");
    ASSERT_EQUAL(number2->type, BAA_NUM_DECIMAL, "Wrong number type for Arabic decimal with standard separator");
    ASSERT_DOUBLE_EQUAL(number2->decimal_value, 123.45, 0.0001, "Wrong value for Arabic decimal with standard separator");

    baa_free_number(number2);
}

// Test mixed digit systems
static void test_mixed_digit_parsing(void) {
    NumberParserFixture fixture;
    setup(&fixture);

    // Test mixed digits (12٣٤5)
    const wchar_t* num1 = L"12\u0663\u06345";
    BaaNumber* number1 = baa_parse_number(num1, wcslen(num1), &fixture.error);

    ASSERT_NOT_NULL(number1, "Failed to parse mixed digits");
    ASSERT_EQUAL(fixture.error, BAA_NUM_SUCCESS, "Unexpected error parsing mixed digits");
    ASSERT_EQUAL(number1->type, BAA_NUM_INTEGER, "Wrong number type for mixed digits");
    ASSERT_EQUAL(number1->int_value, 12345, "Wrong value for mixed digits");

    baa_free_number(number1);
}

// Test error conditions
static void test_error_conditions(void) {
    NumberParserFixture fixture;
    setup(&fixture);

    // Test overflow
    const wchar_t* num1 = L"999999999999999999999999999999";
    BaaNumber* number1 = baa_parse_number(num1, wcslen(num1), &fixture.error);

    ASSERT_NULL(number1, "Should fail on integer overflow");
    ASSERT_EQUAL(fixture.error, BAA_NUM_OVERFLOW, "Expected overflow error");

    // Test multiple decimal points
    const wchar_t* num2 = L"123.45.67";
    BaaNumber* number2 = baa_parse_number(num2, wcslen(num2), &fixture.error);

    ASSERT_NULL(number2, "Should fail on multiple decimal points");
    ASSERT_EQUAL(fixture.error, BAA_NUM_MULTIPLE_DOTS, "Expected multiple dots error");

    // Test invalid format
    const wchar_t* num3 = L"123.";
    BaaNumber* number3 = baa_parse_number(num3, wcslen(num3), &fixture.error);

    ASSERT_NULL(number3, "Should fail on invalid format");
    ASSERT_EQUAL(fixture.error, BAA_NUM_INVALID_FORMAT, "Expected invalid format error");

    // Test empty string
    const wchar_t* num4 = L"";
    BaaNumber* number4 = baa_parse_number(num4, wcslen(num4), &fixture.error);

    ASSERT_NULL(number4, "Should fail on empty string");
    ASSERT_EQUAL(fixture.error, BAA_NUM_INVALID_FORMAT, "Expected invalid format error for empty string");
}

// Test error messages
static void test_error_messages(void) {
    // Test all error messages
    const wchar_t* success_msg = baa_number_error_message(BAA_NUM_SUCCESS);
    const wchar_t* overflow_msg = baa_number_error_message(BAA_NUM_OVERFLOW);
    const wchar_t* invalid_char_msg = baa_number_error_message(BAA_NUM_INVALID_CHAR);
    const wchar_t* multiple_dots_msg = baa_number_error_message(BAA_NUM_MULTIPLE_DOTS);
    const wchar_t* invalid_format_msg = baa_number_error_message(BAA_NUM_INVALID_FORMAT);
    const wchar_t* memory_error_msg = baa_number_error_message(BAA_NUM_MEMORY_ERROR);
    const wchar_t* unknown_msg = baa_number_error_message(99); // Invalid error code

    ASSERT_NOT_NULL(success_msg, "Success message should not be NULL");
    ASSERT_NOT_NULL(overflow_msg, "Overflow message should not be NULL");
    ASSERT_NOT_NULL(invalid_char_msg, "Invalid char message should not be NULL");
    ASSERT_NOT_NULL(multiple_dots_msg, "Multiple dots message should not be NULL");
    ASSERT_NOT_NULL(invalid_format_msg, "Invalid format message should not be NULL");
    ASSERT_NOT_NULL(memory_error_msg, "Memory error message should not be NULL");
    ASSERT_NOT_NULL(unknown_msg, "Unknown error message should not be NULL");
}

// Main function
TEST_SUITE_BEGIN()
    TEST_CASE(test_standard_integer_parsing);
    TEST_CASE(test_arabic_integer_parsing);
    TEST_CASE(test_standard_decimal_parsing);
    TEST_CASE(test_arabic_decimal_parsing);
    TEST_CASE(test_mixed_digit_parsing);
    TEST_CASE(test_error_conditions);
    TEST_CASE(test_error_messages);
TEST_SUITE_END()
