#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>

#include "baa/utils/utils.h"
#include "baa/lexer/lexer.h"
#include "baa/parser/parser.h"
#include "baa/codegen/codegen.h"

int compile_baa_file(const char* filename) {
    // Set locale for wide character support
    setlocale(LC_ALL, "");

    // Convert filename to wide string
    size_t len = strlen(filename) + 1;
    wchar_t* wfilename = (wchar_t*)malloc(len * sizeof(wchar_t));
    if (!wfilename) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 1;
    }

    for (size_t i = 0; i < len; i++) {
        wfilename[i] = (wchar_t)filename[i];
    }

    // Read file content
    wchar_t* source = baa_read_file(wfilename);
    if (!source) {
        fprintf(stderr, "Error: Cannot read file %s\n", filename);
        free(wfilename);
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
        fprintf(stderr, "Error: Parsing failed\n");
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
    wchar_t* output_filename = (wchar_t*)malloc((len + 3) * sizeof(wchar_t));
    if (!output_filename) {
        fprintf(stderr, "Error: Memory allocation failed\n");
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
        fprintf(stderr, "Error: Code generation failed: %ls\n", baa_get_codegen_error(&codegen));
        free(output_filename);
        baa_free_program(program);
        free(source);
        free(wfilename);
        return 1;
    }

    printf("Code generation successful. Output written to %ls\n", output_filename);

    // Clean up
    baa_cleanup_codegen();
    free(output_filename);
    baa_free_program(program);
    free(source);
    free(wfilename);

    return 0;
}

// Renamed from main to avoid duplicate definition
int baa_compiler_main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename.ب>\n", argv[0]);
        return 1;
    }

    return compile_baa_file(argv[1]);
}
