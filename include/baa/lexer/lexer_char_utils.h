#ifndef BAA_LEXER_CHAR_UTILS_H
#define BAA_LEXER_CHAR_UTILS_H

#include <wchar.h>
#include <stdbool.h>
#include <wctype.h> // For iswdigit

// Function declarations
bool is_arabic_letter(wchar_t c);
bool is_arabic_digit(wchar_t c);
bool is_baa_digit(wchar_t c); // Helper to check ASCII or Arabic-Indic digits
bool is_baa_bin_digit(wchar_t c); // Helper for binary digits
bool is_baa_hex_digit(wchar_t c); // Helper for hexadecimal digits
bool is_arabic_punctuation(wchar_t c);

#endif // BAA_LEXER_CHAR_UTILS_H
