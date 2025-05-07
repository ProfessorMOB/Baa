#include "baa/lexer/lexer.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h> // For error messages
#include <string.h>
#include <wctype.h>
#include <stdarg.h> // Needed for va_list, etc.
// lexer_char_utils.h is included via lexer_internal.h
#include "baa/lexer/lexer_internal.h"      // For internal helper declarations
#include "baa/lexer/token_scanners.h" // For scan_* function declarations (public as requested)

// Array of keywords and their corresponding token types (remains static to this file)
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

// --- Implementations of core lexer helper functions (no longer static) ---
bool is_at_end(BaaLexer *lexer)
{
    return lexer->current >= lexer->source_length;
}

wchar_t peek(BaaLexer *lexer)
{
    if (is_at_end(lexer))
        return L'\0';
    return lexer->source[lexer->current];
}

wchar_t peek_next(BaaLexer *lexer)
{
    if (is_at_end(lexer))
        return L'\0';
    return lexer->source[lexer->current + 1];
}

wchar_t advance(BaaLexer *lexer)
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
    return c; // Return the character that was consumed
}

bool match(BaaLexer *lexer, wchar_t expected)
{
    if (is_at_end(lexer))
        return false;
    if (lexer->source[lexer->current] != expected)
        return false;
    advance(lexer);
    return true;
}

// Corresponds to baa_init_lexer in header
void baa_init_lexer(BaaLexer *lexer, const wchar_t *source, const wchar_t *filename) // Added definition
{
    // filename parameter is currently unused in the struct, but included for signature match
    (void)filename; // Mark as unused to prevent compiler warnings

    if (!lexer || !source)
        return; // Basic validation

    lexer->source = source;
    lexer->source_length = wcslen(source); // Calculate and store source length
    lexer->start = 0;
    lexer->current = 0;
    lexer->line = 1;
    lexer->column = 0;
    // Note: No had_error field in the struct per lexer.h
}

// Creates a token by copying the lexeme from the source
BaaToken *make_token(BaaLexer *lexer, BaaTokenType type)
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
BaaToken *make_error_token(BaaLexer *lexer, const wchar_t *format, ...)
{
    BaaToken *token = malloc(sizeof(BaaToken));
    if (!token)
    {
        fprintf(stderr, "FATAL: Failed to allocate memory for error token struct.\n");
        return NULL;
    }

    wchar_t *buffer = NULL;
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
    if (!buffer)
    {
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
    if (needed < 0)
    {
        // Fallback or error
        wcscpy(buffer, L"خطأ غير معروف في تنسيق رسالة الخطأ الداخلية."); // Fallback message
        needed = wcslen(buffer);
    }
#else
    int needed = vswprintf(buffer, initial_size, format, args);
    // If needed >= initial_size, buffer was too small. Reallocate.
    if (needed < 0 || (size_t)needed >= initial_size)
    {
        // Need to calculate the *actual* required size now
        va_end(args);                                   // End the first va_list
        needed = vswprintf(NULL, 0, format, args_copy); // POSIX way to get size
        va_end(args_copy);                              // End the copied va_list

        if (needed < 0)
        {                                                                    // Error during size calculation
            wcscpy(buffer, L"خطأ غير معروف في تنسيق رسالة الخطأ الداخلية."); // Fallback message
            needed = wcslen(buffer);
        }
        else
        {
            buffer_size = (size_t)needed + 1; // +1 for null terminator
            wchar_t *new_buffer = realloc(buffer, buffer_size * sizeof(wchar_t));
            if (!new_buffer)
            {
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
            if (needed < 0)
            {
                wcscpy(buffer, L"خطأ فادح في تنسيق رسالة الخطأ الداخلية."); // Final fallback
                needed = wcslen(buffer);
            }
        }
    }
    else
    {
        va_end(args_copy); // End the copied va_list if not used
        va_end(args);      // End the va_list used for formatting
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
// This remains static as it's only used by baa_lexer_next_token within this file.
static BaaToken *skip_whitespace(BaaLexer *lexer)
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
        // Removed legacy '#' comment support.
        // The preprocessor handles lines starting with # for directives.
        // Any non-directive '#' reaching the lexer should be an error or part of other syntax.
        case L'/':
            if (peek_next(lexer) == L'*')
            { // Start of /* comment */
                size_t start_line = lexer->line;
                size_t start_col = lexer->column;
                advance(lexer); // Consume '/'
                advance(lexer); // Consume '*'
                while (!(peek(lexer) == L'*' && peek_next(lexer) == L'/') && !is_at_end(lexer))
                {
                    // Need to handle newline increments within the comment
                    if (peek(lexer) == L'\n')
                    {
                        // advance() handles line increment and column reset
                        advance(lexer);
                    }
                    else
                    {
                        advance(lexer);
                    }
                }
                if (!is_at_end(lexer))
                {
                    advance(lexer); // Consume '*'
                    advance(lexer); // Consume '/'
                }
                else
                {
                    // Reached EOF without closing comment - error!
                    BaaToken *error_token = make_error_token(lexer, L"تعليق متعدد الأسطر غير منتهٍ (بدأ في السطر %zu، العمود %zu)",
                                                             start_line, start_col);
                    // No synchronization needed at EOF
                    return error_token;
                }
            }
            else if (peek_next(lexer) == L'/')
            { // Start of // comment
                // Skip // style comments
                while (peek(lexer) != L'\n' && !is_at_end(lexer))
                    advance(lexer);
                // No need to consume newline here, outer loop handles it.
            }
            else
            {
                // Just a slash, not a comment start
                return NULL; // Finished skipping whitespace/comments
            }
            break;
        default:
            return NULL; // Finished skipping whitespace/comments
        }
    }
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

// Implementations of scan_identifier, scan_number, scan_string, scan_char_literal
// will be moved to token_scanners.c

// Helper function to scan exactly 'length' hexadecimal digits
// Returns the integer value, or -1 if not enough digits or invalid digits found.
// This is used by scan_string and scan_char_literal, so it's a shared utility.
int scan_hex_escape(BaaLexer *lexer, int length)
{
    int value = 0;
    for (int i = 0; i < length; i++)
    {
        if (is_at_end(lexer))
            return -1; // EOF
        wchar_t c = peek(lexer);
        int digit;
        if (c >= L'0' && c <= L'9')
        {
            digit = c - L'0';
        }
        else if (c >= L'a' && c <= L'f')
        {
            digit = 10 + (c - L'a');
        }
        else if (c >= L'A' && c <= L'F')
        {
            digit = 10 + (c - L'A');
        }
        else
        {
            return -1; // Not a hex digit
        }
        value = (value * 16) + digit;
        advance(lexer);
    }
    return value;
}

// Helper to add character to dynamic buffer, resizing if needed
// This is used by scan_string, so it's a shared utility.
void append_char_to_buffer(wchar_t **buffer, size_t *len, size_t *capacity, wchar_t c)
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

// Implementations of scan_identifier, scan_number, scan_string, scan_char_literal
// are now in token_scanners.c

// Attempts to discard characters until it finds a likely synchronization point.
// This remains static as it's only used by baa_lexer_next_token within this file.
void synchronize(BaaLexer *lexer) // No longer static
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
        if (peek(lexer) == L'.')
        {
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
            break;          // Continue the while loop
        }

        // Otherwise, consume the current character and continue scanning.
        advance(lexer);
    }
}

// Corresponds to baa_lexer_next_token in header
BaaToken *baa_lexer_next_token(BaaLexer *lexer)
{
    // Skip whitespace and check for errors (like unterminated comments)
    BaaToken *whitespace_error = skip_whitespace(lexer);
    if (whitespace_error != NULL)
    {
        return whitespace_error;
    }

    lexer->start = lexer->current; // Set start for every token attempt

    if (is_at_end(lexer))
    {
        return make_token(lexer, BAA_TOKEN_EOF); // Returns BaaToken*
    }

    // Peek first to decide the type of token
    wchar_t current_char_peeked = peek(lexer);

    // Handle raw string literals (خ"..." and خ"""...""")
    if (current_char_peeked == L'\u062E') { // 'خ'
        if (lexer->current + 1 < lexer->source_length && lexer->source[lexer->current + 1] == L'"') {
            // Potential خ" or خ"""
            if (lexer->current + 3 < lexer->source_length && // Check for خ"""
                lexer->source[lexer->current + 2] == L'"' &&
                lexer->source[lexer->current + 3] == L'"') {
                // It's خ"""
                size_t start_line = lexer->line;
                size_t start_col = lexer->column;
                advance(lexer); // Consume خ
                advance(lexer); // Consume "
                advance(lexer); // Consume "
                advance(lexer); // Consume "
                return scan_raw_string_literal(lexer, true, start_line, start_col); // true for multiline
            } else {
                // It's خ"
                size_t start_line = lexer->line;
                size_t start_col = lexer->column;
                advance(lexer); // Consume خ
                advance(lexer); // Consume "
                return scan_raw_string_literal(lexer, false, start_line, start_col); // false for single line
            }
        }
        // If 'خ' is not followed by '"' or '"""', it will be treated as a normal identifier start below.
    }

    // Handle regular string literals (single and multiline) by peeking
    if (current_char_peeked == L'"') {
        // Check for """
        if (lexer->current + 2 < lexer->source_length &&
            lexer->source[lexer->current + 1] == L'"' &&
            lexer->source[lexer->current + 2] == L'"') {
            // It's a multiline string.
            size_t start_line = lexer->line;   // Capture line before consuming """
            size_t start_col = lexer->column; // Capture column before consuming """
            advance(lexer); // Consume first "
            advance(lexer); // Consume second "
            advance(lexer); // Consume third "
            return scan_multiline_string_literal(lexer, start_line, start_col);
        } else {
            // It's a regular string. scan_string consumes the opening quote.
            return scan_string(lexer);
        }
    }

    // DEBUG PRINT
    // fwprintf(stderr, L"DEBUG: baa_lexer_next_token: char_peeked='%lc'(%u), is_alpha_etc=%d, is_baa_digit=%d, is_arabic_digit=%d, iswdigit=%d\n",
    //          current_char_peeked, (unsigned int)current_char_peeked,
    //          (iswalpha(current_char_peeked) || current_char_peeked == L'_' || is_arabic_letter(current_char_peeked)),
    //          is_baa_digit(current_char_peeked), is_arabic_digit(current_char_peeked), iswdigit(current_char_peeked));

    // Prioritize digits: Arabic-Indic first, then Western, then general identifier characters.
    if (is_arabic_digit(current_char_peeked)) {
        // fwprintf(stderr, L"DEBUG: Calling scan_number for Arabic digit '%lc'\n", current_char_peeked);
        return scan_number(lexer);
    }
    if (iswdigit(current_char_peeked)) { // For '0'-'9'
        // fwprintf(stderr, L"DEBUG: Calling scan_number for Western digit '%lc'\n", current_char_peeked);
        return scan_number(lexer);
    }
    if (iswalpha(current_char_peeked) || current_char_peeked == L'_' || is_arabic_letter(current_char_peeked)) {
        // This will catch letters and underscore. is_arabic_letter is specific to letters.
        // iswalpha might be true for Arabic-Indic digits in some locales, but we've already handled them.
        // fwprintf(stderr, L"DEBUG: Calling scan_identifier for '%lc'\n", current_char_peeked);
        return scan_identifier(lexer);
    }
    // fwprintf(stderr, L"DEBUG: char '%lc' is not identifier start or digit. Advancing for operator/delimiter.\n", current_char_peeked);

    // If not identifier, number, or string, then advance to consume the character and use switch for other tokens
    wchar_t c = advance(lexer); // This char is for single/double char tokens

    // Safeguard: If advance() returned L'\0' because we were already at the end
    // (e.g., if the initial is_at_end() check was somehow bypassed or state was inconsistent),
    // this should definitively become an EOF token.
    if (c == L'\0' && is_at_end(lexer)) {
        // lexer->start should ideally be lexer->current here for a zero-length EOF token.
        // The main is_at_end() check at the top of the function should have caught this.
        // This is a defensive measure.
        lexer->start = lexer->current; // Ensure start is at current for a clean EOF token
        return make_token(lexer, BAA_TOKEN_EOF);
    }

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

    // String literal case L'"' is handled above by peeking
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
        if (!match(lexer, L'&'))
        {
            BaaToken *error_token = make_error_token(lexer, L"عامل غير صالح: علامة '&' مفردة (هل تقصد '&&'؟)");
            synchronize(lexer);
            return error_token;
        }
        return make_token(lexer, BAA_TOKEN_AND);
    case L'|':
        if (!match(lexer, L'|'))
        {
            BaaToken *error_token = make_error_token(lexer, L"عامل غير صالح: علامة '|' مفردة (هل تقصد '||'؟)");
            synchronize(lexer);
            return error_token;
        }
        return make_token(lexer, BAA_TOKEN_OR);
    }

    // If no case matched, it's an unexpected character
    // Provide the character in the error message.
    // Note: The character 'c' was already consumed by advance() before the switch.
    BaaToken *error_token = make_error_token(lexer, L"حرف غير متوقع: '%lc' (الكود: %u) في السطر %zu، العمود %zu",
                                             c, (unsigned int)c, lexer->line, lexer->column);
    synchronize(lexer);
    return error_token;
}

// Remove baa_lexer_had_error as errors are now handled via BAA_TOKEN_ERROR
// bool baa_lexer_had_error(const BaaLexer *lexer)
// {
//     return lexer->had_error;
// }

const wchar_t* baa_token_type_to_string(BaaTokenType type) {
    switch (type) {
        case BAA_TOKEN_EOF: return L"BAA_TOKEN_EOF";
        case BAA_TOKEN_ERROR: return L"BAA_TOKEN_ERROR";
        case BAA_TOKEN_UNKNOWN: return L"BAA_TOKEN_UNKNOWN";
        case BAA_TOKEN_COMMENT: return L"BAA_TOKEN_COMMENT";
        case BAA_TOKEN_IDENTIFIER: return L"BAA_TOKEN_IDENTIFIER";
        case BAA_TOKEN_INT_LIT: return L"BAA_TOKEN_INT_LIT";
        case BAA_TOKEN_FLOAT_LIT: return L"BAA_TOKEN_FLOAT_LIT";
        case BAA_TOKEN_CHAR_LIT: return L"BAA_TOKEN_CHAR_LIT";
        case BAA_TOKEN_STRING_LIT: return L"BAA_TOKEN_STRING_LIT";
        case BAA_TOKEN_BOOL_LIT: return L"BAA_TOKEN_BOOL_LIT";
        case BAA_TOKEN_FUNC: return L"BAA_TOKEN_FUNC";
        case BAA_TOKEN_VAR: return L"BAA_TOKEN_VAR";
        case BAA_TOKEN_CONST: return L"BAA_TOKEN_CONST";
        case BAA_TOKEN_IF: return L"BAA_TOKEN_IF";
        case BAA_TOKEN_ELSE: return L"BAA_TOKEN_ELSE";
        case BAA_TOKEN_WHILE: return L"BAA_TOKEN_WHILE";
        case BAA_TOKEN_FOR: return L"BAA_TOKEN_FOR";
        case BAA_TOKEN_DO: return L"BAA_TOKEN_DO";
        case BAA_TOKEN_CASE: return L"BAA_TOKEN_CASE";
        case BAA_TOKEN_SWITCH: return L"BAA_TOKEN_SWITCH";
        case BAA_TOKEN_RETURN: return L"BAA_TOKEN_RETURN";
        case BAA_TOKEN_BREAK: return L"BAA_TOKEN_BREAK";
        case BAA_TOKEN_CONTINUE: return L"BAA_TOKEN_CONTINUE";
        case BAA_TOKEN_TYPE_INT: return L"BAA_TOKEN_TYPE_INT";
        case BAA_TOKEN_TYPE_FLOAT: return L"BAA_TOKEN_TYPE_FLOAT";
        case BAA_TOKEN_TYPE_CHAR: return L"BAA_TOKEN_TYPE_CHAR";
        case BAA_TOKEN_TYPE_VOID: return L"BAA_TOKEN_TYPE_VOID";
        case BAA_TOKEN_TYPE_BOOL: return L"BAA_TOKEN_TYPE_BOOL";
        case BAA_TOKEN_PLUS: return L"BAA_TOKEN_PLUS";
        case BAA_TOKEN_MINUS: return L"BAA_TOKEN_MINUS";
        case BAA_TOKEN_STAR: return L"BAA_TOKEN_STAR";
        case BAA_TOKEN_SLASH: return L"BAA_TOKEN_SLASH";
        case BAA_TOKEN_PERCENT: return L"BAA_TOKEN_PERCENT";
        case BAA_TOKEN_EQUAL: return L"BAA_TOKEN_EQUAL";
        case BAA_TOKEN_EQUAL_EQUAL: return L"BAA_TOKEN_EQUAL_EQUAL";
        case BAA_TOKEN_BANG: return L"BAA_TOKEN_BANG";
        case BAA_TOKEN_BANG_EQUAL: return L"BAA_TOKEN_BANG_EQUAL";
        case BAA_TOKEN_LESS: return L"BAA_TOKEN_LESS";
        case BAA_TOKEN_LESS_EQUAL: return L"BAA_TOKEN_LESS_EQUAL";
        case BAA_TOKEN_GREATER: return L"BAA_TOKEN_GREATER";
        case BAA_TOKEN_GREATER_EQUAL: return L"BAA_TOKEN_GREATER_EQUAL";
        case BAA_TOKEN_AND: return L"BAA_TOKEN_AND";
        case BAA_TOKEN_OR: return L"BAA_TOKEN_OR";
        case BAA_TOKEN_PLUS_EQUAL: return L"BAA_TOKEN_PLUS_EQUAL";
        case BAA_TOKEN_MINUS_EQUAL: return L"BAA_TOKEN_MINUS_EQUAL";
        case BAA_TOKEN_STAR_EQUAL: return L"BAA_TOKEN_STAR_EQUAL";
        case BAA_TOKEN_SLASH_EQUAL: return L"BAA_TOKEN_SLASH_EQUAL";
        case BAA_TOKEN_PERCENT_EQUAL: return L"BAA_TOKEN_PERCENT_EQUAL";
        case BAA_TOKEN_INCREMENT: return L"BAA_TOKEN_INCREMENT";
        case BAA_TOKEN_DECREMENT: return L"BAA_TOKEN_DECREMENT";
        case BAA_TOKEN_LPAREN: return L"BAA_TOKEN_LPAREN";
        case BAA_TOKEN_RPAREN: return L"BAA_TOKEN_RPAREN";
        case BAA_TOKEN_LBRACE: return L"BAA_TOKEN_LBRACE";
        case BAA_TOKEN_RBRACE: return L"BAA_TOKEN_RBRACE";
        case BAA_TOKEN_LBRACKET: return L"BAA_TOKEN_LBRACKET";
        case BAA_TOKEN_RBRACKET: return L"BAA_TOKEN_RBRACKET";
        case BAA_TOKEN_COMMA: return L"BAA_TOKEN_COMMA";
        case BAA_TOKEN_DOT: return L"BAA_TOKEN_DOT";
        case BAA_TOKEN_SEMICOLON: return L"BAA_TOKEN_SEMICOLON";
        case BAA_TOKEN_COLON: return L"BAA_TOKEN_COLON";
        default: return L"BAA_TOKEN_INVALID_TYPE_IN_TO_STRING"; // Should not happen
    }
}

// --- Token Utility Implementations ---

bool baa_token_is_keyword(BaaTokenType type) {
    // Check if the type is within the keyword range
    return type >= BAA_TOKEN_FUNC && type <= BAA_TOKEN_CONTINUE;
}

bool baa_token_is_type(BaaTokenType type) {
    // Check if the type is within the type name range
    return type >= BAA_TOKEN_TYPE_INT && type <= BAA_TOKEN_TYPE_BOOL;
}

bool baa_token_is_operator(BaaTokenType type) {
    // Check if the type is within any of the operator ranges
    return (type >= BAA_TOKEN_PLUS && type <= BAA_TOKEN_OR) ||          // Arithmetic, Comparison, Logical
           (type >= BAA_TOKEN_PLUS_EQUAL && type <= BAA_TOKEN_PERCENT_EQUAL) || // Compound Assignment
           (type >= BAA_TOKEN_INCREMENT && type <= BAA_TOKEN_DECREMENT);       // Increment/Decrement
    // Note: Does not include assignment (=) or delimiters like dot (.).
    // Adjust ranges if definition of "operator" needs to change.
}
