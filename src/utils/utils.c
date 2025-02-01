#include "baa/utils.h"
#include "baa_errors.h"
#include <stdlib.h>
#include <string.h>

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

int baa_strcmp(const wchar_t *s1, const wchar_t *s2) {
    if (!s1 || !s2) {
        return s1 ? 1 : (s2 ? -1 : 0);
    }
    return wcscmp(s1, s2);
}
