#include "preprocessor_internal.h"

// Processes a regular code line (not a directive) for macro substitution.
// Appends the processed line (with substitutions) to the output_buffer.
// Returns true on success, false on error.
// Sets error_message on error.
bool process_code_line_for_macros(BaaPreprocessor *pp_state, const wchar_t *current_line, size_t line_len, DynamicWcharBuffer *output_buffer, wchar_t **error_message)
{
    bool success = true;
    DynamicWcharBuffer substituted_line_buffer;
    if (!init_dynamic_buffer(&substituted_line_buffer, line_len + 128))
    {
        PpSourceLocation error_loc = get_current_original_location(pp_state);
        *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في تخصيص الذاكرة لمخزن السطر المؤقت للاستبدال.");
        return false;
    }

    const wchar_t *line_ptr = current_line;
    size_t original_line_number = pp_state->current_line_number; // Store line number for error reporting within this line

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
                // Adjust location for error within the line
                error_loc.line = original_line_number;
                error_loc.column = id_start_col;
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
                    if (*p == L'\\') { *ep++ = L'\\'; }
                    *ep++ = *p;
                }
                *ep = L'\0';

                swprintf(quoted_file_path, sizeof(quoted_file_path)/sizeof(wchar_t), L"\"%ls\"", escaped_path);
                if (!append_to_dynamic_buffer(&substituted_line_buffer, quoted_file_path)) { success = false; }
                free(identifier); // Free the parsed identifier
            } else if (wcscmp(identifier, L"__السطر__") == 0) {
                wchar_t line_str[20]; // Buffer for line number string
                // Use the original line number where the macro appeared
                swprintf(line_str, sizeof(line_str)/sizeof(wchar_t), L"\"%zu\"", original_line_number);
                if (!append_to_dynamic_buffer(&substituted_line_buffer, line_str)) { success = false; }
                free(identifier); // Free the parsed identifier
            } else {
                // Not a predefined dynamic macro, proceed with normal macro lookup
                const BaaMacro *macro = find_macro(pp_state, identifier);
                if (macro && !is_macro_expanding(pp_state, macro))
                {
                    // --- Push macro invocation location ---
                    PpSourceLocation invocation_loc = {.file_path = pp_state->current_file_path, .line = original_line_number, .column = id_start_col};
                    if (!push_location(pp_state, &invocation_loc)) {
                        *error_message = format_preprocessor_error_at_location(&invocation_loc, L"فشل في دفع موقع استدعاء الماكرو (نفاد الذاكرة؟).");
                        success = false; free(identifier); break;
                    }

                    // --- Push macro onto expansion stack ---
                    if (!push_macro_expansion(pp_state, macro)) {
                        *error_message = format_preprocessor_error_at_location(&invocation_loc, L"فشل في دفع الماكرو '%ls' إلى مكدس التوسيع (نفاد الذاكرة؟).", macro->name);
                        pop_location(pp_state); success = false; free(identifier); break;
                    }

                    bool expansion_success = true;
                    if (macro->is_function_like) {
                        const wchar_t *invocation_ptr = line_ptr;
                        size_t col_before_args = pp_state->current_column_number; // Track column before skipping space
                        while (iswspace(*invocation_ptr)) { invocation_ptr++; pp_state->current_column_number++; }

                        if (*invocation_ptr == L'(') {
                            invocation_ptr++; pp_state->current_column_number++; // Consume '('
                            size_t actual_arg_count = 0;
                            wchar_t **arguments = parse_macro_arguments(pp_state, &invocation_ptr, macro->param_count, &actual_arg_count, error_message);
                            if (!arguments) { expansion_success = false; } // Error set by parser
                            else {
                                if (actual_arg_count != macro->param_count) {
                                    PpSourceLocation current_loc = get_current_original_location(pp_state);
                                    *error_message = format_preprocessor_error_at_location(&current_loc, L"عدد وسيطات غير صحيح للماكرو '%ls' (متوقع %zu، تم الحصول على %zu).", macro->name, macro->param_count, actual_arg_count);
                                    expansion_success = false;
                                } else {
                                    DynamicWcharBuffer expansion_result_buffer;
                                    if (!init_dynamic_buffer(&expansion_result_buffer, 128)) {
                                        PpSourceLocation current_loc = get_current_original_location(pp_state);
                                        *error_message = format_preprocessor_error_at_location(&current_loc, L"فشل في تهيئة المخزن المؤقت لنتيجة توسيع الماكرو '%ls'.", macro->name);
                                        expansion_success = false;
                                    } else {
                                        if (!substitute_macro_body(pp_state, &expansion_result_buffer, macro, arguments, actual_arg_count, error_message)) {
                                            expansion_success = false; // Error set by substitute
                                        } else {
                                            // TODO: Rescan expansion_result_buffer
                                            if (!append_to_dynamic_buffer(&substituted_line_buffer, expansion_result_buffer.buffer)) {
                                                expansion_success = false;
                                            }
                                        }
                                        free_dynamic_buffer(&expansion_result_buffer);
                                    }
                                }
                                for (size_t i = 0; i < actual_arg_count; ++i) free(arguments[i]);
                                free(arguments);
                            }
                            line_ptr = invocation_ptr; // Update main pointer
                        } else {
                            // Not followed by '(', treat as normal identifier
                            pp_state->current_column_number = col_before_args; // Reset column if no args parsed
                            if (!append_dynamic_buffer_n(&substituted_line_buffer, id_start, id_len)) expansion_success = false;
                        }
                    } else { // Object-like macro
                        DynamicWcharBuffer expansion_result_buffer;
                        if (!init_dynamic_buffer(&expansion_result_buffer, 128)) {
                            PpSourceLocation current_loc = get_current_original_location(pp_state);
                            *error_message = format_preprocessor_error_at_location(&current_loc, L"فشل في تهيئة المخزن المؤقت لنتيجة توسيع الماكرو '%ls'.", macro->name);
                            expansion_success = false;
                        } else {
                            if (!substitute_macro_body(pp_state, &expansion_result_buffer, macro, NULL, 0, error_message)) {
                                expansion_success = false; // Error set by substitute
                            } else {
                                // TODO: Rescan expansion_result_buffer
                                if (!append_to_dynamic_buffer(&substituted_line_buffer, expansion_result_buffer.buffer)) {
                                    expansion_success = false;
                                }
                            }
                            free_dynamic_buffer(&expansion_result_buffer);
                        }
                    }

                    pop_macro_expansion(pp_state); // Pop macro from expansion stack
                    pop_location(pp_state);        // Pop location stack

                    if (!expansion_success) { success = false; }
                } else if (macro) { // Recursive expansion detected
                    PpSourceLocation current_loc = get_current_original_location(pp_state);
                    *error_message = format_preprocessor_error_at_location(&current_loc, L"تم اكتشاف استدعاء ذاتي للماكرو '%ls'.", macro->name);
                    success = false;
                } else { // Not a macro
                    if (!append_dynamic_buffer_n(&substituted_line_buffer, id_start, id_len)) success = false;
                }
                free(identifier); // Free the parsed identifier here, after all checks
            } // End of 'else' for predefined dynamic macros check
        }
        else
        {
            // Append non-identifier character
            if (!append_dynamic_buffer_n(&substituted_line_buffer, line_ptr, 1)) success = false;
            line_ptr++;
            pp_state->current_column_number++; // Update physical column
        }
    } // End while processing line chars

    if (success) {
        if (!append_to_dynamic_buffer(output_buffer, substituted_line_buffer.buffer)) {
            success = false;
            if (!*error_message) {
                 PpSourceLocation current_loc = get_current_original_location(pp_state);
                 *error_message = format_preprocessor_error_at_location(&current_loc, L"فشل في إلحاق السطر المستبدل بمخزن الإخراج المؤقت.");
            }
        }
    }
    free_dynamic_buffer(&substituted_line_buffer);

    return success;
}
