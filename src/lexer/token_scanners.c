#include "baa/lexer/token_scanners.h" // For BaaToken and BaaLexer types, and scan_* declarations
#include "baa/lexer/lexer_internal.h"      // For make_token, make_error_token, peek, advance etc.
#include <stdlib.h>              // For malloc, realloc, free
#include <string.h>              // For wcslen, wcsncpy, wcscpy
#include <wctype.h>              // For iswalnum

// This file contains implementations for:
// BaaToken *scan_identifier(BaaLexer *lexer);
// BaaToken *scan_number(BaaLexer *lexer);
// BaaToken *scan_string(BaaLexer *lexer);
// BaaToken *scan_char_literal(BaaLexer *lexer);

// Array of keywords and their corresponding token types (copied from lexer.c)
// This needs to be accessible here if scan_identifier uses it.
// Alternatively, keyword checking could be a separate utility.
// For now, duplicate for simplicity of this step.
static struct
{
    const wchar_t *keyword;
    BaaTokenType token;
} keywords_for_scanners[] = {
    {L"دالة", BAA_TOKEN_FUNC},
    {L"إرجع", BAA_TOKEN_RETURN},
    {L"إذا", BAA_TOKEN_IF},
    {L"إلا", BAA_TOKEN_ELSE},
    {L"طالما", BAA_TOKEN_WHILE},
    {L"لأجل", BAA_TOKEN_FOR},
    {L"افعل", BAA_TOKEN_DO},
    {L"اختر", BAA_TOKEN_SWITCH},
    {L"حالة", BAA_TOKEN_CASE},
    {L"توقف", BAA_TOKEN_BREAK},
    {L"استمر", BAA_TOKEN_CONTINUE},
    {L"متغير", BAA_TOKEN_VAR},
    {L"ثابت", BAA_TOKEN_CONST},
    {L"صحيح", BAA_TOKEN_BOOL_LIT},
    {L"خطأ", BAA_TOKEN_BOOL_LIT},
    {L"عدد_صحيح", BAA_TOKEN_TYPE_INT},
    {L"عدد_حقيقي", BAA_TOKEN_TYPE_FLOAT},
    {L"حرف", BAA_TOKEN_TYPE_CHAR},
    {L"فراغ", BAA_TOKEN_TYPE_VOID},
    {L"منطقي", BAA_TOKEN_TYPE_BOOL}
};


BaaToken *scan_identifier(BaaLexer *lexer)
{
    // lexer->start is already set before calling this function
    while (iswalnum(peek(lexer)) || peek(lexer) == L'_' ||
           is_arabic_letter(peek(lexer)) || is_arabic_digit(peek(lexer)))
    {
        advance(lexer);
    }

    // Check if identifier is a keyword
    size_t length = lexer->current - lexer->start;
    for (size_t i = 0; i < sizeof(keywords_for_scanners) / sizeof(keywords_for_scanners[0]); i++)
    {
        if (wcslen(keywords_for_scanners[i].keyword) == length &&
            wcsncmp(&lexer->source[lexer->start], keywords_for_scanners[i].keyword, length) == 0)
        {
            return make_token(lexer, keywords_for_scanners[i].token);
        }
    }

    return make_token(lexer, BAA_TOKEN_IDENTIFIER);
}

BaaToken *scan_number(BaaLexer *lexer)
{
    // lexer->start is already set
    bool is_float = false;
    wchar_t base_prefix = 0;
    bool (*is_valid_digit)(wchar_t) = is_baa_digit;
    size_t number_start_index = lexer->current;

    if (peek(lexer) == L'0')
    {
        wchar_t next = peek_next(lexer);
        if (next == L'x' || next == L'X')
        {
            base_prefix = 'x';
            is_valid_digit = is_baa_hex_digit;
            advance(lexer);
            advance(lexer);
            number_start_index = lexer->current;
            if (!is_valid_digit(peek(lexer)))
            {
                BaaToken *error_token = make_error_token(lexer, L"عدد سداسي عشر غير صالح: يجب أن يتبع البادئة 0x/0X رقم سداسي عشري واحد على الأقل (السطر %zu، العمود %zu)", lexer->line, lexer->column);
                synchronize(lexer);
                return error_token;
            }
        }
        else if (next == L'b' || next == L'B')
        {
            base_prefix = 'b';
            is_valid_digit = is_baa_bin_digit;
            advance(lexer);
            advance(lexer);
            number_start_index = lexer->current;
            if (!is_valid_digit(peek(lexer)))
            {
                BaaToken *error_token = make_error_token(lexer, L"عدد ثنائي غير صالح: يجب أن يتبع البادئة 0b/0B رقم ثنائي واحد على الأقل (السطر %zu، العمود %zu)", lexer->line, lexer->column);
                synchronize(lexer);
                return error_token;
            }
        }
    }

    bool last_char_was_underscore = false;
    while (is_valid_digit(peek(lexer)) || peek(lexer) == L'_')
    {
        if (peek(lexer) == L'_')
        {
            if (last_char_was_underscore || lexer->current == number_start_index)
            {
                BaaToken *error_token = make_error_token(lexer, L"شرطة سفلية غير صالحة في العدد (السطر %zu، العمود %zu): لا يمكن أن تكون متتالية أو في بداية الرقم بعد البادئة.", lexer->line, lexer->column);
                synchronize(lexer);
                return error_token;
            }
            last_char_was_underscore = true;
        }
        else
        {
            if (!is_valid_digit(peek(lexer))) break;
            last_char_was_underscore = false;
        }
        advance(lexer);
    }

    if (last_char_was_underscore)
    {
        BaaToken *error_token = make_error_token(lexer, L"شرطة سفلية غير صالحة في العدد (السطر %zu، العمود %zu): لا يمكن أن تكون في نهاية الرقم.", lexer->line, lexer->column - 1);
        synchronize(lexer);
        return error_token;
    }

    wchar_t current_peek = peek(lexer);
    if (base_prefix == 0 && (current_peek == L'.' || current_peek == 0x066B ))
    {
        wchar_t next_peek = peek_next(lexer);
        if (next_peek == L'_')
        {
            BaaToken *error_token = make_error_token(lexer, L"شرطة سفلية غير صالحة في العدد (السطر %zu، العمود %zu): لا يمكن أن تتبع الفاصلة العشرية مباشرة.", lexer->line, lexer->column + 1);
            synchronize(lexer);
            return error_token;
        }
        if (is_baa_digit(next_peek))
        {
            is_float = true;
            advance(lexer);
            last_char_was_underscore = false;
            while (is_baa_digit(peek(lexer)) || peek(lexer) == L'_')
            {
                if (peek(lexer) == L'_')
                {
                    if (last_char_was_underscore)
                    {
                        BaaToken *error_token = make_error_token(lexer, L"شرطة سفلية غير صالحة في العدد (السطر %zu، العمود %zu): لا يمكن أن تكون متتالية.", lexer->line, lexer->column);
                        synchronize(lexer);
                        return error_token;
                    }
                    last_char_was_underscore = true;
                }
                else
                {
                    if (!is_baa_digit(peek(lexer))) break;
                    last_char_was_underscore = false;
                }
                advance(lexer);
            }
            if (last_char_was_underscore)
            {
                BaaToken *error_token = make_error_token(lexer, L"شرطة سفلية غير صالحة في العدد (السطر %zu، العمود %zu): لا يمكن أن تكون في نهاية الجزء الكسري.", lexer->line, lexer->column - 1);
                synchronize(lexer);
                return error_token;
            }
        }
    }

    current_peek = peek(lexer);
    if (base_prefix == 0 && (current_peek == L'e' || current_peek == L'E'))
    {
        wchar_t next_peek_exp = peek_next(lexer);
        if (next_peek_exp == L'_')
        {
            BaaToken *error_token = make_error_token(lexer, L"شرطة سفلية غير صالحة في العدد (السطر %zu، العمود %zu): لا يمكن أن تتبع علامة الأس 'e'/'E' مباشرة.", lexer->line, lexer->column + 1);
            synchronize(lexer);
            return error_token;
        }
        bool has_exponent_part = false;
        int sign_offset = 0;
        if (next_peek_exp == L'+' || next_peek_exp == L'-')
        {
            sign_offset = 1;
            wchar_t after_sign_peek = lexer->source[lexer->current + 2];
            if (after_sign_peek == L'_')
            {
                 BaaToken *error_token = make_error_token(lexer, L"شرطة سفلية غير صالحة في العدد (السطر %zu، العمود %zu): لا يمكن أن تتبع علامة الأس (+/-) مباشرة.", lexer->line, lexer->column + 2);
                synchronize(lexer);
                return error_token;
            }
            if (is_baa_digit(after_sign_peek)) has_exponent_part = true;
        }
        else if (is_baa_digit(next_peek_exp))
        {
            has_exponent_part = true;
        }

        if (has_exponent_part)
        {
            is_float = true;
            advance(lexer);
            if (sign_offset == 1) advance(lexer);
            if (!is_baa_digit(peek(lexer)))
            {
                BaaToken *error_token = make_error_token(lexer, L"تنسيق أس غير صالح بعد 'e' أو 'E' (السطر %zu، العمود %zu)", lexer->line, lexer->column);
                synchronize(lexer);
                return error_token;
            }
            last_char_was_underscore = false;
            while (is_baa_digit(peek(lexer)) || peek(lexer) == L'_')
            {
                if (peek(lexer) == L'_')
                {
                    if (last_char_was_underscore) {
                        BaaToken *error_token = make_error_token(lexer, L"شرطة سفلية غير صالحة في العدد (السطر %zu، العمود %zu): لا يمكن أن تكون متتالية في الأس.", lexer->line, lexer->column);
                        synchronize(lexer);
                        return error_token;
                    }
                    last_char_was_underscore = true;
                }
                else
                {
                    if (!is_baa_digit(peek(lexer))) break;
                    last_char_was_underscore = false;
                }
                advance(lexer);
            }
            if (last_char_was_underscore)
            {
                BaaToken *error_token = make_error_token(lexer, L"شرطة سفلية غير صالحة في العدد (السطر %zu، العمود %zu): لا يمكن أن تكون في نهاية الأس.", lexer->line, lexer->column - 1);
                synchronize(lexer);
                return error_token;
            }
        }
    }

    if (base_prefix != 0 && is_float)
    {
        BaaToken *error_token = make_error_token(lexer, L"الأعداد السداسية عشرية والثنائية لا يمكن أن تكون أعدادًا حقيقية (السطر %zu، العمود %zu)", lexer->line, number_start_index);
        synchronize(lexer);
        return error_token;
    }
    return make_token(lexer, is_float ? BAA_TOKEN_FLOAT_LIT : BAA_TOKEN_INT_LIT);
}

BaaToken *scan_string(BaaLexer *lexer)
{
    size_t buffer_cap = 64;
    size_t buffer_len = 0;
    wchar_t *buffer = malloc(buffer_cap * sizeof(wchar_t));
    if (!buffer)
    {
        return make_error_token(lexer, L"فشل في تخصيص ذاكرة لسلسلة نصية (السطر %zu)", lexer->line);
    }
    size_t start_line = lexer->line;
    size_t start_col = lexer->column;

    while (peek(lexer) != L'"' && !is_at_end(lexer))
    {
        wchar_t c = peek(lexer);
        if (c == L'\\')
        {
            advance(lexer);
            if (is_at_end(lexer)) break;
            wchar_t escaped_char = advance(lexer);
            switch (escaped_char)
            {
            case L'n': append_char_to_buffer(&buffer, &buffer_len, &buffer_cap, L'\n'); break;
            case L't': append_char_to_buffer(&buffer, &buffer_len, &buffer_cap, L'\t'); break;
            case L'\\': append_char_to_buffer(&buffer, &buffer_len, &buffer_cap, L'\\'); break;
            case L'"': append_char_to_buffer(&buffer, &buffer_len, &buffer_cap, L'"'); break;
            case L'r': append_char_to_buffer(&buffer, &buffer_len, &buffer_cap, L'\r'); break;
            case L'0': append_char_to_buffer(&buffer, &buffer_len, &buffer_cap, L'\0'); break;
            case L'u':
            {
                int value = scan_hex_escape(lexer, 4);
                if (value == -1)
                {
                    free(buffer);
                    BaaToken *error_token = make_error_token(lexer, L"تسلسل هروب يونيكود غير صالح (\\uXXXX) في السطر %zu، العمود %zu", start_line, start_col);
                    synchronize(lexer);
                    return error_token;
                }
                append_char_to_buffer(&buffer, &buffer_len, &buffer_cap, (wchar_t)value);
                break;
            }
            default:
                free(buffer);
                BaaToken *error_token = make_error_token(lexer, L"تسلسل هروب غير صالح '\%lc' في سلسلة نصية (السطر %zu)", escaped_char, start_line);
                synchronize(lexer);
                return error_token;
            }
            if (buffer == NULL) return make_error_token(lexer, L"فشل في إعادة تخصيص ذاكرة لسلسلة نصية (السطر %zu)", start_line);
        }
        else
        {
            if (c == L'\n') { lexer->line++; lexer->column = 0; }
            append_char_to_buffer(&buffer, &buffer_len, &buffer_cap, c);
            if (buffer == NULL) return make_error_token(lexer, L"فشل في إعادة تخصيص ذاكرة لسلسلة نصية (السطر %zu)", start_line);
            advance(lexer);
        }
    }

    if (is_at_end(lexer) || peek(lexer) != L'"')
    {
        free(buffer);
        BaaToken *error_token = make_error_token(lexer, L"سلسلة نصية غير منتهية (بدأت في السطر %zu، العمود %zu)", start_line, start_col);
        if (!is_at_end(lexer)) synchronize(lexer);
        return error_token;
    }
    advance(lexer);
    append_char_to_buffer(&buffer, &buffer_len, &buffer_cap, L'\0');
    if (buffer == NULL) return make_error_token(lexer, L"فشل في إعادة تخصيص الذاكرة عند إنهاء السلسلة النصية (بدأت في السطر %zu)", start_line);

    BaaToken *token = malloc(sizeof(BaaToken));
    if (!token) { free(buffer); return NULL; }
    token->type = BAA_TOKEN_STRING_LIT;
    token->lexeme = buffer;
    token->length = buffer_len - 1;
    token->line = start_line;
    token->column = start_col;
    return token;
}

BaaToken *scan_char_literal(BaaLexer *lexer)
{
    size_t start_line = lexer->line;
    size_t start_col = lexer->column -1; // Opening ' was consumed
    wchar_t value_char;

    if (is_at_end(lexer))
    {
        return make_error_token(lexer, L"قيمة حرفية غير منتهية (EOF بعد \' )");
    }

    if (peek(lexer) == L'\\')
    {
        advance(lexer);
        if (is_at_end(lexer))
        {
            return make_error_token(lexer, L"تسلسل هروب غير منته في قيمة حرفية (EOF بعد '\' في السطر %zu، العمود %zu)", start_line, start_col);
        }
        wchar_t escape = advance(lexer);
        switch (escape)
        {
        case L'n': value_char = L'\n'; break;
        case L't': value_char = L'\t'; break;
        case L'\\': value_char = L'\\'; break;
        case L'\'': value_char = L'\''; break;
        case L'"': value_char = L'"'; break;
        case L'r': value_char = L'\r'; break;
        case L'0': value_char = L'\0'; break;
        case L'u':
        {
            int value = scan_hex_escape(lexer, 4);
            if (value == -1)
            {
                BaaToken *error_token = make_error_token(lexer, L"تسلسل هروب يونيكود غير صالح (\\uXXXX) في قيمة حرفية (بدأت في السطر %zu، العمود %zu)", start_line, start_col);
                synchronize(lexer);
                return error_token;
            }
            value_char = (wchar_t)value;
            break;
        }
        default:
            BaaToken *error_token = make_error_token(lexer, L"تسلسل هروب غير صالح '\%lc' في قيمة حرفية (بدأت في السطر %zu، العمود %zu)", escape, start_line, start_col);
            synchronize(lexer);
            return error_token;
        }
    }
    else
    {
        value_char = advance(lexer);
        if (value_char == L'\n')
        {
            BaaToken *error_token = make_error_token(lexer, L"سطر جديد غير مسموح به في قيمة حرفية (بدأت في السطر %zu، العمود %zu)", start_line, start_col);
            synchronize(lexer);
            return error_token;
        }
        if (value_char == L'\'')
        {
            BaaToken *error_token = make_error_token(lexer, L"قيمة حرفية فارغة ('') (بدأت في السطر %zu، العمود %zu)", start_line, start_col);
            synchronize(lexer);
            return error_token;
        }
    }

    if (peek(lexer) == L'\'')
    {
        advance(lexer);
        // The make_token function will copy the raw lexeme including quotes and escape sequences.
        // The actual value_char is not directly stored in the token's lexeme by make_token.
        // If the interpreted value is needed, it should be handled by the parser/consumer.
        return make_token(lexer, BAA_TOKEN_CHAR_LIT);
    }
    else
    {
        if (is_at_end(lexer))
        {
            return make_error_token(lexer, L"قيمة حرفية غير منتهية (علامة اقتباس أحادية ' مفقودة، بدأت في السطر %zu، العمود %zu)", start_line, start_col);
        }
        else
        {
            BaaToken *error_token = make_error_token(lexer, L"قيمة حرفية غير صالحة (متعددة الأحرف أو علامة اقتباس مفقودة؟ بدأت في السطر %zu، العمود %zu)", start_line, start_col);
            synchronize(lexer);
            return error_token;
        }
    }
}
