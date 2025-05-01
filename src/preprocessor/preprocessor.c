#include "baa/preprocessor/preprocessor.h"
#include "baa/utils/utils.h"
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

// Implementation of wcsndup for Windows compatibility
static wchar_t *wcsndup(const wchar_t *s, size_t n)
{
    wchar_t *result = (wchar_t *)malloc((n + 1) * sizeof(wchar_t));
    if (result)
    {
        wcsncpy(result, s, n);
        result[n] = L'\0';
    }
    return result;
}

// Forward declarations for static functions
// Updated add_macro signature
static bool add_macro(BaaPreprocessor *pp_state, const wchar_t *name, const wchar_t *body, bool is_function_like, size_t param_count, wchar_t **param_names);
static const BaaMacro *find_macro(const BaaPreprocessor *pp_state, const wchar_t *name);
static bool undefine_macro(BaaPreprocessor *pp_state, const wchar_t *name); // Added for #undef
static bool push_conditional(BaaPreprocessor *pp_state, bool is_active);    // Added for #ifdef
static bool pop_conditional(BaaPreprocessor *pp_state);                     // Added for #endif
static void update_skipping_state(BaaPreprocessor *pp_state);               // Added for conditional state
void free_conditional_stack(BaaPreprocessor *pp);                           // Added for cleanup

// --- Macro Expansion Stack Helpers ---
static bool push_macro_expansion(BaaPreprocessor *pp_state, const BaaMacro *macro);
static void pop_macro_expansion(BaaPreprocessor *pp_state);
static bool is_macro_expanding(const BaaPreprocessor *pp_state, const BaaMacro *macro);
static void free_macro_expansion_stack(BaaPreprocessor *pp_state);

// Forward declaration for argument parsing
static wchar_t** parse_macro_arguments(const wchar_t** invocation_ptr_ref, size_t expected_arg_count, size_t* actual_arg_count, wchar_t** error_message, const char* abs_path);

// Forward declaration for substitution
static bool substitute_macro_body(DynamicWcharBuffer* output_buffer, const BaaMacro* macro, wchar_t** arguments, size_t arg_count, wchar_t** error_message, const char* abs_path);


// --- Helper Functions ---

// Helper to format error messages (similar to make_error_token)
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
        wchar_t *error_msg = wcsdup(fallback);
        return error_msg;
    }

    size_t buffer_size = (size_t)needed + 1;
    wchar_t *buffer = malloc(buffer_size * sizeof(wchar_t));
    if (!buffer)
    {
        va_end(args);
        const wchar_t *fallback = L"فشل في تخصيص الذاكرة لرسالة خطأ المعالج المسبق.";
        wchar_t *error_msg = wcsdup(fallback);
        return error_msg; // Allocation failed
    }

    vswprintf(buffer, buffer_size, format, args);
    va_end(args);

    return buffer;
}

// Reads the content of a UTF-16LE encoded file.
// Returns a dynamically allocated wchar_t* string (caller must free).
// Returns NULL on error and sets error_message.
static wchar_t *read_file_content_utf16le(const char *file_path, wchar_t **error_message)
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
        *error_message = format_preprocessor_error(L"فشل في تحويل مسار الملف '%hs' إلى UTF-16.", file_path);
        return NULL;
    }
    wchar_t *w_file_path = malloc(required_wchars * sizeof(wchar_t));
    if (!w_file_path)
    {
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

    if (!file)
    {
        *error_message = format_preprocessor_error(L"فشل في فتح الملف '%hs'.", file_path);
        return NULL;
    }

    // Check for UTF-16LE BOM (0xFF, 0xFE)
    unsigned char bom[2];
    if (fread(bom, 1, 2, file) != 2)
    {
        *error_message = format_preprocessor_error(L"فشل في قراءة BOM من الملف '%hs'.", file_path);
        fclose(file);
        return NULL;
    }
    if (bom[0] != 0xFF || bom[1] != 0xFE)
    {
        // Check for UTF-16BE BOM (0xFE, 0xFF) - specific error?
        if (bom[0] == 0xFE && bom[1] == 0xFF)
        {
            *error_message = format_preprocessor_error(L"الملف '%hs' يستخدم ترميز UTF-16BE (Big Endian)، مطلوب UTF-16LE (Little Endian).", file_path);
        }
        else
        {
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

    if (file_size_bytes < 2)
    { // Only BOM present or error
        fclose(file);
        buffer = malloc(sizeof(wchar_t)); // Allocate space for null terminator
        if (!buffer)
        {
            *error_message = format_preprocessor_error(L"فشل في تخصيص الذاكرة لملف فارغ (بعد BOM) '%hs'.", file_path);
            return NULL;
        }
        buffer[0] = L'\0';
        return buffer; // Return empty, null-terminated string
    }

    long content_size_bytes = file_size_bytes - 2;
    if (content_size_bytes % sizeof(wchar_t) != 0)
    {
        *error_message = format_preprocessor_error(L"حجم محتوى الملف '%hs' (بعد BOM) ليس من مضاعفات حجم wchar_t.", file_path);
        fclose(file);
        return NULL;
    }

    size_t num_wchars = content_size_bytes / sizeof(wchar_t);

    // Allocate buffer (+1 for null terminator)
    buffer = malloc((num_wchars + 1) * sizeof(wchar_t));
    if (!buffer)
    {
        *error_message = format_preprocessor_error(L"فشل في تخصيص الذاكرة لمحتوى الملف '%hs'.", file_path);
        fclose(file);
        return NULL;
    }

    // Read content
    size_t bytes_read = fread(buffer, 1, content_size_bytes, file);
    if (bytes_read != (size_t)content_size_bytes)
    {
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
    char *path_copy = strdup(file_path);
    if (!path_copy)
        return NULL;

#ifdef _WIN32
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    _splitpath(path_copy, drive, dir, NULL, NULL);
    free(path_copy);
    char *dir_part = malloc((strlen(drive) + strlen(dir) + 1) * sizeof(char));
    if (!dir_part)
        return NULL;
    strcpy(dir_part, drive);
    strcat(dir_part, dir);
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

// --- Dynamic Buffer for Output ---

typedef struct
{
    wchar_t *buffer;
    size_t length;
    size_t capacity;
} DynamicWcharBuffer;

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
    wcscat(db->buffer, str_to_append);
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

    char *abs_path = get_absolute_path(file_path);
    if (!abs_path)
    {
        *error_message = format_preprocessor_error(L"فشل في الحصول على المسار المطلق للملف '%hs'.", file_path);
        return NULL;
    }

    // 1. Circular Include Check
    if (!push_file_stack(pp_state, abs_path))
    {
        *error_message = format_preprocessor_error(L"تم اكتشاف تضمين دائري: الملف '%hs' مضمن بالفعل.", abs_path);
        free(abs_path);
        return NULL;
    }

    // 2. Read File Content
    wchar_t *file_content = read_file_content_utf16le(abs_path, error_message);
    if (!file_content)
    {
        // error_message should be set by read_file_content_utf16le
        pop_file_stack(pp_state); // Pop before returning error
        free(abs_path);
        return NULL;
    }

    DynamicWcharBuffer output_buffer;
    if (!init_dynamic_buffer(&output_buffer, wcslen(file_content) + 1024))
    { // Initial capacity
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
            *error_message = format_preprocessor_error(L"فشل في تخصيص الذاكرة لسطر في الملف '%hs'.", abs_path);
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

            bool is_conditional_directive = false;

            // --- Process Conditional Directives FIRST, regardless of skipping state ---
            if (wcsncmp(current_line, ifdef_directive, ifdef_directive_len) == 0 &&
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
                    *error_message = format_preprocessor_error(L"تنسيق #إذا_عرف غير صالح في الملف '%hs': اسم الماكرو مفقود.", abs_path);
                    success = false;
                }
                else
                {
                    size_t name_len = name_end - name_start;
                    wchar_t *macro_name = wcsndup(name_start, name_len);
                    if (!macro_name)
                    {
                        *error_message = format_preprocessor_error(L"فشل في تخصيص ذاكرة لاسم الماكرو في #إذا_عرف في '%hs'.", abs_path);
                        success = false;
                    }
                    else
                    {
                        bool is_defined = (find_macro(pp_state, macro_name) != NULL);
                        if (!push_conditional(pp_state, is_defined))
                        {
                            *error_message = format_preprocessor_error(L"فشل في دفع الحالة الشرطية لـ #إذا_عرف في '%hs'.", abs_path);
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
                    *error_message = format_preprocessor_error(L"تنسيق #إذا_لم_يعرف غير صالح في الملف '%hs': اسم الماكرو مفقود.", abs_path);
                    success = false;
                }
                else
                {
                    size_t name_len = name_end - name_start;
                    wchar_t *macro_name = wcsndup(name_start, name_len);
                    if (!macro_name)
                    {
                        *error_message = format_preprocessor_error(L"فشل في تخصيص ذاكرة لاسم الماكرو في #إذا_لم_يعرف في '%hs'.", abs_path);
                        success = false;
                    }
                    else
                    {
                        bool is_defined = (find_macro(pp_state, macro_name) != NULL);
                        // Push the *negated* definition status
                        if (!push_conditional(pp_state, !is_defined))
                        {
                            *error_message = format_preprocessor_error(L"فشل في دفع الحالة الشرطية لـ #إذا_لم_يعرف في '%hs'.", abs_path);
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
                    *error_message = format_preprocessor_error(L"#نهاية_إذا بدون #إذا_عرف مطابق في الملف '%hs'.", abs_path);
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
                    *error_message = format_preprocessor_error(L"#إلا بدون #إذا_عرف مطابق في الملف '%hs'.", abs_path);
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
                    *error_message = format_preprocessor_error(L"#وإلا_إذا بدون #إذا_عرف مطابق في الملف '%hs'.", abs_path);
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
                            *error_message = format_preprocessor_error(L"تنسيق #وإلا_إذا غير صالح في الملف '%hs': اسم الماكرو مفقود.", abs_path);
                            success = false;
                        }
                        else
                        {
                            size_t name_len = name_end - name_start;
                            wchar_t *macro_name = wcsndup(name_start, name_len);
                            if (!macro_name)
                            {
                                *error_message = format_preprocessor_error(L"فشل في تخصيص ذاكرة لاسم الماكرو في #وإلا_إذا في '%hs'.", abs_path);
                                success = false;
                            }
                            else
                            {
                                // Evaluate the condition (simple defined(MACRO) for now)
                                bool condition_met = (find_macro(pp_state, macro_name) != NULL);
                                free(macro_name);

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
                        *error_message = format_preprocessor_error(L"تنسيق #تضمين غير صالح في الملف '%hs': يجب أن يتبع اسم الملف بـ \" أو <.", abs_path);
                        success = false;
                    }

                    if (success && path_end != NULL)
                    {
                        size_t include_path_len = path_end - path_start;
                        if (include_path_len == 0)
                        {
                            *error_message = format_preprocessor_error(L"تنسيق #تضمين غير صالح في الملف '%hs': مسار الملف فارغ.", abs_path);
                            success = false;
                        }
                        else
                        {
                            wchar_t *include_path_w = wcsndup(path_start, include_path_len);
                            if (!include_path_w)
                            {
                                *error_message = format_preprocessor_error(L"فشل في تخصيص ذاكرة لمسار التضمين في '%hs'.", abs_path);
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
                                        *error_message = format_preprocessor_error(L"فشل في تخصيص ذاكرة لمسار التضمين (MB) في '%hs'.", abs_path);
                                        success = false;
                                    }
                                }
                                else
                                {
                                    *error_message = format_preprocessor_error(L"فشل في تحويل مسار التضمين إلى UTF-8 في '%hs'.", abs_path);
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
                                            FILE *test_file = fopen(temp_path, "rb"); // Test existence
                                            if (test_file)
                                            {
                                                fclose(test_file);
                                                full_include_path = strdup(temp_path);
                                                found = true;
                                                break;
                                            }
                                        }
                                        if (!found)
                                        {
                                            *error_message = format_preprocessor_error(L"تعذر العثور على ملف التضمين '%hs' في مسارات التضمين.", include_path_mb);
                                            success = false;
                                        }
                                    }
                                    else
                                    {
                                        // Relative path: combine with current file's directory
                                        char *current_dir = get_directory_part(abs_path);
                                        if (!current_dir)
                                        {
                                            *error_message = format_preprocessor_error(L"فشل في الحصول على دليل الملف الحالي '%hs'.", abs_path);
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
                                                *error_message = format_preprocessor_error(L"فشل في إلحاق المحتوى المضمن من '%hs' بمخزن الإخراج المؤقت.", full_include_path);
                                                success = false;
                                            }
                                            free(included_content);
                                        }
                                    }
                                    else if (success)
                                    { // full_include_path was NULL or creation failed
                                        *error_message = format_preprocessor_error(L"فشل في بناء المسار الكامل لملف التضمين '%hs'.", include_path_mb);
                                        success = false;
                                    }
                                    free(full_include_path);
                                }
                                free(include_path_mb);
                                free(include_path_w);
                            }
                        }
                    }
                    else if (success)
                    { // path_end was NULL
                        *error_message = format_preprocessor_error(L"تنسيق #تضمين غير صالح في الملف '%hs': علامة الاقتباس أو القوس الختامي مفقود.", abs_path);
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
                        *error_message = format_preprocessor_error(L"تنسيق #تعريف غير صالح في الملف '%hs': اسم الماكرو مفقود.", abs_path);
                        success = false;
                    }
                    else
                    {
                        size_t name_len = name_end - name_start;
                        wchar_t *macro_name = wcsndup(name_start, name_len);
                        if (!macro_name) {
                             *error_message = format_preprocessor_error(L"فشل في تخصيص ذاكرة لاسم الماكرو في #تعريف في '%hs'.", abs_path);
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
                        if (*name_end == L'(') {
                            is_function_like = true;
                            wchar_t *param_ptr = name_end + 1; // Start parsing after '('

                            // Loop to parse parameters
                            while (success) {
                                // Skip whitespace before parameter or ')'
                                while (iswspace(*param_ptr)) param_ptr++;

                                if (*param_ptr == L')') { // End of parameter list
                                    param_ptr++; // Consume ')'
                                    break; // Exit parameter parsing loop
                                }

                                // If not ')', expect an identifier (parameter name)
                                if (!iswalpha(*param_ptr) && *param_ptr != L'_') {
                                    *error_message = format_preprocessor_error(L"تنسيق #تعريف غير صالح في '%hs': متوقع اسم معامل أو ')' بعد '('.", abs_path);
                                    success = false;
                                    break;
                                }

                                // Find parameter name start and end
                                wchar_t *param_name_start = param_ptr;
                                while (iswalnum(*param_ptr) || *param_ptr == L'_') {
                                    param_ptr++;
                                }
                                wchar_t *param_name_end = param_ptr;
                                size_t param_name_len = param_name_end - param_name_start;

                                if (param_name_len == 0) { // Should not happen if check above is correct, but safety
                                     *error_message = format_preprocessor_error(L"تنسيق #تعريف غير صالح في '%hs': اسم معامل فارغ.", abs_path);
                                     success = false;
                                     break;
                                }

                                // Duplicate and store parameter name
                                wchar_t *param_name = wcsndup(param_name_start, param_name_len);
                                if (!param_name) {
                                    *error_message = format_preprocessor_error(L"فشل في تخصيص ذاكرة لاسم المعامل في #تعريف في '%hs'.", abs_path);
                                    success = false;
                                    break;
                                }

                                // Resize params array if needed
                                if (param_count >= params_capacity) {
                                    size_t new_capacity = (params_capacity == 0) ? 4 : params_capacity * 2;
                                    wchar_t **new_params = realloc(params, new_capacity * sizeof(wchar_t*));
                                    if (!new_params) {
                                        free(param_name); // Free the name we just allocated
                                        *error_message = format_preprocessor_error(L"فشل في إعادة تخصيص الذاكرة لمعاملات الماكرو في #تعريف في '%hs'.", abs_path);
                                        success = false;
                                        break;
                                    }
                                    params = new_params;
                                    params_capacity = new_capacity;
                                }
                                params[param_count++] = param_name; // Store the duplicated name

                                // Skip whitespace after parameter name
                                while (iswspace(*param_ptr)) param_ptr++;

                                // Expect ',' or ')'
                                if (*param_ptr == L',') {
                                    param_ptr++; // Consume ',' and continue loop
                                } else if (*param_ptr == L')') {
                                    param_ptr++; // Consume ')' and break loop
                                    break;
                                } else {
                                    *error_message = format_preprocessor_error(L"تنسيق #تعريف غير صالح في '%hs': متوقع ',' أو ')' بعد اسم المعامل.", abs_path);
                                    success = false;
                                    break;
                                }
                            } // End while parsing parameters

                            if (success) {
                                body_start = param_ptr; // Body starts after the ')'
                            } else {
                                // Cleanup partially parsed params on error
                                if (params) {
                                    for (size_t i = 0; i < param_count; ++i) free(params[i]);
                                    free(params);
                                    params = NULL;
                                    param_count = 0;
                                }
                            }
                        } // End if function-like

                        // --- End Function-Like Macro Parsing ---

                        if (success) {
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
                                *error_message = format_preprocessor_error(L"فشل في إضافة تعريف الماكرو '%ls' في الملف '%hs'. قد يكون بسبب خطأ في الذاكرة.", macro_name, abs_path);
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
                        *error_message = format_preprocessor_error(L"تنسيق #الغاء_تعريف غير صالح في الملف '%hs': اسم الماكرو مفقود.", abs_path);
                        success = false;
                    }
                    else
                    {
                        size_t name_len = name_end - name_start;
                        wchar_t *macro_name = wcsndup(name_start, name_len);
                        if (!macro_name)
                        {
                            *error_message = format_preprocessor_error(L"فشل في تخصيص ذاكرة لاسم الماكرو في #الغاء_تعريف في '%hs'.", abs_path);
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
                        *error_message = format_preprocessor_error(L"فشل في إلحاق السطر بمخزن الإخراج المؤقت في '%hs'.", abs_path);
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
                *error_message = format_preprocessor_error(L"فشل في تخصيص الذاكرة لمخزن السطر المؤقت للاستبدال في '%hs'.", abs_path);
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
                            *error_message = format_preprocessor_error(L"فشل في تخصيص ذاكرة للمعرف للاستبدال في '%hs'.", abs_path);
                            success = false;
                            break;
                        }
                        const BaaMacro *macro = find_macro(pp_state, identifier);
                        if (macro)
                        {
                            // --- Recursion Check ---
                            if (is_macro_expanding(pp_state, macro)) {
                                *error_message = format_preprocessor_error(L"تم اكتشاف استدعاء ذاتي للماكرو '%ls' في '%hs'.", macro->name, abs_path);
                                success = false;
                                free(identifier); // Free identifier before breaking
                                break;
                            }

                            // --- Push macro onto expansion stack ---
                            if (!push_macro_expansion(pp_state, macro)) {
                                *error_message = format_preprocessor_error(L"فشل في دفع الماكرو '%ls' إلى مكدس التوسيع في '%hs'.", macro->name, abs_path);
                                success = false;
                                free(identifier);
                                break;
                            }

                            bool expansion_success = true; // Track success of this specific expansion

                            if (macro->is_function_like) {
                                // Potential function-like macro invocation
                                const wchar_t *invocation_ptr = line_ptr; // Point after the identifier
                                // Skip whitespace before potential '('
                                while (iswspace(*invocation_ptr)) invocation_ptr++;

                                if (*invocation_ptr == L'(') {
                                    // Found '(', this IS a function-like macro invocation
                                    invocation_ptr++; // Consume '('
                                    //const wchar_t* args_start_ptr = invocation_ptr; // Save start for advancing line_ptr

                                    size_t actual_arg_count = 0;
                                    wchar_t** arguments = parse_macro_arguments(&invocation_ptr, macro->param_count, &actual_arg_count, error_message, abs_path);

                                    if (!arguments) {
                                        // Error occurred during argument parsing
                                        success = false;
                                    } else {
                                        // Check argument count
                                        if (actual_arg_count != macro->param_count) {
                                            *error_message = format_preprocessor_error(L"عدد وسيطات غير صحيح للماكرو '%ls' في '%hs' (متوقع %zu، تم الحصول على %zu).",
                                                                                       macro->name, abs_path, macro->param_count, actual_arg_count);
                                            success = false;
                                        } else {
                                            // Perform substitution
                                            if (!substitute_macro_body(&substituted_line_buffer, macro, arguments, actual_arg_count, error_message, abs_path)) {
                                                success = false;
                                            }
                                        }

                                        // Free parsed arguments
                                        // Note: parse_macro_arguments currently returns placeholder, so this loop does nothing yet
                                        for (size_t i = 0; i < actual_arg_count; ++i) {
                                            free(arguments[i]);
                                        }
                                        free(arguments);
                                    }

                                    // Advance the main line pointer past the invocation
                                    line_ptr = invocation_ptr; // invocation_ptr was updated by parse_macro_arguments

                                } else {
                                    // Function-like macro name NOT followed by '(', treat as normal identifier
                                    if (!append_dynamic_buffer_n(&substituted_line_buffer, id_start, id_len))
                                        expansion_success = false;
                                }
                            } else {
                                // Simple object-like macro substitution
                                if (!substitute_macro_body(&substituted_line_buffer, macro, NULL, 0, error_message, abs_path)) {
                                     expansion_success = false;
                                }
                            }

                            // --- Pop macro from expansion stack ---
                            pop_macro_expansion(pp_state);

                            if (!expansion_success) {
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
                *error_message = format_preprocessor_error(L"فشل في إلحاق السطر بمخزن الإخراج المؤقت في '%hs'.", abs_path);
        }

        free(current_line); // Free the duplicated line

        if (line_end != NULL)
        {
            line_start = line_end + 1; // Move to the next line
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
        return NULL;
    }

    free(file_content); // Original content buffer no longer needed

    // 4. Clean up stack for this file
    pop_file_stack(pp_state);
    free(abs_path);

    // Return the final concatenated buffer (ownership transferred)
    return output_buffer.buffer;
}


// --- Function-Like Macro Helpers ---

// Helper function to convert an argument to a string literal, escaping necessary characters.
// Appends the result (including quotes) to output_buffer.
// Returns true on success, false on error.
static bool stringify_argument(DynamicWcharBuffer* output_buffer, const wchar_t* argument, wchar_t** error_message, const char* abs_path) {
    // Estimate needed capacity: quotes + original length + potential escapes
    size_t initial_capacity = wcslen(argument) + 10; // Add some buffer for escapes + quotes
    DynamicWcharBuffer temp_buffer;
    if (!init_dynamic_buffer(&temp_buffer, initial_capacity)) {
         *error_message = format_preprocessor_error(L"فشل في تخصيص ذاكرة مؤقتة لتسلسل الوسيطة في '%hs'.", abs_path);
         return false;
    }

    bool success = true;
    // Append opening quote
    if (!append_to_dynamic_buffer(&temp_buffer, L"\"")) {
        success = false;
    }

    // Iterate through argument, escaping characters as needed
    const wchar_t* ptr = argument;
    while (*ptr != L'\0' && success) {
        wchar_t char_to_append[3] = {0}; // Max 2 chars for escape + null terminator
        bool needs_escape = false;

        if (*ptr == L'\\' || *ptr == L'"') {
            needs_escape = true;
            char_to_append[0] = L'\\';
            char_to_append[1] = *ptr;
        } else {
            char_to_append[0] = *ptr;
        }

        if (!append_to_dynamic_buffer(&temp_buffer, char_to_append)) {
            success = false;
        }
        ptr++;
    }

    // Append closing quote
    if (success && !append_to_dynamic_buffer(&temp_buffer, L"\"")) {
        success = false;
    }

    // Append the final stringified result to the main output buffer
    if (success) {
        if (!append_to_dynamic_buffer(output_buffer, temp_buffer.buffer)) {
             *error_message = format_preprocessor_error(L"فشل في إلحاق الوسيطة المتسلسلة للمخرج في '%hs'.", abs_path);
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
static wchar_t** parse_macro_arguments(const wchar_t** invocation_ptr_ref, size_t expected_arg_count, size_t* actual_arg_count, wchar_t** error_message, const char* abs_path) {
    *actual_arg_count = 0;
    *error_message = NULL;
    const wchar_t* ptr = *invocation_ptr_ref;

    wchar_t** args = NULL;
    size_t args_capacity = 0;

    // Simple loop to find arguments separated by commas until ')'
    while (*ptr != L'\0') {
        // Skip leading whitespace for the argument
        while (iswspace(*ptr)) ptr++;

        if (*ptr == L')') { // End of arguments
            ptr++; // Consume ')'
            break;
        }

        // If not the first argument, expect a comma before it
        if (*actual_arg_count > 0) {
             if (*ptr == L',') {
                 ptr++; // Consume ','
                 while (iswspace(*ptr)) ptr++; // Skip space after comma
             } else {
                 *error_message = format_preprocessor_error(L"تنسيق استدعاء الماكرو غير صالح في '%hs': متوقع ',' أو ')' بين الوسيطات.", abs_path);
                 goto parse_error; // Use goto for cleanup
             }
        }

         // Find the start of the argument
        const wchar_t* arg_start = ptr;

        // Find the end of the argument (next ',' or ')' at the top level, respecting literals)
        int paren_level = 0;
        const wchar_t* arg_end = ptr;
        bool in_string = false;
        bool in_char = false;
        wchar_t prev_char = L'\0';

        while (*arg_end != L'\0') {
            if (in_string) {
                // Inside string literal
                if (*arg_end == L'"' && prev_char != L'\\') {
                    in_string = false; // End of string
                }
                // Ignore commas and parentheses inside strings
            } else if (in_char) {
                // Inside char literal
                 if (*arg_end == L'\'' && prev_char != L'\\') {
                    in_char = false; // End of char
                }
                // Ignore commas and parentheses inside chars
            } else {
                // Not inside a string or char literal
                if (*arg_end == L'(') {
                    paren_level++;
                } else if (*arg_end == L')') {
                    if (paren_level == 0) break; // End of the entire argument list
                    paren_level--;
                    if (paren_level < 0) { // Mismatched parentheses
                         *error_message = format_preprocessor_error(L"تنسيق استدعاء الماكرو غير صالح في '%hs': أقواس غير متطابقة في الوسيطات.", abs_path);
                         goto parse_error;
                    }
                } else if (*arg_end == L',' && paren_level == 0) {
                    break; // End of the current argument (only if not nested)
                } else if (*arg_end == L'"') {
                    in_string = true; // Start of string
                } else if (*arg_end == L'\'') {
                     in_char = true; // Start of char
                }
            }

            // Handle escaped characters (simple version: just track previous char)
            if (*arg_end == L'\\' && prev_char == L'\\') {
                 prev_char = L'\0'; // Treat double backslash as escaped, reset prev_char
            } else {
                 prev_char = *arg_end;
            }
            arg_end++;
        }
        // arg_end now points to the delimiter (',' or ')') or null terminator

        if (paren_level != 0) { // Mismatched parentheses at the end
             *error_message = format_preprocessor_error(L"تنسيق استدعاء الماكرو غير صالح في '%hs': أقواس غير متطابقة في نهاية الوسيطات.", abs_path);
             goto parse_error;
        }
        if (in_string || in_char) { // Unterminated literal at the end
             *error_message = format_preprocessor_error(L"تنسيق استدعاء الماكرو غير صالح في '%hs': علامة اقتباس غير منتهية في الوسيطات.", abs_path);
             goto parse_error;
        }

        // Advance the main pointer 'ptr' to where arg_end stopped
        ptr = arg_end;

        // Trim trailing whitespace from the argument
        while (arg_end > arg_start && iswspace(*(arg_end - 1))) {
            arg_end--;
        }
        size_t arg_len = arg_end - arg_start;

        // Duplicate the argument
        wchar_t* arg_str = wcsndup(arg_start, arg_len);
        if (!arg_str) {
            *error_message = format_preprocessor_error(L"فشل في تخصيص ذاكرة لوسيطة الماكرو في '%hs'.", abs_path);
            goto parse_error;
        }

        // Resize args array if needed
        if (*actual_arg_count >= args_capacity) {
            size_t new_capacity = (args_capacity == 0) ? 4 : args_capacity * 2;
            wchar_t** new_args = realloc(args, new_capacity * sizeof(wchar_t*));
            if (!new_args) {
                free(arg_str);
                *error_message = format_preprocessor_error(L"فشل في إعادة تخصيص الذاكرة لوسيطات الماكرو في '%hs'.", abs_path);
                goto parse_error;
            }
            args = new_args;
            args_capacity = new_capacity;
        }
        args[*actual_arg_count] = arg_str;
        (*actual_arg_count)++;

        // ptr should already be pointing at the delimiter (',' or ')') or end of string
        if (*ptr == L'\0') { // Reached end of string unexpectedly
             *error_message = format_preprocessor_error(L"تنسيق استدعاء الماكرو غير صالح في '%hs': قوس الإغلاق ')' مفقود.", abs_path);
             goto parse_error;
        }
        // If it was ',', the next loop iteration will consume it.
        // If it was ')', the next loop iteration will break.

    } // End while parsing arguments

    // Check if we exited loop because of ')'
    if (*(ptr-1) != L')') { // Check the character before the current ptr position
         *error_message = format_preprocessor_error(L"تنسيق استدعاء الماكرو غير صالح في '%hs': قوس الإغلاق ')' مفقود بعد الوسيطات.", abs_path);
         goto parse_error;
    }


    *invocation_ptr_ref = ptr; // Update the caller's pointer
    return args;

parse_error:
    // Cleanup allocated arguments on error
    if (args) {
        for (size_t i = 0; i < *actual_arg_count; ++i) {
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
// NOTE: This implementation handles basic parameter substitution, stringification (#),
//       and basic token pasting (##).
static bool substitute_macro_body(DynamicWcharBuffer* output_buffer, const BaaMacro* macro, wchar_t** arguments, size_t arg_count, wchar_t** error_message, const char* abs_path) {
    const wchar_t* body_ptr = macro->body;
    bool success = true;
    bool suppress_next_space = false; // Flag for token pasting

    while (*body_ptr != L'\0' && success) {
        // Check for token pasting operator '##'
        if (*body_ptr == L'#' && *(body_ptr + 1) == L'#') {
            // Trim trailing whitespace from the output buffer before pasting
            while (output_buffer->length > 0 && iswspace(output_buffer->buffer[output_buffer->length - 1])) {
                output_buffer->length--;
            }
            output_buffer->buffer[output_buffer->length] = L'\0'; // Re-null-terminate

            suppress_next_space = true; // Suppress leading space on the *next* token
            body_ptr += 2; // Skip '##'
            // Skip any whitespace immediately after ## (before the next token/param)
            while (iswspace(*body_ptr)) body_ptr++;
            continue; // Continue loop to process the token after ##
        }
        // Check for stringification operator '#' (only if not part of ##)
        else if (*body_ptr == L'#') {
            const wchar_t* operator_ptr = body_ptr; // Point to '#'
            body_ptr++; // Move past '#'

            // Skip whitespace (optional, non-standard but maybe lenient)
            // while (iswspace(*body_ptr)) body_ptr++;

            if (iswalpha(*body_ptr) || *body_ptr == L'_') {
                const wchar_t* id_start = body_ptr;
                while (iswalnum(*body_ptr) || *body_ptr == L'_') {
                    body_ptr++;
                }
                size_t id_len = body_ptr - id_start;
                wchar_t* identifier = wcsndup(id_start, id_len);
                if (!identifier) {
                    *error_message = format_preprocessor_error(L"فشل في تخصيص ذاكرة للمعرف بعد '#' في نص الماكرو '%ls' في '%hs'.", macro->name, abs_path);
                    return false;
                }

                bool param_found = false;
                for (size_t i = 0; i < macro->param_count; ++i) {
                    if (wcscmp(identifier, macro->param_names[i]) == 0) {
                    // Found #param, perform stringification
                    // Stringification result includes quotes, so spacing is less critical before it.
                    if (!stringify_argument(output_buffer, arguments[i], error_message, abs_path)) {
                        success = false; // Error message already set by helper
                        }
                        param_found = true;
                        break; // Exit param search loop
                    }
                }
                free(identifier);

                if (!success) break; // Exit main loop on error

                if (!param_found) {
                    // '#' was not followed by a valid parameter name. Treat '#' literally.
                    if (!append_dynamic_buffer_n(output_buffer, operator_ptr, 1)) {
                         *error_message = format_preprocessor_error(L"فشل في إلحاق '#' الحرفية من نص الماكرو '%ls' في '%hs'.", macro->name, abs_path);
                         success = false; break;
                    }
                    // Let the loop continue to process the identifier normally in the next iteration
                    body_ptr = id_start; // Reset body_ptr to the start of the identifier
                } else {
                    // If param_found, body_ptr is already advanced past the identifier, continue main loop
                    suppress_next_space = false; // Stringification produces a single token
                }

            } else {
                 // '#' not followed by identifier, treat '#' as literal
                 // TODO: Consider if space suppression is needed before literal '#'
                 if (!append_dynamic_buffer_n(output_buffer, operator_ptr, 1)) {
                      *error_message = format_preprocessor_error(L"فشل في إلحاق '#' الحرفية من نص الماكرو '%ls' في '%hs'.", macro->name, abs_path);
                      success = false; break;
                 }
                 suppress_next_space = false; // Reset flag after literal append
                 // body_ptr is already advanced past '#'
            }
        }
        // Check for potential parameter identifier (if not handled by # or ##)
        else if (iswalpha(*body_ptr) || *body_ptr == L'_') {
            const wchar_t* id_start = body_ptr;
            while (iswalnum(*body_ptr) || *body_ptr == L'_') {
                body_ptr++;
            }
            size_t id_len = body_ptr - id_start;
            wchar_t* identifier = wcsndup(id_start, id_len);
            if (!identifier) {
                *error_message = format_preprocessor_error(L"فشل في تخصيص ذاكرة للمعرف في نص الماكرو '%ls' في '%hs'.", macro->name, abs_path);
                return false;
            }

            // Check if this identifier matches any parameter name
            bool param_found = false;
            size_t param_index = (size_t)-1; // Store index if found
            for (size_t i = 0; i < macro->param_count; ++i) {
                if (wcscmp(identifier, macro->param_names[i]) == 0) {
                    param_found = true;
                    param_index = i;
                    break;
                }
            }

            if (success && param_found) {
                 // Check for token pasting ## after this parameter
                 const wchar_t* next_ptr = body_ptr; // Look ahead from end of identifier
                 bool pasting = false;
                 const wchar_t* next_id_start = NULL;
                 size_t next_param_index = (size_t)-1;

                 if (*next_ptr == L'#' && *(next_ptr + 1) == L'#') {
                     next_ptr += 2; // Skip ##
                     // Skip whitespace after ##
                     while (iswspace(*next_ptr)) next_ptr++;

                     // Check if the next token is another parameter identifier
                     if (iswalpha(*next_ptr) || *next_ptr == L'_') {
                         next_id_start = next_ptr;
                         while (iswalnum(*next_ptr) || *next_ptr == L'_') {
                             next_ptr++;
                         }
                         size_t next_id_len = next_ptr - next_id_start;
                         wchar_t* next_identifier = wcsndup(next_id_start, next_id_len);
                         if (next_identifier) {
                             for (size_t j = 0; j < macro->param_count; ++j) {
                                 if (wcscmp(next_identifier, macro->param_names[j]) == 0) {
                                     pasting = true;
                                     next_param_index = j;
                                     break;
                                 }
                             }
                             free(next_identifier);
                         }
                     }
                 }

                 if (pasting) {
                     // Perform param##param pasting
                     // Concatenate arguments[param_index] and arguments[next_param_index]
                     size_t arg1_len = wcslen(arguments[param_index]);
                     size_t arg2_len = wcslen(arguments[next_param_index]);
                     wchar_t* concatenated_arg = malloc((arg1_len + arg2_len + 1) * sizeof(wchar_t));
                     if (!concatenated_arg) {
                          *error_message = format_preprocessor_error(L"فشل في تخصيص ذاكرة لوسيطة اللصق ## في '%hs'.", abs_path);
                          success = false;
                     } else {
                         wcscpy(concatenated_arg, arguments[param_index]);
                         wcscat(concatenated_arg, arguments[next_param_index]);

                         // Append the concatenated result
                         if (!append_to_dynamic_buffer(output_buffer, concatenated_arg)) {
                             *error_message = format_preprocessor_error(L"فشل في إلحاق الوسيطة الملصقة ## في '%hs'.", abs_path);
                             success = false;
                         } else {
                             suppress_next_space = false; // Pasting forms one token
                         }
                         free(concatenated_arg);
                     }
                     // Advance body_ptr past the second identifier
                     body_ptr = next_ptr;
                 } else {
                    // Not pasting, substitute the first parameter normally
                    // TODO: Check suppress_next_space
                    if (!append_to_dynamic_buffer(output_buffer, arguments[param_index])) {
                        *error_message = format_preprocessor_error(L"فشل في إلحاق وسيطة الماكرو '%ls' في '%hs'.", macro->name, abs_path);
                        success = false;
                    } else {
                         suppress_next_space = false; // Reset flag after successful append
                    }
                    param_found = true;
                    break;
                }
            }

            if (success && !param_found) {
                // Not a parameter, append the original identifier from the body
                // TODO: Check suppress_next_space
                if (!append_dynamic_buffer_n(output_buffer, id_start, id_len)) {
                     *error_message = format_preprocessor_error(L"فشل في إلحاق المعرف من نص الماكرو '%ls' في '%hs'.", macro->name, abs_path);
                     success = false;
                } else {
                    suppress_next_space = false; // Reset flag
                }
            }
            free(identifier);

        } else {
            // Not an identifier, append the character directly
            // TODO: Check suppress_next_space
            if (!append_dynamic_buffer_n(output_buffer, body_ptr, 1)) {
                 *error_message = format_preprocessor_error(L"فشل في إلحاق حرف من نص الماكرو '%ls' في '%hs'.", macro->name, abs_path);
                 success = false;
            } else {
                 suppress_next_space = false; // Reset flag
            }
            body_ptr++;
        }
    } // End while processing body

    // TODO: Handle case where ## is the very last thing in the body? (Likely an error)

    return success;
}

// --- Macro Expansion Stack Implementation ---

static bool push_macro_expansion(BaaPreprocessor *pp_state, const BaaMacro *macro) {
    if (pp_state->expanding_macros_count >= pp_state->expanding_macros_capacity) {
        size_t new_capacity = (pp_state->expanding_macros_capacity == 0) ? 8 : pp_state->expanding_macros_capacity * 2;
        const BaaMacro **new_stack = realloc(pp_state->expanding_macros_stack, new_capacity * sizeof(BaaMacro*));
        if (!new_stack) return false; // Allocation failure
        pp_state->expanding_macros_stack = new_stack;
        pp_state->expanding_macros_capacity = new_capacity;
    }
    pp_state->expanding_macros_stack[pp_state->expanding_macros_count++] = macro;
    return true;
}

static void pop_macro_expansion(BaaPreprocessor *pp_state) {
    if (pp_state->expanding_macros_count > 0) {
        pp_state->expanding_macros_count--;
    }
}

static bool is_macro_expanding(const BaaPreprocessor *pp_state, const BaaMacro *macro) {
    for (size_t i = 0; i < pp_state->expanding_macros_count; ++i) {
        // Compare pointers - assumes macros are uniquely allocated
        if (pp_state->expanding_macros_stack[i] == macro) {
            return true;
        }
    }
    return false;
}

static void free_macro_expansion_stack(BaaPreprocessor *pp_state) {
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
            if (pp->macros[i].is_function_like && pp->macros[i].param_names) {
                for (size_t j = 0; j < pp->macros[i].param_count; ++j) {
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
    if (!pp_state || !name || !body) {
        // Free potentially allocated params if other args are invalid
        if (is_function_like && param_names) {
             for (size_t j = 0; j < param_count; ++j) { free(param_names[j]); }
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
            if (pp_state->macros[i].is_function_like && pp_state->macros[i].param_names) {
                 for (size_t j = 0; j < pp_state->macros[i].param_count; ++j) {
                     free(pp_state->macros[i].param_names[j]);
                 }
                 free(pp_state->macros[i].param_names);
            }

            pp_state->macros[i].body = wcsdup(body);
            pp_state->macros[i].is_function_like = is_function_like;
            pp_state->macros[i].param_count = param_count;
            pp_state->macros[i].param_names = param_names; // Takes ownership

            // Check if allocations succeeded
            if (!pp_state->macros[i].body) {
                // If body fails, try to clean up params if they were just assigned
                 if (is_function_like && param_names) {
                     for (size_t j = 0; j < param_count; ++j) { free(param_names[j]); }
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
    BaaMacro *new_entry = &pp_state->macros[pp_state->macro_count];
    new_entry->name = wcsdup(name);
    new_entry->body = wcsdup(body);
    new_entry->is_function_like = is_function_like;
    new_entry->param_count = param_count;
    new_entry->param_names = param_names; // Takes ownership of the passed array and its contents

    // Check allocations
    if (!new_entry->name || !new_entry->body) {
        // Clean up everything allocated for this new entry on failure
        free(new_entry->name); // Safe even if NULL
        free(new_entry->body); // Safe even if NULL
        if (is_function_like && param_names) {
            for (size_t j = 0; j < param_count; ++j) {
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
            if (pp_state->macros[i].is_function_like && pp_state->macros[i].param_names) {
                for (size_t j = 0; j < pp_state->macros[i].param_count; ++j) {
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
            *error_message = wcsdup(L"وسيطات غير صالحة تم تمريرها إلى المعالج المسبق.");
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
    // --- End Initialize ---

    // Start recursive processing
    wchar_t *final_output = process_file(&pp_state, main_file_path, error_message);

    // --- Cleanup ---
    free_file_stack(&pp_state);             // Free include stack
    free_macros(&pp_state);                 // Free macro definitions
    free_conditional_stack(&pp_state);      // Free conditional stack
    free_macro_expansion_stack(&pp_state);  // Free macro expansion stack

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
