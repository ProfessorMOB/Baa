#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>

// Include necessary Baa headers
#include "baa/utils/utils.h"
#include "baa/preprocessor/preprocessor.h"
#include "baa/lexer/lexer.h"
#include "baa/parser/parser.h" // Public API
#include "baa/ast/ast.h"       // For BaaNode, even if baa_parse_program is a stub

// Internal parser structure for inspection (ONLY FOR TESTING - NOT FOR PRODUCTION USE)
// This is a bit of a hack to inspect internal state. In a real test suite,
// you'd rely on public API or specific test interfaces.
typedef struct BaaParser_TestInspect
{
    BaaLexer *lexer;
    BaaToken current_token;
    BaaToken previous_token;
    const char *source_filename;
    bool had_error;
    bool panic_mode;
} BaaParser_TestInspect;

// Helper: Convert char* to wchar_t*
static wchar_t *char_to_wchar_tester(const char *str)
{
    if (!str)
        return NULL;
    size_t len = strlen(str) + 1;
    wchar_t *wstr = (wchar_t *)malloc(len * sizeof(wchar_t));
    if (!wstr)
    {
        return NULL;
    }
    size_t converted;
#ifdef _WIN32
    mbstowcs_s(&converted, wstr, len, str, len - 1);
#else
    converted = mbstowcs(wstr, str, len - 1);
    if (converted == (size_t)-1)
    {
        free(wstr);
        return NULL;
    }
    wstr[converted] = L'\0';
#endif
    return wstr;
}

// Helper function to print wide strings correctly
void print_wide_string_parser_tester(FILE *stream, const wchar_t *wstr)
{
    if (!wstr)
    {
        fwprintf(stream, L"(null)");
        return;
    }
    if (fwprintf(stream, L"%ls", wstr) < 0)
    {
        fprintf(stderr, "\n[Warning: fwprintf failed for wide string. Attempting fallback.]\n");
        // Basic fallback for environments where fwprintf might struggle with console output
        while (*wstr)
        {
            if (putwc(*wstr, stream) == WEOF)
                break;
            wstr++;
        }
    }
}

void print_token_details(const char *label, const BaaToken *token)
{
    if (!token)
    {
        wprintf(L"%hs: Token is NULL\n", label);
        return;
    }
    wprintf(L"%hs: Type=%ls (%d), Lexeme='", label, baa_token_type_to_string(token->type), token->type);
    print_wide_string_parser_tester(stdout, token->lexeme);
    wprintf(L"', Line=%zu, Col=%zu, Len=%zu\n", token->line, token->column, token->length);
}

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <input_file.baa>\n", argv[0]);
        return 1;
    }

    const wchar_t *input_filename_char = argv[1];
    wchar_t *w_input_filename = char_to_wchar_tester(input_filename_char);
    if (!w_input_filename)
    {
        fprintf(stderr, "Error: Failed to convert input filename to wchar_t.\n");
        return 1;
    }

    wprintf(L"--- Baa Parser Tester ---\n");
    wprintf(L"Input file: %ls\n", input_filename_char);

    // 1. Preprocess
    wprintf(L"\n[PHASE 1: PREPROCESSING]\n");
    wchar_t *pp_error_message = NULL;
    BaaPpSource pp_source = {
        .type = BAA_PP_SOURCE_FILE,
        .source_name = input_filename_char, // For error context in preprocessor
        .data.file_path = input_filename_char};
    wchar_t *source_code = baa_preprocess(&pp_source, NULL, &pp_error_message);

    if (!source_code)
    {
        fwprintf(stderr, L"Preprocessor Error:\n");
        if (pp_error_message)
        {
            print_wide_string_parser_tester(stderr, pp_error_message);
            free(pp_error_message);
        }
        else
        {
            fwprintf(stderr, L"Unknown preprocessor error for file %hs.\n", input_filename_char);
        }
        fwprintf(stderr, L"\n");
        free(w_input_filename);
        return 1;
    }
    wprintf(L"Preprocessing successful.\n");
    // wprintf(L"Preprocessed Code:\n%ls\n", source_code); // Uncomment to see preprocessed output

    // 2. Lexer Initialization (for the parser)
    wprintf(L"\n[PHASE 2: LEXER INITIALIZATION FOR PARSER]\n");
    BaaLexer lexer;
    baa_init_lexer(&lexer, source_code, w_input_filename); // Pass wide filename
    wprintf(L"Lexer initialized for parser.\n");

    // 3. Parser Creation & Initial State Inspection
    wprintf(L"\n[PHASE 3: PARSER CREATION & TOKEN STREAM]\n");
    BaaParser *parser = baa_parser_create(&lexer, input_filename_char);
    if (!parser)
    {
        fwprintf(stderr, L"Error: Failed to create parser.\n");
        free(source_code);
        free(w_input_filename);
        return 1;
    }
    wprintf(L"Parser created successfully.\n");

    // Inspect the internal state of the parser (using the test-only struct cast)
    BaaParser_TestInspect *parser_inspect = (BaaParser_TestInspect *)parser;

    int token_count = 0;
    wprintf(L"Iterating through tokens via parser's internal lexer consumption:\n");
    do
    {
        wprintf(L"\nIteration %d:\n", token_count);
        print_token_details("  Parser->previous_token", &parser_inspect->previous_token);
        print_token_details("  Parser->current_token ", &parser_inspect->current_token);
        wprintf(L"  Parser: had_error=%s, panic_mode=%s\n",
                parser_inspect->had_error ? L"true" : L"false",
                parser_inspect->panic_mode ? L"true" : L"false");

        if (parser_inspect->current_token.type == BAA_TOKEN_EOF)
        {
            wprintf(L"EOF reached.\n");
            break;
        }
        if (parser_inspect->current_token.type == BAA_TOKEN_ERROR && token_count > 0)
        {
            // If current token IS an error, it means parser_error_at_token was called
            // by advance() because the lexer itself returned an error.
            // `advance` would have reported it. The loop will break below for ERROR.
        }

        // IMPORTANT: We cannot call the *static* advance() from parser.c here.
        // The baa_parser_create call already primed the first token by calling advance().
        // To see subsequent tokens as the parser *would* see them, we would need to
        // call a parsing function. Since baa_parse_program is a stub, we simulate.
        // The most we can do without modifying parser.c for testability is to see
        // the state *after* parser creation (which includes the first advance).
        //
        // If `baa_parse_program` was implemented to advance, we'd call that.
        // For now, to see the token stream that `advance` in `parser.c` *would* process,
        // we effectively just need to observe the initial state and how it handles
        // the first token if it's a lexical error.

        // The test for now is mainly to see the first current_token and previous_token after parser creation.
        // And to check if `had_error` is set if the very first token processed by `advance` (during `baa_parser_create`)
        // was a BAA_TOKEN_ERROR from the lexer.
        if (token_count == 0 && parser_inspect->current_token.type == BAA_TOKEN_ERROR)
        {
            wprintf(L"Lexical error detected on first token by parser's initial advance.\n");
            // `parser_error_at_token` should have been called internally by `advance`
        }

        // Simulate parser consuming tokens one by one for observation.
        // This is NOT how the real parser works but helps inspect what tokens `advance()` would process.
        // This is a bit artificial as `advance` is internal to the parser.
        // We are effectively re-lexing here for the *tester tool's* observation loop.
        // The parser's *internal* `current_token` and `previous_token` would be managed by its *internal* `advance`.
        BaaToken *next_obs_token = baa_lexer_next_token(parser_inspect->lexer);
        if (!next_obs_token)
        {
            wprintf(L"Lexer returned NULL, critical error.\n");
            break;
        }
        // "Manually" update the inspected tokens for the *tester's* observational loop
        // This does NOT reflect the internal `previous_token` of the parser after its *internal* `advance` calls.
        if (parser_inspect->previous_token.lexeme)
            baa_free((void *)parser_inspect->previous_token.lexeme);
        parser_inspect->previous_token = parser_inspect->current_token; // For the tester's display
        parser_inspect->current_token = *next_obs_token;                // For the tester's display
        next_obs_token->lexeme = NULL;                                  // Sever ownership
        baa_free_token(next_obs_token);

        token_count++;
        if (token_count > 50)
        { // Safety break for the tester loop
            wprintf(L"Tester loop safety break.\n");
            break;
        }

    } while (parser_inspect->current_token.type != BAA_TOKEN_EOF && parser_inspect->current_token.type != BAA_TOKEN_ERROR);

    if (parser_inspect->current_token.type == BAA_TOKEN_ERROR)
    {
        wprintf(L"\nLoop terminated due to BAA_TOKEN_ERROR in observed stream.\n");
    }

    // 4. Attempt to "parse" (call the stub)
    // BaaNode *ast_root = baa_parse_program(parser); // Currently a stub
    // wprintf(L"\n[PHASE 4: PARSING (STUB)]\n");
    // if (ast_root) {
    //     wprintf(L"baa_parse_program returned a non-NULL BaaNode (unexpected for stub).\n");
    //     // baa_ast_free_node(ast_root); // If we had an AST printer and actual parsing
    // } else if (baa_parser_had_error(parser)) {
    //     wprintf(L"baa_parse_program returned NULL, and parser reported errors (as expected if errors occurred).\n");
    // } else {
    //     wprintf(L"baa_parse_program returned NULL, and parser reported no errors (expected for stub on valid input).\n");
    // }

    // 5. Cleanup
    wprintf(L"\n[PHASE 5: CLEANUP]\n");
    baa_parser_free(parser);
    wprintf(L"Parser freed.\n");
    free(source_code);
    wprintf(L"Source code buffer freed.\n");
    free(w_input_filename);
    wprintf(L"Filename buffer freed.\n");

    wprintf(L"\n--- Parser Tester Finished ---\n");
    return 0;
}
