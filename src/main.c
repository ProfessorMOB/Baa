#include "baa/lexer/lexer.h"
#include "baa/parser/parser.h"
#include "baa/codegen/codegen.h"
#include "baa/utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

/**
 * @brief Print usage information.
 *
 * This function prints the correct usage of the program to the console.
 *
 * @param argv The argument vector containing the program name and arguments.
 */
static void print_usage(const char *program_name) {
    printf("الاستخدام خاطئ\n");
    printf("الرجاء استخدام الأمر بالشكل الصحيح\n");
    printf("%s \n<المسار_إلي_الملف_المراد_تجميعه>", program_name);
}

/**
 * @brief Convert char* to wchar_t*
 *
 * This function converts a regular string to a wide string.
 *
 * @param str The string to convert
 * @return A newly allocated wide string that must be freed by the caller
 */
static wchar_t *char_to_wchar(const char *str) {
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
    wstr[converted] = L'\0';
#endif
    return wstr;
}

/**
 * @brief Compile a Baa source file
 *
 * This function compiles a Baa source file to LLVM IR.
 *
 * @param filename The path to the source file
 * @return 0 on success, non-zero on failure
 */
static int compile_file(const wchar_t *filename) {
    // Read file content
    wchar_t *source = baa_read_file(filename);
    if (!source) {
        wprintf(L"فشل في قراءة الملف: %ls\n", filename);
        return 1;
    }

    // Initialize lexer
    BaaLexer lexer;
    baa_init_lexer(&lexer, source, filename);

    // Initialize parser
    BaaParser parser;
    baa_init_parser(&parser, &lexer);

    // Parse program
    BaaProgram *program = baa_parse_program(&parser);
    if (!program) {
        wprintf(L"فشل في تحليل الملف: %ls\n", filename);
        free(source);
        return 1;
    }

    // Create output filename (replace .ب with .ll)
    size_t len = wcslen(filename);
    wchar_t *output_filename = (wchar_t *)malloc((len + 4) * sizeof(wchar_t));
    if (!output_filename) {
        wprintf(L"فشل في تخصيص الذاكرة\n");
        baa_free_program(program);
        free(source);
        return 1;
    }

    wcscpy(output_filename, filename);
    wchar_t *ext = wcsrchr(output_filename, L'.');
    if (ext) {
        wcscpy(ext, L".ll");
    } else {
        wcscat(output_filename, L".ll");
    }

    // Initialize code generator
    BaaCodeGen codegen;
    BaaCodeGenOptions options;
    options.target = BAA_TARGET_X86_64;
    options.optimize = true;
    options.debug_info = true;
    options.output_file = output_filename;

    baa_init_codegen(&codegen, program, &options);

    // Generate code
    if (!baa_generate_code(&codegen)) {
        wprintf(L"فشل في توليد الكود: %ls\n", baa_get_codegen_error(&codegen));
        free(output_filename);
        baa_free_program(program);
        free(source);
        return 1;
    }

    wprintf(L"تم توليد الكود بنجاح. تم كتابة المخرجات إلى %ls\n", output_filename);

    // Clean up
    baa_cleanup_codegen();
    free(output_filename);
    baa_free_program(program);
    free(source);

    return 0;
}

/**
 * @brief The main entry point of the program.
 *
 * This function checks the number of command-line arguments and processes the input file.
 *
 * @param argc The argument count.
 * @param argv The argument vector.
 * @return An integer representing the exit status of the program.
 */
int main(int argc, char **argv) {
    setlocale(LC_ALL, "");  // Set locale for proper wide char handling

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    wchar_t *path = char_to_wchar(argv[1]);
    if (!path) {
        printf("فشل في تحويل مسار الملف\n");
        return 1;
    }

    int result = compile_file(path);
    free(path);

    return result;
}
