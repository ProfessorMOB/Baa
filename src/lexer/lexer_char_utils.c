#include "baa/lexer/lexer_char_utils.h"

bool is_arabic_letter(wchar_t c)
{
    return (c >= 0x0600 && c <= 0x06FF) || // Basic Arabic
           (c >= 0xFB50 && c <= 0xFDFF) || // Arabic Presentation Forms-A
           (c >= 0xFE70 && c <= 0xFEFF);   // Arabic Presentation Forms-B
}

bool is_arabic_digit(wchar_t c)
{
    return (c >= 0x0660 && c <= 0x0669); // Arabic-Indic digits
}

bool is_baa_digit(wchar_t c)
{
    return iswdigit(c) || is_arabic_digit(c);
}

bool is_baa_bin_digit(wchar_t c)
{
    return c == L'0' || c == L'1';
}

bool is_baa_hex_digit(wchar_t c)
{
    return (c >= L'0' && c <= L'9') ||
           (c >= L'a' && c <= L'f') ||
           (c >= L'A' && c <= L'F');
}

bool is_arabic_punctuation(wchar_t c)
{
    return (c == 0x060C) || // Arabic comma
           (c == 0x061B) || // Arabic semicolon
           (c == 0x061F) || // Arabic question mark
           (c == 0x066D);   // Arabic five pointed star
}
