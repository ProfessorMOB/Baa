#include "baa/utils/utils.h"
#include "baa/utils/errors.h"
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <stdio.h>

// Error handling state
static BaaError current_error = BAA_SUCCESS;
static wchar_t error_message[1024] = {0};

void baa_set_error(BaaError error, const wchar_t *message) {
    current_error = error;
    if (message) {
        wcsncpy(error_message, message, sizeof(error_message)/sizeof(wchar_t) - 1);
        error_message[sizeof(error_message)/sizeof(wchar_t) - 1] = L'\0';
    } else {
        error_message[0] = L'\0';
    }
}

const wchar_t *baa_get_error_message(void) {
    return error_message;
}

BaaError baa_get_error(void) {
    return current_error;
}

void *baa_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        baa_set_error(BAA_ERROR_MEMORY, L"فشل في تخصيص الذاكرة");
    }
    return ptr;
}

void *baa_realloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        baa_set_error(BAA_ERROR_MEMORY, L"فشل في إعادة تخصيص الذاكرة");
    }
    return new_ptr;
}

void baa_free(void *ptr) {
    free(ptr);
}

wchar_t *baa_strdup(const wchar_t *str) {
    if (!str) {
        return NULL;
    }

    size_t len = wcslen(str) + 1;
    wchar_t *new_str = baa_malloc(len * sizeof(wchar_t));

    if (new_str) {
        wcscpy(new_str, str);
    }

    return new_str;
}

wchar_t *baa_strndup(const wchar_t *str, size_t n) {
    if (!str) {
        return NULL;
    }

    size_t len = wcsnlen(str, n);
    wchar_t *new_str = baa_malloc((len + 1) * sizeof(wchar_t));

    if (new_str) {
        wcsncpy(new_str, str, len);
        new_str[len] = L'\0';
    }

    return new_str;
}

int baa_strcmp(const wchar_t *s1, const wchar_t *s2) {
    if (!s1 || !s2) {
        return s1 ? 1 : (s2 ? -1 : 0);
    }
    return wcscmp(s1, s2);
}

/**
 * Read the contents of a file into a wide string
 *
 * @param filename The path to the file to read
 * @return A newly allocated string with the file contents, or NULL on error
 */
wchar_t* baa_read_file(const wchar_t* filename) {
    FILE* file = NULL;

#ifdef _WIN32
    file = _wfopen(filename, L"rb");
#else
    // Convert wchar_t* to char* for non-Windows systems
    size_t len = wcslen(filename);
    char* narrow_filename = (char*)malloc(len * 4 + 1); // UTF-8 can use up to 4 bytes per character
    if (!narrow_filename) {
        baa_set_error(BAA_ERROR_MEMORY, L"فشل في تخصيص الذاكرة");
        return NULL;
    }

    wcstombs(narrow_filename, filename, len * 4);
    narrow_filename[len * 4] = '\0';

    file = fopen(narrow_filename, "rb");
    free(narrow_filename);
#endif

    if (!file) {
        baa_set_error(BAA_ERROR_FILE_OPEN, L"فشل في فتح الملف");
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(file);
        baa_set_error(BAA_ERROR_FILE_EMPTY, L"الملف فارغ أو غير صالح");
        return NULL;
    }

    // Allocate buffer for file content
    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fclose(file);
        baa_set_error(BAA_ERROR_MEMORY, L"فشل في تخصيص الذاكرة");
        return NULL;
    }

    // Read file content
    size_t read_size = fread(buffer, 1, file_size, file);
    fclose(file);

    if (read_size != (size_t)file_size) {
        free(buffer);
        baa_set_error(BAA_ERROR_FILE_READ, L"فشل في قراءة الملف");
        return NULL;
    }

    buffer[file_size] = '\0';

    // Convert to wide string
    size_t wide_len = mbstowcs(NULL, buffer, 0) + 1;
    if (wide_len == (size_t)-1) {
        free(buffer);
        baa_set_error(BAA_ERROR_ENCODING, L"فشل في تحويل الترميز");
        return NULL;
    }

    wchar_t* wide_buffer = (wchar_t*)malloc(wide_len * sizeof(wchar_t));
    if (!wide_buffer) {
        free(buffer);
        baa_set_error(BAA_ERROR_MEMORY, L"فشل في تخصيص الذاكرة");
        return NULL;
    }

    mbstowcs(wide_buffer, buffer, wide_len);
    free(buffer);

    return wide_buffer;
}
