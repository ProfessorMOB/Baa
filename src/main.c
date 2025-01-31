#include "baa/lexer.h"
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
    mbstowcs_s(&converted, wstr, len, str, len - 1);
    return wstr;
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
    
    wchar_t *content = baa_file_content(path);
    free(path);
    
    if (!content) {
        return 1;
    }
    
    printf("محتوى الملف %s:\n---\n%ls\n---\n", argv[1], content);
    free(content);
    return 0;
}