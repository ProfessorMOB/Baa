#include "baa/lexer/lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void test_scientific_notation() {
    // Test positive exponent
    const wchar_t* sci_pos = L"1.23e4";
    BaaLexer* lexer = baa_create_lexer(sci_pos);
    BaaToken* token = baa_scan_token(lexer);
    
    assert(token->type == BAA_TOKEN_FLOAT_LIT);
    printf("Scientific notation (positive exponent) test passed.\n");
    
    baa_free_token(token);
    baa_free_lexer(lexer);
    
    // Test negative exponent
    const wchar_t* sci_neg = L"5.67e-3";
    lexer = baa_create_lexer(sci_neg);
    token = baa_scan_token(lexer);
    
    assert(token->type == BAA_TOKEN_FLOAT_LIT);
    printf("Scientific notation (negative exponent) test passed.\n");
    
    baa_free_token(token);
    baa_free_lexer(lexer);
    
    // Test without decimal point
    const wchar_t* sci_no_decimal = L"42E2";
    lexer = baa_create_lexer(sci_no_decimal);
    token = baa_scan_token(lexer);
    
    assert(token->type == BAA_TOKEN_FLOAT_LIT);
    printf("Scientific notation (no decimal point) test passed.\n");
    
    baa_free_token(token);
    baa_free_lexer(lexer);
}

void test_hexadecimal_format() {
    // Test lowercase 'x'
    const wchar_t* hex_lower = L"0x1a3f";
    BaaLexer* lexer = baa_create_lexer(hex_lower);
    BaaToken* token = baa_scan_token(lexer);
    
    assert(token->type == BAA_TOKEN_INT_LIT);
    printf("Hexadecimal format (lowercase x) test passed.\n");
    
    baa_free_token(token);
    baa_free_lexer(lexer);
    
    // Test uppercase 'X'
    const wchar_t* hex_upper = L"0X1A3F";
    lexer = baa_create_lexer(hex_upper);
    token = baa_scan_token(lexer);
    
    assert(token->type == BAA_TOKEN_INT_LIT);
    printf("Hexadecimal format (uppercase X) test passed.\n");
    
    baa_free_token(token);
    baa_free_lexer(lexer);
    
    // Test mixed case hex digits
    const wchar_t* hex_mixed = L"0xaBcD";
    lexer = baa_create_lexer(hex_mixed);
    token = baa_scan_token(lexer);
    
    assert(token->type == BAA_TOKEN_INT_LIT);
    printf("Hexadecimal format (mixed case digits) test passed.\n");
    
    baa_free_token(token);
    baa_free_lexer(lexer);
}

void test_binary_format() {
    // Test lowercase 'b'
    const wchar_t* bin_lower = L"0b1010";
    BaaLexer* lexer = baa_create_lexer(bin_lower);
    BaaToken* token = baa_scan_token(lexer);
    
    assert(token->type == BAA_TOKEN_INT_LIT);
    printf("Binary format (lowercase b) test passed.\n");
    
    baa_free_token(token);
    baa_free_lexer(lexer);
    
    // Test uppercase 'B'
    const wchar_t* bin_upper = L"0B1100";
    lexer = baa_create_lexer(bin_upper);
    token = baa_scan_token(lexer);
    
    assert(token->type == BAA_TOKEN_INT_LIT);
    printf("Binary format (uppercase B) test passed.\n");
    
    baa_free_token(token);
    baa_free_lexer(lexer);
}

void test_error_handling() {
    // Test invalid hex digit
    const wchar_t* invalid_hex = L"0xG";
    BaaLexer* lexer = baa_create_lexer(invalid_hex);
    BaaToken* token = baa_scan_token(lexer);
    
    assert(token->type == BAA_TOKEN_ERROR);
    printf("Invalid hex digit error test passed.\n");
    
    baa_free_token(token);
    baa_free_lexer(lexer);
    
    // Test invalid binary digit
    const wchar_t* invalid_bin = L"0b102";
    lexer = baa_create_lexer(invalid_bin);
    token = baa_scan_token(lexer);
    
    assert(token->type == BAA_TOKEN_ERROR);
    printf("Invalid binary digit error test passed.\n");
    
    baa_free_token(token);
    baa_free_lexer(lexer);
    
    // Test invalid exponent in scientific notation
    const wchar_t* invalid_exp = L"1.2e";
    lexer = baa_create_lexer(invalid_exp);
    token = baa_scan_token(lexer);
    
    assert(token->type == BAA_TOKEN_ERROR);
    printf("Invalid scientific notation exponent test passed.\n");
    
    baa_free_token(token);
    baa_free_lexer(lexer);
}

int main() {
    printf("Running number format tests...\n");
    
    test_scientific_notation();
    test_hexadecimal_format();
    test_binary_format();
    test_error_handling();
    
    printf("All tests passed!\n");
    return 0;
}
