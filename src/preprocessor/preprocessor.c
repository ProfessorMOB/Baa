#include "baa/preprocessor/preprocessor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// TODO: Include necessary headers for file operations, path manipulation
#ifdef _WIN32
#include <windows.h>
#include <direct.h> // _getcwd
#define PATH_SEPARATOR '\\'
#define MAX_PATH_LEN MAX_PATH
#else
#include <unistd.h> // getcwd
#include <limits.h> // PATH_MAX
#include <libgen.h> // dirname
#define PATH_SEPARATOR '/'
#define MAX_PATH_LEN PATH_MAX
#endif

// --- Helper Functions ---

// Helper to format error messages (similar to make_error_token)
wchar_t* format_preprocessor_error(const wchar_t* format, ...) {
    va_list args;
    va_start(args, format);

    // Determine required size
    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vswprintf(NULL, 0, format, args_copy);
    va_end(args_copy);

    if (needed < 0) {
        va_end(args);
        // Fallback error message
        const wchar_t* fallback = L"فشل في تنسيق رسالة خطأ المعالج المسبق.";
        wchar_t* error_msg = wcsdup(fallback);
        return error_msg;
    }

    size_t buffer_size = (size_t)needed + 1;
    wchar_t* buffer = malloc(buffer_size * sizeof(wchar_t));
    if (!buffer) {
        va_end(args);
         const wchar_t* fallback = L"فشل في تخصيص الذاكرة لرسالة خطأ المعالج المسبق.";
        wchar_t* error_msg = wcsdup(fallback);
        return error_msg; // Allocation failed
    }

    vswprintf(buffer, buffer_size, format, args);
    va_end(args);

    return buffer;
}

// Reads the content of a UTF-16LE encoded file.
// Returns a dynamically allocated wchar_t* string (caller must free).
// Returns NULL on error and sets error_message.
static wchar_t* read_file_content_utf16le(const char* file_path, wchar_t** error_message) {
    *error_message = NULL;
    FILE* file = NULL;
    wchar_t* buffer = NULL;

#ifdef _WIN32
    // Use _wfopen on Windows for potentially non-ASCII paths
    // Convert UTF-8 path (common) to UTF-16
    int required_wchars = MultiByteToWideChar(CP_UTF8, 0, file_path, -1, NULL, 0);
    if (required_wchars <= 0) {
        *error_message = format_preprocessor_error(L"فشل في تحويل مسار الملف '%hs' إلى UTF-16.", file_path);
        return NULL;
    }
    wchar_t* w_file_path = malloc(required_wchars * sizeof(wchar_t));
    if (!w_file_path) {
         *error_message = format_preprocessor_error(L"فشل في تخصيص الذاكرة لمسار الملف (UTF-16) '%hs'.", file_path);
         return NULL;
    }
    MultiByteToWideChar(CP_UTF8, 0, file_path, -1, w_file_path, required_wchars);
    file = _wfopen(w_file_path, L"rb");
    free(w_file_path);
#else
    // Standard fopen for non-Windows
    file = fopen(file_path, "rb");
#endif

    if (!file) {
        *error_message = format_preprocessor_error(L"فشل في فتح الملف '%hs'.", file_path);
        return NULL;
    }

    // Check for UTF-16LE BOM (0xFF, 0xFE)
    unsigned char bom[2];
    if (fread(bom, 1, 2, file) != 2) {
        *error_message = format_preprocessor_error(L"فشل في قراءة BOM من الملف '%hs'.", file_path);
        fclose(file);
        return NULL;
    }
    if (bom[0] != 0xFF || bom[1] != 0xFE) {
        // Check for UTF-16BE BOM (0xFE, 0xFF) - specific error?
        if (bom[0] == 0xFE && bom[1] == 0xFF) {
             *error_message = format_preprocessor_error(L"الملف '%hs' يستخدم ترميز UTF-16BE (Big Endian)، مطلوب UTF-16LE (Little Endian).", file_path);
        } else {
             *error_message = format_preprocessor_error(L"الملف '%hs' ليس UTF-16LE (BOM غير موجود أو غير صحيح).", file_path);
        }
        fclose(file);
        return NULL;
    }

    // Get file size (after BOM)
    fseek(file, 0, SEEK_END);
    long file_size_bytes = ftell(file);
    // Reset position to after BOM
    fseek(file, 2, SEEK_SET);

    if (file_size_bytes < 2) { // Only BOM present or error
        fclose(file);
        buffer = malloc(sizeof(wchar_t)); // Allocate space for null terminator
        if (!buffer) {
            *error_message = format_preprocessor_error(L"فشل في تخصيص الذاكرة لملف فارغ (بعد BOM) '%hs'.", file_path);
            return NULL;
        }
        buffer[0] = L'\0';
        return buffer; // Return empty, null-terminated string
    }

    long content_size_bytes = file_size_bytes - 2;
    if (content_size_bytes % sizeof(wchar_t) != 0) {
         *error_message = format_preprocessor_error(L"حجم محتوى الملف '%hs' (بعد BOM) ليس من مضاعفات حجم wchar_t.", file_path);
         fclose(file);
         return NULL;
    }

    size_t num_wchars = content_size_bytes / sizeof(wchar_t);

    // Allocate buffer (+1 for null terminator)
    buffer = malloc((num_wchars + 1) * sizeof(wchar_t));
    if (!buffer) {
        *error_message = format_preprocessor_error(L"فشل في تخصيص الذاكرة لمحتوى الملف '%hs'.", file_path);
        fclose(file);
        return NULL;
    }

    // Read content
    size_t bytes_read = fread(buffer, 1, content_size_bytes, file);
    if (bytes_read != (size_t)content_size_bytes) {
        *error_message = format_preprocessor_error(L"فشل في قراءة محتوى الملف بالكامل من '%hs'.", file_path);
        free(buffer);
        fclose(file);
        return NULL;
    }

    // Null-terminate
    buffer[num_wchars] = L'\0';

    fclose(file);
    return buffer;
}

// --- Path Helpers ---

// Gets the absolute path for a given file path. Returns allocated string (caller frees) or NULL.
char* get_absolute_path(const char* file_path) {
    char* abs_path_buf = malloc(MAX_PATH_LEN * sizeof(char));
    if (!abs_path_buf) return NULL;

#ifdef _WIN32
    if (_fullpath(abs_path_buf, file_path, MAX_PATH_LEN) == NULL) {
        free(abs_path_buf);
        return NULL;
    }
#else
    if (realpath(file_path, abs_path_buf) == NULL) {
        free(abs_path_buf);
        return NULL;
    }
#endif
    return abs_path_buf;
}

// Gets the directory part of a file path. Returns allocated string (caller frees) or NULL.
char* get_directory_part(const char* file_path) {
    char* path_copy = strdup(file_path);
    if (!path_copy) return NULL;

#ifdef _WIN32
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    _splitpath(path_copy, drive, dir, NULL, NULL);
    free(path_copy);
    char* dir_part = malloc((strlen(drive) + strlen(dir) + 1) * sizeof(char));
    if (!dir_part) return NULL;
    strcpy(dir_part, drive);
    strcat(dir_part, dir);
    // Remove trailing separator if present and not just root
    size_t len = strlen(dir_part);
    if (len > 0 && (dir_part[len - 1] == '\\' || dir_part[len - 1] == '/')) {
         if (!(len == 1 || (len == 3 && dir_part[1] == ':'))) { // Handle C:\ root
             dir_part[len - 1] = '\0';
         }
    }
    return dir_part;
#else
    char* dir_name_result = dirname(path_copy);
    char* dir_part = strdup(dir_name_result);
    free(path_copy);
    return dir_part;
#endif
}

// --- Dynamic Buffer for Output ---

typedef struct {
    wchar_t* buffer;
    size_t length;
    size_t capacity;
} DynamicWcharBuffer;

bool init_dynamic_buffer(DynamicWcharBuffer* db, size_t initial_capacity) {
    db->buffer = malloc(initial_capacity * sizeof(wchar_t));
    if (!db->buffer) {
        db->length = 0;
        db->capacity = 0;
        return false;
    }
    db->buffer[0] = L'\0';
    db->length = 0;
    db->capacity = initial_capacity;
    return true;
}

bool append_to_dynamic_buffer(DynamicWcharBuffer* db, const wchar_t* str_to_append) {
    size_t append_len = wcslen(str_to_append);
    if (db->length + append_len + 1 > db->capacity) {
        size_t new_capacity = (db->capacity == 0) ? append_len + 1 : db->capacity * 2;
        while (new_capacity < db->length + append_len + 1) {
            new_capacity *= 2;
        }
        wchar_t* new_buffer = realloc(db->buffer, new_capacity * sizeof(wchar_t));
        if (!new_buffer) {
            return false; // Reallocation failed
        }
        db->buffer = new_buffer;
        db->capacity = new_capacity;
    }
    wcscat(db->buffer, str_to_append);
    db->length += append_len;
    return true;
}

// Appends exactly n characters from str_to_append
bool append_dynamic_buffer_n(DynamicWcharBuffer* db, const wchar_t* str_to_append, size_t n) {
    if (n == 0) return true; // Nothing to append
    if (db->length + n + 1 > db->capacity) {
        size_t new_capacity = (db->capacity == 0) ? n + 1 : db->capacity * 2;
        while (new_capacity < db->length + n + 1) {
            new_capacity *= 2;
        }
        wchar_t* new_buffer = realloc(db->buffer, new_capacity * sizeof(wchar_t));
        if (!new_buffer) {
            return false; // Reallocation failed
        }
        db->buffer = new_buffer;
        db->capacity = new_capacity;
    }
    // Use wcsncat for safe appending
    wcsncat(db->buffer, str_to_append, n);
    // Ensure null termination manually as wcsncat might not if n is exact size
    db->length += n;
    db->buffer[db->length] = L'\0';
    return true;
}

void free_dynamic_buffer(DynamicWcharBuffer* db) {
    free(db->buffer);
    db->buffer = NULL;
    db->length = 0;
    db->capacity = 0;
}

// --- File Stack for Circular Include Detection ---

bool push_file_stack(BaaPreprocessor* pp, const char* abs_path) {
    for (size_t i = 0; i < pp->open_files_count; ++i) {
        if (strcmp(pp->open_files_stack[i], abs_path) == 0) {
            return false; // Circular include detected
        }
    }

    if (pp->open_files_count >= pp->open_files_capacity) {
        size_t new_capacity = (pp->open_files_capacity == 0) ? 8 : pp->open_files_capacity * 2;
        char** new_stack = realloc(pp->open_files_stack, new_capacity * sizeof(char*));
        if (!new_stack) return false; // Allocation failure
        pp->open_files_stack = new_stack;
        pp->open_files_capacity = new_capacity;
    }

    char* path_copy = strdup(abs_path);
    if (!path_copy) return false;
    pp->open_files_stack[pp->open_files_count++] = path_copy;
    return true;
}

void pop_file_stack(BaaPreprocessor* pp) {
    if (pp->open_files_count > 0) {
        pp->open_files_count--;
        free(pp->open_files_stack[pp->open_files_count]);
        pp->open_files_stack[pp->open_files_count] = NULL;
    }
}

void free_file_stack(BaaPreprocessor* pp) {
     while(pp->open_files_count > 0) {
         pop_file_stack(pp);
     }
     free(pp->open_files_stack);
     pp->open_files_stack = NULL;
     pp->open_files_capacity = 0;
}

// --- Core Recursive Processing Function ---

// Processes a single file, handling includes recursively.
// Returns a dynamically allocated string with processed content (caller frees), or NULL on error.
static wchar_t* process_file(BaaPreprocessor* pp_state, const char* file_path, wchar_t** error_message) {
    *error_message = NULL;

    char* abs_path = get_absolute_path(file_path);
    if (!abs_path) {
        *error_message = format_preprocessor_error(L"فشل في الحصول على المسار المطلق للملف '%hs'.", file_path);
        return NULL;
    }

    // 1. Circular Include Check
    if (!push_file_stack(pp_state, abs_path)) {
        *error_message = format_preprocessor_error(L"تم اكتشاف تضمين دائري: الملف '%hs' مضمن بالفعل.", abs_path);
        free(abs_path);
        return NULL;
    }

    // 2. Read File Content
    wchar_t* file_content = read_file_content_utf16le(abs_path, error_message);
    if (!file_content) {
        // error_message should be set by read_file_content_utf16le
        pop_file_stack(pp_state); // Pop before returning error
        free(abs_path);
        return NULL;
    }

    DynamicWcharBuffer output_buffer;
    if (!init_dynamic_buffer(&output_buffer, wcslen(file_content) + 1024)) { // Initial capacity
        *error_message = format_preprocessor_error(L"فشل في تخصيص الذاكرة لمخزن الإخراج المؤقت للملف '%hs'.", abs_path);
        free(file_content);
        pop_file_stack(pp_state);
        free(abs_path);
        return NULL;
    }

    // TODO: 3. Process Lines and Directives (including #تضمين)
    wchar_t *line_start = file_content;
    wchar_t *line_end;
    bool success = true;

    while (*line_start != L'\0' && success) {
        line_end = wcschr(line_start, L'\n');
        size_t line_len;
        if (line_end != NULL) {
            line_len = line_end - line_start;
            // Handle \r\n endings
            if (line_len > 0 && line_start[line_len - 1] == L'\r') {
                line_len--;
            }
        } else {
            // Last line without newline
            line_len = wcslen(line_start);
        }

        // Create a temporary buffer for the current line
        wchar_t* current_line = wcsndup(line_start, line_len);
        if (!current_line) {
            *error_message = format_preprocessor_error(L"فشل في تخصيص الذاكرة لسطر في الملف '%hs'.", abs_path);
            success = false;
            break;
        }

        // Check for directives
        if (current_line[0] == L'#') {
            const wchar_t* include_directive = L"#تضمين";
            size_t include_directive_len = wcslen(include_directive);

            if (wcsncmp(current_line, include_directive, include_directive_len) == 0 &&
                iswspace(current_line[include_directive_len]))
            {
                // Found #تضمين directive
                wchar_t* path_start = current_line + include_directive_len;
                while (iswspace(*path_start)) {
                    path_start++; // Skip space after directive
                }

                wchar_t start_char = path_start[0];
                wchar_t end_char = 0;
                bool use_include_paths = false;
                wchar_t* path_end = NULL;

                if (start_char == L'"') {
                    end_char = L'"';
                    use_include_paths = false;
                    path_start++; // Move past opening quote
                    path_end = wcschr(path_start, end_char);
                } else if (start_char == L'<') {
                    end_char = L'>';
                    use_include_paths = true;
                    path_start++; // Move past opening bracket
                    path_end = wcschr(path_start, end_char);
                } else {
                    *error_message = format_preprocessor_error(L"تنسيق #تضمين غير صالح في الملف '%hs': يجب أن يتبع اسم الملف بـ \" أو <.", abs_path);
                    success = false;
                }

                if (success && path_end != NULL) {
                    size_t include_path_len = path_end - path_start;
                    if (include_path_len == 0) {
                        *error_message = format_preprocessor_error(L"تنسيق #تضمين غير صالح في الملف '%hs': مسار الملف فارغ.", abs_path);
                        success = false;
                    } else {
                        wchar_t* include_path_w = wcsndup(path_start, include_path_len);
                        if (!include_path_w) {
                            *error_message = format_preprocessor_error(L"فشل في تخصيص ذاكرة لمسار التضمين في '%hs'.", abs_path);
                            success = false;
                        } else {
                            // Convert wchar_t include path to char* for file operations
                            char* include_path_mb = NULL;
                            int required_bytes = WideCharToMultiByte(CP_UTF8, 0, include_path_w, -1, NULL, 0, NULL, NULL);
                            if (required_bytes > 0) {
                                include_path_mb = malloc(required_bytes);
                                if (include_path_mb) {
                                    WideCharToMultiByte(CP_UTF8, 0, include_path_w, -1, include_path_mb, required_bytes, NULL, NULL);
                                } else {
                                      *error_message = format_preprocessor_error(L"فشل في تخصيص ذاكرة لمسار التضمين (MB) في '%hs'.", abs_path);
                                      success = false;
                                }
                            } else {
                                *error_message = format_preprocessor_error(L"فشل في تحويل مسار التضمين إلى UTF-8 في '%hs'.", abs_path);
                                success = false;
                            }

                            if (success && include_path_mb) {
                                char* full_include_path = NULL;
                                if (use_include_paths) {
                                    // Search include paths
                                    bool found = false;
                                    for (size_t i = 0; i < pp_state->include_path_count; ++i) {
                                         char temp_path[MAX_PATH_LEN];
                                         snprintf(temp_path, MAX_PATH_LEN, "%s%c%s", pp_state->include_paths[i], PATH_SEPARATOR, include_path_mb);
                                         FILE* test_file = fopen(temp_path, "rb"); // Test existence
                                         if (test_file) {
                                             fclose(test_file);
                                             full_include_path = strdup(temp_path);
                                             found = true;
                                             break;
                                         }
                                    }
                                    if (!found) {
                                         *error_message = format_preprocessor_error(L"تعذر العثور على ملف التضمين '%hs' في مسارات التضمين.", include_path_mb);
                                         success = false;
                                    }
                                } else {
                                    // Relative path: combine with current file's directory
                                    char* current_dir = get_directory_part(abs_path);
                                    if (!current_dir) {
                                        *error_message = format_preprocessor_error(L"فشل في الحصول على دليل الملف الحالي '%hs'.", abs_path);
                                        success = false;
                                    } else {
                                        char temp_path[MAX_PATH_LEN];
                                        snprintf(temp_path, MAX_PATH_LEN, "%s%c%s", current_dir, PATH_SEPARATOR, include_path_mb);
                                        full_include_path = strdup(temp_path);
                                        free(current_dir);
                                    }
                                }

                                // Recursive call
                                if (success && full_include_path) {
                                    wchar_t* included_content = process_file(pp_state, full_include_path, error_message);
                                    if (!included_content) {
                                        // error_message should be set by recursive call
                                        success = false;
                                    } else {
                                        // Append result to output buffer
                                        if (!append_to_dynamic_buffer(&output_buffer, included_content)) {
                                            *error_message = format_preprocessor_error(L"فشل في إلحاق المحتوى المضمن من '%hs' بمخزن الإخراج المؤقت.", full_include_path);
                                            success = false;
                                        }
                                        free(included_content);
                                    }
                                } else if (success) { // full_include_path was NULL or creation failed
                                     *error_message = format_preprocessor_error(L"فشل في بناء المسار الكامل لملف التضمين '%hs'.", include_path_mb);
                                     success = false;
                                }
                                free(full_include_path);
                            }
                            free(include_path_mb);
                            free(include_path_w);
                        }
                    }
                } else if (success) { // path_end was NULL
                    *error_message = format_preprocessor_error(L"تنسيق #تضمين غير صالح في الملف '%hs': علامة الاقتباس أو القوس الختامي مفقود.", abs_path);
                    success = false;
                }
            } else {
                // TODO: Handle other directives (#define, #ifdef, etc.)
                // For now, just append the original directive line itself
                if (!append_dynamic_buffer_n(&output_buffer, line_start, line_len)) success = false;
                if (success && !append_to_dynamic_buffer(&output_buffer, L"\n")) success = false; // Append newline
                if (!success && !*error_message) *error_message = format_preprocessor_error(L"فشل في إلحاق السطر بمخزن الإخراج المؤقت في '%hs'.", abs_path);
            }
        } else {
            // Not a directive, append the original line
             if (!append_dynamic_buffer_n(&output_buffer, line_start, line_len)) success = false;
            if (success && !append_to_dynamic_buffer(&output_buffer, L"\n")) success = false; // Append newline
             if (!success && !*error_message) *error_message = format_preprocessor_error(L"فشل في إلحاق السطر بمخزن الإخراج المؤقت في '%hs'.", abs_path);
        }

        free(current_line); // Free the duplicated line

        if (line_end != NULL) {
            line_start = line_end + 1; // Move to the next line
        } else {
            break; // End of file content
        }
    }

    // Error handling for append failures during loop
    if (!success) {
        free_dynamic_buffer(&output_buffer);
        free(file_content);
        pop_file_stack(pp_state);
        free(abs_path);
        return NULL;
    }

    free(file_content); // Original content buffer no longer needed

    // 4. Clean up stack for this file
    pop_file_stack(pp_state);
    free(abs_path);

    // Return the final concatenated buffer (ownership transferred)
    return output_buffer.buffer;
}

wchar_t* baa_preprocess(const char* main_file_path, const char** include_paths, wchar_t** error_message)
{
    if (!main_file_path || !error_message) {
        if(error_message) *error_message = wcsdup(L"وسيطات غير صالحة تم تمريرها إلى المعالج المسبق.");
        return NULL;
    }
    *error_message = NULL; // Initialize error message to NULL

    BaaPreprocessor pp_state = {0};
    pp_state.include_paths = include_paths;
    pp_state.include_path_count = 0;
    if (include_paths) {
        while(include_paths[pp_state.include_path_count] != NULL) {
            pp_state.include_path_count++;
        }
    }
    // Initialize file stack
    pp_state.open_files_stack = NULL;
    pp_state.open_files_count = 0;
    pp_state.open_files_capacity = 0;

    // TODO: Implement the main preprocessing loop - NOW CALL process_file
    wchar_t* final_output = process_file(&pp_state, main_file_path, error_message);

    // --- Cleanup ---
    free_file_stack(&pp_state); // Free any remaining stack elements (should be empty on success)

    if (!final_output) {
        // error_message should be set by process_file or earlier checks
        return NULL;
    }

    // Return the fully processed string
    return final_output;

    // Old placeholder error:
    // *error_message = format_preprocessor_error(L"المعالج المسبق لم يتم تنفيذه بعد.");
    // return NULL;
}
