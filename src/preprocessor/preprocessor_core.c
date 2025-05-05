#include "preprocessor_internal.h"

// --- Core Recursive Processing Function ---

// Processes a single file, handling includes recursively.
// Returns a dynamically allocated string with processed content (caller frees), or NULL on error.
wchar_t *process_file(BaaPreprocessor *pp_state, const char *file_path, wchar_t **error_message)
{
    *error_message = NULL;

    // Store previous context for restoration after include/error
    const char* prev_file_path = pp_state->current_file_path;
    size_t prev_line_number = pp_state->current_line_number;

    char *abs_path = get_absolute_path(file_path);
    if (!abs_path)
    {
        // Use the simpler error formatter here as full context might not be set
        *error_message = format_preprocessor_error(L"فشل في الحصول على المسار المطلق للملف '%hs'.", file_path);
        // Restore context before returning on error (though it might be the same)
        pp_state->current_file_path = prev_file_path;
        pp_state->current_line_number = prev_line_number;
        return NULL;
    }

    // Set current context for this file
    pp_state->current_file_path = abs_path; // Use the allocated absolute path
    pp_state->current_line_number = 1;      // Start at line 1
    pp_state->current_column_number = 1;    // Start at column 1

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
        // error_message should be set by read_file_content_utf16le
        pop_file_stack(pp_state); // Pop before returning error
        free(abs_path);
        pp_state->current_file_path = prev_file_path; // Restore before returning
        pp_state->current_line_number = prev_line_number;
        return NULL;
    }

    DynamicWcharBuffer output_buffer;
    // Estimate initial capacity: file length + some overhead
    if (!init_dynamic_buffer(&output_buffer, wcslen(file_content) + 1024))
    {
        *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص الذاكرة لمخزن الإخراج المؤقت.");
        free(file_content);
        pop_file_stack(pp_state);
        free(abs_path);
        pp_state->current_file_path = prev_file_path; // Restore before returning
        pp_state->current_line_number = prev_line_number;
        return NULL;
    }

    // 3. Process Lines and Directives
    wchar_t *line_start = file_content;
    wchar_t *current_char_ptr = line_start; // Pointer for column tracking
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

        // Create a temporary buffer for the current line (use internal wcsndup)
        wchar_t *current_line = wcsndup_internal(line_start, line_len);
        if (!current_line)
        {
            *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص الذاكرة لسطر.");
            success = false;
            break;
        }

        // Check for directives
        if (current_line[0] == L'#')
        {
            // Trim leading whitespace after '#' for easier comparison
            wchar_t* directive_start = current_line + 1;
            pp_state->current_column_number = 2; // Start after '#'
            while(iswspace(*directive_start)) {
                 directive_start++;
                 pp_state->current_column_number++;
            }

            // Define directive keywords and lengths
            const wchar_t *include_directive = L"تضمين";
            size_t include_directive_len = wcslen(include_directive);
            const wchar_t *define_directive = L"تعريف";
            size_t define_directive_len = wcslen(define_directive);
            const wchar_t *undef_directive = L"الغاء_تعريف";
            size_t undef_directive_len = wcslen(undef_directive);
            const wchar_t *ifdef_directive = L"إذا_عرف";
            size_t ifdef_directive_len = wcslen(ifdef_directive);
            const wchar_t *ifndef_directive = L"إذا_لم_يعرف";
            size_t ifndef_directive_len = wcslen(ifndef_directive);
            const wchar_t *else_directive = L"إلا";
            size_t else_directive_len = wcslen(else_directive);
            const wchar_t *elif_directive = L"وإلا_إذا";
            size_t elif_directive_len = wcslen(elif_directive);
            const wchar_t *endif_directive = L"نهاية_إذا";
            size_t endif_directive_len = wcslen(endif_directive);
            const wchar_t *if_directive = L"إذا";
            size_t if_directive_len = wcslen(if_directive);

            bool is_conditional_directive = false;

            // --- Process Conditional Directives FIRST, regardless of skipping state ---
            if (wcsncmp(directive_start, if_directive, if_directive_len) == 0 &&
                (directive_start[if_directive_len] == L'\0' || iswspace(directive_start[if_directive_len])))
            {
                is_conditional_directive = true;
                wchar_t *expr_start = directive_start + if_directive_len;
                while (iswspace(*expr_start)) expr_start++;

                if (*expr_start == L'\0') {
                    *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق #إذا غير صالح: التعبير مفقود.");
                    success = false;
                } else {
                    bool expr_value = false;
                    if (!evaluate_preprocessor_expression(pp_state, expr_start, &expr_value, error_message, abs_path)) {
                        if (!*error_message) *error_message = format_preprocessor_error_with_context(pp_state, L"خطأ في تقييم تعبير #إذا.");
                        success = false;
                    } else {
                        if (!push_conditional(pp_state, expr_value)) {
                            *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في دفع الحالة الشرطية لـ #إذا (نفاد الذاكرة؟).");
                            success = false;
                        }
                    }
                }
            }
            else if (wcsncmp(directive_start, ifdef_directive, ifdef_directive_len) == 0 &&
                     (directive_start[ifdef_directive_len] == L'\0' || iswspace(directive_start[ifdef_directive_len])))
            {
                is_conditional_directive = true;
                wchar_t *name_start = directive_start + ifdef_directive_len;
                while (iswspace(*name_start)) name_start++;
                wchar_t *name_end = name_start;
                while (*name_end != L'\0' && !iswspace(*name_end)) name_end++;

                if (name_start == name_end) {
                    *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق #إذا_عرف غير صالح: اسم الماكرو مفقود.");
                    success = false;
                } else {
                    size_t name_len = name_end - name_start;
                    wchar_t *macro_name = wcsndup_internal(name_start, name_len);
                    if (!macro_name) {
                        *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص ذاكرة لاسم الماكرو في #إذا_عرف.");
                        success = false;
                    } else {
                        bool is_defined = (find_macro(pp_state, macro_name) != NULL);
                        if (!push_conditional(pp_state, is_defined)) {
                            *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في دفع الحالة الشرطية لـ #إذا_عرف (نفاد الذاكرة؟).");
                            success = false;
                        }
                        free(macro_name);
                    }
                }
            }
            else if (wcsncmp(directive_start, ifndef_directive, ifndef_directive_len) == 0 &&
                     (directive_start[ifndef_directive_len] == L'\0' || iswspace(directive_start[ifndef_directive_len])))
            {
                 is_conditional_directive = true;
                wchar_t *name_start = directive_start + ifndef_directive_len;
                while (iswspace(*name_start)) name_start++;
                wchar_t *name_end = name_start;
                while (*name_end != L'\0' && !iswspace(*name_end)) name_end++;

                if (name_start == name_end) {
                    *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق #إذا_لم_يعرف غير صالح: اسم الماكرو مفقود.");
                    success = false;
                } else {
                    size_t name_len = name_end - name_start;
                    wchar_t *macro_name = wcsndup_internal(name_start, name_len);
                    if (!macro_name) {
                        *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص ذاكرة لاسم الماكرو في #إذا_لم_يعرف.");
                        success = false;
                    } else {
                        bool is_defined = (find_macro(pp_state, macro_name) != NULL);
                        if (!push_conditional(pp_state, !is_defined)) { // Note the negation
                            *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في دفع الحالة الشرطية لـ #إذا_لم_يعرف (نفاد الذاكرة؟).");
                            success = false;
                        }
                        free(macro_name);
                    }
                }
            }
            else if (wcsncmp(directive_start, endif_directive, endif_directive_len) == 0 &&
                     (directive_start[endif_directive_len] == L'\0' || iswspace(directive_start[endif_directive_len])))
            {
                is_conditional_directive = true;
                if (!pop_conditional(pp_state)) {
                    *error_message = format_preprocessor_error_with_context(pp_state, L"#نهاية_إذا بدون #إذا/#إذا_عرف/#إذا_لم_يعرف مطابق.");
                    success = false;
                }
            }
            else if (wcsncmp(directive_start, else_directive, else_directive_len) == 0 &&
                     (directive_start[else_directive_len] == L'\0' || iswspace(directive_start[else_directive_len])))
            {
                is_conditional_directive = true;
                if (pp_state->conditional_stack_count == 0) {
                    *error_message = format_preprocessor_error_with_context(pp_state, L"#إلا بدون #إذا/#إذا_عرف/#إذا_لم_يعرف مطابق.");
                    success = false;
                } else {
                    size_t top_index = pp_state->conditional_stack_count - 1;
                    // Check if a branch was already taken at this level
                    if (pp_state->conditional_branch_taken_stack[top_index]) {
                        // If a branch was already taken, this #else block is inactive
                        pp_state->conditional_stack[top_index] = false;
                    } else {
                        // No branch was taken yet, this #else branch becomes active
                        pp_state->conditional_stack[top_index] = true;
                        pp_state->conditional_branch_taken_stack[top_index] = true; // Mark that a branch (this #else) is now taken
                    }
                    update_skipping_state(pp_state); // Recalculate overall skipping state
                }
            }
            else if (wcsncmp(directive_start, elif_directive, elif_directive_len) == 0 &&
                     (directive_start[elif_directive_len] == L'\0' || iswspace(directive_start[elif_directive_len])))
            {
                is_conditional_directive = true;
                if (pp_state->conditional_stack_count == 0) {
                    *error_message = format_preprocessor_error_with_context(pp_state, L"#وإلا_إذا بدون #إذا/#إذا_عرف/#إذا_لم_يعرف مطابق.");
                    success = false;
                } else {
                    size_t top_index = pp_state->conditional_stack_count - 1;
                    // Check if a branch was already taken at this level
                    if (pp_state->conditional_branch_taken_stack[top_index]) {
                        // If a branch was already taken, this #elif is inactive
                        pp_state->conditional_stack[top_index] = false;
                    } else {
                        // No branch taken yet, evaluate this #elif condition
                        wchar_t *expr_start = directive_start + elif_directive_len;
                        while (iswspace(*expr_start)) expr_start++;

                        if (*expr_start == L'\0') {
                            *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق #وإلا_إذا غير صالح: التعبير مفقود.");
                            success = false;
                        } else {
                            bool condition_met = false;
                            if (!evaluate_preprocessor_expression(pp_state, expr_start, &condition_met, error_message, abs_path)) {
                                if (!*error_message) *error_message = format_preprocessor_error_with_context(pp_state, L"خطأ في تقييم تعبير #وإلا_إذا.");
                                success = false;
                            } else {
                                if (condition_met) {
                                    // Condition met and no prior branch taken: this branch is active
                                    pp_state->conditional_stack[top_index] = true;
                                    pp_state->conditional_branch_taken_stack[top_index] = true; // Mark branch taken
                                } else {
                                    // Condition not met: this branch is inactive
                                    pp_state->conditional_stack[top_index] = false;
                                    // Don't mark branch taken yet
                                }
                            }
                        }
                    }
                    update_skipping_state(pp_state); // Recalculate overall skipping state
                }
            }

            // --- Process other directives ONLY if not skipping AND not a conditional directive ---
            if (!is_conditional_directive && !pp_state->skipping_lines)
            {
                if (wcsncmp(directive_start, include_directive, include_directive_len) == 0 &&
                    (directive_start[include_directive_len] == L'\0' || iswspace(directive_start[include_directive_len])))
                {
                    // Found #تضمين directive
                    wchar_t *path_spec_start = directive_start + include_directive_len;
                    // Update column past the directive keyword itself
                    pp_state->current_column_number += include_directive_len;
                    while (iswspace(*path_spec_start)) {
                         path_spec_start++;
                         pp_state->current_column_number++;
                    }

                    wchar_t start_char = path_spec_start[0];
                    wchar_t end_char = 0;
                    bool use_include_paths = false;
                    wchar_t *path_start = NULL;
                    wchar_t *path_end = NULL;

                    if (start_char == L'"') {
                        end_char = L'"';
                        use_include_paths = false;
                        path_start = path_spec_start + 1;
                        path_end = wcschr(path_start, end_char);
                    } else if (start_char == L'<') {
                        end_char = L'>';
                        use_include_paths = true;
                        path_start = path_spec_start + 1;
                        path_end = wcschr(path_start, end_char);
                    } else {
                        *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق #تضمين غير صالح: يجب أن يتبع اسم الملف بـ \" أو <.");
                        success = false;
                    }

                    if (success && path_end != NULL) {
                        size_t include_path_len = path_end - path_start;
                        if (include_path_len == 0) {
                            *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق #تضمين غير صالح: مسار الملف فارغ.");
                            success = false;
                        } else {
                            wchar_t *include_path_w = wcsndup_internal(path_start, include_path_len);
                            if (!include_path_w) {
                                *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص ذاكرة لمسار التضمين.");
                                success = false;
                            } else {
                                // Convert wchar_t include path to char* for file operations
                                char *include_path_mb = NULL;
                                int required_bytes = WideCharToMultiByte(CP_UTF8, 0, include_path_w, -1, NULL, 0, NULL, NULL);
                                if (required_bytes > 0) {
                                    include_path_mb = malloc(required_bytes);
                                    if (include_path_mb) {
                                        WideCharToMultiByte(CP_UTF8, 0, include_path_w, -1, include_path_mb, required_bytes, NULL, NULL);
                                    } else {
                                        *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص ذاكرة لمسار التضمين (MB).");
                                        success = false;
                                    }
                                } else {
                                    *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تحويل مسار التضمين إلى UTF-8.");
                                    success = false;
                                }

                                if (success && include_path_mb) {
                                    char *full_include_path = NULL;
                                    bool found = false;
                                    if (use_include_paths) {
                                        // Search include paths
                                        for (size_t i = 0; i < pp_state->include_path_count; ++i) {
                                            char temp_path[MAX_PATH_LEN];
                                            snprintf(temp_path, MAX_PATH_LEN, "%s%c%s", pp_state->include_paths[i], PATH_SEPARATOR, include_path_mb);
                                            // Check if file exists (simple check)
                                            FILE *test_file = NULL;
                                            #ifdef _WIN32
                                                errno_t err = fopen_s(&test_file, temp_path, "rb");
                                                if (test_file && err == 0)
                                            #else
                                                test_file = fopen(temp_path, "rb");
                                                if (test_file)
                                            #endif
                                                {
                                                    fclose(test_file);
                                                    full_include_path = _strdup(temp_path);
                                                    found = true;
                                                    break;
                                                }
                                        }
                                        if (!found) {
                                            *error_message = format_preprocessor_error_with_context(pp_state, L"تعذر العثور على ملف التضمين '<%hs>' في مسارات التضمين.", include_path_mb);
                                            success = false;
                                        }
                                    } else {
                                        // Relative path: combine with current file's directory
                                        char *current_dir = get_directory_part(pp_state->current_file_path);
                                        if (!current_dir) {
                                            *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في الحصول على دليل الملف الحالي.");
                                            success = false;
                                        } else {
                                            char temp_path[MAX_PATH_LEN];
                                            snprintf(temp_path, MAX_PATH_LEN, "%s%c%s", current_dir, PATH_SEPARATOR, include_path_mb);
                                            full_include_path = _strdup(temp_path); // Assume relative path exists for now
                                            free(current_dir);
                                            if (!full_include_path) {
                                                 *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص ذاكرة للمسار النسبي المدمج.");
                                                 success = false;
                                            } else {
                                                found = true; // Mark as found for processing
                                            }
                                        }
                                    }

                                    // Recursive call if path was resolved
                                    if (success && found && full_include_path) {
                                        wchar_t *included_content = process_file(pp_state, full_include_path, error_message);
                                        if (!included_content) {
                                            // error_message should be set by recursive call
                                            success = false;
                                        } else {
                                            // Append result to output buffer
                                            if (!append_to_dynamic_buffer(&output_buffer, included_content)) {
                                                if (!*error_message) *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في إلحاق المحتوى المضمن من '%hs'.", full_include_path);
                                                success = false;
                                            }
                                            free(included_content);
                                        }
                                    } else if (success && !found) {
                                        // Error message already set if not found in include paths
                                        // If relative path construction failed, error is set above
                                    }
                                    free(full_include_path);
                                }
                                free(include_path_mb);
                                free(include_path_w);
                            }
                        }
                    } else if (success) { // path_end was NULL
                        *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق #تضمين غير صالح: علامة الاقتباس أو القوس الختامي مفقود.");
                        success = false;
                    }
                    // #تضمين processed, don't append original line
                }
                else if (wcsncmp(directive_start, define_directive, define_directive_len) == 0 &&
                         (directive_start[define_directive_len] == L'\0' || iswspace(directive_start[define_directive_len])))
                {
                    // Found #تعريف directive
                    wchar_t *name_start = directive_start + define_directive_len;
                    pp_state->current_column_number += define_directive_len; // Update past directive
                    while (iswspace(*name_start)) {
                        name_start++;
                        pp_state->current_column_number++;
                    }

                    wchar_t *name_end = name_start;
                    // Macro name cannot have whitespace, but check for '(' immediately after name
                    while (*name_end != L'\0' && !iswspace(*name_end) && *name_end != L'(') {
                        name_end++;
                    }

                    if (name_start == name_end) {
                        *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق #تعريف غير صالح: اسم الماكرو مفقود.");
                        success = false;
                    } else {
                        size_t name_len = name_end - name_start;
                        wchar_t *macro_name = wcsndup_internal(name_start, name_len);
                        if (!macro_name) {
                            *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص ذاكرة لاسم الماكرو في #تعريف.");
                            success = false;
                        } else {
                            bool is_function_like = false;
                            size_t param_count = 0;
                            wchar_t **params = NULL;
                            wchar_t *body_start = name_end; // Default body start

                            // Check for function-like macro: '(' immediately after name
                            if (*name_end == L'(') {
                                is_function_like = true;
                                wchar_t *param_ptr = name_end + 1; // Start parsing after '('
                                size_t params_capacity = 0;

                                // Loop to parse parameters
                                while (success) {
                                    while (iswspace(*param_ptr)) param_ptr++; // Skip whitespace

                                    if (*param_ptr == L')') { // End of parameter list
                                        param_ptr++;
                                        break;
                                    }

                                    // If not first param, expect comma
                                    if (param_count > 0) {
                                        if (*param_ptr == L',') {
                                            param_ptr++;
                                            while (iswspace(*param_ptr)) param_ptr++;
                                        } else {
                                             *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق #تعريف غير صالح: متوقع ',' أو ')' بين معاملات الماكرو الوظيفي.");
                                             success = false;
                                             break;
                                        }
                                    }

                                    // Expect identifier
                                    if (!iswalpha(*param_ptr) && *param_ptr != L'_') {
                                        *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق #تعريف غير صالح: متوقع اسم معامل أو ')' بعد '('.");
                                        success = false;
                                        break;
                                    }

                                    wchar_t *param_name_start = param_ptr;
                                    while (iswalnum(*param_ptr) || *param_ptr == L'_') param_ptr++;
                                    wchar_t *param_name_end = param_ptr;
                                    size_t param_name_len = param_name_end - param_name_start;

                                    if (param_name_len == 0) {
                                        *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق #تعريف غير صالح: اسم معامل فارغ.");
                                        success = false;
                                        break;
                                    }

                                    wchar_t *param_name = wcsndup_internal(param_name_start, param_name_len);
                                    if (!param_name) {
                                        *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص ذاكرة لاسم المعامل في #تعريف.");
                                        success = false;
                                        break;
                                    }

                                    // Resize params array if needed
                                    if (param_count >= params_capacity) {
                                        size_t new_capacity = (params_capacity == 0) ? 4 : params_capacity * 2;
                                        wchar_t **new_params = realloc(params, new_capacity * sizeof(wchar_t *));
                                        if (!new_params) {
                                            free(param_name);
                                            *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في إعادة تخصيص الذاكرة لمعاملات الماكرو في #تعريف.");
                                            success = false;
                                            break;
                                        }
                                        params = new_params;
                                        params_capacity = new_capacity;
                                    }
                                    params[param_count++] = param_name;
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

                            if (success) {
                                // Find actual start of the body (skip space)
                                while (iswspace(*body_start)) body_start++;
                                // Body is the rest of the line (trim trailing maybe?)

                                if (!add_macro(pp_state, macro_name, body_start, is_function_like, param_count, params)) {
                                    // add_macro frees params if it fails after taking ownership
                                    *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في إضافة تعريف الماكرو '%ls' (نفاد الذاكرة؟).", macro_name);
                                    success = false;
                                    // Ensure params are NULL if add_macro failed early and didn't take ownership
                                    if (params) { // If add_macro failed before taking ownership
                                        for (size_t i = 0; i < param_count; ++i) free(params[i]);
                                        free(params);
                                        params = NULL;
                                    }
                                }
                                // Ownership of params transferred to add_macro if successful
                            }
                            free(macro_name);
                        }
                    }
                    // #تعريف processed, don't append original line
                }
                else if (wcsncmp(directive_start, undef_directive, undef_directive_len) == 0 &&
                         (directive_start[undef_directive_len] == L'\0' || iswspace(directive_start[undef_directive_len])))
                {
                    // Found #الغاء_تعريف directive
                    wchar_t *name_start = directive_start + undef_directive_len;
                    pp_state->current_column_number += undef_directive_len; // Update past directive
                    while (iswspace(*name_start)) {
                        name_start++;
                        pp_state->current_column_number++;
                    }
                    wchar_t *name_end = name_start;
                    while (*name_end != L'\0' && !iswspace(*name_end)) name_end++;

                    if (name_start == name_end) {
                        *error_message = format_preprocessor_error_with_context(pp_state, L"تنسيق #الغاء_تعريف غير صالح: اسم الماكرو مفقود.");
                        success = false;
                    } else {
                        size_t name_len = name_end - name_start;
                        wchar_t *macro_name = wcsndup_internal(name_start, name_len);
                        if (!macro_name) {
                            *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص ذاكرة لاسم الماكرو في #الغاء_تعريف.");
                            success = false;
                        } else {
                            undefine_macro(pp_state, macro_name); // Handles 'not found' case
                            free(macro_name);
                        }
                    }
                    // #الغاء_تعريف processed, don't append original line
                }
                else
                {
                    // Unrecognized directive while NOT skipping - issue warning? Pass through?
                    // For now, let's treat unrecognized directives as errors.
                     *error_message = format_preprocessor_error_with_context(pp_state, L"توجيه معالج مسبق غير معروف يبدأ بـ '#'.");
                     success = false;
                    // Alternative: Pass through if needed
                    // if (!append_dynamic_buffer_n(&output_buffer, line_start, line_len)) success = false;
                    // if (success && !append_to_dynamic_buffer(&output_buffer, L"\n")) success = false;
                    // if (!success && !*error_message) *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في إلحاق السطر بمخزن الإخراج المؤقت.");
                }
            }
            // If it was a conditional directive OR we are skipping, we do nothing further with this line.

        } // End if directive ('#')
        else if (!pp_state->skipping_lines)
        {
            // Not a directive AND not skipping: process line for macro substitution
            DynamicWcharBuffer substituted_line_buffer;
            if (!init_dynamic_buffer(&substituted_line_buffer, line_len + 128)) { // Initial capacity estimate
                *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص الذاكرة لمخزن السطر المؤقت للاستبدال.");
                success = false;
            } else {
                const wchar_t *line_ptr = current_line;
                while (*line_ptr != L'\0' && success) {
                    // Find potential identifier start
                    if (iswalpha(*line_ptr) || *line_ptr == L'_') {
                        const wchar_t *id_start = line_ptr;
                        while (iswalnum(*line_ptr) || *line_ptr == L'_') {
                            line_ptr++;
                        }
                        size_t id_len = line_ptr - id_start;
                        // Column update happens *after* processing the identifier/macro
                        wchar_t *identifier = wcsndup_internal(id_start, id_len);
                        if (!identifier) {
                            *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تخصيص ذاكرة للمعرف للاستبدال.");
                            success = false;
                            break;
                        }
                        const BaaMacro *macro = find_macro(pp_state, identifier);
                        if (macro && !is_macro_expanding(pp_state, macro)) { // Check for macro and recursion
                            // --- Push macro onto expansion stack ---
                            if (!push_macro_expansion(pp_state, macro)) {
                                *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في دفع الماكرو '%ls' إلى مكدس التوسيع (نفاد الذاكرة؟).", macro->name);
                                success = false;
                                free(identifier);
                                break;
                            }

                            bool expansion_success = true; // Track success of this specific expansion

                            if (macro->is_function_like) {
                                const wchar_t *invocation_ptr = line_ptr; // Point after the identifier
                                while (iswspace(*invocation_ptr)) invocation_ptr++; // Skip whitespace

                                if (*invocation_ptr == L'(') { // Found '(', it's an invocation
                                    invocation_ptr++; // Consume '('
                                    if (pp_state) pp_state->current_column_number++; // Update column past '('
                                    size_t actual_arg_count = 0;
                                    // Pass pp_state to the argument parser
                                    wchar_t **arguments = parse_macro_arguments(pp_state, &invocation_ptr, macro->param_count, &actual_arg_count, error_message);

                                    if (!arguments) { // Error during argument parsing
                                        if (!*error_message) *error_message = format_preprocessor_error_with_context(pp_state, L"خطأ في تحليل وسيطات الماكرو '%ls'.", macro->name);
                                        expansion_success = false; // Use expansion_success flag
                                    } else {
                                        if (actual_arg_count != macro->param_count) {
                                            *error_message = format_preprocessor_error_with_context(pp_state, L"عدد وسيطات غير صحيح للماكرو '%ls' (متوقع %zu، تم الحصول على %zu).",
                                                                                       macro->name, macro->param_count, actual_arg_count);
                                            expansion_success = false;
                                        } else {
                                            // Perform substitution into a temporary buffer first
                                            DynamicWcharBuffer expansion_result_buffer;
                                            if (!init_dynamic_buffer(&expansion_result_buffer, 128)) {
                                                 *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تهيئة المخزن المؤقت لنتيجة توسيع الماكرو '%ls'.", macro->name);
                                                 expansion_success = false;
                                            } else {
                                                if (!substitute_macro_body(pp_state, &expansion_result_buffer, macro, arguments, actual_arg_count, error_message)) {
                                                    if (!*error_message) *error_message = format_preprocessor_error_with_context(pp_state, L"خطأ أثناء استبدال نص الماكرو '%ls'.", macro->name);
                                                    expansion_success = false;
                                                } else {
                                                    // TODO: Rescan the expansion_result_buffer for further macros
                                                    // This requires a more complex setup, potentially pushing the expanded
                                                    // content back onto an input stream or buffer.
                                                    // For now, append the direct result.
                                                    if (!append_to_dynamic_buffer(&substituted_line_buffer, expansion_result_buffer.buffer)) {
                                                        expansion_success = false;
                                                    }
                                                }
                                                free_dynamic_buffer(&expansion_result_buffer);
                                            }
                                        }
                                        // Free parsed arguments
                                        for (size_t i = 0; i < actual_arg_count; ++i) free(arguments[i]);
                                        free(arguments);
                                    }
                                    // Advance the main line pointer past the invocation
                                    // TODO: Column update needs to happen within parse_macro_arguments
                                    line_ptr = invocation_ptr;
                                } else {
                                    // Function-like macro name NOT followed by '(', treat as normal identifier
                                    if (!append_dynamic_buffer_n(&substituted_line_buffer, id_start, id_len))
                                        expansion_success = false;
                                    pp_state->current_column_number += id_len; // Update column past identifier
                                }
                            } else { // Object-like macro
                                // Perform substitution into a temporary buffer first
                                DynamicWcharBuffer expansion_result_buffer;
                                if (!init_dynamic_buffer(&expansion_result_buffer, 128)) {
                                     *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في تهيئة المخزن المؤقت لنتيجة توسيع الماكرو '%ls'.", macro->name);
                                     expansion_success = false;
                                } else {
                                    if (!substitute_macro_body(pp_state, &expansion_result_buffer, macro, NULL, 0, error_message)) {
                                        if (!*error_message) *error_message = format_preprocessor_error_with_context(pp_state, L"خطأ أثناء استبدال نص الماكرو '%ls'.", macro->name);
                                        expansion_success = false;
                                    } else {
                                         // TODO: Rescan the expansion_result_buffer for further macros
                                         if (!append_to_dynamic_buffer(&substituted_line_buffer, expansion_result_buffer.buffer)) {
                                             expansion_success = false;
                                         }
                                    }
                                    free_dynamic_buffer(&expansion_result_buffer);
                                }
                            }

                            // --- Pop macro from expansion stack ---
                            pop_macro_expansion(pp_state);

                            if (!expansion_success) {
                                success = false; // Propagate error
                            }
                        } else if (macro) { // Macro found, but is currently expanding (recursion)
                             *error_message = format_preprocessor_error_with_context(pp_state, L"تم اكتشاف استدعاء ذاتي للماكرو '%ls'.", macro->name);
                             success = false;
                        }
                        else { // Not a macro or recursive
                            // Append original identifier
                            if (!append_dynamic_buffer_n(&substituted_line_buffer, id_start, id_len))
                                success = false;
                            pp_state->current_column_number += id_len; // Update column past identifier
                        }
                        free(identifier);
                    } else {
                        // Append non-identifier character
                        if (!append_dynamic_buffer_n(&substituted_line_buffer, line_ptr, 1))
                            success = false;
                        line_ptr++;
                        pp_state->current_column_number++; // Update column
                    }
                } // End while processing line

                if (success) {
                    // Append the fully substituted line to the main output
                    if (!append_to_dynamic_buffer(&output_buffer, substituted_line_buffer.buffer))
                        success = false;
                }
                free_dynamic_buffer(&substituted_line_buffer); // Free the temp line buffer
            }
            // Append newline after processing the line (substituted or not)
            if (success && !append_to_dynamic_buffer(&output_buffer, L"\n"))
                success = false;
            if (!success && !*error_message)
                *error_message = format_preprocessor_error_with_context(pp_state, L"فشل في إلحاق السطر بمخزن الإخراج المؤقت.");
        } // End if not skipping

        free(current_line); // Free the duplicated line

        if (!success) break; // Exit loop on error

        if (line_end != NULL) {
            line_start = line_end + 1; // Move to the next line
            line_start = line_end + 1; // Move to the next line
            current_char_ptr = line_start; // Reset column pointer
            pp_state->current_line_number++; // Increment line number
            pp_state->current_column_number = 1; // Reset column number
        } else {
            break; // End of file content
        }
    } // End while processing lines

    // Error handling for append failures or other errors during loop
    if (!success) {
        free_dynamic_buffer(&output_buffer); // Free partially built output
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
