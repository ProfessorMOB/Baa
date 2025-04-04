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

static void skip_whitespace(BaaLexer *lexer)
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
                    // Reached EOF without closing comment - error?
                    // The next call to baa_lexer_next_token will return EOF or an error
                    // based on where the comment started relative to useful tokens.
                    // For skip_whitespace, we just stop skipping.
                    return;
                }
            } else if (peek_next(lexer) == L'/') { // Start of // comment
                 // Skip // style comments
                 while (peek(lexer) != L'\n' && !is_at_end(lexer))
                     advance(lexer);
                 // No need to consume newline here, outer loop handles it.
            } else {
                 // Just a slash, not a comment start
                 return;
            }
            break;
        default:
            return;
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

    // Scan initial digits
    while (iswdigit(peek(lexer)) || is_arabic_digit(peek(lexer)))
    {
        advance(lexer);
    }

    // Check for fractional part (decimal point)
    // Requires a digit *after* the decimal point for it to be considered part of the float literal
    wchar_t current_peek = peek(lexer);
    wchar_t next_peek = peek_next(lexer);
    if ((current_peek == L'.' || current_peek == 0x066B /* Arabic Decimal Separator */) &&
        (iswdigit(next_peek) || is_arabic_digit(next_peek)))
    {
        is_float = true;
        advance(lexer); // Consume the decimal point '.' or '٫'

        // Consume digits after decimal point
        while (iswdigit(peek(lexer)) || is_arabic_digit(peek(lexer)))
        {
            advance(lexer);
        }
    }

    // Check for exponent part ('e' or 'E')
    current_peek = peek(lexer); // Re-peek after potentially consuming fractional part
    if (current_peek == L'e' || current_peek == L'E')
    {
        // Check if there's something after 'e'/'E' (digit or sign+digit)
        next_peek = peek_next(lexer);
        bool has_exponent_part = false;
        if (next_peek == L'+' || next_peek == L'-') {
            // Check for digit after the sign
            wchar_t after_sign_peek = lexer->source[lexer->current + 2]; // Peek two ahead
             if (iswdigit(after_sign_peek) || is_arabic_digit(after_sign_peek)) {
                 has_exponent_part = true;
             }
        } else if (iswdigit(next_peek) || is_arabic_digit(next_peek)) {
            has_exponent_part = true;
        }

        if (has_exponent_part) {
            is_float = true;
            advance(lexer); // Consume 'e' or 'E'

            // Consume optional sign
            if (peek(lexer) == L'+' || peek(lexer) == L'-')
            {
                advance(lexer);
            }

            // Consume exponent digits
            // Need at least one digit after 'e'/'E' or sign
            if (!(iswdigit(peek(lexer)) || is_arabic_digit(peek(lexer)))) {
                 // This indicates an invalid format like "1e" or "1e+", but we've already decided it's a float pattern.
                 // The detailed number parser should handle this error later.
                 // For the lexer's purpose, the 'e'/'E' marks it as float-like.
            }
            while (iswdigit(peek(lexer)) || is_arabic_digit(peek(lexer)))
            {
                advance(lexer);
            }
        }
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
                    return make_error_token(lexer, L"تسلسل هروب يونيكود غير صالح (\uXXXX) في السطر %zu، العمود %zu", start_line, start_col);
                }
                // TODO: Check if value is a valid Unicode code point if necessary?
                append_char_to_buffer(&buffer, &buffer_len, &buffer_cap, (wchar_t)value);
                break;
            }
            // TODO: Add more escape sequences (\r, \0, \xNN) ?
            default:
                // Invalid escape sequence
                free(buffer);
                return make_error_token(lexer, L"تسلسل هروب غير صالح '\%lc' في سلسلة نصية (السطر %zu)", escaped_char, start_line);
            }
            // Check if append_char_to_buffer failed due to realloc error
            if (buffer == NULL)
            {
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
                return make_error_token(lexer, L"فشل في إعادة تخصيص ذاكرة لسلسلة نصية (السطر %zu)", start_line);
            }
            advance(lexer);
        }
    }

    if (is_at_end(lexer) || peek(lexer) != L'"')
    {
        // Unterminated string error
        free(buffer); // Clean up allocated buffer
        return make_error_token(lexer, L"سلسلة نصية غير منتهية (بدأت في السطر %zu، العمود %zu)", start_line, start_col);
    }

    // Consume the closing quote
    advance(lexer);

    // Null-terminate the buffer
    append_char_to_buffer(&buffer, &buffer_len, &buffer_cap, L'\0');
    // Check if append_char_to_buffer failed due to realloc error
    if (buffer == NULL)
    {
        // Buffer is already freed by the helper in case of error
        return make_error_token(lexer, L"فشل في إعادة تخصيص الذاكرة عند إنهاء السلسلة النصية (بدأت في السطر %zu)", start_line);
    }

    // Create the token using the interpreted buffer content
    BaaToken *token = malloc(sizeof(BaaToken));
    if (!token)
    {
        fprintf(stderr, "FATAL: Failed to allocate memory for string token.\n");
        free(buffer); // Free the interpreted string buffer
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
    // lexer->start points to the opening quote (')

    wchar_t value_char; // To hold the interpreted value (though not stored in token directly here)
    bool had_escape = false;

    if (is_at_end(lexer))
    {
        // EOF right after opening '
        return make_error_token(lexer, L"قيمة حرفية غير منتهية (EOF بعد \' )" );
    }

    // Handle escape sequences
    if (peek(lexer) == L'\\')
    {
        had_escape = true;
        advance(lexer); // Consume '\'
        if (is_at_end(lexer))
        {
            // EOF after '\'
            return make_error_token(lexer, L"تسلسل هروب غير منته في قيمة حرفية (EOF بعد \'\\')");
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
                 return make_error_token(lexer, L"تسلسل هروب يونيكود غير صالح (\uXXXX) في قيمة حرفية (السطر %zu)", lexer->line);
            }
            // TODO: Check if value is a valid Unicode code point if necessary?
            // TODO: Ensure the resulting wchar_t fits if wchar_t is smaller than the code point range?
            value_char = (wchar_t)value;
            break;
        }
        // TODO: Add hex (\xNN), octal (\OOO) later if needed
        default:
            return make_error_token(lexer, L"تسلسل هروب غير صالح '\%lc' في قيمة حرفية", escape);
        }
    }
    else
    {
        // Handle regular character
        value_char = advance(lexer); // Consume the character

        // Disallow newline directly within char literal (must use \n)
        if (value_char == L'\n')
        {
            // Advance consumed the newline, adjust position info back potentially?
            // Error message is sufficient for now.
            return make_error_token(lexer, L"سطر جديد غير مسموح به في قيمة حرفية");
        }
        // Disallow closing quote immediately (empty literal '')
        if (value_char == L'\'')
        {
            return make_error_token(lexer, L"قيمة حرفية فارغة ('')");
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
            return make_error_token(lexer, L"قيمة حرفية غير منتهية (علامة اقتباس أحادية ' مفقودة)");
        }
        else
        {
            // Found something other than ' after the first char/escape
            // Advance to include the problematic char in the error reporting span? No, keep it simple.
            return make_error_token(lexer, L"قيمة حرفية غير صالحة (علامة اقتباس أحادية ' مفقودة أو في غير محلها؟ متعددة الأحرف؟)");
        }
    }
}

// Corresponds to baa_lexer_next_token in header
BaaToken *baa_lexer_next_token(BaaLexer *lexer)
{
    skip_whitespace(lexer);

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
            return make_error_token(lexer, L"عامل غير صالح: علامة '&' مفردة (هل تقصد '&&'؟)");
        }
        return make_token(lexer, BAA_TOKEN_AND);
    case L'|':
        if (!match(lexer, L'|')) {
            return make_error_token(lexer, L"عامل غير صالح: علامة '|' مفردة (هل تقصد '||'؟)");
        }
        return make_token(lexer, BAA_TOKEN_OR);
    }

    // If no case matched, it's an unexpected character
    return make_error_token(lexer, L"حرف غير متوقع: '%lc'", c);
}

// Remove baa_lexer_had_error as errors are now handled via BAA_TOKEN_ERROR
// bool baa_lexer_had_error(const BaaLexer *lexer)
// {
//     return lexer->had_error;
// }
