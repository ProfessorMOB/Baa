#include "baa/preprocessor/preprocessor.h"
#include "baa/utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h> // For isdigit in expression evaluator

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

// --- Dynamic Buffer for Output ---

typedef struct
{
    wchar_t *buffer;
    size_t length;
    size_t capacity;
} DynamicWcharBuffer;

bool init_dynamic_buffer(DynamicWcharBuffer *db, size_t initial_capacity);
bool append_to_dynamic_buffer(DynamicWcharBuffer *db, const wchar_t *str_to_append);
bool append_dynamic_buffer_n(DynamicWcharBuffer *db, const wchar_t *str_to_append, size_t n);
void free_dynamic_buffer(DynamicWcharBuffer *db);

// Implementation of wcsndup for Windows compatibility
static wchar_t *wcsndup(const wchar_t *s, size_t n)
{
    wchar_t *result = (wchar_t *)malloc((n + 1) * sizeof(wchar_t));
    if (result)
    {
#ifdef _WIN32
        wcsncpy_s(result, n + 1, s, n);
#else
        wcsncpy(result, s, n);
        result[n] = L'\0';
#endif
    }
    return result;
}

// Forward declarations for static functions
static bool add_macro(BaaPreprocessor *pp_state, const wchar_t *name, const wchar_t *body, bool is_function_like, size_t param_count, wchar_t **param_names);
static const BaaMacro *find_macro(const BaaPreprocessor *pp_state, const wchar_t *name);
static bool undefine_macro(BaaPreprocessor *pp_state, const wchar_t *name);
static bool push_conditional(BaaPreprocessor *pp_state, bool is_active);
static bool pop_conditional(BaaPreprocessor *pp_state);
static void update_skipping_state(BaaPreprocessor *pp_state);
void free_conditional_stack(BaaPreprocessor *pp);

// --- Preprocessor Expression Evaluation ---
// Updated signature: removed abs_path parameter
static bool evaluate_preprocessor_expression(BaaPreprocessor *pp_state, const wchar_t *expression, bool *value, wchar_t **error_message);

// --- Macro Expansion Stack Helpers ---
static bool push_macro_expansion(BaaPreprocessor *pp_state, const BaaMacro *macro);
static void pop_macro_expansion(BaaPreprocessor *pp_state);
static bool is_macro_expanding(const BaaPreprocessor *pp_state, const BaaMacro *macro);
static void free_macro_expansion_stack(BaaPreprocessor *pp_state);

// Forward declaration for argument parsing
static wchar_t **parse_macro_arguments(BaaPreprocessor *pp_state, const wchar_t **invocation_ptr_ref, size_t expected_arg_count, size_t *actual_arg_count, wchar_t **error_message);

// Forward declaration for substitution
static bool substitute_macro_body(BaaPreprocessor *pp_state, DynamicWcharBuffer *output_buffer, const BaaMacro *macro, wchar_t **arguments, size_t arg_count, wchar_t **error_message);

// Forward declaration for stringification helper
static bool stringify_argument(BaaPreprocessor *pp_state, DynamicWcharBuffer *output_buffer, const wchar_t *argument, wchar_t **error_message);

// Forward declaration for file reading
static wchar_t *read_file_content_utf16le(BaaPreprocessor *pp_state, const char *file_path, wchar_t **error_message);

// --- Helper Functions ---

// Helper to format error messages *with* file/line context
// Prepends "filepath:line: error: " to the message.
wchar_t *format_preprocessor_error_with_context(BaaPreprocessor *pp_state, const wchar_t *format, ...)
{
    // Prepare the prefix
    // Estimate buffer size: path len + ~10 for line num + ~20 for ": error: " + null
    size_t prefix_base_len = wcslen(L":%zu: خطأ: ");
    size_t path_len = pp_state->current_file_path ? strlen(pp_state->current_file_path) : strlen("(unknown file)");
    size_t prefix_buffer_size = path_len + prefix_base_len + 20; // Extra space for line number digits
    char *prefix_mb = malloc(prefix_buffer_size);
    if (!prefix_mb)
    {
        // Fallback if prefix allocation fails - return original format attempt
        // This is suboptimal but better than crashing.
        va_list args_fallback;
        va_start(args_fallback, format);
        // Determine required size for original format
        va_list args_copy_fallback;
        va_copy(args_copy_fallback, args_fallback);
        int needed_fallback = vswprintf(NULL, 0, format, args_copy_fallback);
        va_end(args_copy_fallback);
        if (needed_fallback < 0)
        {
            va_end(args_fallback);
            return wcsdup(L"فشل في تنسيق رسالة الخطأ (وفشل تخصيص البادئة).");
        }
        size_t buffer_size_fallback = (size_t)needed_fallback + 1;
        wchar_t *buffer_fallback = malloc(buffer_size_fallback * sizeof(wchar_t));
        if (!buffer_fallback)
        {
            va_end(args_fallback);
            return wcsdup(L"فشل في تنسيق رسالة الخطأ (وفشل تخصيص البادئة والمخزن المؤقت).");
        }
        vswprintf(buffer_fallback, buffer_size_fallback, format, args_fallback);
        va_end(args_fallback);
        return buffer_fallback;
    }

    snprintf(prefix_mb, prefix_buffer_size, "%hs:%zu: خطأ: ",
             pp_state->current_file_path ? pp_state->current_file_path : "(unknown file)",
             pp_state->current_line_number);

    // Convert prefix to wchar_t
    wchar_t *prefix_w = NULL;
    int required_wchars = MultiByteToWideChar(CP_UTF8, 0, prefix_mb, -1, NULL, 0);
    if (required_wchars > 0)
    {
        prefix_w = malloc(required_wchars * sizeof(wchar_t));
        if (prefix_w)
        {
            MultiByteToWideChar(CP_UTF8, 0, prefix_mb, -1, prefix_w, required_wchars);
        }
    }
    free(prefix_mb); // Free the multibyte prefix buffer

    if (!prefix_w)
    {
        // Fallback if prefix conversion fails - similar to allocation failure above
        va_list args_fallback;
        va_start(args_fallback, format);
        va_list args_copy_fallback;
        va_copy(args_copy_fallback, args_fallback);
        int needed_fallback = vswprintf(NULL, 0, format, args_copy_fallback);
        va_end(args_copy_fallback);
        if (needed_fallback < 0)
        {
            va_end(args_fallback);
            return wcsdup(L"فشل في تنسيق رسالة الخطأ (وفشل تحويل البادئة).");
        }
        size_t buffer_size_fallback = (size_t)needed_fallback + 1;
        wchar_t *buffer_fallback = malloc(buffer_size_fallback * sizeof(wchar_t));
        if (!buffer_fallback)
        {
            va_end(args_fallback);
            return wcsdup(L"فشل في تنسيق رسالة الخطأ (وفشل تحويل البادئة والمخزن المؤقت).");
        }
        vswprintf(buffer_fallback, buffer_size_fallback, format, args_fallback);
        va_end(args_fallback);
        return buffer_fallback;
    }

    // Now format the user's message part
    va_list args;
    va_start(args, format);
    va_list args_copy;
    va_copy(args_copy, args);
    int needed_msg = vswprintf(NULL, 0, format, args_copy);
    va_end(args_copy);

    if (needed_msg < 0)
    {
        va_end(args);
        free(prefix_w);
        return wcsdup(L"فشل في تنسيق جزء الرسالة من خطأ المعالج المسبق."); // Basic fallback
    }

    // Combine prefix and message
    size_t prefix_len = wcslen(prefix_w);
    size_t msg_len = (size_t)needed_msg;
    size_t total_len = prefix_len + msg_len;
    wchar_t *final_buffer = malloc((total_len + 1) * sizeof(wchar_t));

    if (!final_buffer)
    {
        va_end(args);
        free(prefix_w);
        return wcsdup(L"فشل في تخصيص الذاكرة لرسالة خطأ المعالج المسبق الكاملة."); // Basic fallback
    }

    // Copy prefix
    wcscpy(final_buffer, prefix_w);
    free(prefix_w); // Free the wide prefix buffer now

    // Append formatted message
    vswprintf(final_buffer + prefix_len, msg_len + 1, format, args);
    va_end(args);

    return final_buffer;
}

// Original error formatter (without context) - kept for potential use where context is unavailable
wchar_t *format_preprocessor_error(const wchar_t *format, ...)
{
    va_list args;
    va_start(args, format);

    // Determine required size
    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vswprintf(NULL, 0, format, args_copy);
    va_end(args_copy);

    if (needed < 0)
    {
        va_end(args);
        // Fallback error message
        const wchar_t *fallback = L"فشل في تنسيق رسالة خطأ المعالج المسبق.";
        wchar_t *error_msg;
#ifdef _WIN32
        error_msg = _wcsdup(fallback);
#else
        error_msg = wcsdup(fallback);
#endif
        return error_msg;
    }

    size_t buffer_size = (size_t)needed + 1;
    wchar_t *buffer = malloc(buffer_size * sizeof(wchar_t));
    if (!buffer)
    {
        va_end(args);
        const wchar_t *fallback = L"فشل في تخصيص الذاكرة لرسالة خطأ المعالج المسبق.";
        wchar_t *error_msg;
#ifdef _WIN32
        error_msg = _wcsdup(fallback);
#else
        error_msg = wcsdup(fallback);
#endif
        return error_msg; // Allocation failed
    }

    vswprintf(buffer, buffer_size, format, args);
    va_end(args);

    return buffer;
}

// Reads the content of a UTF-16LE encoded file.
// Returns a dynamically allocated wchar_t* string (caller must free).
// Returns NULL on error and sets error_message.
static wchar_t *read_file_content_utf16le(BaaPreprocessor *pp_state, const char *file_path, wchar_t **error_message)
{
    *error_message = NULL;
    FILE *file = NULL;
    wchar_t *buffer = NULL;

#ifdef _WIN32
    // Use _wfopen on Windows for potentially non-ASCII paths
    // Convert UTF-8 path (common) to UTF-16
    int required_wchars = MultiByteToWideChar(CP_UTF8, 0, file_path, -1, NULL, 0);
    if (required_wchars <= 0)
    {
        // Context might not be fully set here if this is the *first* file, but attempt anyway.
        *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تحويل مسار الملف '%hs' إلى UTF-16.", file_path);
        return NULL;
    }
    wchar_t *w_file_path = malloc(required_wchars * sizeof(wchar_t));
    if (!w_file_path)
    {
        *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص الذاكرة لمسار الملف (UTF-16) '%hs'.", file_path);
        return NULL;
    }
    MultiByteToWideChar(CP_UTF8, 0, file_path, -1, w_file_path, required_wchars);
    file = _wfopen(w_file_path, L"rb");
    free(w_file_path);
#else
    // Standard fopen for non-Windows
    file = fopen(file_path, "rb");
#endif

    if (!file)
    {
        *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في فتح الملف '%hs'.", file_path);
        return NULL;
    }

    // Check for UTF-16LE BOM (0xFF, 0xFE)
    unsigned char bom[2];
    if (fread(bom, 1, 2, file) != 2)
    {
        *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في قراءة BOM من الملف '%hs'.", file_path);
        fclose(file);
        return NULL;
    }
    if (bom[0] != 0xFF || bom[1] != 0xFE)
    {
        // Check for UTF-16BE BOM (0xFE, 0xFF) - specific error?
        if (bom[0] == 0xFE && bom[1] == 0xFF)
        {
            *error_message = format_preprocessor_error_with_context(pp_state, L"الملف '%hs' يستخدم ترميز UTF-16BE (Big Endian)، مطلوب UTF-16LE (Little Endian).", file_path);
        }
        else
        {
            *error_message = format_preprocessor_error_with_context(pp_state, L"الملف '%hs' ليس UTF-16LE (BOM غير موجود أو غير صحيح).", file_path);
        }
        fclose(file);
        return NULL;
    }

    // Get file size (after BOM)
    fseek(file, 0, SEEK_END);
    long file_size_bytes = ftell(file);
    // Reset position to after BOM
    fseek(file, 2, SEEK_SET);

    if (file_size_bytes < 2)
    { // Only BOM present or error
        fclose(file);
        buffer = malloc(sizeof(wchar_t)); // Allocate space for null terminator
        if (!buffer)
        {
            *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص الذاكرة لملف فارغ (بعد BOM) '%hs'.", file_path);
            return NULL;
        }
        buffer[0] = L'\0';
        return buffer; // Return empty, null-terminated string
    }

    long content_size_bytes = file_size_bytes - 2;
    if (content_size_bytes % sizeof(wchar_t) != 0)
    {
        *error_message = format_preprocessor_error_with_context(pp_state, L"حجم محتوى الملف '%hs' (بعد BOM) ليس من مضاعفات حجم wchar_t.", file_path);
        fclose(file);
        return NULL;
    }

    size_t num_wchars = content_size_bytes / sizeof(wchar_t);

    // Allocate buffer (+1 for null terminator)
    buffer = malloc((num_wchars + 1) * sizeof(wchar_t));
    if (!buffer)
    {
        *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص الذاكرة لمحتوى الملف '%hs'.", file_path);
        fclose(file);
        return NULL;
    }

    // Read content
    size_t bytes_read = fread(buffer, 1, content_size_bytes, file);
    if (bytes_read != (size_t)content_size_bytes)
    {
        *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في قراءة محتوى الملف بالكامل من '%hs'.", file_path);
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
char *get_absolute_path(const char *file_path)
{
    char *abs_path_buf = malloc(MAX_PATH_LEN * sizeof(char));
    if (!abs_path_buf)
        return NULL;

#ifdef _WIN32
    if (_fullpath(abs_path_buf, file_path, MAX_PATH_LEN) == NULL)
    {
        free(abs_path_buf);
        return NULL;
    }
#else
    if (realpath(file_path, abs_path_buf) == NULL)
    {
        free(abs_path_buf);
        return NULL;
    }
#endif
    return abs_path_buf;
}

// Gets the directory part of a file path. Returns allocated string (caller frees) or NULL.
char *get_directory_part(const char *file_path)
{
#ifdef _WIN32
    char *path_copy = _strdup(file_path);
#else
    char *path_copy = strdup(file_path);
#endif
    if (!path_copy)
        return NULL;

#ifdef _WIN32
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    errno_t err = _splitpath_s(path_copy, drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0);
    free(path_copy);
    if (err != 0)
        return NULL;
    char *dir_part = malloc((strlen(drive) + strlen(dir) + 1) * sizeof(char));
    if (!dir_part)
        return NULL;
    strcpy_s(dir_part, strlen(drive) + 1, drive);
    strcat_s(dir_part, strlen(drive) + strlen(dir) + 1, dir);
    // Remove trailing separator if present and not just root
    size_t len = strlen(dir_part);
    if (len > 0 && (dir_part[len - 1] == '\\' || dir_part[len - 1] == '/'))
    {
        if (!(len == 1 || (len == 3 && dir_part[1] == ':')))
        { // Handle C:\ root
            dir_part[len - 1] = '\0';
        }
    }
    return dir_part;
#else
    char *dir_name_result = dirname(path_copy);
    char *dir_part = strdup(dir_name_result);
    free(path_copy);
    return dir_part;
#endif
}

bool init_dynamic_buffer(DynamicWcharBuffer *db, size_t initial_capacity)
{
    db->buffer = malloc(initial_capacity * sizeof(wchar_t));
    if (!db->buffer)
    {
        db->length = 0;
        db->capacity = 0;
        return false;
    }
    db->buffer[0] = L'\0';
    db->length = 0;
    db->capacity = initial_capacity;
    return true;
}

bool append_to_dynamic_buffer(DynamicWcharBuffer *db, const wchar_t *str_to_append)
{
    size_t append_len = wcslen(str_to_append);
    if (db->length + append_len + 1 > db->capacity)
    {
        size_t new_capacity = (db->capacity == 0) ? append_len + 1 : db->capacity * 2;
        while (new_capacity < db->length + append_len + 1)
        {
            new_capacity *= 2;
        }
        wchar_t *new_buffer = realloc(db->buffer, new_capacity * sizeof(wchar_t));
        if (!new_buffer)
        {
            return false; // Reallocation failed
        }
        db->buffer = new_buffer;
        db->capacity = new_capacity;
    }
#ifdef _WIN32
    wcscat_s(db->buffer, db->capacity, str_to_append);
#else
    wcscat(db->buffer, str_to_append);
#endif
    db->length += append_len;
    return true;
}

// Appends exactly n characters from str_to_append
bool append_dynamic_buffer_n(DynamicWcharBuffer *db, const wchar_t *str_to_append, size_t n)
{
    if (n == 0)
        return true; // Nothing to append
    if (db->length + n + 1 > db->capacity)
    {
        size_t new_capacity = (db->capacity == 0) ? n + 1 : db->capacity * 2;
        while (new_capacity < db->length + n + 1)
        {
            new_capacity *= 2;
        }
        wchar_t *new_buffer = realloc(db->buffer, new_capacity * sizeof(wchar_t));
        if (!new_buffer)
        {
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

void free_dynamic_buffer(DynamicWcharBuffer *db)
{
    free(db->buffer);
    db->buffer = NULL;
    db->length = 0;
    db->capacity = 0;
}

// --- File Stack for Circular Include Detection ---

bool push_file_stack(BaaPreprocessor *pp, const char *abs_path)
{
    for (size_t i = 0; i < pp->open_files_count; ++i)
    {
        if (strcmp(pp->open_files_stack[i], abs_path) == 0)
        {
            return false; // Circular include detected
        }
    }

    if (pp->open_files_count >= pp->open_files_capacity)
    {
        size_t new_capacity = (pp->open_files_capacity == 0) ? 8 : pp->open_files_capacity * 2;
        char **new_stack = realloc(pp->open_files_stack, new_capacity * sizeof(char *));
        if (!new_stack)
            return false; // Allocation failure
        pp->open_files_stack = new_stack;
        pp->open_files_capacity = new_capacity;
    }

    char *path_copy = strdup(abs_path);
    if (!path_copy)
        return false;
    pp->open_files_stack[pp->open_files_count++] = path_copy;
    return true;
}

void pop_file_stack(BaaPreprocessor *pp)
{
    if (pp->open_files_count > 0)
    {
        pp->open_files_count--;
        free(pp->open_files_stack[pp->open_files_count]);
        pp->open_files_stack[pp->open_files_count] = NULL;
    }
}

void free_file_stack(BaaPreprocessor *pp)
{
    while (pp->open_files_count > 0)
    {
        pop_file_stack(pp);
    }
    free(pp->open_files_stack);
    pp->open_files_stack = NULL;
    pp->open_files_capacity = 0;
}

// --- Conditional Compilation Stack Helpers ---

// Updates the skipping_lines flag based on the current conditional stack.
// We skip if *any* level on the stack is currently inactive (false).
static void update_skipping_state(BaaPreprocessor *pp_state)
{
    pp_state->skipping_lines = false;
    for (size_t i = 0; i < pp_state->conditional_stack_count; ++i)
    {
        if (!pp_state->conditional_stack[i])
        {
            pp_state->skipping_lines = true;
            return; // No need to check further levels
        }
    }
}

// Pushes a new state onto both conditional stacks
static bool push_conditional(BaaPreprocessor *pp_state, bool condition_met)
{
    // Resize main stack if needed
    if (pp_state->conditional_stack_count >= pp_state->conditional_stack_capacity)
    {
        size_t new_capacity = (pp_state->conditional_stack_capacity == 0) ? 4 : pp_state->conditional_stack_capacity * 2;
        bool *new_main_stack = realloc(pp_state->conditional_stack, new_capacity * sizeof(bool));
        if (!new_main_stack)
            return false;
        pp_state->conditional_stack = new_main_stack;
        pp_state->conditional_stack_capacity = new_capacity;
    }
    // Resize branch taken stack if needed
    if (pp_state->conditional_branch_taken_stack_count >= pp_state->conditional_branch_taken_stack_capacity)
    {
        size_t new_capacity = (pp_state->conditional_branch_taken_stack_capacity == 0) ? 4 : pp_state->conditional_branch_taken_stack_capacity * 2;
        bool *new_branch_stack = realloc(pp_state->conditional_branch_taken_stack, new_capacity * sizeof(bool));
        if (!new_branch_stack)
            return false; // Should ideally handle potential mismatch if one realloc fails
        pp_state->conditional_branch_taken_stack = new_branch_stack;
        pp_state->conditional_branch_taken_stack_capacity = new_capacity;
    }

    // Determine if this new block is *potentially* active (ignoring parent state for now)
    bool branch_taken = condition_met;

    // Push the condition result onto the main stack
    pp_state->conditional_stack[pp_state->conditional_stack_count++] = condition_met;
    // Push whether this specific branch (#if or #ifndef) was taken onto the branch stack
    pp_state->conditional_branch_taken_stack[pp_state->conditional_branch_taken_stack_count++] = branch_taken;

    update_skipping_state(pp_state); // Update overall skipping state based on the stacks
    return true;
}

// Pops the top state from both conditional stacks
static bool pop_conditional(BaaPreprocessor *pp_state)
{
    if (pp_state->conditional_stack_count == 0)
    {
        return false; // Stack underflow
    }
    // Ensure counts match before decrementing (should always be true if logic is correct)
    if (pp_state->conditional_branch_taken_stack_count != pp_state->conditional_stack_count)
    {
        // Internal error state, maybe log or handle?
        return false;
    }
    pp_state->conditional_stack_count--;
    pp_state->conditional_branch_taken_stack_count--;
    update_skipping_state(pp_state); // Update skipping state
    return true;
}

// Frees both conditional stack memories
void free_conditional_stack(BaaPreprocessor *pp)
{
    free(pp->conditional_stack);
    pp->conditional_stack = NULL;
    pp->conditional_stack_count = 0;
    pp->conditional_stack_capacity = 0;

    free(pp->conditional_branch_taken_stack);
    pp->conditional_branch_taken_stack = NULL;
    pp->conditional_branch_taken_stack_count = 0;
    pp->conditional_branch_taken_stack_capacity = 0;

    pp->skipping_lines = false;
}

// --- Core Recursive Processing Function ---

// Processes a single file, handling includes recursively.
// Returns a dynamically allocated string with processed content (caller frees), or NULL on error.
static wchar_t *process_file(BaaPreprocessor *pp_state, const char *file_path, wchar_t **error_message)
{
    *error_message = NULL;

    // Store previous context for restoration after include
    const char *prev_file_path = pp_state->current_file_path;
    size_t prev_line_number = pp_state->current_line_number;

    char *abs_path = get_absolute_path(file_path);
    if (!abs_path)
    {
        *error_message = format_preprocessor_error(L"فشل في الحصول على المسار المطلق للملف '%hs'.", file_path);
        // Restore context before returning on error
        pp_state->current_file_path = prev_file_path;
        pp_state->current_line_number = prev_line_number;
        return NULL;
    }

    // Set current context for this file
    pp_state->current_file_path = abs_path; // Use the allocated absolute path
    pp_state->current_line_number = 1;      // Start at line 1

    // 1. Circular Include Check
    if (!push_file_stack(pp_state, abs_path))
    {
        *error_message = format_preprocessor_error_with_context(pp_state, L"تم اكتشاف تضمين دائري: الملف '%hs' مضمن بالفعل.", abs_path);
        free(abs_path);
        pp_state->current_file_path = prev_file_path; // Restore before returning
        pp_state->current_line_number = prev_line_number;
        return NULL;
    }

    // 2. Read File Content
    wchar_t *file_content = read_file_content_utf16le(pp_state, abs_path, error_message);
    if (!file_content)
    {
        // error_message should be set by read_file_content_utf16le (will be updated next)
        // TODO: Ensure error message includes file/line context if possible
        pop_file_stack(pp_state); // Pop before returning error
        free(abs_path);
        pp_state->current_file_path = prev_file_path; // Restore before returning
        pp_state->current_line_number = prev_line_number;
        return NULL;
    }

    DynamicWcharBuffer output_buffer;
    if (!init_dynamic_buffer(&output_buffer, wcslen(file_content) + 1024))
    { // Initial capacity
        *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص الذاكرة لمخزن الإخراج المؤقت.");
        free(file_content);
        pop_file_stack(pp_state);
        free(abs_path);
        pp_state->current_file_path = prev_file_path; // Restore before returning
        pp_state->current_line_number = prev_line_number;
        return NULL;
    }

    // TODO: 3. Process Lines and Directives (including #تضمين)
    wchar_t *line_start = file_content;
    wchar_t *line_end;
    bool success = true;

    while (*line_start != L'\0' && success)
    {
        line_end = wcschr(line_start, L'\n');
        size_t line_len;
        if (line_end != NULL)
        {
            line_len = line_end - line_start;
            // Handle \r\n endings
            if (line_len > 0 && line_start[line_len - 1] == L'\r')
            {
                line_len--;
            }
        }
        else
        {
            // Last line without newline
            line_len = wcslen(line_start);
        }

        // Create a temporary buffer for the current line
        wchar_t *current_line = wcsndup(line_start, line_len);
        if (!current_line)
        {
            *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص الذاكرة لسطر.");
            success = false;
            break;
        }

        // Check for directives
        if (current_line[0] == L'#')
        {
            const wchar_t *include_directive = L"#تضمين";
            size_t include_directive_len = wcslen(include_directive);
            const wchar_t *define_directive = L"#تعريف";
            size_t define_directive_len = wcslen(define_directive);
            const wchar_t *undef_directive = L"#الغاء_تعريف"; // Keyword for #undef
            size_t undef_directive_len = wcslen(undef_directive);
            const wchar_t *ifdef_directive = L"#إذا_عرف"; // Keyword for #ifdef
            size_t ifdef_directive_len = wcslen(ifdef_directive);
            const wchar_t *ifndef_directive = L"#إذا_لم_يعرف"; // Keyword for #ifndef
            size_t ifndef_directive_len = wcslen(ifndef_directive);
            const wchar_t *else_directive = L"#إلا"; // Keyword for #else
            size_t else_directive_len = wcslen(else_directive);
            const wchar_t *elif_directive = L"#وإلا_إذا"; // Keyword for #elif
            size_t elif_directive_len = wcslen(elif_directive);
            const wchar_t *endif_directive = L"#نهاية_إذا"; // Keyword for #endif
            size_t endif_directive_len = wcslen(endif_directive);
            const wchar_t *if_directive = L"#إذا"; // Keyword for #if
            size_t if_directive_len = wcslen(if_directive);

            bool is_conditional_directive = false;

            // --- Process Conditional Directives FIRST, regardless of skipping state ---
            if (wcsncmp(current_line, if_directive, if_directive_len) == 0 &&
                (current_line[if_directive_len] == L'\0' || iswspace(current_line[if_directive_len])))
            {
                is_conditional_directive = true;
                // Found #إذا (#if) directive
                wchar_t *expr_start = current_line + if_directive_len;
                while (iswspace(*expr_start))
                    expr_start++; // Skip space

                if (*expr_start == L'\0')
                {
                    *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق #إذا غير صالح: التعبير مفقود.");
                    success = false;
                }
                else
                {
                    bool expr_value = false;
                    // TODO: Pass pp_state to evaluate_preprocessor_expression and update its error reporting
                    if (!evaluate_preprocessor_expression(pp_state, expr_start, &expr_value, error_message))
                    {
                        // Error message should be set by evaluator (needs update)
                        if (!*error_message) // Set a generic one if evaluator didn't
                            *error_message = format_preprocessor_error_with_context(pp_state, L"خطأ في تقييم تعبير #إذا.");
                        success = false;
                    }
                    else
                    {
                        if (!push_conditional(pp_state, expr_value))
                        {
                            *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في دفع الحالة الشرطية لـ #إذا (نفاد الذاكرة؟).");
                            success = false;
                        }
                    }
                }
                // Directive processed, don't append to output
            }
            else if (wcsncmp(current_line, ifdef_directive, ifdef_directive_len) == 0 &&
                     (current_line[ifdef_directive_len] == L'\0' || iswspace(current_line[ifdef_directive_len])))
            {
                is_conditional_directive = true;
                // Found #إذا_عرف directive
                wchar_t *name_start = current_line + ifdef_directive_len;
                while (iswspace(*name_start))
                    name_start++;
                wchar_t *name_end = name_start;
                while (*name_end != L'\0' && !iswspace(*name_end))
                    name_end++;

                if (name_start == name_end)
                {
                    *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق #إذا_عرف غير صالح: اسم الماكرو مفقود.");
                    success = false;
                }
                else
                {
                    size_t name_len = name_end - name_start;
                    wchar_t *macro_name = wcsndup(name_start, name_len);
                    if (!macro_name)
                    {
                        *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص ذاكرة لاسم الماكرو في #إذا_عرف.");
                        success = false;
                    }
                    else
                    {
                        bool is_defined = (find_macro(pp_state, macro_name) != NULL);
                        if (!push_conditional(pp_state, is_defined))
                        {
                            *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في دفع الحالة الشرطية لـ #إذا_عرف (نفاد الذاكرة؟).");
                            success = false;
                        }
                        free(macro_name);
                    }
                }
                // Directive processed, don't append to output
            }
            else if (wcsncmp(current_line, ifndef_directive, ifndef_directive_len) == 0 &&
                     (current_line[ifndef_directive_len] == L'\0' || iswspace(current_line[ifndef_directive_len])))
            {
                is_conditional_directive = true;
                // Found #إذا_لم_يعرف directive
                wchar_t *name_start = current_line + ifndef_directive_len;
                while (iswspace(*name_start))
                    name_start++;
                wchar_t *name_end = name_start;
                while (*name_end != L'\0' && !iswspace(*name_end))
                    name_end++;

                if (name_start == name_end)
                {
                    *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق #إذا_لم_يعرف غير صالح: اسم الماكرو مفقود.");
                    success = false;
                }
                else
                {
                    size_t name_len = name_end - name_start;
                    wchar_t *macro_name = wcsndup(name_start, name_len);
                    if (!macro_name)
                    {
                        *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص ذاكرة لاسم الماكرو في #إذا_لم_يعرف.");
                        success = false;
                    }
                    else
                    {
                        bool is_defined = (find_macro(pp_state, macro_name) != NULL);
                        // Push the *negated* definition status
                        if (!push_conditional(pp_state, !is_defined))
                        {
                            *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في دفع الحالة الشرطية لـ #إذا_لم_يعرف (نفاد الذاكرة؟).");
                            success = false;
                        }
                        free(macro_name);
                    }
                }
                // Directive processed, don't append to output
            }
            else if (wcsncmp(current_line, endif_directive, endif_directive_len) == 0 &&
                     (current_line[endif_directive_len] == L'\0' || iswspace(current_line[endif_directive_len])))
            {
                is_conditional_directive = true;
                // Found #نهاية_إذا directive
                if (!pop_conditional(pp_state))
                {
                    *error_message = format_preprocessor_error_with_context(pp_state, L"#نهاية_إذا بدون #إذا/#إذا_عرف/#إذا_لم_يعرف مطابق.");
                    success = false;
                }
                // Directive processed, don't append to output
            }
            else if (wcsncmp(current_line, else_directive, else_directive_len) == 0 &&
                     (current_line[else_directive_len] == L'\0' || iswspace(current_line[else_directive_len])))
            {
                is_conditional_directive = true;
                // Found #إلا directive
                if (pp_state->conditional_stack_count == 0)
                {
                    *error_message = format_preprocessor_error_with_context(pp_state, L"#إلا بدون #إذا/#إذا_عرف/#إذا_لم_يعرف مطابق.");
                    success = false;
                }
                else
                {
                    size_t top_index = pp_state->conditional_stack_count - 1;
                    // Check if a branch was already taken at this level
                    if (pp_state->conditional_branch_taken_stack[top_index])
                    {
                        // If a branch (#if, #ifndef, #elif) was already taken, this #else block is inactive
                        pp_state->conditional_stack[top_index] = false;
                    }
                    else
                    {
                        // No branch was taken yet, this #else branch becomes active
                        // (Its activity is still governed by parent blocks via update_skipping_state)
                        pp_state->conditional_stack[top_index] = true;              // Mark this level as active
                        pp_state->conditional_branch_taken_stack[top_index] = true; // Mark that a branch (this #else) is now taken
                    }
                    update_skipping_state(pp_state); // Recalculate overall skipping state
                }
                // Directive processed, don't append to output
            }
            else if (wcsncmp(current_line, elif_directive, elif_directive_len) == 0 &&
                     (current_line[elif_directive_len] == L'\0' || iswspace(current_line[elif_directive_len])))
            {
                is_conditional_directive = true;
                // Found #وإلا_إذا (#elif) directive
                if (pp_state->conditional_stack_count == 0)
                {
                    *error_message = format_preprocessor_error_with_context(pp_state, L"#وإلا_إذا بدون #إذا/#إذا_عرف/#إذا_لم_يعرف مطابق.");
                    success = false;
                }
                else
                {
                    size_t top_index = pp_state->conditional_stack_count - 1;
                    // Check if a branch was already taken at this level
                    if (pp_state->conditional_branch_taken_stack[top_index])
                    {
                        // If a branch was already taken, this #elif is inactive
                        pp_state->conditional_stack[top_index] = false;
                    }
                    else
                    {
                        // No branch taken yet, evaluate this #elif condition
                        wchar_t *name_start = current_line + elif_directive_len;
                        while (iswspace(*name_start))
                            name_start++;
                        wchar_t *name_end = name_start;
                        while (*name_end != L'\0' && !iswspace(*name_end))
                            name_end++;

                        if (name_start == name_end)
                        {
                            *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق #وإلا_إذا غير صالح: التعبير مفقود.");
                            success = false;
                        }
                        else
                        {
                            size_t name_len = name_end - name_start;
                            wchar_t *macro_name = wcsndup(name_start, name_len); // Temporary for error message if needed
                            if (!macro_name)
                            {
                                *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص ذاكرة لاسم الماكرو في #وإلا_إذا.");
                                success = false;
                            }
                            else
                            {
                                // Evaluate the condition using the new expression evaluator
                                bool condition_met = false;
                                // TODO: Pass pp_state to evaluate_preprocessor_expression and update its error reporting
                                if (!evaluate_preprocessor_expression(pp_state, name_start, &condition_met, error_message))
                                {
                                    // Error message should be set by evaluator (needs update)
                                    if (!*error_message) // Set a generic one if evaluator didn't
                                        *error_message = format_preprocessor_error_with_context(pp_state, L"خطأ في تقييم تعبير #وإلا_إذا.");
                                    success = false;
                                }
                                else
                                {
                                    // Proceed with the evaluated condition
                                    if (condition_met)
                                    {
                                        // Condition met and no prior branch taken: this branch is active
                                        pp_state->conditional_stack[top_index] = true;
                                        pp_state->conditional_branch_taken_stack[top_index] = true; // Mark branch taken
                                    }
                                    else
                                    {
                                        // Condition not met or prior branch taken: this branch is inactive
                                        pp_state->conditional_stack[top_index] = false;
                                        // Don't mark branch taken yet
                                    }
                                }
                                free(macro_name); // Free the temporary name
                                // Original defined() check removed, now uses expression evaluator
                                /* Old logic:
                                if (condition_met)
                                {
                                    // Condition met and no prior branch taken: this branch is active
                                    pp_state->conditional_stack[top_index] = true;
                                    pp_state->conditional_branch_taken_stack[top_index] = true; // Mark branch taken
                                }
                                else
                                {
                                    // Condition not met or prior branch taken: this branch is inactive
                                    pp_state->conditional_stack[top_index] = false;
                                    // Don't mark branch taken yet
                                }
                                */
                            }
                        }
                    }
                    update_skipping_state(pp_state); // Recalculate overall skipping state
                }
                // Directive processed, don't append to output
            }

            // --- Process other directives ONLY if not skipping AND not a conditional directive ---
            if (!is_conditional_directive && !pp_state->skipping_lines)
            {
                if (wcsncmp(current_line, include_directive, include_directive_len) == 0 &&
                    (current_line[include_directive_len] == L'\0' || iswspace(current_line[include_directive_len])))
                {
                    // Found #تضمين directive
                    wchar_t *path_start = current_line + include_directive_len;
                    while (iswspace(*path_start))
                    {
                        path_start++; // Skip space after directive
                    }

                    wchar_t start_char = path_start[0];
                    wchar_t end_char = 0;
                    bool use_include_paths = false;
                    wchar_t *path_end = NULL;

                    if (start_char == L'"')
                    {
                        end_char = L'"';
                        use_include_paths = false;
                        path_start++; // Move past opening quote
                        path_end = wcschr(path_start, end_char);
                    }
                    else if (start_char == L'<')
                    {
                        end_char = L'>';
                        use_include_paths = true;
                        path_start++; // Move past opening bracket
                        path_end = wcschr(path_start, end_char);
                    }
                    else
                    {
                        *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق #تضمين غير صالح: يجب أن يتبع اسم الملف بـ \" أو <.");
                        success = false;
                    }

                    if (success && path_end != NULL)
                    {
                        size_t include_path_len = path_end - path_start;
                        if (include_path_len == 0)
                        {
                            *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق #تضمين غير صالح: مسار الملف فارغ.");
                            success = false;
                        }
                        else
                        {
                            wchar_t *include_path_w = wcsndup(path_start, include_path_len);
                            if (!include_path_w)
                            {
                                *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص ذاكرة لمسار التضمين.");
                                success = false;
                            }
                            else
                            {
                                // Convert wchar_t include path to char* for file operations
                                char *include_path_mb = NULL;
                                int required_bytes = WideCharToMultiByte(CP_UTF8, 0, include_path_w, -1, NULL, 0, NULL, NULL);
                                if (required_bytes > 0)
                                {
                                    include_path_mb = malloc(required_bytes);
                                    if (include_path_mb)
                                    {
                                        WideCharToMultiByte(CP_UTF8, 0, include_path_w, -1, include_path_mb, required_bytes, NULL, NULL);
                                    }
                                    else
                                    {
                                        *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص ذاكرة لمسار التضمين (MB).");
                                        success = false;
                                    }
                                }
                                else
                                {
                                    *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تحويل مسار التضمين إلى UTF-8.");
                                    success = false;
                                }

                                if (success && include_path_mb)
                                {
                                    char *full_include_path = NULL;
                                    if (use_include_paths)
                                    {
                                        // Search include paths
                                        bool found = false;
                                        for (size_t i = 0; i < pp_state->include_path_count; ++i)
                                        {
                                            char temp_path[MAX_PATH_LEN];
                                            snprintf(temp_path, MAX_PATH_LEN, "%s%c%s", pp_state->include_paths[i], PATH_SEPARATOR, include_path_mb);
                                            FILE *test_file = NULL;
#ifdef _WIN32
                                            errno_t err = fopen_s(&test_file, temp_path, "rb"); // Test existence
                                            if (test_file && err == 0)
#else
                                            test_file = fopen(temp_path, "rb"); // Test existence
                                            if (test_file)
#endif
                                            {
                                                fclose(test_file);
                                                full_include_path = strdup(temp_path);
                                                found = true;
                                                break;
                                            }
                                        }
                                        if (!found)
                                        {
                                            *error_message = format_preprocessor_error_with_context(pp_state, L"تعذر العثور على ملف التضمين '<%hs>' في مسارات التضمين.", include_path_mb);
                                            success = false;
                                        }
                                    }
                                    else
                                    {
                                        // Relative path: combine with current file's directory
                                        char *current_dir = get_directory_part(pp_state->current_file_path); // Use current file path from state
                                        if (!current_dir)
                                        {
                                            *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في الحصول على دليل الملف الحالي.");
                                            success = false;
                                        }
                                        else
                                        {
                                            char temp_path[MAX_PATH_LEN];
                                            snprintf(temp_path, MAX_PATH_LEN, "%s%c%s", current_dir, PATH_SEPARATOR, include_path_mb);
                                            full_include_path = strdup(temp_path);
                                            free(current_dir);
                                        }
                                    }

                                    // Recursive call
                                    if (success && full_include_path)
                                    {
                                        wchar_t *included_content = process_file(pp_state, full_include_path, error_message);
                                        if (!included_content)
                                        {
                                            // error_message should be set by recursive call
                                            success = false;
                                        }
                                        else
                                        {
                                            // Append result to output buffer
                                            if (!append_to_dynamic_buffer(&output_buffer, included_content))
                                            {
                                                // Error message from recursive call might be more specific, but provide a fallback
                                                if (!*error_message)
                                                    *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في إلحاق المحتوى المضمن من '%hs'.", full_include_path);
                                                success = false;
                                            }
                                            free(included_content);
                                        }
                                    }
                                    else if (success && !use_include_paths)
                                    { // full_include_path creation failed only for relative paths here
                                        *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في بناء المسار الكامل لملف التضمين '\"%hs\"'.", include_path_mb);
                                        success = false;
                                    }
                                    // 'found' handles the error message for include paths case
                                    free(full_include_path);
                                }
                                free(include_path_mb);
                                free(include_path_w);
                            }
                        }
                    }
                    else if (success)
                    { // path_end was NULL
                        *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق #تضمين غير صالح: علامة الاقتباس أو القوس الختامي مفقود.");
                        success = false;
                    }
                }
                else if (wcsncmp(current_line, define_directive, define_directive_len) == 0 &&
                         (current_line[define_directive_len] == L'\0' || iswspace(current_line[define_directive_len])))
                {
                    // Found #تعريف directive
                    wchar_t *name_start = current_line + define_directive_len;
                    while (iswspace(*name_start))
                    {
                        name_start++; // Skip space after directive
                    }
                    wchar_t *name_end = name_start;
                    while (*name_end != L'\0' && !iswspace(*name_end))
                    {
                        name_end++; // Find end of name
                    }

                    if (name_start == name_end)
                    {
                        *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق #تعريف غير صالح: اسم الماكرو مفقود.");
                        success = false;
                    }
                    else
                    {
                        size_t name_len = name_end - name_start;
                        wchar_t *macro_name = wcsndup(name_start, name_len);
                        if (!macro_name)
                        {
                            *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص ذاكرة لاسم الماكرو في #تعريف.");
                            success = false;
                            break; // Exit directive processing for this line
                        }

                        // --- Start Function-Like Macro Parsing ---
                        bool is_function_like = false;
                        size_t param_count = 0;
                        wchar_t **params = NULL; // Array of parameter names
                        size_t params_capacity = 0;
                        wchar_t *body_start = name_end; // Default body start if not function-like

                        // Check for '(' immediately after name (no space allowed by C standard)
                        if (*name_end == L'(')
                        {
                            is_function_like = true;
                            wchar_t *param_ptr = name_end + 1; // Start parsing after '('

                            // Loop to parse parameters
                            while (success)
                            {
                                // Skip whitespace before parameter or ')'
                                while (iswspace(*param_ptr))
                                    param_ptr++;

                                if (*param_ptr == L')')
                                {                // End of parameter list
                                    param_ptr++; // Consume ')'
                                    break;       // Exit parameter parsing loop
                                }

                                // If not ')', expect an identifier (parameter name)
                                if (!iswalpha(*param_ptr) && *param_ptr != L'_')
                                {
                                    *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق #تعريف غير صالح: متوقع اسم معامل أو ')' بعد '('.");
                                    success = false;
                                    break;
                                }

                                // Find parameter name start and end
                                wchar_t *param_name_start = param_ptr;
                                while (iswalnum(*param_ptr) || *param_ptr == L'_')
                                {
                                    param_ptr++;
                                }
                                wchar_t *param_name_end = param_ptr;
                                size_t param_name_len = param_name_end - param_name_start;

                                if (param_name_len == 0)
                                { // Should not happen if check above is correct, but safety
                                    *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق #تعريف غير صالح: اسم معامل فارغ.");
                                    success = false;
                                    break;
                                }

                                // Duplicate and store parameter name
                                wchar_t *param_name = wcsndup(param_name_start, param_name_len);
                                if (!param_name)
                                {
                                    *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص ذاكرة لاسم المعامل في #تعريف.");
                                    success = false;
                                    break;
                                }

                                // Resize params array if needed
                                if (param_count >= params_capacity)
                                {
                                    size_t new_capacity = (params_capacity == 0) ? 4 : params_capacity * 2;
                                    wchar_t **new_params = realloc(params, new_capacity * sizeof(wchar_t *));
                                    if (!new_params)
                                    {
                                        free(param_name); // Free the name we just allocated
                                        *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في إعادة تخصيص الذاكرة لمعاملات الماكرو في #تعريف.");
                                        success = false;
                                        break;
                                    }
                                    params = new_params;
                                    params_capacity = new_capacity;
                                }
                                params[param_count++] = param_name; // Store the duplicated name

                                // Skip whitespace after parameter name
                                while (iswspace(*param_ptr))
                                    param_ptr++;

                                // Expect ',' or ')'
                                if (*param_ptr == L',')
                                {
                                    param_ptr++; // Consume ',' and continue loop
                                }
                                else if (*param_ptr == L')')
                                {
                                    param_ptr++; // Consume ')' and break loop
                                    break;
                                }
                                else
                                {
                                    *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق #تعريف غير صالح: متوقع ',' أو ')' بعد اسم المعامل.");
                                    success = false;
                                    break;
                                }
                            } // End while parsing parameters

                            if (success)
                            {
                                body_start = param_ptr; // Body starts after the ')'
                            }
                            else
                            {
                                // Cleanup partially parsed params on error
                                if (params)
                                {
                                    for (size_t i = 0; i < param_count; ++i)
                                        free(params[i]);
                                    free(params);
                                    params = NULL;
                                    param_count = 0;
                                }
                            }
                        } // End if function-like

                        // --- End Function-Like Macro Parsing ---

                        if (success)
                        {
                            // Find actual start of the body (skip space after name or ')')
                            while (iswspace(*body_start))
                            {
                                body_start++; // Skip space before body
                            }
                            // Body is the rest of the line
                            // TODO: Consider trimming trailing whitespace from body_start?

                            // Call add_macro with potentially parsed parameters
                            if (!add_macro(pp_state, macro_name, body_start, is_function_like, param_count, params))
                            {
                                // add_macro should free params if it fails internally after taking ownership
                                *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في إضافة تعريف الماكرو '%ls' (نفاد الذاكرة؟).", macro_name);
                                success = false;
                                // If add_macro failed but we allocated params, they should be freed by add_macro
                                params = NULL; // Ensure we don't double-free if add_macro failed early
                                param_count = 0;
                            }
                            // Ownership of params array and its contents is transferred to add_macro if successful
                        }

                        free(macro_name); // Free duplicated name

                        // #تعريف line is processed, do not append to output
                    }
                }
                else if (wcsncmp(current_line, undef_directive, undef_directive_len) == 0 &&
                         (current_line[undef_directive_len] == L'\0' || iswspace(current_line[undef_directive_len])))
                {
                    // Found #الغاء_تعريف directive
                    wchar_t *name_start = current_line + undef_directive_len;
                    while (iswspace(*name_start))
                    {
                        name_start++; // Skip space after directive
                    }
                    wchar_t *name_end = name_start;
                    while (*name_end != L'\0' && !iswspace(*name_end))
                    {
                        name_end++; // Find end of name
                    }

                    if (name_start == name_end)
                    {
                        *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق #الغاء_تعريف غير صالح: اسم الماكرو مفقود.");
                        success = false;
                    }
                    else
                    {
                        size_t name_len = name_end - name_start;
                        wchar_t *macro_name = wcsndup(name_start, name_len);
                        if (!macro_name)
                        {
                            *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص ذاكرة لاسم الماكرو في #الغاء_تعريف.");
                            success = false;
                        }
                        else
                        {
                            // Attempt to undefine the macro (function handles 'not found' case gracefully)
                            undefine_macro(pp_state, macro_name);
                            free(macro_name);
                            // #الغاء_تعريف line is processed, do not append to output
                        }
                    }
                }
                // End of non-conditional directives processing
                else
                {
                    // Unrecognized directive while NOT skipping - pass through
                    if (!append_dynamic_buffer_n(&output_buffer, line_start, line_len))
                        success = false;
                    if (success && !append_to_dynamic_buffer(&output_buffer, L"\n"))
                        success = false;
                    if (!success && !*error_message)
                        *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في إلحاق السطر بمخزن الإخراج المؤقت.");
                }
            }
            // If it was a conditional directive OR we are skipping, we do nothing further with this line.
        }
        else if (!pp_state->skipping_lines)
        {
            // Not a directive AND not skipping: process line for macro substitution
            DynamicWcharBuffer substituted_line_buffer;
            if (!init_dynamic_buffer(&substituted_line_buffer, line_len + 64))
            { // Initial capacity estimate
                *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص الذاكرة لمخزن السطر المؤقت للاستبدال.");
                success = false;
            }
            else
            {
                const wchar_t *line_ptr = current_line;
                while (*line_ptr != L'\0' && success)
                {
                    // Find potential identifier start
                    if (iswalpha(*line_ptr) || *line_ptr == L'_')
                    {
                        const wchar_t *id_start = line_ptr;
                        while (iswalnum(*line_ptr) || *line_ptr == L'_')
                        {
                            line_ptr++;
                        }
                        size_t id_len = line_ptr - id_start;
                        wchar_t *identifier = wcsndup(id_start, id_len);
                        if (!identifier)
                        {
                            *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص ذاكرة للمعرف للاستبدال.");
                            success = false;
                            break;
                        }
                        const BaaMacro *macro = find_macro(pp_state, identifier);
                        if (macro)
                        {
                            // --- Recursion Check ---
                            if (is_macro_expanding(pp_state, macro))
                            {
                                *error_message = format_preprocessor_error_with_context(pp_state, L"تم اكتشاف استدعاء ذاتي للماكرو '%ls'.", macro->name);
                                success = false;
                                free(identifier); // Free identifier before breaking
                                break;
                            }

                            // --- Push macro onto expansion stack ---
                            if (!push_macro_expansion(pp_state, macro))
                            {
                                *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في دفع الماكرو '%ls' إلى مكدس التوسيع (نفاد الذاكرة؟).", macro->name);
                                success = false;
                                free(identifier);
                                break;
                            }

                            bool expansion_success = true; // Track success of this specific expansion

                            if (macro->is_function_like)
                            {
                                // Potential function-like macro invocation
                                const wchar_t *invocation_ptr = line_ptr; // Point after the identifier
                                // Skip whitespace before potential '('
                                while (iswspace(*invocation_ptr))
                                    invocation_ptr++;

                                if (*invocation_ptr == L'(')
                                {
                                    // Found '(', this IS a function-like macro invocation
                                    invocation_ptr++; // Consume '('
                                    // const wchar_t* args_start_ptr = invocation_ptr; // Save start for advancing line_ptr

                                    size_t actual_arg_count = 0;
                                    wchar_t **arguments = parse_macro_arguments(pp_state, &invocation_ptr, macro->param_count, &actual_arg_count, error_message);

                                    if (!arguments)
                                    {
                                        // Error occurred during argument parsing (needs update)
                                        if (!*error_message)
                                            *error_message = format_preprocessor_error_with_context(pp_state, L"خطأ في تحليل وسيطات الماكرو '%ls'.", macro->name);
                                        success = false;
                                    }
                                    else
                                    {
                                        // Check argument count
                                        if (actual_arg_count != macro->param_count)
                                        {
                                            *error_message = format_preprocessor_error_with_context(pp_state, L"عدد وسيطات غير صحيح للماكرو '%ls' (متوقع %zu، تم الحصول على %zu).",
                                                                                                    macro->name, macro->param_count, actual_arg_count);
                                            success = false;
                                        }
                                        else
                                        {
                                            // Perform substitution
                                            if (!substitute_macro_body(pp_state, &substituted_line_buffer, macro, arguments, actual_arg_count, error_message))
                                            {
                                                // Error during substitution
                                                if (!*error_message)
                                                    *error_message = format_preprocessor_error_with_context(pp_state, L"خطأ أثناء استبدال نص الماكرو '%ls'.", macro->name);
                                                success = false;
                                            }
                                        }

                                        // Free parsed arguments
                                        // Note: parse_macro_arguments currently returns placeholder, so this loop does nothing yet
                                        for (size_t i = 0; i < actual_arg_count; ++i)
                                        {
                                            free(arguments[i]);
                                        }
                                        free(arguments);
                                    }

                                    // Advance the main line pointer past the invocation
                                    line_ptr = invocation_ptr; // invocation_ptr was updated by parse_macro_arguments
                                }
                                else
                                {
                                    // Function-like macro name NOT followed by '(', treat as normal identifier
                                    if (!append_dynamic_buffer_n(&substituted_line_buffer, id_start, id_len))
                                        expansion_success = false;
                                }
                            }
                            else
                            {
                                // Simple object-like macro substitution
                                if (!substitute_macro_body(pp_state, &substituted_line_buffer, macro, NULL, 0, error_message))
                                {
                                    // Error during substitution
                                    if (!*error_message)
                                        *error_message = format_preprocessor_error_with_context(pp_state, L"خطأ أثناء استبدال نص الماكرو '%ls'.", macro->name);
                                    expansion_success = false;
                                }
                            }

                            // --- Pop macro from expansion stack ---
                            pop_macro_expansion(pp_state);

                            if (!expansion_success)
                            {
                                success = false; // Propagate error
                            }
                        }
                        else // Not a macro
                        {
                            // Append original identifier
                            if (!append_dynamic_buffer_n(&substituted_line_buffer, id_start, id_len))
                                success = false;
                        }
                        free(identifier);
                    }
                    else
                    {
                        // Append non-identifier character
                        if (!append_dynamic_buffer_n(&substituted_line_buffer, line_ptr, 1))
                            success = false;
                        line_ptr++;
                    }
                } // End while processing line

                if (success)
                {
                    // Append the fully substituted line to the main output
                    if (!append_to_dynamic_buffer(&output_buffer, substituted_line_buffer.buffer))
                        success = false;
                }
                free_dynamic_buffer(&substituted_line_buffer); // Free the temp line buffer
            }
            // Append newline after processing the line (substituted or not)
            if (success && !append_to_dynamic_buffer(&output_buffer, L"\n"))
                success = false; // Append newline
            if (!success && !*error_message)
                *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في إلحاق السطر بمخزن الإخراج المؤقت.");
        }

        free(current_line); // Free the duplicated line

        if (line_end != NULL)
        {
            line_start = line_end + 1;       // Move to the next line
            pp_state->current_line_number++; // Increment line number
        }
        else
        {
            break; // End of file content
        }
    }

    // Error handling for append failures during loop
    if (!success)
    {
        free_dynamic_buffer(&output_buffer);
        free(file_content);
        pop_file_stack(pp_state);
        free(abs_path);
        pp_state->current_file_path = prev_file_path; // Restore before returning
        pp_state->current_line_number = prev_line_number;
        return NULL;
    }

    free(file_content); // Original content buffer no longer needed

    // 4. Clean up stack and restore context for this file
    pop_file_stack(pp_state);
    free(abs_path); // Free the absolute path string we allocated
    pp_state->current_file_path = prev_file_path;
    pp_state->current_line_number = prev_line_number;

    // Return the final concatenated buffer (ownership transferred)
    return output_buffer.buffer;
}

// --- Function-Like Macro Helpers ---

// Helper function to convert an argument to a string literal, escaping necessary characters.
// Appends the result (including quotes) to output_buffer.
// Returns true on success, false on error.
static bool stringify_argument(BaaPreprocessor *pp_state, DynamicWcharBuffer *output_buffer, const wchar_t *argument, wchar_t **error_message)
{
    // Estimate needed capacity: quotes + original length + potential escapes
    size_t initial_capacity = wcslen(argument) + 10; // Add some buffer for escapes + quotes
    DynamicWcharBuffer temp_buffer;
    if (!init_dynamic_buffer(&temp_buffer, initial_capacity))
    {
        *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص ذاكرة مؤقتة لتسلسل الوسيطة.");
        return false;
    }

    bool success = true;
    // Append opening quote
    if (!append_to_dynamic_buffer(&temp_buffer, L"\""))
    {
        success = false;
    }

    // Iterate through argument, escaping characters as needed
    const wchar_t *ptr = argument;
    while (*ptr != L'\0' && success)
    {
        wchar_t char_to_append[3] = {0}; // Max 2 chars for escape + null terminator
        bool needs_escape = false;

        if (*ptr == L'\\' || *ptr == L'"')
        {
            needs_escape = true;
            char_to_append[0] = L'\\';
            char_to_append[1] = *ptr;
        }
        else
        {
            char_to_append[0] = *ptr;
        }

        if (!append_to_dynamic_buffer(&temp_buffer, char_to_append))
        {
            success = false;
        }
        ptr++;
    }

    // Append closing quote
    if (success && !append_to_dynamic_buffer(&temp_buffer, L"\""))
    {
        success = false;
    }

    // Append the final stringified result to the main output buffer
    if (success)
    {
        if (!append_to_dynamic_buffer(output_buffer, temp_buffer.buffer))
        {
            *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في إلحاق الوسيطة المتسلسلة للمخرج.");
            success = false;
        }
    }

    free_dynamic_buffer(&temp_buffer);
    return success;
}

// Parses macro arguments from an invocation string.
// Updates invocation_ptr_ref to point after the closing parenthesis.
// Returns a dynamically allocated array of argument strings (caller must free each string and the array).
// Returns NULL on error and sets error_message.
// NOTE: This is a simplified implementation. It doesn't handle nested parentheses,
//       commas inside strings/chars, or complex whitespace correctly yet.
static wchar_t **parse_macro_arguments(BaaPreprocessor *pp_state, const wchar_t **invocation_ptr_ref, size_t expected_arg_count, size_t *actual_arg_count, wchar_t **error_message)
{
    *actual_arg_count = 0;
    *error_message = NULL;
    const wchar_t *ptr = *invocation_ptr_ref;

    wchar_t **args = NULL;
    size_t args_capacity = 0;

    // Simple loop to find arguments separated by commas until ')'
    while (*ptr != L'\0')
    {
        // Skip leading whitespace for the argument
        while (iswspace(*ptr))
            ptr++;

        if (*ptr == L')')
        {          // End of arguments
            ptr++; // Consume ')'
            break;
        }

        // If not the first argument, expect a comma before it
        if (*actual_arg_count > 0)
        {
            if (*ptr == L',')
            {
                ptr++; // Consume ','
                while (iswspace(*ptr))
                    ptr++; // Skip space after comma
            }
            else
            {
                *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق استدعاء الماكرو غير صالح: متوقع ',' أو ')' بين الوسيطات.");
                goto parse_error; // Use goto for cleanup
            }
        }

        // Find the start of the argument
        const wchar_t *arg_start = ptr;

        // Find the end of the argument (next ',' or ')' at the top level, respecting literals)
        int paren_level = 0;
        const wchar_t *arg_end = ptr;
        bool in_string = false;
        bool in_char = false;
        wchar_t prev_char = L'\0';

        while (*arg_end != L'\0')
        {
            if (in_string)
            {
                // Inside string literal
                if (*arg_end == L'"' && prev_char != L'\\')
                {
                    in_string = false; // End of string
                }
                // Ignore commas and parentheses inside strings
            }
            else if (in_char)
            {
                // Inside char literal
                if (*arg_end == L'\'' && prev_char != L'\\')
                {
                    in_char = false; // End of char
                }
                // Ignore commas and parentheses inside chars
            }
            else
            {
                // Not inside a string or char literal
                if (*arg_end == L'(')
                {
                    paren_level++;
                }
                else if (*arg_end == L')')
                {
                    if (paren_level == 0)
                        break; // End of the entire argument list
                    paren_level--;
                    if (paren_level < 0)
                    { // Mismatched parentheses
                        *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق استدعاء الماكرو غير صالح: أقواس غير متطابقة في الوسيطات.");
                        goto parse_error;
                    }
                }
                else if (*arg_end == L',' && paren_level == 0)
                {
                    break; // End of the current argument (only if not nested)
                }
                else if (*arg_end == L'"')
                {
                    in_string = true; // Start of string
                }
                else if (*arg_end == L'\'')
                {
                    in_char = true; // Start of char
                }
            }

            // Handle escaped characters (simple version: just track previous char)
            if (*arg_end == L'\\' && prev_char == L'\\')
            {
                prev_char = L'\0'; // Treat double backslash as escaped, reset prev_char
            }
            else
            {
                prev_char = *arg_end;
            }
            arg_end++;
        }
        // arg_end now points to the delimiter (',' or ')') or null terminator

        if (paren_level != 0)
        { // Mismatched parentheses at the end
            *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق استدعاء الماكرو غير صالح: أقواس غير متطابقة في نهاية الوسيطات.");
            goto parse_error;
        }
        if (in_string || in_char)
        { // Unterminated literal at the end
            *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق استدعاء الماكرو غير صالح: علامة اقتباس غير منتهية في الوسيطات.");
            goto parse_error;
        }

        // Advance the main pointer 'ptr' to where arg_end stopped
        ptr = arg_end;

        // Trim trailing whitespace from the argument
        while (arg_end > arg_start && iswspace(*(arg_end - 1)))
        {
            arg_end--;
        }
        size_t arg_len = arg_end - arg_start;

        // Duplicate the argument
        wchar_t *arg_str = wcsndup(arg_start, arg_len);
        if (!arg_str)
        {
            *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص ذاكرة لوسيطة الماكرو.");
            goto parse_error;
        }

        // Resize args array if needed
        if (*actual_arg_count >= args_capacity)
        {
            size_t new_capacity = (args_capacity == 0) ? 4 : args_capacity * 2;
            wchar_t **new_args = realloc(args, new_capacity * sizeof(wchar_t *));
            if (!new_args)
            {
                free(arg_str);
                *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في إعادة تخصيص الذاكرة لوسيطات الماكرو.");
                goto parse_error;
            }
            args = new_args;
            args_capacity = new_capacity;
        }
        args[*actual_arg_count] = arg_str;
        (*actual_arg_count)++;

        // ptr should already be pointing at the delimiter (',' or ')') or end of string
        if (*ptr == L'\0')
        { // Reached end of string unexpectedly
            *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق استدعاء الماكرو غير صالح: قوس الإغلاق ')' مفقود.");
            goto parse_error;
        }
        // If it was ',', the next loop iteration will consume it.
        // If it was ')', the next loop iteration will break.

    } // End while parsing arguments

    // Check if we exited loop because of ')'
    if (*(ptr - 1) != L')')
    { // Check the character before the current ptr position
        *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق استدعاء الماكرو غير صالح: قوس الإغلاق ')' مفقود بعد الوسيطات.");
        goto parse_error;
    }

    *invocation_ptr_ref = ptr; // Update the caller's pointer
    return args;

parse_error:
    // Cleanup allocated arguments on error
    if (args)
    {
        for (size_t i = 0; i < *actual_arg_count; ++i)
        {
            free(args[i]);
        }
        free(args);
    }
    *actual_arg_count = 0;
    return NULL;
}

// Performs substitution of parameters within a macro body.
// Appends the result to the output_buffer.
// Returns true on success, false on error (setting error_message).
// Handles parameter substitution, stringification (#), and token pasting (##).
static bool substitute_macro_body(BaaPreprocessor *pp_state, DynamicWcharBuffer *output_buffer, const BaaMacro *macro, wchar_t **arguments, size_t arg_count, wchar_t **error_message)
{
    const wchar_t *body_ptr = macro->body;
    bool success = true;
    DynamicWcharBuffer pending_token_buffer; // Buffer for the token being built/substituted before pasting/appending
    bool pending_token_active = false;       // True if pending_token_buffer holds a token part

    if (!init_dynamic_buffer(&pending_token_buffer, 64))
    {
        *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تهيئة المخزن المؤقت للرمز المميز المعلق.");
        return false;
    }

    while (*body_ptr != L'\0' && success)
    {
        // Skip whitespace between tokens, but only if not currently building a pending token for pasting
        if (iswspace(*body_ptr))
        {
            if (pending_token_active)
            {
                // Flush the pending token before appending whitespace
                if (!append_to_dynamic_buffer(output_buffer, pending_token_buffer.buffer))
                {
                    success = false;
                    break;
                }
                free_dynamic_buffer(&pending_token_buffer);
                if (!init_dynamic_buffer(&pending_token_buffer, 64))
                {
                    success = false;
                    break;
                } // Re-init
                pending_token_active = false;
            }
            // Append the whitespace directly to the main output
            if (!append_dynamic_buffer_n(output_buffer, body_ptr, 1))
            {
                success = false;
                break;
            }
            body_ptr++;
            continue;
        }

        // --- Check for ## (Token Pasting) ---
        if (*body_ptr == L'#' && *(body_ptr + 1) == L'#')
        {
            if (!pending_token_active)
            {
                // ## cannot appear at the start or after whitespace without a preceding token
                *error_message = format_preprocessor_error_with_context(pp_state, L"المعامل ## يظهر في موقع غير صالح في الماكرو '%ls'.", macro->name);
                success = false;
                break;
            }
            // We have an active pending token (LHS). Now parse RHS.
            body_ptr += 2; // Skip ##
            while (iswspace(*body_ptr))
                body_ptr++; // Skip space after ##

            // Parse RHS token (must be identifier/parameter for now)
            if (!(iswalpha(*body_ptr) || *body_ptr == L'_'))
            {
                *error_message = format_preprocessor_error_with_context(pp_state, L"المعامل ## يجب أن يتبعه معرف في الماكرو '%ls'.", macro->name);
                success = false;
                break;
            }
            const wchar_t *rhs_id_start = body_ptr;
            while (iswalnum(*body_ptr) || *body_ptr == L'_')
                body_ptr++;
            size_t rhs_id_len = body_ptr - rhs_id_start;
            wchar_t *rhs_identifier = wcsndup(rhs_id_start, rhs_id_len);
            if (!rhs_identifier)
            {
                *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص ذاكرة لمعرف RHS لـ ##.");
                success = false;
                break;
            }

            wchar_t *rhs_value = NULL;
            bool rhs_is_param = false;
            size_t rhs_param_index = (size_t)-1;
            // Check if RHS is parameter
            for (size_t i = 0; i < macro->param_count; ++i)
            {
                if (wcscmp(rhs_identifier, macro->param_names[i]) == 0)
                {
                    rhs_is_param = true;
                    rhs_param_index = i;
                    break;
                }
            }

            if (rhs_is_param)
            {
                rhs_value = wcsdup(arguments[rhs_param_index]);
            }
            else
            {
                rhs_value = wcsdup(rhs_identifier); // Use literal RHS identifier
            }
            free(rhs_identifier);
            if (!rhs_value)
            {
                *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص ذاكرة لقيمة RHS لـ ##.");
                success = false;
                break;
            }

            // Concatenate pending_token_buffer.buffer (LHS) and rhs_value (RHS)
            size_t lhs_len = pending_token_buffer.length;
            size_t rhs_len = wcslen(rhs_value);
            size_t combined_len = lhs_len + rhs_len;
            wchar_t *pasted_token = malloc((combined_len + 1) * sizeof(wchar_t));
            if (!pasted_token)
            {
                *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص ذاكرة للرمز المميز الملصق ##.");
                free(rhs_value);
                success = false;
                break;
            }
            // Build the pasted token
            if (lhs_len > 0)
                wcscpy(pasted_token, pending_token_buffer.buffer);
            else
                pasted_token[0] = L'\0';
            wcscat(pasted_token, rhs_value);
            free(rhs_value);

            // The pasted token becomes the new pending token
            free_dynamic_buffer(&pending_token_buffer); // Free old pending buffer
            if (!init_dynamic_buffer(&pending_token_buffer, combined_len + 64))
            { // Re-init with better capacity
                *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في إعادة تهيئة المخزن المؤقت للرمز المميز المعلق بعد ##.");
                free(pasted_token);
                success = false;
                break;
            }
            if (!append_to_dynamic_buffer(&pending_token_buffer, pasted_token))
            {
                *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في إلحاق الرمز المميز الملصق بالمخزن المؤقت المعلق.");
                free(pasted_token);
                success = false;
                break;
            }
            free(pasted_token);
            pending_token_active = true; // The pasted token is now pending

            continue; // Continue loop, body_ptr is past RHS
        }

        // --- If not ##, flush any existing pending token to main output ---
        // (Unless the pending token was just created by pasting above)
        if (pending_token_active && !(*body_ptr == L'#' && *(body_ptr + 1) == L'#'))
        { // Check again to avoid double flush if ## follows immediately
            if (!append_to_dynamic_buffer(output_buffer, pending_token_buffer.buffer))
            {
                success = false;
                break;
            }
            free_dynamic_buffer(&pending_token_buffer);
            if (!init_dynamic_buffer(&pending_token_buffer, 64))
            {
                success = false;
                break;
            } // Re-init
            pending_token_active = false;
        }
        if (!success)
            break; // Check after potential flush failure

        // --- Check for # (Stringify) ---
        if (*body_ptr == L'#')
        {
            const wchar_t *operator_ptr = body_ptr;
            body_ptr++; // Move past '#'

            // Stringification requires an identifier immediately after (parameter name)
            if (iswalpha(*body_ptr) || *body_ptr == L'_')
            {
                const wchar_t *id_start = body_ptr;
                while (iswalnum(*body_ptr) || *body_ptr == L'_')
                    body_ptr++;
                size_t id_len = body_ptr - id_start;
                wchar_t *identifier = wcsndup(id_start, id_len);
                if (!identifier)
                {
                    *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص ذاكرة للمعرف بعد '#' في نص الماكرو '%ls'.", macro->name);
                    success = false;
                    break;
                }

                bool param_found = false;
                for (size_t i = 0; i < macro->param_count; ++i)
                {
                    if (wcscmp(identifier, macro->param_names[i]) == 0)
                    {
                        // Found #param, perform stringification directly into output buffer
                        if (!stringify_argument(pp_state, output_buffer, arguments[i], error_message))
                        {
                            // Error message set by helper (needs update)
                            if (!*error_message)
                                *error_message = format_preprocessor_error_with_context(pp_state, L"خطأ في تسلسل الوسيطة '%ls' للماكرو '%ls'.", arguments[i], macro->name);
                            success = false;
                        }
                        param_found = true;
                        break;
                    }
                }
                free(identifier);

                if (!success)
                    break; // Exit main loop on stringify error

                if (!param_found)
                {
                    // '#' was not followed by a valid parameter name. Treat '#' literally.
                    if (!append_dynamic_buffer_n(output_buffer, operator_ptr, 1))
                    {
                        *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في إلحاق '#' الحرفية من نص الماكرو '%ls'.", macro->name);
                        success = false;
                        break;
                    }
                    // Let the loop continue to process the identifier normally in the next iteration
                    body_ptr = id_start; // Reset body_ptr to the start of the identifier
                }
                // If param_found, body_ptr is already advanced past the identifier, continue main loop
            }
            else
            {
                // '#' not followed by identifier, treat '#' as literal
                if (!append_dynamic_buffer_n(output_buffer, operator_ptr, 1))
                {
                    *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في إلحاق '#' الحرفية من نص الماكرو '%ls'.", macro->name);
                    success = false;
                    break;
                }
                // body_ptr is already advanced past '#'
            }
            continue; // Continue loop after handling #
        }

        // --- Parse next token (identifier or single char) and place in pending_token_buffer ---
        if (iswalpha(*body_ptr) || *body_ptr == L'_')
        {
            // Identifier token
            const wchar_t *id_start = body_ptr;
            while (iswalnum(*body_ptr) || *body_ptr == L'_')
                body_ptr++;
            size_t id_len = body_ptr - id_start;
            wchar_t *identifier = wcsndup(id_start, id_len);
            if (!identifier)
            {
                *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص ذاكرة للمعرف في نص الماكرو '%ls'.", macro->name);
                success = false;
                break;
            }

            // Check if parameter
            bool param_found = false;
            size_t param_index = (size_t)-1;
            for (size_t i = 0; i < macro->param_count; ++i)
            {
                if (wcscmp(identifier, macro->param_names[i]) == 0)
                {
                    param_found = true;
                    param_index = i;
                    break;
                }
            }

            if (param_found)
            {
                // Substitute parameter into pending_token_buffer
                if (!append_to_dynamic_buffer(&pending_token_buffer, arguments[param_index]))
                {
                    success = false;
                }
            }
            else
            {
                // Use literal identifier in pending_token_buffer
                if (!append_to_dynamic_buffer(&pending_token_buffer, identifier))
                {
                    success = false;
                }
            }
            free(identifier);
            if (!success)
                break;
            pending_token_active = true;
        }
        else
        {
            // Single character token (e.g., '+', ';', '(' etc.)
            if (!append_dynamic_buffer_n(&pending_token_buffer, body_ptr, 1))
            {
                success = false;
                break;
            }
            body_ptr++;
            pending_token_active = true;
        }
        // End of token parsing for this iteration

    } // End while processing body

    // Flush any remaining pending token after loop finishes
    if (success && pending_token_active)
    {
        if (!append_to_dynamic_buffer(output_buffer, pending_token_buffer.buffer))
        {
            success = false;
            if (!*error_message)
            { // Set error if not already set
                *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في إلحاق الرمز المميز المعلق الأخير في الماكرو '%ls'.", macro->name);
            }
        }
    }

    free_dynamic_buffer(&pending_token_buffer); // Clean up the pending buffer

    // Handle case where ## is the very last thing in the body?
    // The logic should prevent this because ## requires a following token.
    // If the loop ends after ## processing, the pasted token is in pending_token_buffer and gets flushed above.

    return success;
}

// --- Macro Expansion Stack Implementation ---

static bool push_macro_expansion(BaaPreprocessor *pp_state, const BaaMacro *macro)
{
    if (pp_state->expanding_macros_count >= pp_state->expanding_macros_capacity)
    {
        size_t new_capacity = (pp_state->expanding_macros_capacity == 0) ? 8 : pp_state->expanding_macros_capacity * 2;
        const BaaMacro **new_stack = realloc(pp_state->expanding_macros_stack, new_capacity * sizeof(BaaMacro *));
        if (!new_stack)
            return false; // Allocation failure
        pp_state->expanding_macros_stack = new_stack;
        pp_state->expanding_macros_capacity = new_capacity;
    }
    pp_state->expanding_macros_stack[pp_state->expanding_macros_count++] = macro;
    return true;
}

static void pop_macro_expansion(BaaPreprocessor *pp_state)
{
    if (pp_state->expanding_macros_count > 0)
    {
        pp_state->expanding_macros_count--;
    }
}

static bool is_macro_expanding(const BaaPreprocessor *pp_state, const BaaMacro *macro)
{
    for (size_t i = 0; i < pp_state->expanding_macros_count; ++i)
    {
        // Compare pointers - assumes macros are uniquely allocated
        if (pp_state->expanding_macros_stack[i] == macro)
        {
            return true;
        }
    }
    return false;
}

static void free_macro_expansion_stack(BaaPreprocessor *pp_state)
{
    free(pp_state->expanding_macros_stack);
    pp_state->expanding_macros_stack = NULL;
    pp_state->expanding_macros_count = 0;
    pp_state->expanding_macros_capacity = 0;
}

// Helper function to free macro storage
void free_macros(BaaPreprocessor *pp)
{
    if (pp && pp->macros)
    {
        for (size_t i = 0; i < pp->macro_count; ++i)
        {
            free(pp->macros[i].name);
            free(pp->macros[i].body);
            // Free parameter names if it's a function-like macro
            if (pp->macros[i].is_function_like && pp->macros[i].param_names)
            {
                for (size_t j = 0; j < pp->macros[i].param_count; ++j)
                {
                    free(pp->macros[i].param_names[j]);
                }
                free(pp->macros[i].param_names);
            }
        }
        free(pp->macros);
        pp->macros = NULL;
        pp->macro_count = 0;
        pp->macro_capacity = 0;
    }
}

// Helper function to add or update a macro definition (updated signature)
// Returns true on success, false on allocation failure.
// Handles reallocation of the macro array.
static bool add_macro(BaaPreprocessor *pp_state, const wchar_t *name, const wchar_t *body, bool is_function_like, size_t param_count, wchar_t **param_names)
{
    if (!pp_state || !name || !body)
    {
        // Free potentially allocated params if other args are invalid
        if (is_function_like && param_names)
        {
            for (size_t j = 0; j < param_count; ++j)
            {
                free(param_names[j]);
            }
            free(param_names);
        }
        return false;
    }

    // Check if macro already exists (simple linear search for now)
    for (size_t i = 0; i < pp_state->macro_count; ++i)
    {
        if (wcscmp(pp_state->macros[i].name, name) == 0)
        {
            // Redefinition: Free old body and params, update with new ones
            // Note: C standard usually warns/errors on incompatible redefinition.
            // Here, we just replace everything.
            free(pp_state->macros[i].body);
            if (pp_state->macros[i].is_function_like && pp_state->macros[i].param_names)
            {
                for (size_t j = 0; j < pp_state->macros[i].param_count; ++j)
                {
                    free(pp_state->macros[i].param_names[j]);
                }
                free(pp_state->macros[i].param_names);
            }

            pp_state->macros[i].body = wcsdup(body);
            pp_state->macros[i].is_function_like = is_function_like;
            pp_state->macros[i].param_count = param_count;
            pp_state->macros[i].param_names = param_names; // Takes ownership

            // Check if allocations succeeded
            if (!pp_state->macros[i].body)
            {
                // If body fails, try to clean up params if they were just assigned
                if (is_function_like && param_names)
                {
                    for (size_t j = 0; j < param_count; ++j)
                    {
                        free(param_names[j]);
                    }
                    free(param_names);
                }
                pp_state->macros[i].param_names = NULL; // Ensure it's NULL on failure
                // Macro entry might be in a bad state, but we signal failure
                return false;
            }
            return true; // Redefinition successful
        }
    }

    // New macro: Check capacity
    if (pp_state->macro_count >= pp_state->macro_capacity)
    {
        size_t new_capacity = (pp_state->macro_capacity == 0) ? 8 : pp_state->macro_capacity * 2;
        BaaMacro *new_macros = realloc(pp_state->macros, new_capacity * sizeof(BaaMacro));
        if (!new_macros)
        {
            return false; // Reallocation failed
        }
        pp_state->macros = new_macros;
        pp_state->macro_capacity = new_capacity;
    }

    // Add the new macro
    BaaMacro *new_entry = &pp_state->macros[pp_state->macro_count];
    new_entry->name = wcsdup(name);
    new_entry->body = wcsdup(body);
    new_entry->is_function_like = is_function_like;
    new_entry->param_count = param_count;
    new_entry->param_names = param_names; // Takes ownership of the passed array and its contents

    // Check allocations
    if (!new_entry->name || !new_entry->body)
    {
        // Clean up everything allocated for this new entry on failure
        free(new_entry->name); // Safe even if NULL
        free(new_entry->body); // Safe even if NULL
        if (is_function_like && param_names)
        {
            for (size_t j = 0; j < param_count; ++j)
            {
                free(param_names[j]); // Assumes param_names contains allocated strings
            }
            free(param_names);
        }
        return false; // Allocation failed
    }

    pp_state->macro_count++;
    return true;
}

// Helper function to find a macro by name
// Returns the macro definition or NULL if not found.
static const BaaMacro *find_macro(const BaaPreprocessor *pp_state, const wchar_t *name)
{
    if (!pp_state || !name || !pp_state->macros)
        return NULL;

    // Simple linear search for now. Could use hash map for performance later.
    for (size_t i = 0; i < pp_state->macro_count; ++i)
    {
        if (wcscmp(pp_state->macros[i].name, name) == 0)
        {
            return &pp_state->macros[i];
        }
    }
    return NULL; // Not found
}

// Helper function to remove a macro definition by name
// Returns true if found and removed, false otherwise.
static bool undefine_macro(BaaPreprocessor *pp_state, const wchar_t *name)
{
    if (!pp_state || !name || !pp_state->macros)
        return false;

    for (size_t i = 0; i < pp_state->macro_count; ++i)
    {
        if (wcscmp(pp_state->macros[i].name, name) == 0)
        {
            // Found the macro, now remove it
            free(pp_state->macros[i].name);
            free(pp_state->macros[i].body);
            // Free parameters if function-like
            if (pp_state->macros[i].is_function_like && pp_state->macros[i].param_names)
            {
                for (size_t j = 0; j < pp_state->macros[i].param_count; ++j)
                {
                    free(pp_state->macros[i].param_names[j]);
                }
                free(pp_state->macros[i].param_names);
            }

            // Shift subsequent elements down
            if (i < pp_state->macro_count - 1)
            {
                memmove(&pp_state->macros[i], &pp_state->macros[i + 1],
                        (pp_state->macro_count - 1 - i) * sizeof(BaaMacro));
            }

            pp_state->macro_count--;

            // Optional: Shrink capacity if desired, but not strictly necessary here
            // if (pp_state->macro_count < pp_state->macro_capacity / 4) { ... }

            return true; // Successfully removed
        }
    }

    return false; // Macro not found
}

wchar_t *baa_preprocess(const char *main_file_path, const char **include_paths, wchar_t **error_message)
{
    if (!main_file_path || !error_message)
    {
        if (error_message)
#ifdef _WIN32
            *error_message = _wcsdup(L"وسيطات غير صالحة تم تمريرها إلى المعالج المسبق.");
#else
            *error_message = wcsdup(L"وسيطات غير صالحة تم تمريرها إلى المعالج المسبق.");
#endif
        return NULL;
    }
    *error_message = NULL; // Initialize error message to NULL

    BaaPreprocessor pp_state = {0};
    pp_state.include_paths = include_paths;
    pp_state.include_path_count = 0;
    if (include_paths)
    {
        while (include_paths[pp_state.include_path_count] != NULL)
        {
            pp_state.include_path_count++;
        }
    }
    // Initialize file stack
    pp_state.open_files_stack = NULL;
    pp_state.open_files_count = 0;
    pp_state.open_files_capacity = 0;
    // --- Initialize Macro storage ---
    pp_state.macros = NULL;
    pp_state.macro_count = 0;
    pp_state.macro_capacity = 0;
    // Initialize conditional stack
    pp_state.conditional_stack = NULL;
    pp_state.conditional_stack_count = 0;
    pp_state.conditional_stack_capacity = 0;
    pp_state.conditional_branch_taken_stack = NULL; // Initialize new stack
    pp_state.conditional_branch_taken_stack_count = 0;
    pp_state.conditional_branch_taken_stack_capacity = 0;
    pp_state.skipping_lines = false; // Start not skipping
    // Initialize macro expansion stack
    pp_state.expanding_macros_stack = NULL;
    pp_state.expanding_macros_count = 0;
    pp_state.expanding_macros_capacity = 0;
    // Initialize error reporting context
    pp_state.current_file_path = NULL; // Will be set within process_file
    pp_state.current_line_number = 0;  // Will be set within process_file
    // --- End Initialize ---

    // Start recursive processing
    wchar_t *final_output = process_file(&pp_state, main_file_path, error_message);

    // --- Cleanup ---
    free_file_stack(&pp_state);            // Free include stack
    free_macros(&pp_state);                // Free macro definitions
    free_conditional_stack(&pp_state);     // Free conditional stack
    free_macro_expansion_stack(&pp_state); // Free macro expansion stack

    // Check for unterminated conditional block after processing is complete
    if (pp_state.conditional_stack_count > 0 && !*error_message)
    {
        *error_message = format_preprocessor_error(L"كتلة شرطية غير منتهية في نهاية المعالجة (مفقود #نهاية_إذا).");
        free(final_output); // Free potentially partially generated output
        final_output = NULL;
    }

    if (!final_output)
    {
        // error_message should be set by process_file or earlier checks
        return NULL;
    }

    // Return the fully processed string
    return final_output;

    // Old placeholder error:
    // *error_message = format_preprocessor_error(L"المعالج المسبق لم يتم تنفيذه بعد.");
    // return NULL;
}

// --- Preprocessor Expression Evaluation Implementation ---

#include <ctype.h> // For isdigit

// Token types for the expression evaluator
typedef enum
{
    PP_EXPR_TOKEN_EOF,
    PP_EXPR_TOKEN_ERROR,
    PP_EXPR_TOKEN_INT_LITERAL, // Integer literal
    PP_EXPR_TOKEN_IDENTIFIER,  // Identifier (could be macro or 'defined')
    PP_EXPR_TOKEN_DEFINED,     // 'defined' keyword
    PP_EXPR_TOKEN_LPAREN,      // (
    PP_EXPR_TOKEN_RPAREN,      // )
    // Operators
    PP_EXPR_TOKEN_PLUS,     // +
    PP_EXPR_TOKEN_MINUS,    // -
    PP_EXPR_TOKEN_STAR,     // *
    PP_EXPR_TOKEN_SLASH,    // /
    PP_EXPR_TOKEN_PERCENT,  // %
    PP_EXPR_TOKEN_EQEQ,     // ==
    PP_EXPR_TOKEN_BANGEQ,   // !=
    PP_EXPR_TOKEN_LT,       // <
    PP_EXPR_TOKEN_GT,       // >
    PP_EXPR_TOKEN_LTEQ,     // <=
    PP_EXPR_TOKEN_GTEQ,     // >=
    PP_EXPR_TOKEN_AMPAMP,   // &&
    PP_EXPR_TOKEN_PIPEPIPE, // ||
    PP_EXPR_TOKEN_BANG,     // !
    // TODO: Add bitwise operators? &, |, ^, ~, <<, >>
} PpExprTokenType;

// Token structure
typedef struct
{
    PpExprTokenType type;
    wchar_t *text; // For identifiers
    long value;    // For integer literals
} PpExprToken;

// Simple tokenizer state
typedef struct
{
    const wchar_t *current;
    const wchar_t *start;
    BaaPreprocessor *pp_state; // Access to macro definitions
    // const char *abs_path;      // For error reporting - REMOVED
    wchar_t **error_message;
} PpExprTokenizer;

// Helper to create an error token
static PpExprToken make_error_token(PpExprTokenizer *tz, const wchar_t *message)
{
    if (tz->error_message && !*tz->error_message)
    { // Set error only once
        // Use the context-aware error formatter now
        *tz->error_message = format_preprocessor_error_with_context(tz->pp_state, message);
    }
    return (PpExprToken){.type = PP_EXPR_TOKEN_ERROR};
}

// Helper to create a simple token
static PpExprToken make_token(PpExprTokenType type)
{
    return (PpExprToken){.type = type};
}

// Helper to create an integer literal token
static PpExprToken make_int_token(long value)
{
    return (PpExprToken){.type = PP_EXPR_TOKEN_INT_LITERAL, .value = value};
}

// Helper to create an identifier or 'defined' token
static PpExprToken make_identifier_token(PpExprTokenizer *tz)
{
    size_t len = tz->current - tz->start;
    wchar_t *text = wcsndup(tz->start, len);
    if (!text)
    {
        return make_error_token(tz, L"فشل في تخصيص ذاكرة للمعرف في التعبير الشرطي في '%hs'.");
    }

    // Check if it's the 'defined' keyword
    if (wcscmp(text, L"defined") == 0)
    {
        free(text);
        return make_token(PP_EXPR_TOKEN_DEFINED);
    }

    return (PpExprToken){.type = PP_EXPR_TOKEN_IDENTIFIER, .text = text};
}

// Skips whitespace
static void skip_whitespace(PpExprTokenizer *tz)
{
    while (iswspace(*tz->current))
    {
        tz->current++;
    }
}

// Gets the next token from the expression string
static PpExprToken get_next_pp_expr_token(PpExprTokenizer *tz)
{
    skip_whitespace(tz);
    tz->start = tz->current;

    if (*tz->current == L'\0')
        return make_token(PP_EXPR_TOKEN_EOF);

    // Check for operators and parentheses first
    switch (*tz->current)
    {
    case L'(':
        tz->current++;
        return make_token(PP_EXPR_TOKEN_LPAREN);
    case L')':
        tz->current++;
        return make_token(PP_EXPR_TOKEN_RPAREN);
    case L'+':
        tz->current++;
        return make_token(PP_EXPR_TOKEN_PLUS);
    case L'-':
        tz->current++;
        return make_token(PP_EXPR_TOKEN_MINUS);
    case L'*':
        tz->current++;
        return make_token(PP_EXPR_TOKEN_STAR);
    case L'/':
        tz->current++;
        return make_token(PP_EXPR_TOKEN_SLASH);
    case L'%':
        tz->current++;
        return make_token(PP_EXPR_TOKEN_PERCENT);
    case L'!':
        if (*(tz->current + 1) == L'=')
        {
            tz->current += 2;
            return make_token(PP_EXPR_TOKEN_BANGEQ);
        }
        tz->current++;
        return make_token(PP_EXPR_TOKEN_BANG);
    case L'=':
        if (*(tz->current + 1) == L'=')
        {
            tz->current += 2;
            return make_token(PP_EXPR_TOKEN_EQEQ);
        }
        // Single '=' is assignment, invalid in preprocessor expr
        return make_error_token(tz, L"المعامل '=' غير صالح في التعبير الشرطي في '%hs'.");
    case L'<':
        if (*(tz->current + 1) == L'=')
        {
            tz->current += 2;
            return make_token(PP_EXPR_TOKEN_LTEQ);
        }
        // TODO: Handle << later if needed
        tz->current++;
        return make_token(PP_EXPR_TOKEN_LT);
    case L'>':
        if (*(tz->current + 1) == L'=')
        {
            tz->current += 2;
            return make_token(PP_EXPR_TOKEN_GTEQ);
        }
        // TODO: Handle >> later if needed
        tz->current++;
        return make_token(PP_EXPR_TOKEN_GT);
    case L'&':
        if (*(tz->current + 1) == L'&')
        {
            tz->current += 2;
            return make_token(PP_EXPR_TOKEN_AMPAMP);
        }
        // TODO: Handle single '&' later if needed
        return make_error_token(tz, L"المعامل '&' غير مدعوم حاليًا في التعبير الشرطي في '%hs'.");
    case L'|':
        if (*(tz->current + 1) == L'|')
        {
            tz->current += 2;
            return make_token(PP_EXPR_TOKEN_PIPEPIPE);
        }
        // TODO: Handle single '|' later if needed
        return make_error_token(tz, L"المعامل '|' غير مدعوم حاليًا في التعبير الشرطي في '%hs'.");
        // TODO: Add cases for other operators like ^, ~
    }

    // Check for integer literals
    if (iswdigit(*tz->current))
    {
        wchar_t *endptr;
        long value = wcstol(tz->start, &endptr, 10); // Base 10 only for now
        if (endptr == tz->start)
        { // Should not happen if iswdigit passed, but safety
            return make_error_token(tz, L"حرف رقمي غير صالح في التعبير الشرطي في '%hs'.");
        }
        tz->current = endptr; // Advance tokenizer past the number
        return make_int_token(value);
    }

    // Check for identifiers (including 'defined')
    if (iswalpha(*tz->current) || *tz->current == L'_')
    {
        while (iswalnum(*tz->current) || *tz->current == L'_')
        {
            tz->current++;
        }
        return make_identifier_token(tz);
    }

    // Unknown character
    return make_error_token(tz, L"رمز غير متوقع في التعبير الشرطي في '%hs'.");
}

// --- Actual Expression Evaluation Function ---

// Forward declarations for the expression parser/evaluator
static bool parse_and_evaluate_pp_expr(PpExprTokenizer *tz, long *result);
static bool parse_primary_pp_expr(PpExprTokenizer *tz, long *result);
static bool parse_binary_expression_rhs(PpExprTokenizer *tz, int expr_prec, long *lhs, long *result);
static int get_token_precedence(PpExprTokenType type);

// Evaluates preprocessor constant expressions.
// Currently handles integer literals, defined(MACRO), and unary !.
// Needs expansion for full operator precedence (e.g., using Shunting-yard).
static bool evaluate_preprocessor_expression(BaaPreprocessor *pp_state, const wchar_t *expression, bool *value, wchar_t **error_message)
{
    *error_message = NULL;
    *value = false; // Default to false

    PpExprTokenizer tz = {
        .current = expression,
        .start = expression,
        .pp_state = pp_state,
        // .abs_path = abs_path, // REMOVED
        .error_message = error_message};

    long result_value = 0;
    if (!parse_and_evaluate_pp_expr(&tz, &result_value))
    {
        // Error message should be set by the parser/evaluator
        return false;
    }

    // Check if the entire expression was consumed
    PpExprToken eof_token = get_next_pp_expr_token(&tz);
    if (eof_token.type != PP_EXPR_TOKEN_EOF)
    {
        if (eof_token.type != PP_EXPR_TOKEN_ERROR)
        { // Avoid overwriting tokenizer error
            // Use make_error_token to ensure consistent context reporting
            make_error_token(&tz, L"رموز زائدة في نهاية التعبير الشرطي.");
        }
        return false;
    }

    *value = (result_value != 0); // Final result: 0 is false, non-zero is true
    return true;
}

// Simple recursive descent parser (starting point)
// TODO: Expand this to handle operator precedence (e.g., Shunting-yard)
static bool parse_and_evaluate_pp_expr(PpExprTokenizer *tz, long *result)
{
    // For now, just parse a primary expression possibly preceded by unary operators
    // This doesn't handle binary operators or precedence yet.

    bool negate = false;
    // Handle leading unary operators (simple case: only !)
    PpExprToken op_token = get_next_pp_expr_token(tz);

    if (op_token.type == PP_EXPR_TOKEN_BANG)
    {
        negate = true;
    }
    else
    {
        // Put the token back (or handle other unary ops like -, +)
        // Since we don't have a proper token stream/lookahead, we reset the pointer
        tz->current = tz->start; // Reset to beginning of the token we just read
    }

    // Parse the primary expression
    if (!parse_primary_pp_expr(tz, result))
    {
        return false; // Error occurred in primary parsing
    }

    // Apply unary operator if present
    if (negate)
    {
        *result = (*result == 0); // Logical negation: 0 becomes 1, non-zero becomes 0
    }

    // Now parse binary operators with precedence climbing
    if (!parse_binary_expression_rhs(tz, 0, result, result))
    {
        return false;
    }

    return true;
}

// Parses primary expressions: integer literals, defined(MACRO), ( expression )
static bool parse_primary_pp_expr(PpExprTokenizer *tz, long *result)
{
    PpExprToken token = get_next_pp_expr_token(tz);

    if (token.type == PP_EXPR_TOKEN_INT_LITERAL)
    {
        *result = token.value;
        return true;
    }
    else if (token.type == PP_EXPR_TOKEN_DEFINED)
    {
        // Expect '(' or identifier
        bool parens = false;
        PpExprToken next_token = get_next_pp_expr_token(tz);
        if (next_token.type == PP_EXPR_TOKEN_LPAREN)
        {
            parens = true;
            next_token = get_next_pp_expr_token(tz); // Get token inside parens
        }

        if (next_token.type != PP_EXPR_TOKEN_IDENTIFIER)
        {
            if (next_token.type != PP_EXPR_TOKEN_ERROR)
            {
                // Use make_error_token for context
                make_error_token(tz, L"تنسيق defined() غير صالح: متوقع معرف.");
            }
            if (next_token.type == PP_EXPR_TOKEN_IDENTIFIER)
                free(next_token.text);
            return false;
        }

        // Check if the identifier macro is defined
        *result = (find_macro(tz->pp_state, next_token.text) != NULL) ? 1L : 0L; // Result is 1 or 0
        free(next_token.text);

        // Check for closing parenthesis if needed
        if (parens)
        {
            PpExprToken closing_paren = get_next_pp_expr_token(tz);
            if (closing_paren.type != PP_EXPR_TOKEN_RPAREN)
            {
                if (closing_paren.type != PP_EXPR_TOKEN_ERROR)
                {
                    // Use make_error_token for context
                    make_error_token(tz, L"تنسيق defined() غير صالح: قوس الإغلاق ')' مفقود.");
                }
                return false;
            }
        }
        return true; // Successfully evaluated defined()
    }
    else if (token.type == PP_EXPR_TOKEN_LPAREN)
    {
        // Parse expression inside parentheses
        if (!parse_and_evaluate_pp_expr(tz, result))
        {
            return false; // Error inside parentheses
        }
        // Expect closing parenthesis
        PpExprToken closing_paren = get_next_pp_expr_token(tz);
        if (closing_paren.type != PP_EXPR_TOKEN_RPAREN)
        {
            if (closing_paren.type != PP_EXPR_TOKEN_ERROR)
            {
                // Use make_error_token for context
                make_error_token(tz, L"قوس الإغلاق ')' مفقود بعد التعبير.");
            }
            return false;
        }
        return true;
    }
    else if (token.type == PP_EXPR_TOKEN_IDENTIFIER)
    {
        // Undefined identifiers evaluate to 0 in preprocessor expressions
        *result = 0L;
        free(token.text);
        return true;
    }
    else if (token.type == PP_EXPR_TOKEN_ERROR)
    {
        // Error already set by tokenizer
        return false;
    }
    else
    {
        // Unexpected token
        // Use make_error_token for context
        make_error_token(tz, L"رمز غير متوقع في بداية التعبير الأولي.");
        return false;
    }
}

// --- Precedence Climbing Parser Implementation ---

// Gets the precedence level of a binary operator token. Returns -1 if not a binary operator.
static int get_token_precedence(PpExprTokenType type)
{
    switch (type)
    {
    // Lower precedence binds looser
    case PP_EXPR_TOKEN_PIPEPIPE:
        return 10; // ||
    case PP_EXPR_TOKEN_AMPAMP:
        return 20; // &&
    // TODO: Bitwise OR | (30)
    // TODO: Bitwise XOR ^ (40)
    // TODO: Bitwise AND & (50)
    case PP_EXPR_TOKEN_EQEQ:
    case PP_EXPR_TOKEN_BANGEQ:
        return 60; // == !=
    case PP_EXPR_TOKEN_LT:
    case PP_EXPR_TOKEN_GT:
    case PP_EXPR_TOKEN_LTEQ:
    case PP_EXPR_TOKEN_GTEQ:
        return 70; // < > <= >=
    // TODO: Bitwise Shift << >> (80)
    case PP_EXPR_TOKEN_PLUS:
    case PP_EXPR_TOKEN_MINUS:
        return 90; // + -
    case PP_EXPR_TOKEN_STAR:
    case PP_EXPR_TOKEN_SLASH:
    case PP_EXPR_TOKEN_PERCENT:
        return 100; // * / %
    // Higher precedence binds tighter
    default:
        return -1; // Not a binary operator we handle yet
    }
}

// Parses the right-hand side of binary expressions using precedence climbing.
static bool parse_binary_expression_rhs(PpExprTokenizer *tz, int min_prec, long *lhs, long *result)
{
    while (true)
    {
        // Peek at the next token to see if it's a binary operator
        const wchar_t *current_pos_backup = tz->current; // Backup position
        PpExprToken op_token = get_next_pp_expr_token(tz);
        int token_prec = get_token_precedence(op_token.type);

        // If it's not a binary operator OR its precedence is too low, we're done with this level.
        if (token_prec < min_prec)
        {
            tz->current = current_pos_backup; // Put the token back by resetting position
            *result = *lhs;                   // The LHS is the final result for this precedence level
            return true;
        }

        // We have a binary operator with sufficient precedence. Consume it (already done by get_next_pp_expr_token).

        // Parse the primary expression after the operator (this will become the RHS)
        long rhs = 0;
        if (!parse_primary_pp_expr(tz, &rhs))
        {
            // Error parsing RHS primary
            return false;
        }

        // Peek at the *next* operator to handle right-associativity or higher precedence.
        const wchar_t *next_op_pos_backup = tz->current;
        PpExprToken next_op_token = get_next_pp_expr_token(tz);
        int next_prec = get_token_precedence(next_op_token.type);
        tz->current = next_op_pos_backup; // Reset position after peeking

        // If the next operator has higher precedence, recursively parse its RHS first.
        if (next_prec > token_prec)
        {
            if (!parse_binary_expression_rhs(tz, token_prec + 1, &rhs, &rhs))
            {
                // Error parsing nested RHS
                return false;
            }
        }
        // --- Perform the operation for the current operator ---
        long current_lhs = *lhs; // Capture LHS before potential modification by recursive calls
        switch (op_token.type)
        {
        case PP_EXPR_TOKEN_PLUS:
            *lhs = current_lhs + rhs;
            break;
        case PP_EXPR_TOKEN_MINUS:
            *lhs = current_lhs - rhs;
            break;
        case PP_EXPR_TOKEN_STAR:
            *lhs = current_lhs * rhs;
            break;
        case PP_EXPR_TOKEN_SLASH:
            if (rhs == 0)
            {
                // Use make_error_token for context
                make_error_token(tz, L"قسمة على صفر في التعبير الشرطي.");
                return false;
            }
            *lhs = current_lhs / rhs;
            break;
        case PP_EXPR_TOKEN_PERCENT:
            if (rhs == 0)
            {
                // Use make_error_token for context
                make_error_token(tz, L"قسمة على صفر (معامل الباقي) في التعبير الشرطي.");
                return false;
            }
            *lhs = current_lhs % rhs;
            break;
        case PP_EXPR_TOKEN_EQEQ:
            *lhs = (current_lhs == rhs);
            break;
        case PP_EXPR_TOKEN_BANGEQ:
            *lhs = (current_lhs != rhs);
            break;
        case PP_EXPR_TOKEN_LT:
            *lhs = (current_lhs < rhs);
            break;
        case PP_EXPR_TOKEN_GT:
            *lhs = (current_lhs > rhs);
            break;
        case PP_EXPR_TOKEN_LTEQ:
            *lhs = (current_lhs <= rhs);
            break;
        case PP_EXPR_TOKEN_GTEQ:
            *lhs = (current_lhs >= rhs);
            break;
        case PP_EXPR_TOKEN_AMPAMP:
            *lhs = (current_lhs != 0 && rhs != 0);
            break; // Use integer logic
        case PP_EXPR_TOKEN_PIPEPIPE:
            *lhs = (current_lhs != 0 || rhs != 0);
            break; // Use integer logic
        default:
            // Should not happen if get_token_precedence is correct
            // Use make_error_token for context
            make_error_token(tz, L"معامل ثنائي غير متوقع أو غير مدعوم.");
            return false;
        }
        // The result of the operation becomes the new LHS for the next iteration of the loop.
    }
}
