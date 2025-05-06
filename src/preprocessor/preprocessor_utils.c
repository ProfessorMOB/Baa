#include "preprocessor_internal.h"

// --- Location Stack ---

// Pushes a location onto the stack. Returns false on allocation failure.
bool push_location(BaaPreprocessor *pp, const PpSourceLocation *location)
{
    if (pp->location_stack_count >= pp->location_stack_capacity)
    {
        size_t new_capacity = (pp->location_stack_capacity == 0) ? 8 : pp->location_stack_capacity * 2;
        PpSourceLocation *new_stack = realloc(pp->location_stack, new_capacity * sizeof(PpSourceLocation));
        if (!new_stack)
            return false; // Allocation failure
        pp->location_stack = new_stack;
        pp->location_stack_capacity = new_capacity;
    }
    // Copy the location data onto the stack
    pp->location_stack[pp->location_stack_count++] = *location;
    return true;
}

// Pops a location from the stack. Does nothing if the stack is empty.
void pop_location(BaaPreprocessor *pp)
{
    if (pp->location_stack_count > 0)
    {
        pp->location_stack_count--;
        // No need to free anything here as the stack holds copies, not pointers
    }
}

// Gets the location from the top of the stack. Returns a default location if stack is empty.
PpSourceLocation get_current_original_location(const BaaPreprocessor *pp)
{
    if (pp->location_stack_count > 0)
    {
        return pp->location_stack[pp->location_stack_count - 1];
    }
    else
    {
        // Return a default/unknown location if stack is empty
        // Use the current physical location as a fallback if available
        return (PpSourceLocation){
            .file_path = pp->current_file_path ? pp->current_file_path : "(unknown)",
            .line = pp->current_line_number,
            .column = pp->current_column_number};
    }
}

// Frees the memory used by the location stack.
void free_location_stack(BaaPreprocessor *pp)
{
    free(pp->location_stack);
    pp->location_stack = NULL;
    pp->location_stack_count = 0;
    pp->location_stack_capacity = 0;
}

// --- New Error Formatter using Explicit Location ---
wchar_t *format_preprocessor_error_at_location(const PpSourceLocation *location, const wchar_t *format, ...)
{
    // Prepare the prefix using the provided location
    size_t prefix_base_len = wcslen(L":%zu:%zu: خطأ: "); // file:line:col: error:
    size_t path_len = location && location->file_path ? strlen(location->file_path) : strlen("(unknown file)");
    size_t prefix_buffer_size = path_len + prefix_base_len + 30; // Extra space for line/column numbers
    char *prefix_mb = malloc(prefix_buffer_size);
    if (!prefix_mb)
    {
        // Basic fallback if prefix allocation fails
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
            return _wcsdup(L"فشل في تنسيق رسالة الخطأ (وفشل تخصيص البادئة).");
        }
        size_t buffer_size_fallback = (size_t)needed_fallback + 1;
        wchar_t *buffer_fallback = malloc(buffer_size_fallback * sizeof(wchar_t));
        if (!buffer_fallback)
        {
            va_end(args_fallback);
            return _wcsdup(L"فشل في تنسيق رسالة الخطأ (وفشل تخصيص البادئة والمخزن المؤقت).");
        }
        vswprintf(buffer_fallback, buffer_size_fallback, format, args_fallback);
        va_end(args_fallback);
        return buffer_fallback;
    }

    // Format the prefix using the provided location
    snprintf(prefix_mb, prefix_buffer_size, "%hs:%zu:%zu: خطأ: ",
             location && location->file_path ? location->file_path : "(unknown file)",
             location ? location->line : 0,
             location ? location->column : 0);

    // Convert prefix to wchar_t (same logic as format_preprocessor_error_with_context)
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
    free(prefix_mb);

    if (!prefix_w)
    {
        // Fallback if prefix conversion fails
        va_list args_fallback;
        va_start(args_fallback, format);
        va_list args_copy_fallback;
        va_copy(args_copy_fallback, args_fallback);
        int needed_fallback = vswprintf(NULL, 0, format, args_copy_fallback);
        va_end(args_copy_fallback);
        if (needed_fallback < 0)
        {
            va_end(args_fallback);
            return _wcsdup(L"فشل في تنسيق رسالة الخطأ (وفشل تحويل البادئة).");
        }
        size_t buffer_size_fallback = (size_t)needed_fallback + 1;
        wchar_t *buffer_fallback = malloc(buffer_size_fallback * sizeof(wchar_t));
        if (!buffer_fallback)
        {
            va_end(args_fallback);
            return _wcsdup(L"فشل في تنسيق رسالة الخطأ (وفشل تحويل البادئة والمخزن المؤقت).");
        }
        vswprintf(buffer_fallback, buffer_size_fallback, format, args_fallback);
        va_end(args_fallback);
        return buffer_fallback;
    }

    // Format the user's message part (same logic as before)
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
        return _wcsdup(L"فشل في تنسيق جزء الرسالة من خطأ المعالج المسبق.");
    }

    // Combine prefix and message (same logic as before)
    size_t prefix_len = wcslen(prefix_w);
    size_t msg_len = (size_t)needed_msg;
    size_t total_len = prefix_len + msg_len;
    wchar_t *final_buffer = malloc((total_len + 1) * sizeof(wchar_t));

    if (!final_buffer)
    {
        va_end(args);
        free(prefix_w);
        return _wcsdup(L"فشل في تخصيص الذاكرة لرسالة خطأ المعالج المسبق الكاملة.");
    }

    wcscpy(final_buffer, prefix_w);
    free(prefix_w);

    vswprintf(final_buffer + prefix_len, msg_len + 1, format, args);
    va_end(args);

    return final_buffer;
}

// --- Dynamic Buffer for Output ---

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

// --- Compatibility ---

// Implementation of wcsndup for Windows compatibility (renamed)
wchar_t *wcsndup_internal(const wchar_t *s, size_t n)
{
    wchar_t *result = (wchar_t *)malloc((n + 1) * sizeof(wchar_t));
    if (result)
    {
#ifdef _WIN32
        wcsncpy_s(result, n + 1, s, n);
        result[n] = L'\0'; // Ensure null termination even with wcsncpy_s
#else
        wcsncpy(result, s, n);
        result[n] = L'\0';
#endif
    }
    return result;
}

// --- File I/O ---

// Reads the content of a UTF-16LE encoded file.
// Returns a dynamically allocated wchar_t* string (caller must free).
// Returns NULL on error and sets error_message.
wchar_t *read_file_content_utf16le(BaaPreprocessor *pp_state, const char *file_path, wchar_t **error_message)
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
        PpSourceLocation error_loc = get_current_original_location(pp_state);
        *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في تحويل مسار الملف '%hs' إلى UTF-16.", file_path);
        return NULL;
    }
    wchar_t *w_file_path = malloc(required_wchars * sizeof(wchar_t));
    if (!w_file_path)
    {
        PpSourceLocation error_loc = get_current_original_location(pp_state);
        *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في تخصيص الذاكرة لمسار الملف (UTF-16) '%hs'.", file_path);
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
        PpSourceLocation error_loc = get_current_original_location(pp_state);
        *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في فتح الملف '%hs'.", file_path);
        return NULL;
    }

    // Check for UTF-16LE BOM (0xFF, 0xFE)
    unsigned char bom[2];
    if (fread(bom, 1, 2, file) != 2)
    {
        PpSourceLocation error_loc = get_current_original_location(pp_state);
        *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في قراءة BOM من الملف '%hs'.", file_path);
        fclose(file);
        return NULL;
    }
    if (bom[0] != 0xFF || bom[1] != 0xFE)
    {
        // Check for UTF-16BE BOM (0xFE, 0xFF) - specific error?
        PpSourceLocation error_loc = get_current_original_location(pp_state);
        if (bom[0] == 0xFE && bom[1] == 0xFF)
        {
            *error_message = format_preprocessor_error_at_location(&error_loc, L"الملف '%hs' يستخدم ترميز UTF-16BE (Big Endian)، مطلوب UTF-16LE (Little Endian).", file_path);
        }
        else
        {
            *error_message = format_preprocessor_error_at_location(&error_loc, L"الملف '%hs' ليس UTF-16LE (BOM غير موجود أو غير صحيح).", file_path);
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
            PpSourceLocation error_loc = get_current_original_location(pp_state);
            *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في تخصيص الذاكرة لملف فارغ (بعد BOM) '%hs'.", file_path);
            return NULL;
        }
        buffer[0] = L'\0';
        return buffer; // Return empty, null-terminated string
    }

    long content_size_bytes = file_size_bytes - 2;
    if (content_size_bytes % sizeof(wchar_t) != 0)
    {
        PpSourceLocation error_loc = get_current_original_location(pp_state);
        *error_message = format_preprocessor_error_at_location(&error_loc, L"حجم محتوى الملف '%hs' (بعد BOM) ليس من مضاعفات حجم wchar_t.", file_path);
        fclose(file);
        return NULL;
    }

    size_t num_wchars = content_size_bytes / sizeof(wchar_t);

    // Allocate buffer (+1 for null terminator)
    buffer = malloc((num_wchars + 1) * sizeof(wchar_t));
    if (!buffer)
    {
        PpSourceLocation error_loc = get_current_original_location(pp_state);
        *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في تخصيص الذاكرة لمحتوى الملف '%hs'.", file_path);
        fclose(file);
        return NULL;
    }

    // Read content
    size_t bytes_read = fread(buffer, 1, content_size_bytes, file);
    if (bytes_read != (size_t)content_size_bytes)
    {
        PpSourceLocation error_loc = get_current_original_location(pp_state);
        *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في قراءة محتوى الملف بالكامل من '%hs'.", file_path);
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
        // realpath fails if the file doesn't exist, which might be okay
        // if we are just constructing a path. Let's try a simpler approach
        // if realpath fails, like getting CWD and joining.
        // However, for include resolution, the file *should* exist.
        // For now, stick to realpath's behavior.
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
    char *path_copy = _strdup(file_path); // Use _strdup here too
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
    size_t drive_len = strlen(drive);
    size_t dir_len = strlen(dir);
    char *dir_part = malloc((drive_len + dir_len + 1) * sizeof(char));
    if (!dir_part)
        return NULL;
    // Use strcpy_s and strcat_s for safety
    strcpy_s(dir_part, drive_len + 1, drive);
    strcat_s(dir_part, drive_len + dir_len + 1, dir);
    // Remove trailing separator if present and not just root
    size_t len = strlen(dir_part);
    if (len > 0 && (dir_part[len - 1] == '\\' || dir_part[len - 1] == '/'))
    {
        if (!(len == 1 || (len == 3 && dir_part[1] == ':'))) // Handle C:\ root or / root
        {
            dir_part[len - 1] = '\0';
        }
    }
    // Handle case where only drive is present (e.g., "C:") -> should return "C:." or similar?
    // Current logic returns "C:", which might be okay. _splitpath_s behavior is key.
    // If dir is empty, maybe return "."? Let's stick to _splitpath result for now.
    return dir_part;
#else
    char *dir_name_result = dirname(path_copy);
    char *dir_part = _strdup(dir_name_result); // Use _strdup here too
    free(path_copy);
    return dir_part;
#endif
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

    char *path_copy = _strdup(abs_path); // Use _strdup here
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
