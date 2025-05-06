#include "preprocessor_internal.h"

// --- Macro Expansion Stack Implementation ---

bool push_macro_expansion(BaaPreprocessor *pp_state, const BaaMacro *macro)
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

void pop_macro_expansion(BaaPreprocessor *pp_state)
{
    if (pp_state->expanding_macros_count > 0)
    {
        pp_state->expanding_macros_count--;
    }
}

bool is_macro_expanding(const BaaPreprocessor *pp_state, const BaaMacro *macro)
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

void free_macro_expansion_stack(BaaPreprocessor *pp_state)
{
    free(pp_state->expanding_macros_stack);
    pp_state->expanding_macros_stack = NULL;
    pp_state->expanding_macros_count = 0;
    pp_state->expanding_macros_capacity = 0;
}


// --- Function-Like Macro Helpers ---

// Helper function to convert an argument to a string literal, escaping necessary characters.
// Appends the result (including quotes) to output_buffer.
// Returns true on success, false on error.
bool stringify_argument(BaaPreprocessor *pp_state, DynamicWcharBuffer *output_buffer, const wchar_t *argument, wchar_t **error_message)
{
    // Estimate needed capacity: quotes + original length + potential escapes
    size_t initial_capacity = wcslen(argument) + 10; // Add some buffer for escapes + quotes
    DynamicWcharBuffer temp_buffer;
    if (!init_dynamic_buffer(&temp_buffer, initial_capacity))
    {
        PpSourceLocation error_loc = get_current_original_location(pp_state);
        *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في تخصيص ذاكرة مؤقتة لتسلسل الوسيطة.");
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
        // bool needs_escape = false; // Not needed directly

        if (*ptr == L'\\' || *ptr == L'"')
        {
            // needs_escape = true;
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
            PpSourceLocation error_loc = get_current_original_location(pp_state);
            *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في إلحاق الوسيطة المتسلسلة للمخرج.");
            success = false;
        }
    }

    free_dynamic_buffer(&temp_buffer);
    return success;
}

// Parses macro arguments from an invocation string.
// Updates invocation_ptr_ref to point after the closing parenthesis.
// Updates pp_state->current_column_number as it consumes characters.
// Returns a dynamically allocated array of argument strings (caller must free each string and the array).
// Returns NULL on error and sets error_message.
wchar_t **parse_macro_arguments(BaaPreprocessor *pp_state, const wchar_t **invocation_ptr_ref, size_t expected_arg_count, size_t *actual_arg_count, wchar_t **error_message)
{
    *actual_arg_count = 0;
    *error_message = NULL;
    const wchar_t *ptr = *invocation_ptr_ref; // Local pointer for iteration

    wchar_t **args = NULL;
    size_t args_capacity = 0;

    // Simple loop to find arguments separated by commas until ')'
    while (*ptr != L'\0')
    {
        // Skip leading whitespace for the argument
        while (iswspace(*ptr)) {
            ptr++;
            if (pp_state) pp_state->current_column_number++; // Update column
        }

        if (*ptr == L')')
        {          // End of arguments
            ptr++; // Consume ')'
            if (pp_state) pp_state->current_column_number++; // Update column
            break;
        }

        // If not the first argument, expect a comma before it
        if (*actual_arg_count > 0)
        {
            if (*ptr == L',')
            {
                ptr++; // Consume ','
                if (pp_state) pp_state->current_column_number++; // Update column
                while (iswspace(*ptr)) {
                    ptr++; // Skip space after comma
                    if (pp_state) pp_state->current_column_number++; // Update column
                }
            }
            else
            {
                PpSourceLocation error_loc = get_current_original_location(pp_state);
                *error_message = format_preprocessor_error_at_location(&error_loc, L"تنسيق استدعاء الماكرو غير صالح: متوقع ',' أو ')' بين الوسيطات.");
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

        // Track column changes within this argument parsing loop
        size_t arg_start_col = pp_state ? pp_state->current_column_number : 0;
        size_t current_arg_col = arg_start_col;

        while (*arg_end != L'\0')
        {
            // Before processing char, update column tracking for the loop iteration
            current_arg_col++;

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
                        PpSourceLocation error_loc = get_current_original_location(pp_state);
                        *error_message = format_preprocessor_error_at_location(&error_loc, L"تنسيق استدعاء الماكرو غير صالح: أقواس غير متطابقة في الوسيطات.");
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
            // Column is already incremented for this char by current_arg_col++ above
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

        // Update the main column number by the number of chars consumed for this argument
        if (pp_state) pp_state->current_column_number = current_arg_col;

        if (paren_level != 0)
        { // Mismatched parentheses at the end
            PpSourceLocation error_loc = get_current_original_location(pp_state);
            *error_message = format_preprocessor_error_at_location(&error_loc, L"تنسيق استدعاء الماكرو غير صالح: أقواس غير متطابقة في نهاية الوسيطات.");
            goto parse_error;
        }
        if (in_string || in_char)
        { // Unterminated literal at the end
            PpSourceLocation error_loc = get_current_original_location(pp_state);
            *error_message = format_preprocessor_error_at_location(&error_loc, L"تنسيق استدعاء الماكرو غير صالح: علامة اقتباس غير منتهية في الوسيطات.");
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

        // Duplicate the argument using internal wcsndup
        wchar_t *arg_str = wcsndup_internal(arg_start, arg_len);
        if (!arg_str)
        {
            PpSourceLocation error_loc = get_current_original_location(pp_state);
            *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في تخصيص ذاكرة لوسيطة الماكرو.");
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
                PpSourceLocation error_loc = get_current_original_location(pp_state);
                *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في إعادة تخصيص الذاكرة لوسيطات الماكرو.");
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
            PpSourceLocation error_loc = get_current_original_location(pp_state);
            *error_message = format_preprocessor_error_at_location(&error_loc, L"تنسيق استدعاء الماكرو غير صالح: قوس الإغلاق ')' مفقود.");
            goto parse_error;
        }
        // If it was ',', the next loop iteration will consume it.
        // If it was ')', the next loop iteration will break.

    } // End while parsing arguments

    // Check if we exited loop because of ')'
    if (ptr == *invocation_ptr_ref || *(ptr - 1) != L')')
    { // Check the character before the current ptr position, handle empty list case
        PpSourceLocation error_loc = get_current_original_location(pp_state);
        *error_message = format_preprocessor_error_at_location(&error_loc, L"تنسيق استدعاء الماكرو غير صالح: قوس الإغلاق ')' مفقود بعد الوسيطات.");
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
bool substitute_macro_body(BaaPreprocessor *pp_state, DynamicWcharBuffer *output_buffer, const BaaMacro *macro, wchar_t **arguments, size_t arg_count, wchar_t **error_message)
{
    const wchar_t *body_ptr = macro->body;
    bool success = true;
    DynamicWcharBuffer pending_token_buffer; // Buffer for the token being built/substituted before pasting/appending
    bool pending_token_active = false;       // True if pending_token_buffer holds a token part

    if (!init_dynamic_buffer(&pending_token_buffer, 64))
    {
        PpSourceLocation error_loc = get_current_original_location(pp_state);
        *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في تهيئة المخزن المؤقت للرمز المميز المعلق.");
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
                PpSourceLocation error_loc = get_current_original_location(pp_state);
                *error_message = format_preprocessor_error_at_location(&error_loc, L"المعامل ## يظهر في موقع غير صالح في الماكرو '%ls'.", macro->name);
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
                // Allow numbers as RHS for pasting too
                if (!iswdigit(*body_ptr)) {
                    PpSourceLocation error_loc = get_current_original_location(pp_state);
                    *error_message = format_preprocessor_error_at_location(&error_loc, L"المعامل ## يجب أن يتبعه معرف أو رقم في الماكرو '%ls'.", macro->name);
                    success = false;
                    break;
                }
            }

            const wchar_t *rhs_start = body_ptr;
            // Parse identifier or number
            if (iswalpha(*body_ptr) || *body_ptr == L'_') {
                 while (iswalnum(*body_ptr) || *body_ptr == L'_') body_ptr++;
            } else if (iswdigit(*body_ptr)) {
                 while (iswdigit(*body_ptr)) body_ptr++;
            }
            size_t rhs_len = body_ptr - rhs_start;
            wchar_t *rhs_token = wcsndup_internal(rhs_start, rhs_len);
             if (!rhs_token)
            {
                PpSourceLocation error_loc = get_current_original_location(pp_state);
                *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في تخصيص ذاكرة لرمز RHS لـ ##.");
                success = false;
                break;
            }


            wchar_t *rhs_value = NULL;
            bool rhs_is_param = false;
            size_t rhs_param_index = (size_t)-1;
            // Check if RHS is parameter (only if it was an identifier)
            if (iswalpha(rhs_token[0]) || rhs_token[0] == L'_') {
                for (size_t i = 0; i < macro->param_count; ++i)
                {
                    if (wcscmp(rhs_token, macro->param_names[i]) == 0)
                    {
                        rhs_is_param = true;
                        rhs_param_index = i;
                        break;
                    }
                }
            }

            if (rhs_is_param)
            {
                // If the argument itself is empty, the pasting results in just the LHS
                if (arguments[rhs_param_index][0] == L'\0') {
                    rhs_value = _wcsdup(L""); // Empty string
                } else {
                    rhs_value = _wcsdup(arguments[rhs_param_index]);
                }
            }
            else
            {
                rhs_value = _wcsdup(rhs_token); // Use literal RHS token
            }
            free(rhs_token);
            if (!rhs_value)
            {
                PpSourceLocation error_loc = get_current_original_location(pp_state);
                *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في تخصيص ذاكرة لقيمة RHS لـ ##.");
                success = false;
                break;
            }

            // Concatenate pending_token_buffer.buffer (LHS) and rhs_value (RHS)
            // Special case: if LHS is empty (e.g. #define PASTE(a,b) a##b, PASTE(,foo)), just use RHS
            // Special case: if RHS is empty (e.g. #define PASTE(a,b) a##b, PASTE(foo,)), just use LHS
            if (pending_token_buffer.length == 0) { // LHS is empty
                 if (!append_to_dynamic_buffer(&pending_token_buffer, rhs_value)) {
                     success = false;
                 }
            } else if (rhs_value[0] == L'\0') { // RHS is empty
                 // LHS is already in pending_token_buffer, do nothing
            } else { // Both LHS and RHS have content
                if (!append_to_dynamic_buffer(&pending_token_buffer, rhs_value)) {
                    success = false;
                }
            }

            free(rhs_value);
            if (!success) {
                 PpSourceLocation error_loc = get_current_original_location(pp_state);
                 *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في إلحاق قيمة RHS لـ ##.");
                 break;
            }

            // The potentially modified token remains the pending token
            pending_token_active = true;

            continue; // Continue loop, body_ptr is past RHS
        }

        // --- If not ##, flush any existing pending token to main output ---
        if (pending_token_active)
        {
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
                wchar_t *identifier = wcsndup_internal(id_start, id_len);
                if (!identifier)
                {
                    PpSourceLocation error_loc = get_current_original_location(pp_state);
                    *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في تخصيص ذاكرة للمعرف بعد '#' في نص الماكرو '%ls'.", macro->name);
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
                            // Error message set by helper
                            if (!*error_message) {
                                PpSourceLocation error_loc = get_current_original_location(pp_state);
                                *error_message = format_preprocessor_error_at_location(&error_loc, L"خطأ في تسلسل الوسيطة '%ls' للماكرو '%ls'.", arguments[i], macro->name);
                            }
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
                        PpSourceLocation error_loc = get_current_original_location(pp_state);
                        *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في إلحاق '#' الحرفية من نص الماكرو '%ls'.", macro->name);
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
                    PpSourceLocation error_loc = get_current_original_location(pp_state);
                    *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في إلحاق '#' الحرفية من نص الماكرو '%ls'.", macro->name);
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
            wchar_t *identifier = wcsndup_internal(id_start, id_len);
            if (!identifier)
            {
                PpSourceLocation error_loc = get_current_original_location(pp_state);
                *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في تخصيص ذاكرة للمعرف في نص الماكرو '%ls'.", macro->name);
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
                // If argument is empty, pending buffer remains empty for this "token"
                if (arguments[param_index][0] != L'\0') {
                    if (!append_to_dynamic_buffer(&pending_token_buffer, arguments[param_index]))
                    {
                        success = false;
                    }
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
                PpSourceLocation error_loc = get_current_original_location(pp_state);
                *error_message = format_preprocessor_error_at_location(&error_loc, L"فشل في إلحاق الرمز المميز المعلق الأخير في الماكرو '%ls'.", macro->name);
            }
        }
    }

    free_dynamic_buffer(&pending_token_buffer); // Clean up the pending buffer

    return success;
}
