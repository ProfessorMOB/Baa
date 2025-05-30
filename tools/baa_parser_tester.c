// tools/baa_parser_tester.c
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

#ifdef _WIN32
#include <windows.h> // For GetCommandLineW, CommandLineToArgvW, LocalFree
#endif

// Internal parser structure for inspection (ONLY FOR TESTING - NOT FOR PRODUCTION USE)
// This is a bit of a hack to inspect internal state. In a real test suite,
// you'd rely on public API or specific test interfaces.
typedef struct BaaParser_TestInspect
{
    BaaLexer *lexer;
    BaaToken current_token;
    BaaToken previous_token;
    // Order must EXACTLY match BaaParser in src/parser/parser_internal.h
    bool had_error;
    bool panic_mode;
    const wchar_t *source_filename; // Corrected order based on src/parser/parser_internal.h
} BaaParser_TestInspect;

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
    (void)argc; // Suppress unused parameter warning for non-Windows
    (void)argv; // Suppress unused parameter warning for non-Windows

    setlocale(LC_ALL, "");

    const wchar_t *w_input_filename = NULL;
    char *input_filename_char_for_pp = NULL; // char* version for BaaPpSource

#ifdef _WIN32
    int nArgs;
    LPWSTR *szArgList = CommandLineToArgvW(GetCommandLineW(), &nArgs);
    if (NULL == szArgList || nArgs < 2)
    {
        fprintf(stderr, "Usage: baa_parser_tester <input_file.baa>\n");
        if (szArgList)
            LocalFree(szArgList);
        return 1;
    }
    // Duplicate the wide string argument because LocalFree will deallocate szArgList later.
    w_input_filename = baa_strdup(szArgList[1]); // Use your utils' strdup
    if (!w_input_filename)
    {
        fprintf(stderr, "Error: Failed to duplicate input filename.\n");
        LocalFree(szArgList);
        return 1;
    }
    LocalFree(szArgList);

    // Convert w_input_filename to char* (UTF-8) for BaaPpSource
    int required_bytes = WideCharToMultiByte(CP_UTF8, 0, w_input_filename, -1, NULL, 0, NULL, NULL);
    if (required_bytes <= 0)
    {
        fprintf(stderr, "Error: Could not determine size for UTF-8 conversion of filename.\n");
        baa_free((void *)w_input_filename);
        return 1;
    }
    input_filename_char_for_pp = (char *)baa_malloc(required_bytes);
    if (!input_filename_char_for_pp)
    { // Check malloc result
        fprintf(stderr, "Error: Failed to allocate memory for UTF-8 filename.\n");
        baa_free((void *)w_input_filename);
        return 1;
    }
    WideCharToMultiByte(CP_UTF8, 0, w_input_filename, -1, input_filename_char_for_pp, required_bytes, NULL, NULL);

#else // For non-Windows, use argv directly and assume it's UTF-8 or compatible
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <input_file.baa>\n", argv[0]);
        return 1;
    }
    input_filename_char_for_pp = baa_strdup_char(argv[1]); // Duplicate for safety
    if (!input_filename_char_for_pp)
    {
        fprintf(stderr, "Error: Failed to duplicate input filename (char*).\n");
        return 1;
    }
    // Convert char* (hopefully UTF-8) to wchar_t* for display and lexer init
    size_t len_check = mbstowcs(NULL, input_filename_char_for_pp, 0);
    if (len_check == (size_t)-1)
    {
        fprintf(stderr, "Error: Invalid multibyte sequence in input filename or failed to calculate length.\n");
        baa_free(input_filename_char_for_pp);
        return 1;
    }
    w_input_filename = (wchar_t *)baa_malloc((len_check + 1) * sizeof(wchar_t));
    if (!w_input_filename)
    {
        fprintf(stderr, "Error: Failed to allocate memory for widechar input filename.\n");
        baa_free(input_filename_char_for_pp);
        return 1;
    }
    if (mbstowcs(w_input_filename, input_filename_char_for_pp, len_check + 1) == (size_t)-1)
    {
        fprintf(stderr, "Error: Failed to convert input filename to wide characters.\n");
        baa_free((void *)w_input_filename);
        baa_free(input_filename_char_for_pp);
        return 1;
    }
#endif

    wprintf(L"--- Baa Parser Tester ---\n");
    wprintf(L"Input file: ");
    print_wide_string_parser_tester(stdout, w_input_filename);
    wprintf(L"\n");

    // 1. Preprocess
    wprintf(L"\n[PHASE 1: PREPROCESSING]\n");
    wchar_t *pp_error_message = NULL;
    BaaPpSource pp_source = {
        .type = BAA_PP_SOURCE_FILE,
        .source_name = input_filename_char_for_pp, // Use the char* version for BaaPpSource
        .data.file_path = input_filename_char_for_pp};
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
            fwprintf(stderr, L"Unknown preprocessor error for file %s.\n", input_filename_char_for_pp);
        }
        fwprintf(stderr, L"\n");
        baa_free((void *)w_input_filename); // Cast as it might be const from szArgList path originally
        baa_free(input_filename_char_for_pp);
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
    BaaParser *parser = baa_parser_create(&lexer, w_input_filename); // Pass wchar_t* for source_filename
    if (!parser)
    {
        fwprintf(stderr, L"Error: Failed to create parser.\n");
        free(source_code);
        baa_free(input_filename_char_for_pp);
        baa_free((void *)w_input_filename);
        return 1;
    }
    wprintf(L"Parser created successfully.\n");

    // --- Direct inspection immediately after creation ---
    BaaParser_TestInspect *parser_inspect_direct = (BaaParser_TestInspect *)parser;
    printf("Direct check after create: had_error=%d, panic_mode=%d (0=false, 1=true)\n",
           parser_inspect_direct->had_error,
           parser_inspect_direct->panic_mode);
    // --- End direct inspection ---

    BaaParser_TestInspect *parser_inspect = (BaaParser_TestInspect *)parser;

    int token_count = 0;
    wprintf(L"Iterating through tokens via parser's internal lexer consumption:\n");
    do
    {
        wprintf(L"\nIteration %d:\n", token_count);
        print_token_details("  Parser->previous_token", &parser_inspect->previous_token);
        print_token_details("  Parser->current_token ", &parser_inspect->current_token);

        // --- Direct inspection inside the loop ---
        printf("  Direct check in loop: had_error=%d, panic_mode=%d\n",
               parser_inspect->had_error,
               parser_inspect->panic_mode);
        // --- End direct inspection ---

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
        // "Manually" update the inspected tokens for the *tester's* display loop
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
    baa_free((void *)w_input_filename); // Cast as it might be const from szArgList path originally
    baa_free(input_filename_char_for_pp);
    wprintf(L"Filename buffer freed.\n");

    wprintf(L"\n--- Parser Tester Finished ---\n");
    return 0;
}
