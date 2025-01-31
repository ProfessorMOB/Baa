#include "baa/lexer.h"
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

long baa_file_size(FILE *file) {
    if (!file) {
        return 0;
    }
    fpos_t original = 0;
    if (fgetpos(file, &original) != 0) {
        printf("fgetpos() فشلت دالة: %i \n", errno);
        return 0;
    }
    fseek(file, 0, SEEK_END);
    long out = ftell(file);
    if (fsetpos(file, &original) != 0) {
        printf("fsetpos() فشلت دالة: %i \n", errno);
    }
    return out;
}

wchar_t *baa_file_content(const wchar_t *path) {
    FILE *file;
    errno_t err = _wfopen_s(&file, path, L"rb, ccs=UTF-16LE");
    if (err != 0) {
        printf("لا يمكن فتح الملف\n");
        return NULL;
    }

    // Skip BOM if present
    wchar_t bom;
    if (fread(&bom, sizeof(wchar_t), 1, file) == 1) {
        if (bom != 0xFEFF) {
            fseek(file, 0, SEEK_SET);
        }
    }

    long file_size = baa_file_size(file);
    size_t char_count = (file_size / sizeof(wchar_t)) + 1; // +1 for null terminator
    wchar_t *contents = malloc(char_count * sizeof(wchar_t));
    
    if (!contents) {
        fclose(file);
        return NULL;
    }

    size_t chars_read = fread(contents, sizeof(wchar_t), char_count - 1, file);
    contents[chars_read] = L'\0';
    
    fclose(file);
    return contents;
}
