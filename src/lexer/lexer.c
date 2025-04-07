#include "baa/lexer/lexer.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h> // For error messages
#include <string.h>
#include <wctype.h>
#include <stdarg.h> // Needed for va_list, etc.

// Forward declaration for error token creation
static BaaToken *make_error_token(BaaLexer *lexer, const wchar_t *format, ...);
static BaaToken *scan_identifier(BaaLexer *lexer);
static BaaToken *scan_number(BaaLexer *lexer);
static BaaToken *scan_string(BaaLexer *lexer);
static BaaToken *scan_char_literal(BaaLexer *lexer); // <-- Add forward declaration

// Array of keywords and their corresponding token types
static struct
{
    const wchar_t *keyword;
    BaaTokenType token;
} keywords[] = {
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
    {L"متغير", BAA_TOKEN_VAR},            // Keyword for variable declaration
    {L"ثابت", BAA_TOKEN_CONST},           // Keyword for constant declaration
    {L"صحيح", BAA_TOKEN_BOOL_LIT},        // Boolean literal true
    {L"خطأ", BAA_TOKEN_BOOL_LIT},         // Boolean literal false
    {L"عدد_صحيح", BAA_TOKEN_TYPE_INT},    // Type keyword: integer
    {L"عدد_حقيقي", BAA_TOKEN_TYPE_FLOAT}, // Type keyword: float
    {L"حرف", BAA_TOKEN_TYPE_CHAR},        // Type keyword: char
    {L"فراغ", BAA_TOKEN_TYPE_VOID},       // Type keyword: void
    {L"منطقي", BAA_TOKEN_TYPE_BOOL}       // Type keyword: boolean
};

// Helper functions
static bool is_arabic_letter(wchar_t c)
{
    return (c >= 0x0600 && c <= 0x06FF) || (c >= 0xFB50 && c <= 0xFDFF) ||
           (c >= 0xFE70 && c <= 0xFEFF); // Basic Arabic, Arabic Presentation Forms-A and B
}

static bool is_arabic_digit(wchar_t c)
{
    return (c >= 0x0660 && c <= 0x0669); // Arabic-Indic digits
}

// Helper to check if a character is a valid digit (ASCII or Arabic-Indic)
static inline bool is_baa_digit(wchar_t c)
{
    return iswdigit(c) || is_arabic_digit(c);
}

// Helper to check if a character is a valid binary digit ('0' or '1')
static inline bool is_baa_bin_digit(wchar_t c)
{
    return c == L'0' || c == L'1';
}

// Helper to check if a character is a valid hexadecimal digit (0-9, a-f, A-F)
static inline bool is_baa_hex_digit(wchar_t c)
{
    return (c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f') || (c >= L'A' && c <= L'F');
}

static bool is_arabic_punctuation(wchar_t c)
{
    return (c == 0x060C) || // Arabic comma
           (c == 0x061B) || // Arabic semicolon
           (c == 0x061F) || // Arabic question mark
           (c == 0x066D);   // Arabic five pointed star
}

static bool is_at_end(BaaLexer *lexer)
{
    return lexer->source[lexer->current] == L'\0';
}

static wchar_t peek(BaaLexer *lexer)
{
    if (is_at_end(lexer))
        return L'\0';
    return lexer->source[lexer->current];
}

static wchar_t peek_next(BaaLexer *lexer)
{
    if (is_at_end(lexer))
        return L'\0';
    return lexer->source[lexer->current + 1];
}

static wchar_t advance(BaaLexer *lexer)
{
    if (is_at_end(lexer))
        return L'\0';
    wchar_t c = lexer->source[lexer->current++];
    if (c == L'\n')
    {
        lexer->line++;
        lexer->column = 0;
    }
    else
    {
        lexer->column++;
    }
    return c;
}

static bool match(BaaLexer *lexer, wchar_t expected)
{
    if (is_at_end(lexer))
        return false;
    if (lexer->source[lexer->current] != expected)
        return false;
    advance(lexer);
    return true;
}

// Creates a token by copying the lexeme from the source
static BaaToken *make_token(BaaLexer *lexer, BaaTokenType type)
{
    BaaToken *token = malloc(sizeof(BaaToken));
    if (!token)
    {
        // Major issue: Cannot even allocate memory for a token
        fprintf(stderr, "FATAL: Failed to allocate memory for token.\n");
        // In a real scenario, might need a more robust way to signal this
        // For now, returning NULL, caller should check.
        return NULL;
    }

    token->type = type;
    token->length = lexer->current - lexer->start;
    token->lexeme = malloc((token->length + 1) * sizeof(wchar_t));
    if (!token->lexeme)
    {
        fprintf(stderr, "FATAL: Failed to allocate memory for token lexeme.\n");
        free(token);
        return NULL;
    }
    wcsncpy((wchar_t *)token->lexeme, &lexer->source[lexer->start], token->length);
    ((wchar_t *)token->lexeme)[token->length] = L'\0'; // Null-terminate
    token->line = lexer->line;
    // Calculate column based on start position, accounting for line breaks within token?
    // Simple approach: column where token starts.
    // A more precise column might require tracking start column explicitly.
    token->column = lexer->column - token->length; // Approximate start column

    return token;
}

// Creates an error token with a formatted message (dynamically allocated)
static BaaToken *make_error_token(BaaLexer *lexer, const wchar_t *format, ...)
{
    BaaToken *token = malloc(sizeof(BaaToken));
    if (!token)
    {
        fprintf(stderr, "FATAL: Failed to allocate memory for error token struct.\n");
        return NULL;
    }

    wchar_t* buffer = NULL;
    size_t buffer_size = 0;
    va_list args, args_copy;

    va_start(args, format);
    va_copy(args_copy, args); // Copy va_list for the size calculation

    // Calculate required buffer size using vswprintf with NULL buffer
    // On Windows, _vsnwprintf returns -1 if buffer is too small.
    // On POSIX, vsnwprintf returns the number of chars that *would* have been written.
    // We need a portable approach or platform checks.
    // Let's assume a reasonable initial size and reallocate if needed (safer).

    size_t initial_size = 256;
    buffer = malloc(initial_size * sizeof(wchar_t));
    if (!buffer) {
         fprintf(stderr, "FATAL: Failed to allocate initial memory for error message.\n");
         va_end(args_copy);
         va_end(args);
         free(token);
         return NULL;
    }

    // Use vsnwprintf (or _vsnwprintf on Windows) which is safer
#ifdef _WIN32
    int needed = _vsnwprintf(buffer, initial_size, format, args);
    // If -1, it means buffer was too small (or other error). We won't handle resizing for simplicity here.
    if (needed < 0) {
         // Fallback or error
         wcscpy(buffer, L"خطأ غير معروف في تنسيق رسالة الخطأ الداخلية."); // Fallback message
         needed = wcslen(buffer);
    }
#else
    int needed = vswprintf(buffer, initial_size, format, args);
    // If needed >= initial_size, buffer was too small. Reallocate.
    if (needed < 0 || (size_t)needed >= initial_size) {
        // Need to calculate the *actual* required size now
        va_end(args); // End the first va_list
        needed = vswprintf(NULL, 0, format, args_copy); // POSIX way to get size
        va_end(args_copy); // End the copied va_list

        if (needed < 0) { // Error during size calculation
             wcscpy(buffer, L"خطأ غير معروف في تنسيق رسالة الخطأ الداخلية."); // Fallback message
             needed = wcslen(buffer);
        } else {
            buffer_size = (size_t)needed + 1; // +1 for null terminator
            wchar_t* new_buffer = realloc(buffer, buffer_size * sizeof(wchar_t));
            if (!new_buffer) {
                fprintf(stderr, "FATAL: Failed to reallocate memory for error message.\n");
                free(buffer);
                free(token);
                return NULL;
            }
            buffer = new_buffer;
            // Try formatting again with the correct size
            va_start(args, format); // Need to restart the va_list
            needed = vswprintf(buffer, buffer_size, format, args);
            va_end(args);
            // If it still fails here, something is very wrong
            if (needed < 0) {
                 wcscpy(buffer, L"خطأ فادح في تنسيق رسالة الخطأ الداخلية."); // Final fallback
                 needed = wcslen(buffer);
            }
        }
    } else {
         va_end(args_copy); // End the copied va_list if not used
         va_end(args); // End the va_list used for formatting
    }
#endif

    token->type = BAA_TOKEN_ERROR;
    // IMPORTANT: The caller of baa_free_token MUST now free this lexeme
    token->lexeme = buffer;
    token->length = wcslen(buffer); // Use actual length
    token->line = lexer->line;
    token->column = lexer->column; // Error occurs at current position
    return token;
}

// Returns an error token if an unterminated /* comment is found, otherwise NULL.
static BaaToken* skip_whitespace(BaaLexer *lexer)
{
    for (;;)
    {
        wchar_t c = peek(lexer);
        switch (c)
        {
        case L' ':
        case L'\r':
        case L'\t':
            advance(lexer);
            break;
        case L'\n':
            advance(lexer);
            break;
        case L'#':
            // Skip single-line #-style comments
            while (peek(lexer) != L'\n' && !is_at_end(lexer))
                advance(lexer);
            break;
        case L'/':
            if (peek_next(lexer) == L'*') { // Start of /* comment */
                size_t start_line = lexer->line;
                size_t start_col = lexer->column;
                advance(lexer); // Consume '/'
                advance(lexer); // Consume '*'
                while (!(peek(lexer) == L'*' && peek_next(lexer) == L'/') && !is_at_end(lexer)) {
                    // Need to handle newline increments within the comment
                    if (peek(lexer) == L'\n') {
                        // advance() handles line increment and column reset
                        advance(lexer);
                    } else {
                        advance(lexer);
                    }
                }
                if (!is_at_end(lexer)) {
                    advance(lexer); // Consume '*'
                    advance(lexer); // Consume '/'
                } else {
                    // Reached EOF without closing comment - error!
                    BaaToken* error_token = make_error_token(lexer, L"تعليق متعدد الأسطر غير منتهٍ (بدأ في السطر %zu، العمود %zu)",
                                                             start_line, start_col);
                    // No synchronization needed at EOF
                    return error_token;
                }
            } else if (peek_next(lexer) == L'/') { // Start of // comment
                 // Skip // style comments
                 while (peek(lexer) != L'\n' && !is_at_end(lexer))
                     advance(lexer);
                 // No need to consume newline here, outer loop handles it.
            } else {
                 // Just a slash, not a comment start
                 return NULL; // Finished skipping whitespace/comments
            }
            break;
        default:
            return NULL; // Finished skipping whitespace/comments
        }
    }
}

long baa_file_size(FILE *file)
{
    if (!file)
    {
        return 0;
    }
    fpos_t original = 0;
    if (fgetpos(file, &original) != 0)
    {
        printf("fgetpos() فشلت دالة: %i \n", errno);
        return 0;
    }
    fseek(file, 0, SEEK_END);
    long out = ftell(file);
    if (fsetpos(file, &original) != 0)
    {
        printf("fsetpos() فشلت دالة: %i \n", errno);
    }
    return out;
}

wchar_t *baa_file_content(const wchar_t *path)
{
    FILE *file;
    errno_t err = _wfopen_s(&file, path, L"rb, ccs=UTF-16LE");
    if (err != 0)
    {
        printf("لا يمكن فتح الملف\n");
        return NULL;
    }

    // Skip BOM if present
    wchar_t bom;
    if (fread(&bom, sizeof(wchar_t), 1, file) == 1)
    {
        if (bom != 0xFEFF)
        {
            fseek(file, 0, SEEK_SET);
        }
    }

    long file_size = baa_file_size(file);
    size_t char_count = (file_size / sizeof(wchar_t)) + 1; // +1 for null terminator
    wchar_t *contents = malloc(char_count * sizeof(wchar_t));

    if (!contents)
    {
        fclose(file);
        return NULL;
    }

    size_t chars_read = fread(contents, sizeof(wchar_t), char_count - 1, file);
    contents[chars_read] = L'\0';

    fclose(file);
    return contents;
}

// Corresponds to baa_create_lexer in header
BaaLexer *baa_create_lexer(const wchar_t *source)
{
    BaaLexer *lexer = malloc(sizeof(BaaLexer));
    if (!lexer)
        return NULL; // Handle allocation failure
    lexer->source = source;
    lexer->start = 0;
    lexer->current = 0;
    lexer->line = 1;
    lexer->column = 0;
    // Note: No had_error field in the struct per lexer.h
    return lexer;
}

// Corresponds to baa_free_lexer in header
void baa_free_lexer(BaaLexer *lexer)
{
    // The lexer itself doesn't own the source string, so we don't free lexer->source.
    // It also doesn't own the current token's lexeme directly.
    // Freeing tokens is handled separately by baa_free_token.
    if (lexer)
    {
        free(lexer);
    }
}

// Corresponds to baa_free_token in header
void baa_free_token(BaaToken *token)
{
    if (token)
    {
        // Error tokens now have dynamically allocated messages, so always try to free lexeme.
        // make_token also dynamically allocates lexeme.
        if (token->lexeme)
        {
            // Cast needed because lexeme is const in the struct, but we allocated it.
            // Note: For original static error messages, this would be wrong, but
            // make_error_token now always allocates.
            free((wchar_t *)token->lexeme);
        }
        free(token);
    }
}

static BaaToken *scan_identifier(BaaLexer *lexer)
{
    // lexer->start is already set before calling this function
    while (iswalnum(peek(lexer)) || peek(lexer) == L'_' ||
           is_arabic_letter(peek(lexer)) || is_arabic_digit(peek(lexer)))
    {
        advance(lexer);
    }

    // Check if identifier is a keyword
    size_t length = lexer->current - lexer->start; // Correct length calculation
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++)
    {
        // Compare length first for efficiency
        if (wcslen(keywords[i].keyword) == length &&
            // Use lexer->start for the beginning of the potential keyword
            wcsncmp(&lexer->source[lexer->start], keywords[i].keyword, length) == 0)
        {
            return make_token(lexer, keywords[i].token);
        }
    }

    return make_token(lexer, BAA_TOKEN_IDENTIFIER);
}

static BaaToken *scan_number(BaaLexer *lexer)
{
    // lexer->start is already set
    bool is_float = false;
    wchar_t base_prefix = 0; // 0 for decimal, 'b' for binary, 'x' for hex
    bool (*is_valid_digit)(wchar_t) = is_baa_digit; // Default to decimal/arabic
    size_t number_start_index = lexer->current; // Where digits actually start

    // 1. Check for base prefixes (0x, 0b)
    if (peek(lexer) == L'0')
    {
        wchar_t next = peek_next(lexer);
        if (next == L'x' || next == L'X')
        {
            base_prefix = 'x';
            is_valid_digit = is_baa_hex_digit;
            advance(lexer); // Consume '0'
            advance(lexer); // Consume 'x' or 'X'
            number_start_index = lexer->current;
            // Must have at least one hex digit after 0x/0X
            if (!is_valid_digit(peek(lexer))) {
                 BaaToken* error_token = make_error_token(lexer, L"عدد سداسي عشر غير صالح: يجب أن يتبع البادئة 0x/0X رقم سداسي عشري واحد على الأقل (السطر %zu، العمود %zu)", lexer->line, lexer->column);
                 synchronize(lexer);
                 return error_token;
            }
        }
        else if (next == L'b' || next == L'B')
        {
            base_prefix = 'b';
            is_valid_digit = is_baa_bin_digit;
            advance(lexer); // Consume '0'
            advance(lexer); // Consume 'b' or 'B'
            number_start_index = lexer->current;
            // Must have at least one binary digit after 0b/0B
             if (!is_valid_digit(peek(lexer))) {
                 BaaToken* error_token = make_error_token(lexer, L"عدد ثنائي غير صالح: يجب أن يتبع البادئة 0b/0B رقم ثنائي واحد على الأقل (السطر %zu، العمود %zu)", lexer->line, lexer->column);
                 synchronize(lexer);
                 return error_token;
            }
        }
        // If just '0' followed by non-prefix char, it's treated as decimal 0
        // (No octal support currently)
    }

    // 2. Scan the main part of the number (integer part before dot/exponent)
    bool last_char_was_underscore = false;
    while (is_valid_digit(peek(lexer)) || peek(lexer) == L'_')
    {
        if (peek(lexer) == L'_')
        {
            if (last_char_was_underscore || lexer->current == number_start_index) {
                // Error: Consecutive underscores or underscore at the very start (after prefix)
                BaaToken* error_token = make_error_token(lexer, L"شرطة سفلية غير صالحة في العدد (السطر %zu، العمود %zu): لا يمكن أن تكون متتالية أو في بداية الرقم بعد البادئة.", lexer->line, lexer->column);
                synchronize(lexer);
                return error_token;
            }
            last_char_was_underscore = true;
        }
        else
        {
            // Must be a valid digit for the current base
             if (!is_valid_digit(peek(lexer))) {
                // This case should technically not be hit if the base is hex/binary due to the loop condition,
                // but kept for robustness / if base is decimal.
                 break; // Exit loop if it's not a digit or underscore
             }
            last_char_was_underscore = false;
        }
        advance(lexer);
    }

    // Error if number ends with an underscore
    if (last_char_was_underscore)
    {
        BaaToken* error_token = make_error_token(lexer, L"شرطة سفلية غير صالحة في العدد (السطر %zu، العمود %zu): لا يمكن أن تكون في نهاية الرقم.", lexer->line, lexer->column - 1); // Column of the underscore
        synchronize(lexer);
        return error_token;
    }

    // 3. Check for fractional part (only for decimal numbers)
    wchar_t current_peek = peek(lexer);
    if (base_prefix == 0 && (current_peek == L'.' || current_peek == 0x066B /* Arabic Decimal Separator */))
    {
    wchar_t next_peek = peek_next(lexer);
        // Underscore not allowed immediately after dot
        if (next_peek == L'_') {
            BaaToken* error_token = make_error_token(lexer, L"شرطة سفلية غير صالحة في العدد (السطر %zu، العمود %zu): لا يمكن أن تتبع الفاصلة العشرية مباشرة.", lexer->line, lexer->column + 1);
            synchronize(lexer);
            return error_token;
        }

        // Check if there is a digit *after* the decimal point
        if (is_baa_digit(next_peek))
    {
        is_float = true;
        advance(lexer); // Consume the decimal point '.' or '٫'
            last_char_was_underscore = false; // Reset for fractional part

            // Consume digits after decimal point, allowing underscores
            while (is_baa_digit(peek(lexer)) || peek(lexer) == L'_')
            {
                 if (peek(lexer) == L'_')
                 {
                     if (last_char_was_underscore) {
                         BaaToken* error_token = make_error_token(lexer, L"شرطة سفلية غير صالحة في العدد (السطر %zu، العمود %zu): لا يمكن أن تكون متتالية.", lexer->line, lexer->column);
                         synchronize(lexer);
                         return error_token;
                     }
                     // Also check if underscore is the very first char after the dot (already handled above)
                     last_char_was_underscore = true;
                 }
                 else
                 {
                     if (!is_baa_digit(peek(lexer))) break; // Should only be digits here
                     last_char_was_underscore = false;
                 }
            advance(lexer);
        }
             // Error if fractional part ends with an underscore
            if (last_char_was_underscore)
            {
                BaaToken* error_token = make_error_token(lexer, L"شرطة سفلية غير صالحة في العدد (السطر %zu، العمود %zu): لا يمكن أن تكون في نهاية الجزء الكسري.", lexer->line, lexer->column - 1);
                synchronize(lexer);
                return error_token;
            }
        }
        // If it's just a dot not followed by a digit, it might be the dot operator.
        // The lexer doesn't consume the dot here, letting the main loop handle it.
    }

    // 4. Check for exponent part (only for decimal numbers, can follow integer or float part)
    current_peek = peek(lexer); // Re-peek after potentially consuming fractional part
    if (base_prefix == 0 && (current_peek == L'e' || current_peek == L'E'))
    {
        wchar_t next_peek_exp = peek_next(lexer);

        // Underscore not allowed immediately after 'e'/'E'
        if (next_peek_exp == L'_') {
             BaaToken* error_token = make_error_token(lexer, L"شرطة سفلية غير صالحة في العدد (السطر %zu، العمود %zu): لا يمكن أن تتبع علامة الأس 'e'/'E' مباشرة.", lexer->line, lexer->column + 1);
             synchronize(lexer);
             return error_token;
        }

        // Check if there's something valid after 'e'/'E' (digit or sign+digit)
        bool has_exponent_part = false;
        int sign_offset = 0;
        if (next_peek_exp == L'+' || next_peek_exp == L'-') {
            sign_offset = 1;
            wchar_t after_sign_peek = lexer->source[lexer->current + 2]; // Peek two ahead
            // Underscore not allowed immediately after sign in exponent
            if (after_sign_peek == L'_') {
                 BaaToken* error_token = make_error_token(lexer, L"شرطة سفلية غير صالحة في العدد (السطر %zu، العمود %zu): لا يمكن أن تتبع علامة الأس (+/-) مباشرة.", lexer->line, lexer->column + 2);
                 synchronize(lexer);
                 return error_token;
            }
            if (is_baa_digit(after_sign_peek)) {
                 has_exponent_part = true;
             }
        } else if (is_baa_digit(next_peek_exp)) {
            has_exponent_part = true;
        }

        if (has_exponent_part) {
            is_float = true;
            advance(lexer); // Consume 'e' or 'E'

            // Consume optional sign
            if (sign_offset == 1)
            {
                advance(lexer); // Consume '+' or '-'
            }

            // Consume exponent digits, allowing underscores
            // Must have at least one digit after 'e'/'E' or sign
            if (!is_baa_digit(peek(lexer))) {
                 // Should have been caught by has_exponent_part logic, but belt-and-suspenders
                 // This indicates an invalid format like "1e" or "1e+".
                 // The lexer should have already decided based on has_exponent_part.
                 // If we reach here, it implies a logic error above or unexpected input.
                 // For safety, treat as error, though ideally unreachable.
                  BaaToken* error_token = make_error_token(lexer, L"تنسيق أس غير صالح بعد 'e' أو 'E' (السطر %zu، العمود %zu)", lexer->line, lexer->column);
                  synchronize(lexer);
                  return error_token;
            }

            last_char_was_underscore = false; // Reset for exponent part
            while (is_baa_digit(peek(lexer)) || peek(lexer) == L'_')
            {
                 if (peek(lexer) == L'_')
                 {
                      if (last_char_was_underscore) {
                         BaaToken* error_token = make_error_token(lexer, L"شرطة سفلية غير صالحة في العدد (السطر %zu، العمود %zu): لا يمكن أن تكون متتالية في الأس.", lexer->line, lexer->column);
                         synchronize(lexer);
                         return error_token;
                     }
                     // Check if underscore is first char after e/E or sign (already handled above)
                     last_char_was_underscore = true;
                 }
                 else
                 {
                     if (!is_baa_digit(peek(lexer))) break; // Should only be digits here
                     last_char_was_underscore = false;
                 }
                advance(lexer);
            }
             // Error if exponent part ends with an underscore
            if (last_char_was_underscore)
            {
                BaaToken* error_token = make_error_token(lexer, L"شرطة سفلية غير صالحة في العدد (السطر %zu، العمود %zu): لا يمكن أن تكون في نهاية الأس.", lexer->line, lexer->column - 1);
                synchronize(lexer);
                return error_token;
            }
        }
        // If 'e'/'E' is not followed by a valid exponent part, treat 'e'/'E' as separate char (likely identifier)
        // The lexer doesn't consume the 'e'/'E' here in that case.
    }

    // Hex and binary literals cannot be floats
    if (base_prefix != 0 && is_float) {
         // This state should not be reachable due to checks above, but include for safety.
         BaaToken* error_token = make_error_token(lexer, L"الأعداد السداسية عشرية والثنائية لا يمكن أن تكون أعدادًا حقيقية (لا يمكن أن تحتوي على '.' أو 'e') (السطر %zu، العمود %zu)", lexer->line, number_start_index);
         synchronize(lexer);
         return error_token;
    }

    return make_token(lexer, is_float ? BAA_TOKEN_FLOAT_LIT : BAA_TOKEN_INT_LIT);
}

// Helper function to scan exactly 'length' hexadecimal digits
// Returns the integer value, or -1 if not enough digits or invalid digits found.
static int scan_hex_escape(BaaLexer* lexer, int length) {
    int value = 0;
    for (int i = 0; i < length; i++) {
        if (is_at_end(lexer)) return -1; // EOF
        wchar_t c = peek(lexer);
        int digit;
        if (c >= L'0' && c <= L'9') {
            digit = c - L'0';
        } else if (c >= L'a' && c <= L'f') {
            digit = 10 + (c - L'a');
        } else if (c >= L'A' && c <= L'F') {
            digit = 10 + (c - L'A');
        } else {
            return -1; // Not a hex digit
        }
        value = (value * 16) + digit;
        advance(lexer);
    }
    return value;
}

// Helper to add character to dynamic buffer, resizing if needed
static void append_char_to_buffer(wchar_t **buffer, size_t *len, size_t *capacity, wchar_t c)
{
    if (*len + 1 >= *capacity)
    {
        size_t new_capacity = (*capacity == 0) ? 8 : *capacity * 2;
        wchar_t *new_buffer = realloc(*buffer, new_capacity * sizeof(wchar_t));
        if (!new_buffer)
        {
            // Handle realloc failure - crucial!
            fprintf(stderr, "LEXER ERROR: Failed to reallocate string buffer.\n");
            // Cannot continue safely, maybe return NULL or a specific error state?
            // For now, let's try to signal error without crashing.
            // Free the old buffer to prevent leaks if possible.
            free(*buffer);
            *buffer = NULL; // Indicate failure
            *capacity = 0;
            *len = 0;
            return; // Exit the helper function
        }
        *buffer = new_buffer;
        *capacity = new_capacity;
    }
    (*buffer)[(*len)++] = c;
}

static BaaToken *scan_string(BaaLexer *lexer)
{
    size_t buffer_cap = 64; // Initial capacity
    size_t buffer_len = 0;
    wchar_t *buffer = malloc(buffer_cap * sizeof(wchar_t));
    if (!buffer)
    {
        // Handle allocation failure
        // No buffer to free, no synchronization needed as it's a fatal allocation error
        return make_error_token(lexer, L"فشل في تخصيص ذاكرة لسلسلة نصية (السطر %zu)", lexer->line);
    }

    // Store start location for error reporting if unterminated
    size_t start_line = lexer->line;
    size_t start_col = lexer->column;

    while (peek(lexer) != L'"' && !is_at_end(lexer))
    {
        wchar_t c = peek(lexer);
        if (c == L'\\')
        {
            advance(lexer); // Consume '\'
            if (is_at_end(lexer))
                break; // Unterminated escape sequence at EOF

            wchar_t escaped_char = advance(lexer); // Consume character after '\'
            switch (escaped_char)
            {
            case L'n':
                append_char_to_buffer(&buffer, &buffer_len, &buffer_cap, L'\n');
                break;
            case L't':
                append_char_to_buffer(&buffer, &buffer_len, &buffer_cap, L'\t');
                break;
            case L'\\':
                append_char_to_buffer(&buffer, &buffer_len, &buffer_cap, L'\\');
                break;
            case L'"':
                append_char_to_buffer(&buffer, &buffer_len, &buffer_cap, L'"');
                break;
            case L'r': // Carriage Return
                append_char_to_buffer(&buffer, &buffer_len, &buffer_cap, L'\r');
                break;
            case L'0': // Null Character
                append_char_to_buffer(&buffer, &buffer_len, &buffer_cap, L'\0');
                break;
            case L'u': { // Handle Unicode escape \uXXXX
                int value = scan_hex_escape(lexer, 4);
                if (value == -1) {
                    free(buffer);
                    // Note: Using start_line/start_col for location where string started
                    BaaToken* error_token = make_error_token(lexer, L"تسلسل هروب يونيكود غير صالح (\uXXXX) في السطر %zu، العمود %zu", start_line, start_col);
                    synchronize(lexer);
                    return error_token;
                }
                // TODO: Check if value is a valid Unicode code point if necessary?
                append_char_to_buffer(&buffer, &buffer_len, &buffer_cap, (wchar_t)value);
                break;
            }
            // TODO: Add more escape sequences (\r, \0, \xNN) ?
            default:
                // Invalid escape sequence
                free(buffer);
                BaaToken* error_token = make_error_token(lexer, L"تسلسل هروب غير صالح '\%lc' في سلسلة نصية (السطر %zu)", escaped_char, start_line);
                synchronize(lexer);
                return error_token;
            }
            // Check if append_char_to_buffer failed due to realloc error
            if (buffer == NULL)
            {
                // Buffer already freed by helper
                // Fatal allocation error, no sync needed
                return make_error_token(lexer, L"فشل في إعادة تخصيص ذاكرة لسلسلة نصية (السطر %zu)", start_line);
            }
        }
        else
        { // Corrected block structure
            if (c == L'\n')
            { // Track line breaks within the string literal
                lexer->line++;
                lexer->column = 0;
            }
            append_char_to_buffer(&buffer, &buffer_len, &buffer_cap, c);
            // Check if append_char_to_buffer failed due to realloc error
            if (buffer == NULL)
            {
                // Note: Returning error here prevents using buffer later, which is good.
                // Buffer already freed by helper
                // Fatal allocation error, no sync needed
                return make_error_token(lexer, L"فشل في إعادة تخصيص ذاكرة لسلسلة نصية (السطر %zu)", start_line);
            }
            advance(lexer);
        }
    }

    if (is_at_end(lexer) || peek(lexer) != L'"')
    {
        // Unterminated string error
        free(buffer); // Clean up allocated buffer
        BaaToken* error_token = make_error_token(lexer, L"سلسلة نصية غير منتهية (بدأت في السطر %zu، العمود %zu)", start_line, start_col);
        // Sync not needed if EOF caused the termination error
        if (!is_at_end(lexer)) {
             synchronize(lexer);
        }
        return error_token;
    }

    // Consume the closing quote
    advance(lexer);

    // Null-terminate the buffer
    append_char_to_buffer(&buffer, &buffer_len, &buffer_cap, L'\0');
    // Check if append_char_to_buffer failed due to realloc error
    if (buffer == NULL)
    {
        // Buffer is already freed by the helper in case of error
        // Fatal allocation error, no sync needed
        return make_error_token(lexer, L"فشل في إعادة تخصيص الذاكرة عند إنهاء السلسلة النصية (بدأت في السطر %zu)", start_line);
    }

    // Create the token using the interpreted buffer content
    BaaToken *token = malloc(sizeof(BaaToken));
    if (!token)
    {
        fprintf(stderr, "FATAL: Failed to allocate memory for string token.\n");
        free(buffer); // Free the interpreted string buffer
        // Fatal allocation error, no sync needed
        return NULL;
    }

    token->type = BAA_TOKEN_STRING_LIT;
    token->lexeme = buffer;         // Transfer ownership of buffer to token
    token->length = buffer_len - 1; // Exclude null terminator from length
    token->line = start_line;       // String starts at the opening quote line/col
    token->column = start_col;
    // Note: make_token is not used here as it copies from source.
    // lexeme will be freed by baa_free_token.

    return token;
}

/**
 * @brief Scans a character literal (e.g., 'a', '\n', '\\').
 * Assumes the opening ' has already been consumed by advance().
 */
static BaaToken *scan_char_literal(BaaLexer *lexer)
{
    // lexer->start points to the character *after* the opening quote (')
    // Store starting location for better error messages
    size_t start_line = lexer->line;
    // Column needs care: advance() consumed the opening ', so current column is start+1
    // However, make_error_token uses current lexer line/col for error location.
    // We want to report *where the literal started* in the message itself.
    size_t start_col = lexer->column -1; // Column where the opening ' was

    wchar_t value_char; // To hold the interpreted value (though not stored in token directly here)
    bool had_escape = false;

    if (is_at_end(lexer))
    {
        // EOF right after opening '
        BaaToken* error_token = make_error_token(lexer, L"قيمة حرفية غير منتهية (EOF بعد \' )" );
        // No sync needed at EOF
        return error_token;
    }

    // Handle escape sequences
    if (peek(lexer) == L'\\')
    {
        had_escape = true;
        advance(lexer); // Consume '\'
        if (is_at_end(lexer))
        {
            // EOF after '\'
            BaaToken* error_token = make_error_token(lexer, L"تسلسل هروب غير منته في قيمة حرفية (EOF بعد '\' في السطر %zu، العمود %zu)", start_line, start_col);
             // No sync needed at EOF
            return error_token;
        }
        wchar_t escape = advance(lexer); // Consume character after '\'
        switch (escape)
        {
        case L'n':
            value_char = L'\n';
            break;
        case L't':
            value_char = L'\t';
            break;
        case L'\\':
            value_char = L'\\';
            break;
        case L'\'':
            value_char = L'\'';
            break; // Escape sequence for single quote
        case L'\"': // Escape sequence for double quote
            value_char = L'\"';
            break;
        case L'r':
            value_char = L'\r';
            break; // Carriage return
        case L'0':
            value_char = L'\0';
            break; // Null character
        case L'u': { // Handle Unicode escape \uXXXX
            int value = scan_hex_escape(lexer, 4);
            if (value == -1) {
                 BaaToken* error_token = make_error_token(lexer, L"تسلسل هروب يونيكود غير صالح (\uXXXX) في قيمة حرفية (بدأت في السطر %zu، العمود %zu)", start_line, start_col);
                 synchronize(lexer);
                 return error_token;
            }
            // TODO: Check if value is a valid Unicode code point if necessary?
            // TODO: Ensure the resulting wchar_t fits if wchar_t is smaller than the code point range?
            value_char = (wchar_t)value;
            break;
        }
        // TODO: Add hex (\xNN), octal (\OOO) later if needed
        default:
            BaaToken* error_token = make_error_token(lexer, L"تسلسل هروب غير صالح '\%lc' في قيمة حرفية (بدأت في السطر %zu، العمود %zu)", escape, start_line, start_col);
            synchronize(lexer);
            return error_token;
        }
    }
    else
    {
        // Handle regular character
        value_char = advance(lexer); // Consume the character

        // Disallow newline directly within char literal (must use \n)
        if (value_char == L'\n')
        {
            BaaToken* error_token = make_error_token(lexer, L"سطر جديد غير مسموح به في قيمة حرفية (بدأت في السطر %zu، العمود %zu)", start_line, start_col);
            synchronize(lexer);
            return error_token;
        }
        // Disallow closing quote immediately (empty literal '')
        if (value_char == L'\'')
        {
            BaaToken* error_token = make_error_token(lexer, L"قيمة حرفية فارغة ('') (بدأت في السطر %zu، العمود %zu)", start_line, start_col);
            synchronize(lexer);
            return error_token;
        }
    }

    // Check for the closing quote
    if (peek(lexer) == L'\'')
    {
        advance(lexer); // Consume the closing '
        // Successfully scanned a single character (or escape sequence) followed by '
        // Use make_token to create the token with the raw lexeme (e.g., "'a'", "'\n'")
        return make_token(lexer, BAA_TOKEN_CHAR_LIT);
    }
    else
    {
        // Didn't find closing quote immediately after the character/escape
        // This could mean unterminated ('a) or multi-character ('ab')
        // Scan ahead until ' or newline or EOF to provide better context?
        // For now, a simple error is okay.
        if (is_at_end(lexer))
        {
            BaaToken* error_token = make_error_token(lexer, L"قيمة حرفية غير منتهية (علامة اقتباس أحادية ' مفقودة، بدأت في السطر %zu، العمود %zu)", start_line, start_col);
             // No sync needed at EOF
            return error_token;
        }
        else
        {
            // Found something other than ' after the first char/escape
            BaaToken* error_token = make_error_token(lexer, L"قيمة حرفية غير صالحة (متعددة الأحرف أو علامة اقتباس مفقودة؟ بدأت في السطر %zu، العمود %zu)", start_line, start_col);
            synchronize(lexer);
            return error_token;
        }
    }
}

// Attempts to discard characters until it finds a likely synchronization point.
static void synchronize(BaaLexer *lexer)
{
    while (!is_at_end(lexer))
    {
        // 1. Check for end of the previous token/statement boundary.
        //    The dot '.' is our statement terminator.
        //    If we just consumed a dot before entering synchronize, we are likely good.
        //    However, the error might have happened *before* the dot.
        //    Let's advance past the current error character first.
        //    No, the error token was generated, advance() happened before that.
        //    So, peek() looks at the char *after* the error location.

        // If the *previous* character was a statement terminator, we are likely synchronized.
        // Need to check lexer->source[lexer->current - 1], handle boundary conditions.
        // Simpler: just advance until a good starting point is found.
        // Stop if we hit a statement terminator (dot).
        if (peek(lexer) == L'.') {
            advance(lexer); // Consume the dot
            return;         // Likely end of statement, good sync point
        }

        // Stop synchronization if the *next* token looks like it could start a new statement/declaration.
        // Keywords are good indicators.
        switch (peek(lexer))
        {
            // Keywords often start new declarations/statements
            case L'د': // دالة (func)
            case L'إ': // إرجع (return), إذا (if), إلا (else)
            case L'ط': // طالما (while)
            case L'ل': // لأجل (for)
            case L'ا': // افعل (do), اختر (switch)
            case L'ح': // حالة (case)
            case L'ت': // توقف (break)
            case L'س': // استمر (continue)
            case L'م': // متغير (var)
            case L'ث': // ثابت (const)
            case L'#': // Preprocessor directive (future)
            // Delimiters that often start compound statements or expressions
            case L'{':
            case L'(':
                return; // Don't consume the start of the next likely token

            // Consider newline as a weak synchronization point - often separates things
            // but doesn't guarantee recovery. Let's advance past it.
            case L'\n':
                 advance(lexer); // Consume newline and continue synchronizing
                 break; // Continue the while loop
        }

        // Otherwise, consume the current character and continue scanning.
        advance(lexer);
    }
}

// Corresponds to baa_lexer_next_token in header
BaaToken *baa_lexer_next_token(BaaLexer *lexer)
{
    // Skip whitespace and check for errors (like unterminated comments)
    BaaToken* whitespace_error = skip_whitespace(lexer);
    if (whitespace_error != NULL) {
        return whitespace_error;
    }

    lexer->start = lexer->current; // Set start for every token attempt

    if (is_at_end(lexer))
    {
        return make_token(lexer, BAA_TOKEN_EOF); // Returns BaaToken*
    }

    wchar_t c = advance(lexer);

    if (iswalpha(c) || c == L'_' || is_arabic_letter(c)) // Include Arabic letters for identifiers
        return scan_identifier(lexer);                   // Returns BaaToken*
    if (iswdigit(c) || is_arabic_digit(c))               // Include Arabic digits for numbers
        return scan_number(lexer);                       // Returns BaaToken*

    switch (c)
    {
    // Single character tokens
    case L'(':
        return make_token(lexer, BAA_TOKEN_LPAREN);
    case L')':
        return make_token(lexer, BAA_TOKEN_RPAREN);
    case L'{':
        return make_token(lexer, BAA_TOKEN_LBRACE);
    case L'}':
        return make_token(lexer, BAA_TOKEN_RBRACE);
    case L'[':
        return make_token(lexer, BAA_TOKEN_LBRACKET);
    case L']':
        return make_token(lexer, BAA_TOKEN_RBRACKET);
    case L',':
        return make_token(lexer, BAA_TOKEN_COMMA);
    case L'.':
        return make_token(lexer, BAA_TOKEN_DOT);
    case L':':
        return make_token(lexer, BAA_TOKEN_COLON);
    case L';':
        return make_token(lexer, BAA_TOKEN_SEMICOLON);
    case L'%':
        return match(lexer, L'=') ? make_token(lexer, BAA_TOKEN_PERCENT_EQUAL) : make_token(lexer, BAA_TOKEN_PERCENT);
    case L'+':
        return match(lexer, L'=') ? make_token(lexer, BAA_TOKEN_PLUS_EQUAL) : (match(lexer, L'+') ? make_token(lexer, BAA_TOKEN_INCREMENT) : make_token(lexer, BAA_TOKEN_PLUS));
    case L'-':
        return match(lexer, L'=') ? make_token(lexer, BAA_TOKEN_MINUS_EQUAL) : (match(lexer, L'-') ? make_token(lexer, BAA_TOKEN_DECREMENT) : make_token(lexer, BAA_TOKEN_MINUS));
    case L'*':
        return match(lexer, L'=') ? make_token(lexer, BAA_TOKEN_STAR_EQUAL) : make_token(lexer, BAA_TOKEN_STAR);
    case L'/':
        return match(lexer, L'=') ? make_token(lexer, BAA_TOKEN_SLASH_EQUAL) : make_token(lexer, BAA_TOKEN_SLASH);

    // One or two character tokens
    case L'!':
        return match(lexer, L'=') ? make_token(lexer, BAA_TOKEN_BANG_EQUAL) : make_token(lexer, BAA_TOKEN_BANG);
    case L'=':
        return match(lexer, L'=') ? make_token(lexer, BAA_TOKEN_EQUAL_EQUAL) : make_token(lexer, BAA_TOKEN_EQUAL);
    case L'<':
        return match(lexer, L'=') ? make_token(lexer, BAA_TOKEN_LESS_EQUAL) : make_token(lexer, BAA_TOKEN_LESS);
    case L'>':
        return match(lexer, L'=') ? make_token(lexer, BAA_TOKEN_GREATER_EQUAL) : make_token(lexer, BAA_TOKEN_GREATER);

    // String literal
    case L'"':
        return scan_string(lexer); // Returns BaaToken*
    // Character literal
    case L'\'':
        return scan_char_literal(lexer); // Returns BaaToken*

    // Arabic specific punctuation (if needed as distinct tokens)
    case 0x060C:
        return make_token(lexer, BAA_TOKEN_COMMA); // Arabic Comma
    case 0x061B:
        return make_token(lexer, BAA_TOKEN_SEMICOLON); // Arabic Semicolon
    case 0x061F:
        return make_token(lexer, BAA_TOKEN_UNKNOWN); // Arabic Question Mark - Treat as Unknown for now
    case 0x066D:
        return make_token(lexer, BAA_TOKEN_STAR); // Arabic Five Pointed Star - Treat as Star/Multiply

        // Added other operators like &&, || from header
    case L'&':
        if (!match(lexer, L'&')) {
            BaaToken* error_token = make_error_token(lexer, L"عامل غير صالح: علامة '&' مفردة (هل تقصد '&&'؟)");
            synchronize(lexer);
            return error_token;
        }
        return make_token(lexer, BAA_TOKEN_AND);
    case L'|':
        if (!match(lexer, L'|')) {
            BaaToken* error_token = make_error_token(lexer, L"عامل غير صالح: علامة '|' مفردة (هل تقصد '||'؟)");
            synchronize(lexer);
            return error_token;
        }
            return make_token(lexer, BAA_TOKEN_OR);
    }

    // If no case matched, it's an unexpected character
    // Provide the character in the error message.
    // Note: The character 'c' was already consumed by advance() before the switch.
    BaaToken* error_token = make_error_token(lexer, L"حرف غير متوقع: '%lc' (الكود: %u) في السطر %zu، العمود %zu",
                                           c, (unsigned int)c, lexer->line, lexer->column);
    synchronize(lexer);
    return error_token;
}

// Remove baa_lexer_had_error as errors are now handled via BAA_TOKEN_ERROR
// bool baa_lexer_had_error(const BaaLexer *lexer)
// {
//     return lexer->had_error;
// }
