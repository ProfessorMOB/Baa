#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>

#include "baa/compiler.h"
#include "baa/utils/utils.h"
#include "baa/lexer/lexer.h"
#include "baa/parser/parser.h"
#include "baa/codegen/codegen.h"
#include "baa/preprocessor/preprocessor.h"

// Helper: Convert char* to wchar_t* (might move to utils later)
static wchar_t *char_to_wchar_compiler(const char *str) {
    size_t len = strlen(str) + 1;
    wchar_t *wstr = malloc(len * sizeof(wchar_t));
    if (!wstr) {
        return NULL;
    }
    size_t converted;
#ifdef _WIN32
    mbstowcs_s(&converted, wstr, len, str, len - 1);
#else
    converted = mbstowcs(wstr, str, len - 1);
    wstr[converted] = L'\\0';
#endif
    return wstr;
}

int compile_baa_file(const char* filename) {
    // Set locale for wide character support
    setlocale(LC_ALL, "");

    // Call Preprocessor
    wchar_t* error_message = NULL;
    wchar_t* source = baa_preprocess(filename, NULL, &error_message);

    if (!source) {
        if (error_message) {
            fwprintf(stderr, L"Error (Preprocessor): %ls\\n", error_message);
            free(error_message);
        } else {
            fwprintf(stderr, L"Error: Preprocessing failed for file %hs (unknown error).\\n", filename);
        }
        return 1;
    }

    // Convert filename to wide string for lexer/output path
    wchar_t* wfilename = char_to_wchar_compiler(filename);
    if (!wfilename) {
        fprintf(stderr, "Error: Failed to convert filename to wchar_t.\\n");
        free(source);
        return 1;
    }

    // Initialize lexer
    BaaLexer lexer;
    baa_init_lexer(&lexer, source, wfilename);

    // Initialize parser
    BaaParser parser;
    baa_init_parser(&parser, &lexer);

    // Parse program
    BaaProgram* program = baa_parse_program(&parser);
    if (!program) {
        fprintf(stderr, "Error: Parsing failed.\\n");
        free(source);
        free(wfilename);
        return 1;
    }

    // Initialize code generator
    BaaCodeGen codegen;
    BaaCodeGenOptions options;
    options.target = BAA_TARGET_X86_64;
    options.optimize = true;
    options.debug_info = true;

    // Create output filename (replace .ب with .ll)
    size_t wlen = wcslen(wfilename);
    wchar_t* output_filename = (wchar_t*)malloc((wlen + 4) * sizeof(wchar_t));
    if (!output_filename) {
        fprintf(stderr, "Error: Memory allocation failed for output filename.\\n");
        baa_free_program(program);
        free(source);
        free(wfilename);
        return 1;
    }

    wcscpy(output_filename, wfilename);
    wchar_t* ext = wcsrchr(output_filename, L'.');
    if (ext) {
        wcscpy(ext, L".ll");
    } else {
        wcscat(output_filename, L".ll");
    }

    options.output_file = output_filename;

    // Initialize code generator
    baa_init_codegen(&codegen, program, &options);

    // Generate code
    if (!baa_generate_code(&codegen)) {
        fprintf(stderr, "Error: Code generation failed: %ls\\n", baa_get_codegen_error(&codegen));
        free(output_filename);
        baa_free_program(program);
        free(source);
        free(wfilename);
        return 1;
    }

    wprintf(L"Code generation successful. Output written to %ls\\n", output_filename);

    // Clean up
    baa_cleanup_codegen();
    free(output_filename);
    baa_free_program(program);
    free(source);
    free(wfilename);

    return 0;
}
