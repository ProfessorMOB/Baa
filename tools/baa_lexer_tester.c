// Simplified main in baa_lexer_tester.c or a new file
#include "baa/lexer/lexer.h"
#include "baa/utils/utils.h" // For baa_strdup if needed, and baa_file_content if we use it
#include <stdio.h>
#include <wchar.h>
#include <locale.h>

void print_token_for_debug(const BaaToken *token, const char *context_msg)
{
    if (!token)
    {
        fwprintf(stderr, L"%hs: NULL TOKEN\n", context_msg);
        return;
    }
    const wchar_t *type_str = baa_token_type_to_string(token->type);
    fwprintf(stderr, L"%hs: Type=%ls (%d), Lexeme='", context_msg, type_str, token->type);
    for (size_t i = 0; i < token->length; ++i)
    {
        putwc(token->lexeme[i], stderr);
    }
    fwprintf(stderr, L"', Len=%zu, Line=%zu, Col=%zu\n",
             token->length, token->line, token->column);
    fflush(stderr);
}

int main()
{
    setlocale(LC_ALL, "");

    // Preprocessed string, matching the problematic scenario
    const wchar_t *source = L"@//comment\nعدد_صحيح س";
    // Alternative to be absolutely sure about preprocessor output:
    // Use your preprocessor to generate this string from a file, then feed it here.

    BaaLexer lexer;
    baa_init_lexer(&lexer, source, L"test_direct_lex.baa");

    BaaToken *token1, *token2, *token3;

    fwprintf(stderr, L"--- Direct Lexer Test ---\n");

    // Token 1: Should be error for @
    token1 = baa_lexer_next_token(&lexer);
    print_token_for_debug(token1, "Token 1 (Lexer)");
    if (token1)
        baa_free_token(token1);

    // Token 2: Should be "عدد_صحيح"
    token2 = baa_lexer_next_token(&lexer);
    print_token_for_debug(token2, "Token 2 (Lexer)");
    if (token2)
        baa_free_token(token2);

    // Token 3: Should be "س"
    token3 = baa_lexer_next_token(&lexer);
    print_token_for_debug(token3, "Token 3 (Lexer)");
    if (token3)
        baa_free_token(token3);

    // ... continue for more tokens if needed

    fwprintf(stderr, L"--- End Direct Lexer Test ---\n");

    return 0;
}
