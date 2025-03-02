#ifndef BAA_ERRORS_H
#define BAA_ERRORS_H

#include <wchar.h>

// أنواع الأخطاء
typedef enum {
    BAA_SUCCESS = 0,           // نجاح
    BAA_ERROR_FILE_NOT_FOUND,  // ملف غير موجود
    BAA_ERROR_MEMORY,          // خطأ في الذاكرة
    BAA_ERROR_SYNTAX,          // خطأ في الصياغة
    BAA_ERROR_SEMANTIC,        // خطأ في المعنى
    BAA_ERROR_CODEGEN,         // خطأ في توليد الشيفرة
    BAA_ERROR_FILE_OPEN,       // فشل في فتح الملف
    BAA_ERROR_FILE_EMPTY,      // الملف فارغ
    BAA_ERROR_FILE_READ,       // فشل في قراءة الملف
    BAA_ERROR_ENCODING         // خطأ في الترميز
} BaaError;

// دوال معالجة الأخطاء
void baa_set_error(BaaError error, const wchar_t* message);
const wchar_t* baa_get_error_message(void);
BaaError baa_get_error(void);

#endif /* BAA_ERRORS_H */
