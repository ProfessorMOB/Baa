#include "baa/lexer/lexer.h"
#include "baa/utils/utils.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

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
    {L"توقف", BAA_TOKEN_BREAK},
    {L"استمر", BAA_TOKEN_CONTINUE}};

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

// New keyword initialization
void initialize_keywords()
{
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); ++i)
    {
        // Initialize each keyword here
        // This will be used by the lexer to recognize keywords
        printf("Initialized keyword: %ls\n", keywords[i].keyword);
    }
}

// Token utility functions
bool baa_token_is_keyword(BaaTokenType type) {
    switch (type) {
        case BAA_TOKEN_FUNC:
        case BAA_TOKEN_VAR:
        case BAA_TOKEN_CONST:
        case BAA_TOKEN_IF:
        case BAA_TOKEN_ELSE:
        case BAA_TOKEN_WHILE:
        case BAA_TOKEN_FOR:
        case BAA_TOKEN_RETURN:
        case BAA_TOKEN_BREAK:
        case BAA_TOKEN_CONTINUE:
            return true;
        default:
            return false;
    }
}

bool baa_token_is_type(BaaTokenType type) {
    switch (type) {
        case BAA_TOKEN_TYPE_INT:
        case BAA_TOKEN_TYPE_FLOAT:
        case BAA_TOKEN_TYPE_CHAR:
        case BAA_TOKEN_TYPE_VOID:
        case BAA_TOKEN_TYPE_BOOL:
            return true;
        default:
            return false;
    }
}

const wchar_t* baa_token_type_to_string(BaaTokenType type) {
    switch (type) {
        // Special tokens
        case BAA_TOKEN_EOF:        return L"نهاية_الملف";
        case BAA_TOKEN_ERROR:      return L"خطأ";
        case BAA_TOKEN_UNKNOWN:    return L"غير_معروف";
        case BAA_TOKEN_COMMENT:    return L"تعليق";

        // Literals
        case BAA_TOKEN_IDENTIFIER: return L"معرف";
        case BAA_TOKEN_INT_LIT:    return L"عدد_صحيح";
        case BAA_TOKEN_FLOAT_LIT:  return L"عدد_حقيقي";
        case BAA_TOKEN_CHAR_LIT:   return L"حرف";
        case BAA_TOKEN_STRING_LIT: return L"نص";
        case BAA_TOKEN_BOOL_LIT:   return L"صحيح_خطأ";

        // Keywords
        case BAA_TOKEN_FUNC:       return L"دالة";
        case BAA_TOKEN_VAR:        return L"متغير";
        case BAA_TOKEN_CONST:      return L"ثابت";
        case BAA_TOKEN_IF:         return L"إذا";
        case BAA_TOKEN_ELSE:       return L"وإلا";
        case BAA_TOKEN_WHILE:      return L"طالما";
        case BAA_TOKEN_FOR:        return L"لكل";
        case BAA_TOKEN_RETURN:     return L"إرجع";
        case BAA_TOKEN_BREAK:      return L"توقف";
        case BAA_TOKEN_CONTINUE:   return L"أكمل";

        // Types
        case BAA_TOKEN_TYPE_INT:   return L"عدد_صحيح";
        case BAA_TOKEN_TYPE_FLOAT: return L"عدد_حقيقي";
        case BAA_TOKEN_TYPE_CHAR:  return L"حرف";
        case BAA_TOKEN_TYPE_VOID:  return L"فراغ";
        case BAA_TOKEN_TYPE_BOOL:  return L"منطقي";

        // Operators
        case BAA_TOKEN_PLUS:       return L"+";
        case BAA_TOKEN_PLUS_EQUAL: return L"+=";
        case BAA_TOKEN_INCREMENT:  return L"++";
        case BAA_TOKEN_MINUS:      return L"-";
        case BAA_TOKEN_MINUS_EQUAL:return L"-=";
        case BAA_TOKEN_DECREMENT:  return L"--";
        case BAA_TOKEN_STAR:       return L"*";
        case BAA_TOKEN_STAR_EQUAL: return L"*=";
        case BAA_TOKEN_SLASH:      return L"/";
        case BAA_TOKEN_SLASH_EQUAL: return L"/=";
        case BAA_TOKEN_PERCENT:    return L"%";
        case BAA_TOKEN_PERCENT_EQUAL: return L"%=";
        case BAA_TOKEN_EQUAL:      return L"=";
        case BAA_TOKEN_EQUAL_EQUAL:return L"==";
        case BAA_TOKEN_BANG:       return L"!";
        case BAA_TOKEN_BANG_EQUAL: return L"!=";
        case BAA_TOKEN_LESS:       return L"<";
        case BAA_TOKEN_LESS_EQUAL: return L"<=";
        case BAA_TOKEN_GREATER:    return L">";
        case BAA_TOKEN_GREATER_EQUAL: return L">=";
        case BAA_TOKEN_AND:        return L"&&";
        case BAA_TOKEN_OR:         return L"||";

        // Delimiters
        case BAA_TOKEN_LPAREN:     return L"(";
        case BAA_TOKEN_RPAREN:     return L")";
        case BAA_TOKEN_LBRACE:     return L"{";
        case BAA_TOKEN_RBRACE:     return L"}";
        case BAA_TOKEN_LBRACKET:   return L"[";
        case BAA_TOKEN_RBRACKET:   return L"]";
        case BAA_TOKEN_COMMA:      return L",";
        case BAA_TOKEN_DOT:        return L".";
        case BAA_TOKEN_SEMICOLON:  return L";";
        case BAA_TOKEN_COLON:      return L":";

        default:                   return L"غير_معروف";
    }
}

// Lexer functions
BaaLexer* baa_create_lexer(const wchar_t* source) {
    BaaLexer* lexer = baa_malloc(sizeof(BaaLexer));
    if (lexer) {
        lexer->source = source;
        lexer->start = 0;
        lexer->current = 0;
        lexer->line = 1;
        lexer->column = 1;
    }
    return lexer;
}

void baa_free_lexer(BaaLexer* lexer) {
    if (lexer) {
        baa_free(lexer);
    }
}

void baa_free_token(BaaToken* token) {
    if (token) {
        if (token->lexeme) {
            baa_free((void*)token->lexeme);
        }
        baa_free(token);
    }
}

// Helper functions
static bool is_whitespace(wchar_t c) {
    return c == L' ' || c == L'\t' || c == L'\n' || c == L'\r';
}

static bool is_digit(wchar_t c) {
    return c >= L'0' && c <= L'9';
}

static bool is_arabic_letter(wchar_t c) {
    // Arabic Unicode range: 0x0600-0x06FF
    return (c >= 0x0600 && c <= 0x06FF);
}

static bool is_identifier_start(wchar_t c) {
    return is_arabic_letter(c) || c == L'_';
}

static bool is_identifier_part(wchar_t c) {
    return is_arabic_letter(c) || is_digit(c) || c == L'_';
}

static wchar_t peek(BaaLexer* lexer) {
    if (lexer->current >= wcslen(lexer->source)) {
        return L'\0';
    }
    return lexer->source[lexer->current];
}

static wchar_t peek_next(BaaLexer* lexer) {
    if (lexer->current + 1 >= wcslen(lexer->source)) {
        return L'\0';
    }
    return lexer->source[lexer->current + 1];
}

static wchar_t advance(BaaLexer* lexer) {
    wchar_t c = peek(lexer);
    if (c != L'\0') {
        lexer->current++;
        if (c == L'\n') {
            lexer->line++;
            lexer->column = 1;
        } else {
            lexer->column++;
        }
    }
    return c;
}

static void skip_whitespace(BaaLexer* lexer) {
    for (;;) {
        wchar_t c = peek(lexer);
        switch (c) {
            case L' ':
            case L'\r':
            case L'\t':
                advance(lexer);
                break;
            case L'\n':
                lexer->line++;
                lexer->column = 1;
                advance(lexer);
                break;
            default:
                return;
        }
    }
}

static BaaToken* make_token(BaaLexer* lexer, BaaTokenType type) {
    BaaToken* token = baa_malloc(sizeof(BaaToken));
    if (token) {
        token->type = type;
        token->length = lexer->current - lexer->start;
        token->lexeme = baa_strndup(lexer->source + lexer->start, token->length);
        token->line = lexer->line;
        token->column = lexer->column - token->length;
    }
    return token;
}

static BaaToken* error_token(BaaLexer* lexer, const wchar_t* message) {
    BaaToken* token = baa_malloc(sizeof(BaaToken));
    if (token) {
        token->type = BAA_TOKEN_ERROR;
        token->lexeme = baa_strdup(message);
        token->length = wcslen(message);
        token->line = lexer->line;
        token->column = lexer->column;
    }
    return token;
}

BaaToken* baa_scan_token(BaaLexer* lexer) {
    skip_whitespace(lexer);
    lexer->start = lexer->current;

    if (lexer->current >= wcslen(lexer->source)) {
        return make_token(lexer, BAA_TOKEN_EOF);
    }

    wchar_t c = advance(lexer);

    // Check for comments
    if (c == L'/') {
        if (peek(lexer) == L'/') {
            // Single-line comment
            advance(lexer); // Consume the second '/'
            
            // Consume until end of line or end of file
            while (peek(lexer) != L'\n' && peek(lexer) != L'\0') {
                advance(lexer);
            }
            
            return make_token(lexer, BAA_TOKEN_COMMENT);
        } else if (peek(lexer) == L'*') {
            // Multi-line comment
            advance(lexer); // Consume the '*'
            
            // Consume until */ or end of file
            while (!(peek(lexer) == L'*' && peek_next(lexer) == L'/') && peek(lexer) != L'\0') {
                if (peek(lexer) == L'\n') {
                    lexer->line++;
                    lexer->column = 1;
                }
                advance(lexer);
            }
            
            // Consume the closing */
            if (peek(lexer) != L'\0') {
                advance(lexer); // Consume '*'
                advance(lexer); // Consume '/'
            } else {
                return error_token(lexer, L"تعليق غير منتهي");
            }
            
            return make_token(lexer, BAA_TOKEN_COMMENT);
        } else if (peek(lexer) == L'=') {
            advance(lexer);
            return make_token(lexer, BAA_TOKEN_SLASH_EQUAL);
        } else {
            // It's just a division operator
            return make_token(lexer, BAA_TOKEN_SLASH);
        }
    }
    
    // Arabic-style comments
    if (c == L'#' || c == L'٭') { // Using Arabic asterisk as an alternative comment marker
        // Single-line Arabic comment
        
        // Consume until end of line or end of file
        while (peek(lexer) != L'\n' && peek(lexer) != L'\0') {
            advance(lexer);
        }
        
        return make_token(lexer, BAA_TOKEN_COMMENT);
    }

    if (is_identifier_start(c)) {
        // Handle identifiers and keywords
        while (is_identifier_part(peek(lexer))) {
            advance(lexer);
        }

        // Check for keywords
        size_t length = lexer->current - lexer->start;
        const wchar_t* text = lexer->source + lexer->start;

        if (wcsncmp(text, L"دالة", length) == 0) return make_token(lexer, BAA_TOKEN_FUNC);
        if (wcsncmp(text, L"متغير", length) == 0) return make_token(lexer, BAA_TOKEN_VAR);
        if (wcsncmp(text, L"ثابت", length) == 0) return make_token(lexer, BAA_TOKEN_CONST);
        if (wcsncmp(text, L"إذا", length) == 0) return make_token(lexer, BAA_TOKEN_IF);
        if (wcsncmp(text, L"وإلا", length) == 0) return make_token(lexer, BAA_TOKEN_ELSE);
        if (wcsncmp(text, L"طالما", length) == 0) return make_token(lexer, BAA_TOKEN_WHILE);
        if (wcsncmp(text, L"لكل", length) == 0) return make_token(lexer, BAA_TOKEN_FOR);
        if (wcsncmp(text, L"إرجع", length) == 0) return make_token(lexer, BAA_TOKEN_RETURN);
        if (wcsncmp(text, L"توقف", length) == 0) return make_token(lexer, BAA_TOKEN_BREAK);
        if (wcsncmp(text, L"أكمل", length) == 0) return make_token(lexer, BAA_TOKEN_CONTINUE);
        
        // Boolean literals
        if (wcsncmp(text, L"صحيح", length) == 0) return make_token(lexer, BAA_TOKEN_BOOL_LIT);
        if (wcsncmp(text, L"خطأ", length) == 0) return make_token(lexer, BAA_TOKEN_BOOL_LIT);
        
        // Type keywords
        if (wcsncmp(text, L"عدد_صحيح", length) == 0) return make_token(lexer, BAA_TOKEN_TYPE_INT);
        if (wcsncmp(text, L"عدد_حقيقي", length) == 0) return make_token(lexer, BAA_TOKEN_TYPE_FLOAT);
        if (wcsncmp(text, L"حرف", length) == 0) return make_token(lexer, BAA_TOKEN_TYPE_CHAR);
        if (wcsncmp(text, L"فراغ", length) == 0) return make_token(lexer, BAA_TOKEN_TYPE_VOID);
        if (wcsncmp(text, L"منطقي", length) == 0) return make_token(lexer, BAA_TOKEN_TYPE_BOOL);

        return make_token(lexer, BAA_TOKEN_IDENTIFIER);
    }

    if (is_digit(c)) {
        // Handle numbers
        bool has_decimal = false;
        while (is_digit(peek(lexer)) || (!has_decimal && peek(lexer) == L'.')) {
            if (peek(lexer) == L'.') {
                has_decimal = true;
            }
            advance(lexer);
        }
        return make_token(lexer, has_decimal ? BAA_TOKEN_FLOAT_LIT : BAA_TOKEN_INT_LIT);
    }

    // Handle operators and delimiters
    switch (c) {
        case L'(': return make_token(lexer, BAA_TOKEN_LPAREN);
        case L')': return make_token(lexer, BAA_TOKEN_RPAREN);
        case L'{': return make_token(lexer, BAA_TOKEN_LBRACE);
        case L'}': return make_token(lexer, BAA_TOKEN_RBRACE);
        case L'[': return make_token(lexer, BAA_TOKEN_LBRACKET);
        case L']': return make_token(lexer, BAA_TOKEN_RBRACKET);
        case L',': return make_token(lexer, BAA_TOKEN_COMMA);
        case L'.': return make_token(lexer, BAA_TOKEN_DOT);
        case L';': return make_token(lexer, BAA_TOKEN_SEMICOLON);
        case L':': return make_token(lexer, BAA_TOKEN_COLON);

        case L'+':
            if (peek(lexer) == L'=') {
                advance(lexer);
                return make_token(lexer, BAA_TOKEN_PLUS_EQUAL);
            } else if (peek(lexer) == L'+') {
                advance(lexer);
                return make_token(lexer, BAA_TOKEN_INCREMENT);
            }
            return make_token(lexer, BAA_TOKEN_PLUS);
            
        case L'-':
            if (peek(lexer) == L'=') {
                advance(lexer);
                return make_token(lexer, BAA_TOKEN_MINUS_EQUAL);
            } else if (peek(lexer) == L'-') {
                advance(lexer);
                return make_token(lexer, BAA_TOKEN_DECREMENT);
            }
            return make_token(lexer, BAA_TOKEN_MINUS);
            
        case L'*':
            if (peek(lexer) == L'=') {
                advance(lexer);
                return make_token(lexer, BAA_TOKEN_STAR_EQUAL);
            }
            return make_token(lexer, BAA_TOKEN_STAR);
            
        case L'%':
            if (peek(lexer) == L'=') {
                advance(lexer);
                return make_token(lexer, BAA_TOKEN_PERCENT_EQUAL);
            }
            return make_token(lexer, BAA_TOKEN_PERCENT);

        case L'=':
            if (peek(lexer) == L'=') {
                advance(lexer);
                return make_token(lexer, BAA_TOKEN_EQUAL_EQUAL);
            }
            return make_token(lexer, BAA_TOKEN_EQUAL);

        case L'!':
            if (peek(lexer) == L'=') {
                advance(lexer);
                return make_token(lexer, BAA_TOKEN_BANG_EQUAL);
            }
            return make_token(lexer, BAA_TOKEN_BANG);

        case L'<':
            if (peek(lexer) == L'=') {
                advance(lexer);
                return make_token(lexer, BAA_TOKEN_LESS_EQUAL);
            }
            return make_token(lexer, BAA_TOKEN_LESS);

        case L'>':
            if (peek(lexer) == L'=') {
                advance(lexer);
                return make_token(lexer, BAA_TOKEN_GREATER_EQUAL);
            }
            return make_token(lexer, BAA_TOKEN_GREATER);

        case L'&':
            if (peek(lexer) == L'&') {
                advance(lexer);
                return make_token(lexer, BAA_TOKEN_AND);
            }
            break;

        case L'|':
            if (peek(lexer) == L'|') {
                advance(lexer);
                return make_token(lexer, BAA_TOKEN_OR);
            }
            break;

        case L'"':
            // Handle string literals
            while (peek(lexer) != L'"' && peek(lexer) != L'\0') {
                if (peek(lexer) == L'\n') {
                    return error_token(lexer, L"سلسلة غير منتهية");
                }
                
                // Handle escape sequences
                if (peek(lexer) == L'\\') {
                    advance(lexer); // Consume the backslash
                    
                    switch (peek(lexer)) {
                        case L'n':  // Newline
                        case L't':  // Tab
                        case L'r':  // Carriage return
                        case L'\\': // Backslash
                        case L'"':  // Double quote
                        case L'\'': // Single quote
                        case L'0':  // Null character
                            advance(lexer);
                            break;
                        case L'u':  // Unicode escape
                            advance(lexer); // Consume the 'u'
                            // Expect 4 hex digits
                            for (int i = 0; i < 4; i++) {
                                if (!iswxdigit(peek(lexer))) {
                                    return error_token(lexer, L"تسلسل يونيكود غير صالح");
                                }
                                advance(lexer);
                            }
                            break;
                        default:
                            return error_token(lexer, L"تسلسل هروب غير معروف");
                    }
                } else {
                    advance(lexer);
                }
            }

            if (peek(lexer) == L'\0') {
                return error_token(lexer, L"سلسلة غير منتهية");
            }

            // Consume the closing quote
            advance(lexer);
            return make_token(lexer, BAA_TOKEN_STRING_LIT);

        case L'\'':
            // Handle character literals
            if (peek(lexer) == L'\0') {
                return error_token(lexer, L"حرف غير منتهي");
            }
            
            // Handle escape sequences
            if (peek(lexer) == L'\\') {
                advance(lexer); // Consume the backslash
                
                switch (peek(lexer)) {
                    case L'n':  // Newline
                    case L't':  // Tab
                    case L'r':  // Carriage return
                    case L'\\': // Backslash
                    case L'"':  // Double quote
                    case L'\'': // Single quote
                    case L'0':  // Null character
                        advance(lexer);
                        break;
                    case L'u':  // Unicode escape
                        advance(lexer); // Consume the 'u'
                        // Expect 4 hex digits
                        for (int i = 0; i < 4; i++) {
                            if (!iswxdigit(peek(lexer))) {
                                return error_token(lexer, L"تسلسل يونيكود غير صالح");
                            }
                            advance(lexer);
                        }
                        break;
                    default:
                        return error_token(lexer, L"تسلسل هروب غير معروف");
                }
            } else {
                advance(lexer); // Consume the character
            }

            if (peek(lexer) != L'\'') {
                return error_token(lexer, L"حرف غير منتهي");
            }
            advance(lexer); // Consume the closing quote
            return make_token(lexer, BAA_TOKEN_CHAR_LIT);
    }

    return error_token(lexer, L"رمز غير معروف");
}

/**
 * Initialize a lexer with the given source code and filename
 */
void baa_init_lexer(BaaLexer* lexer, const wchar_t* source, const wchar_t* filename) {
    if (lexer) {
        lexer->source = source;
        lexer->start = 0;
        lexer->current = 0;
        lexer->line = 1;
        lexer->column = 1;
    }
}

/**
 * Get the next token from the lexer
 */
BaaToken* baa_lexer_next_token(BaaLexer* lexer) {
    return baa_scan_token(lexer);
}
