#include "preprocessor_internal.h"

// --- Core Recursive Processing Function ---

// Processes a single file, handling includes recursively.
// Returns a dynamically allocated string with processed content (caller frees), or NULL on error.
wchar_t *process_file(BaaPreprocessor *pp_state, const char *file_path, wchar_t **error_message)
{
    *error_message = NULL;

    // Store previous context for restoration after include/error
    const char *prev_file_path = pp_state->current_file_path;
    size_t prev_line_number = pp_state->current_line_number;

    char *abs_path = get_absolute_path(file_path);
    if (!abs_path)
    {
        // Get the location of the #include directive that caused this error
        PpSourceLocation error_loc = get_current_original_location(pp_state);
        *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في الحصول على المسار المطلق لملف التضمين '%hs'.", file_path);
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
        // Use the location stack to report the error at the include site
        PpSourceLocation error_loc = get_current_original_location(pp_state);
        *error_message = format_preprocessor_error_at_location(&error_loc, L"تم اكتشاف تضمين دائري: الملف '%hs' مضمن بالفعل.", abs_path);
        free(abs_path);
        pp_state->current_file_path = prev_file_path; // Restore before returning
        pp_state->current_line_number = prev_line_number;
        return NULL;
    }

    // 2. Read File Content
    wchar_t *file_content = read_file_content_utf16le(pp_state, abs_path, error_message);
    if (!file_content)
    {
        // error_message should be set by read_file_content_utf16le using current physical context
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
        PpSourceLocation error_loc = get_current_original_location(pp_state); // Get location before error
        *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في تخصيص الذاكرة لمخزن الإخراج المؤقت.");
        free(file_content);
        pop_file_stack(pp_state);
        free(abs_path);
        pp_state->current_file_path = prev_file_path; // Restore before returning
        pp_state->current_line_number = prev_line_number;
        return NULL;
    }

    // 3. Process Lines and Directives
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

        // Create a temporary buffer for the current line (use internal wcsndup)
        wchar_t *current_line = wcsndup_internal(line_start, line_len);
        if (!current_line)
        {
            PpSourceLocation error_loc = get_current_original_location(pp_state);
            *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في تخصيص الذاكرة لسطر.");
            success = false;
            break;
        }

        // Reset physical column for the start of the line
        pp_state->current_column_number = 1;

        // Check for directives
        if (current_line[0] == L'#')
        {
            // Trim leading whitespace after '#' for easier comparison
            wchar_t *directive_start = current_line + 1;
            pp_state->current_column_number = 2; // Start after '#'
            while (iswspace(*directive_start))
            {
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
            PpSourceLocation directive_loc = get_current_original_location(pp_state); // Get location for potential errors

            // --- Process Conditional Directives FIRST, regardless of skipping state ---
            if (wcsncmp(directive_start, if_directive, if_directive_len) == 0 &&
                (directive_start[if_directive_len] == L'\0' || iswspace(directive_start[if_directive_len])))
            {
                is_conditional_directive = true;
                wchar_t *expr_start = directive_start + if_directive_len;
                while (iswspace(*expr_start))
                    expr_start++;

                if (*expr_start == L'\0')
                {
                    *error_message = format_preprocessor_error_at_location(&directive_loc, L"تنسيق #إذا غير صالح: التعبير مفقود.");
                    success = false;
                }
                else
                {
                    bool expr_value = false;
                    // Pass directive_loc for potential errors during evaluation
                    if (!evaluate_preprocessor_expression(pp_state, expr_start, &expr_value, error_message, abs_path))
                    {
                        if (!*error_message)
                            *error_message = format_preprocessor_error_at_location(&directive_loc, L"خطأ في تقييم تعبير #إذا.");
                        success = false;
                    }
                    else
                    {
                        if (!push_conditional(pp_state, expr_value))
                        {
                            *error_message = format_preprocessor_error_at_location(&directive_loc, L"فشل في دفع الحالة الشرطية لـ #إذا (نفاد الذاكرة؟).");
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
                while (iswspace(*name_start))
                    name_start++;
                wchar_t *name_end = name_start;
                while (*name_end != L'\0' && !iswspace(*name_end))
                    name_end++;

                if (name_start == name_end)
                {
                    *error_message = format_preprocessor_error_at_location(&directive_loc, L"تنسيق #إذا_عرف غير صالح: اسم الماكرو مفقود.");
                    success = false;
                }
                else
                {
                    size_t name_len = name_end - name_start;
                    wchar_t *macro_name = wcsndup_internal(name_start, name_len);
                    if (!macro_name)
                    {
                        *error_message = format_preprocessor_error_at_location(&directive_loc, L"فشل في تخصيص ذاكرة لاسم الماكرو في #إذا_عرف.");
                        success = false;
                    }
                    else
                    {
                        bool is_defined = (find_macro(pp_state, macro_name) != NULL);
                        if (!push_conditional(pp_state, is_defined))
                        {
                            *error_message = format_preprocessor_error_at_location(&directive_loc, L"فشل في دفع الحالة الشرطية لـ #إذا_عرف (نفاد الذاكرة؟).");
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
                while (iswspace(*name_start))
                    name_start++;
                wchar_t *name_end = name_start;
                while (*name_end != L'\0' && !iswspace(*name_end))
                    name_end++;

                if (name_start == name_end)
                {
                    *error_message = format_preprocessor_error_at_location(&directive_loc, L"تنسيق #إذا_لم_يعرف غير صالح: اسم الماكرو مفقود.");
                    success = false;
                }
                else
                {
                    size_t name_len = name_end - name_start;
                    wchar_t *macro_name = wcsndup_internal(name_start, name_len);
                    if (!macro_name)
                    {
                        *error_message = format_preprocessor_error_at_location(&directive_loc, L"فشل في تخصيص ذاكرة لاسم الماكرو في #إذا_لم_يعرف.");
                        success = false;
                    }
                    else
                    {
                        bool is_defined = (find_macro(pp_state, macro_name) != NULL);
                        if (!push_conditional(pp_state, !is_defined))
                        { // Note the negation
                            *error_message = format_preprocessor_error_at_location(&directive_loc, L"فشل في دفع الحالة الشرطية لـ #إذا_لم_يعرف (نفاد الذاكرة؟).");
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
                if (!pop_conditional(pp_state))
                {
                    *error_message = format_preprocessor_error_at_location(&directive_loc, L"#نهاية_إذا بدون #إذا/#إذا_عرف/#إذا_لم_يعرف مطابق.");
                    success = false;
                }
            }
            else if (wcsncmp(directive_start, else_directive, else_directive_len) == 0 &&
                     (directive_start[else_directive_len] == L'\0' || iswspace(directive_start[else_directive_len])))
            {
                is_conditional_directive = true;
                if (pp_state->conditional_stack_count == 0)
                {
                    *error_message = format_preprocessor_error_at_location(&directive_loc, L"#إلا بدون #إذا/#إذا_عرف/#إذا_لم_يعرف مطابق.");
                    success = false;
                }
                else
                {
                    size_t top_index = pp_state->conditional_stack_count - 1;
                    if (pp_state->conditional_branch_taken_stack[top_index])
                    {
                        pp_state->conditional_stack[top_index] = false;
                    }
                    else
                    {
                        pp_state->conditional_stack[top_index] = true;
                        pp_state->conditional_branch_taken_stack[top_index] = true;
                    }
                    update_skipping_state(pp_state);
                }
            }
            else if (wcsncmp(directive_start, elif_directive, elif_directive_len) == 0 &&
                     (directive_start[elif_directive_len] == L'\0' || iswspace(directive_start[elif_directive_len])))
            {
                is_conditional_directive = true;
                if (pp_state->conditional_stack_count == 0)
                {
                    *error_message = format_preprocessor_error_at_location(&directive_loc, L"#وإلا_إذا بدون #إذا/#إذا_عرف/#إذا_لم_يعرف مطابق.");
                    success = false;
                }
                else
                {
                    size_t top_index = pp_state->conditional_stack_count - 1;
                    if (pp_state->conditional_branch_taken_stack[top_index])
                    {
                        pp_state->conditional_stack[top_index] = false;
                    }
                    else
                    {
                        wchar_t *expr_start = directive_start + elif_directive_len;
                        while (iswspace(*expr_start))
                            expr_start++;

                        if (*expr_start == L'\0')
                        {
                            *error_message = format_preprocessor_error_at_location(&directive_loc, L"تنسيق #وإلا_إذا غير صالح: التعبير مفقود.");
                            success = false;
                        }
                        else
                        {
                            bool condition_met = false;
                            if (!evaluate_preprocessor_expression(pp_state, expr_start, &condition_met, error_message, abs_path))
                            {
                                if (!*error_message)
                                    *error_message = format_preprocessor_error_at_location(&directive_loc, L"خطأ في تقييم تعبير #وإلا_إذا.");
                                success = false;
                            }
                            else
                            {
                                if (condition_met)
                                {
                                    pp_state->conditional_stack[top_index] = true;
                                    pp_state->conditional_branch_taken_stack[top_index] = true;
                                }
                                else
                                {
                                    pp_state->conditional_stack[top_index] = false;
                                }
                            }
                        }
                    }
                    update_skipping_state(pp_state);
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
                    pp_state->current_column_number += include_directive_len;
                    while (iswspace(*path_spec_start))
                    {
                        path_spec_start++;
                        pp_state->current_column_number++;
                    }

                    wchar_t start_char = path_spec_start[0];
                    wchar_t end_char = 0;
                    bool use_include_paths = false;
                    wchar_t *path_start = NULL;
                    wchar_t *path_end = NULL;

                    if (start_char == L'"')
                    {
                        end_char = L'"';
                        use_include_paths = false;
                        path_start = path_spec_start + 1;
                        path_end = wcschr(path_start, end_char);
                    }
                    else if (start_char == L'<')
                    {
                        end_char = L'>';
                        use_include_paths = true;
                        path_start = path_spec_start + 1;
                        path_end = wcschr(path_start, end_char);
                    }
                    else
                    {
                        *error_message = format_preprocessor_error_at_location(&directive_loc, L"تنسيق #تضمين غير صالح: يجب أن يتبع اسم الملف بـ \" أو <.");
                        success = false;
                    }

                    if (success && path_end != NULL)
                    {
                        size_t include_path_len = path_end - path_start;
                        if (include_path_len == 0)
                        {
                            *error_message = format_preprocessor_error_at_location(&directive_loc, L"تنسيق #تضمين غير صالح: مسار الملف فارغ.");
                            success = false;
                        }
                        else
                        {
                            wchar_t *include_path_w = wcsndup_internal(path_start, include_path_len);
                            if (!include_path_w)
                            {
                                *error_message = format_preprocessor_error_at_location(&directive_loc, L"فشل في تخصيص ذاكرة لمسار التضمين.");
                                success = false;
                            }
                            else
                            {
                                char *include_path_mb = NULL;
                                int required_bytes = WideCharToMultiByte(CP_UTF8, 0, include_path_w, -1, NULL, 0, NULL, NULL);
                                if (required_bytes > 0)
                                {
                                    include_path_mb = malloc(required_bytes);
                                    if (include_path_mb)
                                        WideCharToMultiByte(CP_UTF8, 0, include_path_w, -1, include_path_mb, required_bytes, NULL, NULL);
                                    else
                                    {
                                        *error_message = format_preprocessor_error_at_location(&directive_loc, L"فشل في تخصيص ذاكرة لمسار التضمين (MB).");
                                        success = false;
                                    }
                                }
                                else
                                {
                                    *error_message = format_preprocessor_error_at_location(&directive_loc, L"فشل في تحويل مسار التضمين إلى UTF-8.");
                                    success = false;
                                }

                                if (success && include_path_mb)
                                {
                                    char *full_include_path = NULL;
                                    bool found = false;
                                    if (use_include_paths)
                                    {
                                        for (size_t i = 0; i < pp_state->include_path_count; ++i)
                                        {
                                            char temp_path[MAX_PATH_LEN];
                                            snprintf(temp_path, MAX_PATH_LEN, "%s%c%s", pp_state->include_paths[i], PATH_SEPARATOR, include_path_mb);
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
                                        if (!found)
                                        {
                                            *error_message = format_preprocessor_error_at_location(&directive_loc, L"تعذر العثور على ملف التضمين '<%hs>' في مسارات التضمين.", include_path_mb);
                                            success = false;
                                        }
                                    }
                                    else
                                    {
                                        char *current_dir = get_directory_part(pp_state->current_file_path);
                                        if (!current_dir)
                                        {
                                            *error_message = format_preprocessor_error_at_location(&directive_loc, L"فشل في الحصول على دليل الملف الحالي.");
                                            success = false;
                                        }
                                        else
                                        {
                                            char temp_path[MAX_PATH_LEN];
                                            snprintf(temp_path, MAX_PATH_LEN, "%s%c%s", current_dir, PATH_SEPARATOR, include_path_mb);
                                            full_include_path = _strdup(temp_path);
                                            free(current_dir);
                                            if (!full_include_path)
                                            {
                                                *error_message = format_preprocessor_error_at_location(&directive_loc, L"فشل في تخصيص ذاكرة للمسار النسبي المدمج.");
                                                success = false;
                                            }
                                            else
                                            {
                                                found = true;
                                            }
                                        }
                                    }

                                    if (success && found && full_include_path)
                                    {
                                        // --- Push location before include ---
                                        PpSourceLocation include_loc = {.file_path = pp_state->current_file_path, .line = pp_state->current_line_number, .column = 1};
                                        if (!push_location(pp_state, &include_loc))
                                        {
                                            *error_message = format_preprocessor_error_at_location(&include_loc, L"فشل في دفع موقع التضمين (نفاد الذاكرة؟).");
                                            success = false;
                                        }
                                        else
                                        {
                                            // --- Recursive call ---
                                            wchar_t *included_content = process_file(pp_state, full_include_path, error_message);
                                            // --- Pop location after include ---
                                            pop_location(pp_state);

                                            if (!included_content)
                                            {
                                                success = false;
                                            } // error_message set by recursive call
                                            else
                                            {
                                                if (!append_to_dynamic_buffer(&output_buffer, included_content))
                                                {
                                                    if (!*error_message)
                                                    {
                                                        PpSourceLocation current_loc = get_current_original_location(pp_state);
                                                        *error_message = format_preprocessor_error_at_location(&current_loc, L"فشل في إلحاق المحتوى المضمن من '%hs'.", full_include_path);
                                                    }
                                                    success = false;
                                                }
                                                free(included_content);
                                            }
                                        }
                                    }
                                    else if (success && !found)
                                    { /* Error already set */
                                    }
                                    free(full_include_path);
                                }
                                free(include_path_mb);
                                free(include_path_w);
                            }
                        }
                    }
                    else if (success)
                    {
                        *error_message = format_preprocessor_error_at_location(&directive_loc, L"تنسيق #تضمين غير صالح: علامة الاقتباس أو القوس الختامي مفقود.");
                        success = false;
                    }
                    // #تضمين processed, don't append original line
                }
                else if (wcsncmp(directive_start, define_directive, define_directive_len) == 0 &&
                         (directive_start[define_directive_len] == L'\0' || iswspace(directive_start[define_directive_len])))
                {
                    wchar_t *name_start = directive_start + define_directive_len;
                    pp_state->current_column_number += define_directive_len;
                    while (iswspace(*name_start))
                    {
                        name_start++;
                        pp_state->current_column_number++;
                    }
                    wchar_t *name_end = name_start;
                    while (*name_end != L'\0' && !iswspace(*name_end) && *name_end != L'(')
                        name_end++;

                    if (name_start == name_end)
                    {
                        *error_message = format_preprocessor_error_at_location(&directive_loc, L"تنسيق #تعريف غير صالح: اسم الماكرو مفقود.");
                        success = false;
                    }
                    else
                    {
                        size_t name_len = name_end - name_start;
                        wchar_t *macro_name = wcsndup_internal(name_start, name_len);
                        if (!macro_name)
                        {
                            *error_message = format_preprocessor_error_at_location(&directive_loc, L"فشل في تخصيص ذاكرة لاسم الماكرو في #تعريف.");
                            success = false;
                        }
                        else
                        {
                            bool is_function_like = false;
                            size_t param_count = 0;
                            wchar_t **params = NULL;
                            wchar_t *body_start = name_end;

                            if (*name_end == L'(')
                            {
                                is_function_like = true;
                                wchar_t *param_ptr = name_end + 1;
                                size_t params_capacity = 0;
                                while (success)
                                {
                                    while (iswspace(*param_ptr))
                                        param_ptr++;
                                    if (*param_ptr == L')')
                                    {
                                        param_ptr++;
                                        break;
                                    }
                                    if (param_count > 0)
                                    {
                                        if (*param_ptr == L',')
                                        {
                                            param_ptr++;
                                            while (iswspace(*param_ptr))
                                                param_ptr++;
                                        }
                                        else
                                        {
                                            *error_message = format_preprocessor_error_at_location(&directive_loc, L"تنسيق #تعريف غير صالح: متوقع ',' أو ')' بين معاملات الماكرو الوظيفي.");
                                            success = false;
                                            break;
                                        }
                                    }
                                    if (!iswalpha(*param_ptr) && *param_ptr != L'_')
                                    {
                                        *error_message = format_preprocessor_error_at_location(&directive_loc, L"تنسيق #تعريف غير صالح: متوقع اسم معامل أو ')' بعد '('.");
                                        success = false;
                                        break;
                                    }
                                    wchar_t *param_name_start = param_ptr;
                                    while (iswalnum(*param_ptr) || *param_ptr == L'_')
                                        param_ptr++;
                                    wchar_t *param_name_end = param_ptr;
                                    size_t param_name_len = param_name_end - param_name_start;
                                    if (param_name_len == 0)
                                    {
                                        *error_message = format_preprocessor_error_at_location(&directive_loc, L"تنسيق #تعريف غير صالح: اسم معامل فارغ.");
                                        success = false;
                                        break;
                                    }
                                    wchar_t *param_name = wcsndup_internal(param_name_start, param_name_len);
                                    if (!param_name)
                                    {
                                        *error_message = format_preprocessor_error_at_location(&directive_loc, L"فشل في تخصيص ذاكرة لاسم المعامل في #تعريف.");
                                        success = false;
                                        break;
                                    }
                                    if (param_count >= params_capacity)
                                    {
                                        size_t new_capacity = (params_capacity == 0) ? 4 : params_capacity * 2;
                                        wchar_t **new_params = realloc(params, new_capacity * sizeof(wchar_t *));
                                        if (!new_params)
                                        {
                                            free(param_name);
                                            *error_message = format_preprocessor_error_at_location(&directive_loc, L"فشل في إعادة تخصيص الذاكرة لمعاملات الماكرو في #تعريف.");
                                            success = false;
                                            break;
                                        }
                                        params = new_params;
                                        params_capacity = new_capacity;
                                    }
                                    params[param_count++] = param_name;
                                }
                                if (success)
                                {
                                    body_start = param_ptr;
                                }
                                else
                                {
                                    if (params)
                                    {
                                        for (size_t i = 0; i < param_count; ++i)
                                            free(params[i]);
                                        free(params);
                                        params = NULL;
                                        param_count = 0;
                                    }
                                }
                            }

                            if (success)
                            {
                                while (iswspace(*body_start))
                                    body_start++;
                                if (!add_macro(pp_state, macro_name, body_start, is_function_like, param_count, params))
                                {
                                    *error_message = format_preprocessor_error_at_location(&directive_loc, L"فشل في إضافة تعريف الماكرو '%ls' (نفاد الذاكرة؟).", macro_name);
                                    success = false;
                                    if (params)
                                    {
                                        for (size_t i = 0; i < param_count; ++i)
                                            free(params[i]);
                                        free(params);
                                        params = NULL;
                                    }
                                }
                            }
                            free(macro_name);
                        }
                    }
                    // #تعريف processed
                }
                else if (wcsncmp(directive_start, undef_directive, undef_directive_len) == 0 &&
                         (directive_start[undef_directive_len] == L'\0' || iswspace(directive_start[undef_directive_len])))
                {
                    wchar_t *name_start = directive_start + undef_directive_len;
                    pp_state->current_column_number += undef_directive_len;
                    while (iswspace(*name_start))
                    {
                        name_start++;
                        pp_state->current_column_number++;
                    }
                    wchar_t *name_end = name_start;
                    while (*name_end != L'\0' && !iswspace(*name_end))
                        name_end++;
                    if (name_start == name_end)
                    {
                        *error_message = format_preprocessor_error_at_location(&directive_loc, L"تنسيق #الغاء_تعريف غير صالح: اسم الماكرو مفقود.");
                        success = false;
                    }
                    else
                    {
                        size_t name_len = name_end - name_start;
                        wchar_t *macro_name = wcsndup_internal(name_start, name_len);
                        if (!macro_name)
                        {
                            *error_message = format_preprocessor_error_at_location(&directive_loc, L"فشل في تخصيص ذاكرة لاسم الماكرو في #الغاء_تعريف.");
                            success = false;
                        }
                        else
                        {
                            undefine_macro(pp_state, macro_name);
                            free(macro_name);
                        }
                    }
                    // #الغاء_تعريف processed
                }
                else
                {
                    *error_message = format_preprocessor_error_at_location(&directive_loc, L"توجيه معالج مسبق غير معروف يبدأ بـ '#'.");
                    success = false;
                }
            }
            // Conditional directives and skipped lines don't produce output.
        }
        else if (!pp_state->skipping_lines)
        {
            // Not a directive AND not skipping: process line for macro substitution
            DynamicWcharBuffer substituted_line_buffer;
            if (!init_dynamic_buffer(&substituted_line_buffer, line_len + 128))
            {
                PpSourceLocation error_loc = get_current_original_location(pp_state);
                *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في تخصيص الذاكرة لمخزن السطر المؤقت للاستبدال.");
                success = false;
            }
            else
            {
                const wchar_t *line_ptr = current_line;
                while (*line_ptr != L'\0' && success)
                {
                    size_t current_char_col = pp_state->current_column_number; // Capture column before processing char/token

                    if (iswalpha(*line_ptr) || *line_ptr == L'_')
                    {
                        const wchar_t *id_start = line_ptr;
                        size_t id_start_col = current_char_col; // Use captured column
                        while (iswalnum(*line_ptr) || *line_ptr == L'_')
                        {
                            line_ptr++;
                            pp_state->current_column_number++; // Update physical column as we scan
                        }
                        size_t id_len = line_ptr - id_start;
                        wchar_t *identifier = wcsndup_internal(id_start, id_len);
                        if (!identifier)
                        {
                            PpSourceLocation error_loc = get_current_original_location(pp_state);
                            *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في تخصيص ذاكرة للمعرف للاستبدال.");
                            success = false;
                            break;
                        }

                        // Check for predefined dynamic macros first
                        if (wcscmp(identifier, L"__الملف__") == 0) {
                            wchar_t quoted_file_path[MAX_PATH_LEN + 3]; // For quotes, path, and null
                            PpSourceLocation current_original_loc = get_current_original_location(pp_state);
                            const char* original_file_path = current_original_loc.file_path ? current_original_loc.file_path : "unknown_file";

                            // Convert char* path to wchar_t* for swprintf
                            wchar_t w_current_physical_path[MAX_PATH_LEN];
                            mbstowcs(w_current_physical_path, original_file_path, MAX_PATH_LEN);

                            // Escape backslashes in the path for the string literal
                            wchar_t escaped_path[MAX_PATH_LEN * 2]; // Worst case: all backslashes
                            wchar_t *ep = escaped_path;
                            for (const wchar_t *p = w_current_physical_path; *p; ++p) {
                                if (*p == L'\\') {
                                    *ep++ = L'\\';
                                }
                                *ep++ = *p;
                            }
                            *ep = L'\0';

                            swprintf(quoted_file_path, sizeof(quoted_file_path)/sizeof(wchar_t), L"\"%ls\"", escaped_path);
                            if (!append_to_dynamic_buffer(&substituted_line_buffer, quoted_file_path)) {
                                success = false;
                            }
                            free(identifier); // Free the parsed identifier
                        } else if (wcscmp(identifier, L"__السطر__") == 0) {
                            wchar_t line_str[20]; // Buffer for line number string
                            // Use the current physical line number being processed in the current file buffer
                            swprintf(line_str, sizeof(line_str)/sizeof(wchar_t), L"\"%zu\"", pp_state->current_line_number);
                            if (!append_to_dynamic_buffer(&substituted_line_buffer, line_str)) {
                                success = false;
                            }
                            free(identifier); // Free the parsed identifier
                        } else {
                            // Not a predefined dynamic macro, proceed with normal macro lookup
                            const BaaMacro *macro = find_macro(pp_state, identifier);
                            if (macro && !is_macro_expanding(pp_state, macro))
                            {
                                // --- Push macro invocation location ---
                            PpSourceLocation invocation_loc = {.file_path = pp_state->current_file_path, .line = pp_state->current_line_number, .column = id_start_col};
                            if (!push_location(pp_state, &invocation_loc))
                            {
                                *error_message = format_preprocessor_error_at_location(&invocation_loc, L"فشل في دفع موقع استدعاء الماكرو (نفاد الذاكرة؟).");
                                success = false;
                                free(identifier);
                                break;
                            }

                            // --- Push macro onto expansion stack ---
                            if (!push_macro_expansion(pp_state, macro))
                            {
                                *error_message = format_preprocessor_error_at_location(&invocation_loc, L"فشل في دفع الماكرو '%ls' إلى مكدس التوسيع (نفاد الذاكرة؟).", macro->name);
                                pop_location(pp_state); // Pop location if expansion push fails
                                success = false;
                                free(identifier);
                                break;
                            }

                            bool expansion_success = true;
                            if (macro->is_function_like)
                            {
                                const wchar_t *invocation_ptr = line_ptr;
                                size_t col_before_args = pp_state->current_column_number; // Track column before skipping space
                                while (iswspace(*invocation_ptr))
                                {
                                    invocation_ptr++;
                                    pp_state->current_column_number++;
                                }

                                if (*invocation_ptr == L'(')
                                {
                                    invocation_ptr++;
                                    pp_state->current_column_number++; // Consume '('
                                    size_t actual_arg_count = 0;
                                    wchar_t **arguments = parse_macro_arguments(pp_state, &invocation_ptr, macro->param_count, &actual_arg_count, error_message);
                                    if (!arguments)
                                    {
                                        expansion_success = false;
                                    } // Error set by parser
                                    else
                                    {
                                        if (actual_arg_count != macro->param_count)
                                        {
                                            PpSourceLocation current_loc = get_current_original_location(pp_state);
                                            *error_message = format_preprocessor_error_at_location(&current_loc, L"عدد وسيطات غير صحيح للماكرو '%ls' (متوقع %zu، تم الحصول على %zu).", macro->name, macro->param_count, actual_arg_count);
                                            expansion_success = false;
                                        }
                                        else
                                        {
                                            DynamicWcharBuffer expansion_result_buffer;
                                            if (!init_dynamic_buffer(&expansion_result_buffer, 128))
                                            {
                                                PpSourceLocation current_loc = get_current_original_location(pp_state);
                                                *error_message = format_preprocessor_error_at_location(&current_loc, L"فشل في تهيئة المخزن المؤقت لنتيجة توسيع الماكرو '%ls'.", macro->name);
                                                expansion_success = false;
                                            }
                                            else
                                            {
                                                if (!substitute_macro_body(pp_state, &expansion_result_buffer, macro, arguments, actual_arg_count, error_message))
                                                {
                                                    expansion_success = false;
                                                } // Error set by substitute
                                                else
                                                {
                                                    // TODO: Rescan expansion_result_buffer
                                                    if (!append_to_dynamic_buffer(&substituted_line_buffer, expansion_result_buffer.buffer))
                                                    {
                                                        expansion_success = false;
                                                    }
                                                }
                                                free_dynamic_buffer(&expansion_result_buffer);
                                            }
                                        }
                                        for (size_t i = 0; i < actual_arg_count; ++i)
                                            free(arguments[i]);
                                        free(arguments);
                                    }
                                    line_ptr = invocation_ptr; // Update main pointer
                                }
                                else
                                {
                                    // Not followed by '(', treat as normal identifier
                                    pp_state->current_column_number = col_before_args; // Reset column if no args parsed
                                    if (!append_dynamic_buffer_n(&substituted_line_buffer, id_start, id_len))
                                        expansion_success = false;
                                    // Physical column already updated by initial scan
                                }
                            }
                            else
                            { // Object-like macro
                                // This is part of the original 'if (macro && !is_macro_expanding(pp_state, macro))' block
                                DynamicWcharBuffer expansion_result_buffer;
                                if (!init_dynamic_buffer(&expansion_result_buffer, 128))
                                {
                                    PpSourceLocation current_loc = get_current_original_location(pp_state);
                                    *error_message = format_preprocessor_error_at_location(&current_loc, L"فشل في تهيئة المخزن المؤقت لنتيجة توسيع الماكرو '%ls'.", macro->name);
                                    expansion_success = false;
                                }
                                else
                                {
                                    if (!substitute_macro_body(pp_state, &expansion_result_buffer, macro, NULL, 0, error_message))
                                    {
                                        expansion_success = false;
                                    } // Error set by substitute
                                    else
                                    {
                                        // TODO: Rescan expansion_result_buffer
                                        if (!append_to_dynamic_buffer(&substituted_line_buffer, expansion_result_buffer.buffer))
                                        {
                                            expansion_success = false;
                                        }
                                    }
                                    free_dynamic_buffer(&expansion_result_buffer);
                                }
                            }

                            // --- Pop macro from expansion stack ---
                            pop_macro_expansion(pp_state);
                            // --- Pop location stack ---
                            pop_location(pp_state);

                            if (!expansion_success)
                            {
                                success = false;
                            }
                           // End of 'if (macro && !is_macro_expanding(pp_state, macro))'
                           } else if (macro) { // This 'else if' pairs with 'if (macro && !is_macro_expanding)'
                                // Recursive expansion
                                PpSourceLocation current_loc = get_current_original_location(pp_state);
                                *error_message = format_preprocessor_error_at_location(&current_loc, L"تم اكتشاف استدعاء ذاتي للماكرو '%ls'.", macro->name);
                                success = false;
                           } else { // This 'else' pairs with 'if (macro && !is_macro_expanding)'
                                // Not a macro (and not __الملف__ or __السطر__)
                                if (!append_dynamic_buffer_n(&substituted_line_buffer, id_start, id_len))
                                    success = false;
                                // Physical column already updated by initial scan
                           }
                           free(identifier); // Free the parsed identifier here, after all checks
                        } // End of 'else' for predefined dynamic macros check
                    }
                    else
                    {
                        // Append non-identifier character
                        if (!append_dynamic_buffer_n(&substituted_line_buffer, line_ptr, 1))
                            success = false;
                        line_ptr++;
                        pp_state->current_column_number++; // Update physical column
                    }
                } // End while processing line chars

                if (success)
                {
                    if (!append_to_dynamic_buffer(&output_buffer, substituted_line_buffer.buffer))
                        success = false;
                }
                free_dynamic_buffer(&substituted_line_buffer);
            }

            // Append newline after processing the line (substituted or not)
            if (success && !append_to_dynamic_buffer(&output_buffer, L"\n"))
                success = false;
            if (!success && !*error_message)
            {
                PpSourceLocation current_loc = get_current_original_location(pp_state);
                *error_message = format_preprocessor_error_at_location(&current_loc, L"فشل في إلحاق السطر بمخزن الإخراج المؤقت.");
            }
        } // End if not skipping

        free(current_line); // Free the duplicated line

        if (!success)
            break; // Exit loop on error

        if (line_end != NULL)
        {
            line_start = line_end + 1;       // Move to the next line
            pp_state->current_line_number++; // Increment line number
            // Column reset happens at the start of the next line processing
        }
        else
        {
            break; // End of file content
        }
    } // End while processing lines

    // Error handling for append failures or other errors during loop
    if (!success)
    {
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
