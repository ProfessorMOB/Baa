#ifndef BAA_UTILS_H
#define BAA_UTILS_H

#include <stddef.h>
#include <wchar.h>

// Error handling
typedef enum {
    BAA_SUCCESS = 0,
    BAA_ERROR_FILE_NOT_FOUND,
    BAA_ERROR_MEMORY,
    BAA_ERROR_SYNTAX,
    BAA_ERROR_SEMANTIC,
    BAA_ERROR_CODEGEN
} BaaError;

// Error reporting
void baa_set_error(BaaError error, const wchar_t *message);
const wchar_t *baa_get_error_message(void);
BaaError baa_get_error(void);

// Memory management
void *baa_malloc(size_t size);
void *baa_realloc(void *ptr, size_t size);
void baa_free(void *ptr);

// String utilities
wchar_t *baa_strdup(const wchar_t *str);
int baa_strcmp(const wchar_t *s1, const wchar_t *s2);

#endif /* BAA_UTILS_H */
