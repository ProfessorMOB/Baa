#ifndef BAA_UTILS_H
#define BAA_UTILS_H

#include <stddef.h>
#include <wchar.h>

// Error handling
typedef enum {
    BAA_نجاح = 0,                  // Success
    BAA_خطأ_ملف_غير_موجود,         // File not found
    BAA_خطأ_ذاكرة,                 // Memory error
    BAA_خطأ_صياغة,                 // Syntax error
    BAA_خطأ_دلالي,                 // Semantic error
    BAA_خطأ_توليد_الشفرة           // Code generation error
} BaaError;

// Error reporting
void baa_set_error(BaaError error, const wchar_t *message);
const wchar_t *baa_get_error_message(void);
BaaError baa_get_error(void);

// Memory management - ادارة الذاكرة
void *baa_malloc(size_t size);
void *baa_realloc(void *ptr, size_t size);
void baa_free(void *ptr);

// String utilities - أدوات النصوص
wchar_t *baa_strdup(const wchar_t *str);
int baa_strcmp(const wchar_t *s1, const wchar_t *s2);

#endif /* BAA_UTILS_H */
