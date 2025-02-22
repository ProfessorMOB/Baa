#include "baa/lexer.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

// Array of keywords and their corresponding token types
static struct {
    const wchar_t *keyword;
    TokenType token;
} keywords[] = {
    {L"دالة", TOKEN_FUNCTION},
    {L"إرجع", TOKEN_RETURN},
    {L"إذا", TOKEN_IF},
    {L"إلا", TOKEN_ELSE},
    {L"طالما", TOKEN_WHILE},
    {L"لأجل", TOKEN_FOR},
    {L"افعل", TOKEN_DO},
    {L"اختر", TOKEN_SWITCH},
    {L"حالة", TOKEN_CASE},
    {L"توقف", TOKEN_BREAK},
    {L"استمر", TOKEN_CONTINUE}
};

// Helper functions
static bool is_arabic_letter(wchar_t c) {
    return (c >= 0x0600 && c <= 0x06FF) || (c >= 0xFB50 && c <= 0xFDFF) || 
           (c >= 0xFE70 && c <= 0xFEFF); // Basic Arabic, Arabic Presentation Forms-A and B
}

static bool is_arabic_digit(wchar_t c) {
    return (c >= 0x0660 && c <= 0x0669); // Arabic-Indic digits
}

static bool is_arabic_punctuation(wchar_t c) {
    return (c == 0x060C) || // Arabic comma
           (c == 0x061B) || // Arabic semicolon
           (c == 0x061F) || // Arabic question mark
           (c == 0x066D);   // Arabic five pointed star
}

static bool is_at_end(Lexer *lexer) {
    return lexer->source[lexer->position] == L'\0';
}

static wchar_t peek(Lexer *lexer) {
    if (is_at_end(lexer)) return L'\0';
    return lexer->source[lexer->position];
}

static wchar_t peek_next(Lexer *lexer) {
    if (is_at_end(lexer)) return L'\0';
    return lexer->source[lexer->position + 1];
}

static wchar_t advance(Lexer *lexer) {
    if (is_at_end(lexer)) return L'\0';
    wchar_t c = lexer->source[lexer->position++];
    if (c == L'\n') {
        lexer->line++;
        lexer->column = 0;
    } else {
        lexer->column++;
    }
    return c;
}

static bool match(Lexer *lexer, wchar_t expected) {
    if (is_at_end(lexer)) return false;
    if (lexer->source[lexer->position] != expected) return false;
    advance(lexer);
    return true;
}

static Token make_token(Lexer *lexer, TokenType type) {
    Token token;
    token.type = type;
    size_t start = lexer->position - lexer->current.length;
    token.lexeme = malloc((lexer->current.length + 1) * sizeof(wchar_t));
    wcsncpy(token.lexeme, &lexer->source[start], lexer->current.length);
    token.lexeme[lexer->current.length] = L'\0';
    token.line = lexer->line;
    token.column = lexer->column - lexer->current.length;
    token.length = lexer->current.length;
    return token;
}

static void skip_whitespace(Lexer *lexer) {
    for (;;) {
        wchar_t c = peek(lexer);
        switch (c) {
            case L' ':
            case L'\r':
            case L'\t':
                advance(lexer);
                break;
            case L'\n':
                advance(lexer);
                break;
            case L'#':
                // Skip single-line comments
                while (peek(lexer) != L'\n' && !is_at_end(lexer)) advance(lexer);
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

Lexer *baa_lexer_init(const wchar_t *source) {
    Lexer *lexer = malloc(sizeof(Lexer));
    lexer->source = source;
    lexer->position = 0;
    lexer->line = 1;
    lexer->column = 0;
    lexer->had_error = false;
    lexer->current.lexeme = NULL;
    lexer->current.length = 0;
    return lexer;
}

void baa_lexer_free(Lexer *lexer) {
    if (lexer->current.lexeme) {
        free(lexer->current.lexeme);
    }
    free(lexer);
}

static Token scan_identifier(Lexer *lexer) {
    while (iswalnum(peek(lexer)) || peek(lexer) == L'_' || 
           is_arabic_letter(peek(lexer)) || is_arabic_digit(peek(lexer))) {
        advance(lexer);
    }
    
    // Check if identifier is a keyword
    size_t length = lexer->position - (lexer->position - lexer->current.length);
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        if (wcslen(keywords[i].keyword) == length &&
            wcsncmp(&lexer->source[lexer->position - length],
                    keywords[i].keyword, length) == 0) {
            return make_token(lexer, keywords[i].token);
        }
    }
    
    return make_token(lexer, TOKEN_IDENTIFIER);
}

static Token scan_number(Lexer *lexer) {
    while (iswdigit(peek(lexer)) || is_arabic_digit(peek(lexer))) {
        advance(lexer);
    }
    return make_token(lexer, TOKEN_NUMBER);
}

static Token scan_string(Lexer *lexer) {
    while (peek(lexer) != L'"' && !is_at_end(lexer)) {
        if (peek(lexer) == L'\n') lexer->line++;
        advance(lexer);
    }
    
    if (is_at_end(lexer)) {
        lexer->had_error = true;
        Token token = make_token(lexer, TOKEN_EOF);
        token.lexeme = NULL;
        return token;
    }
    
    // The closing quote
    advance(lexer);
    return make_token(lexer, TOKEN_STRING);
}

Token baa_lexer_next_token(Lexer *lexer) {
    skip_whitespace(lexer);
    
    if (is_at_end(lexer)) {
        return make_token(lexer, TOKEN_EOF);
    }
    
    lexer->current.length = 0;
    wchar_t c = advance(lexer);
    lexer->current.length = 1;
    
    if (iswalpha(c) || c == L'_') return scan_identifier(lexer);
    if (iswdigit(c)) return scan_number(lexer);
    
    switch (c) {
        case L'"': return scan_string(lexer);
        case L'+': return make_token(lexer, TOKEN_PLUS);
        case L'-': return make_token(lexer, TOKEN_MINUS);
        case L'*': return make_token(lexer, TOKEN_STAR);
        case L'/': return make_token(lexer, TOKEN_SLASH);
        case L'=': return make_token(lexer, TOKEN_EQUALS);
        case 0x060C: return make_token(lexer, TOKEN_COMMA);
        case 0x061B: return make_token(lexer, TOKEN_SEMICOLON);
        case 0x061F: return make_token(lexer, TOKEN_QUESTION);
        case 0x066D: return make_token(lexer, TOKEN_STAR); // Treating Arabic star as multiplication
    }
    
    lexer->had_error = true;
    Token token = make_token(lexer, TOKEN_EOF);
    token.lexeme = NULL;
    return token;
}

bool baa_lexer_had_error(const Lexer *lexer) {
    return lexer->had_error;
}
