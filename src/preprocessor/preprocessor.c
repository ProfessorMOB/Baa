#include "baa/preprocessor/preprocessor.h" // Public API
#include "preprocessor_internal.h"       // Internal definitions and function declarations

// --- Public Preprocessor Function ---

wchar_t *baa_preprocess(const char *main_file_path, const char **include_paths, wchar_t **error_message)
{
    if (!main_file_path || !error_message)
    {
        if (error_message) {
            // Use the basic error formatter as full state isn't initialized
            *error_message = format_preprocessor_error(L"وسيطات غير صالحة تم تمريرها إلى المعالج المسبق (main_file_path أو error_message هو NULL).");
        }
        return NULL;
    }
    *error_message = NULL; // Initialize error message to NULL

    // --- Initialize Preprocessor State ---
    BaaPreprocessor pp_state = {0}; // Zero-initialize the structure

    // Set include paths
    pp_state.include_paths = include_paths;
    pp_state.include_path_count = 0;
    if (include_paths)
    {
        while (include_paths[pp_state.include_path_count] != NULL)
        {
            pp_state.include_path_count++;
        }
    }

    // Other fields are initialized to 0/NULL by the zero-initialization:
    // pp_state.open_files_stack = NULL;
    // pp_state.open_files_count = 0;
    // pp_state.open_files_capacity = 0;
    // pp_state.macros = NULL;
    // pp_state.macro_count = 0;
    // pp_state.macro_capacity = 0;
    // pp_state.conditional_stack = NULL;
    // pp_state.conditional_stack_count = 0;
    // pp_state.conditional_stack_capacity = 0;
    // pp_state.conditional_branch_taken_stack = NULL;
    // pp_state.conditional_branch_taken_stack_count = 0;
    // pp_state.conditional_branch_taken_stack_capacity = 0;
    // pp_state.skipping_lines = false;
    // pp_state.expanding_macros_stack = NULL;
    // pp_state.expanding_macros_count = 0;
    // pp_state.expanding_macros_capacity = 0;
    // pp_state.current_file_path = NULL; // Will be set within process_file
    // pp_state.current_line_number = 0;    // Will be set within process_file
    // --- End Initialize ---

    // Start recursive processing by calling the core function
    wchar_t *final_output = process_file(&pp_state, main_file_path, error_message);

    // --- Cleanup ---
    // Free all dynamically allocated resources held by the state struct
    free_file_stack(&pp_state);
    free_macros(&pp_state);
    free_conditional_stack(&pp_state);
    free_macro_expansion_stack(&pp_state);
    // Note: pp_state.current_file_path is managed within process_file and its callers

    // Check for unterminated conditional block after processing is complete
    // This check needs to happen *after* cleanup of stacks but *before* returning potentially bad output
    if (pp_state.conditional_stack_count > 0 && final_output && !*error_message)
    {
        // If an error already occurred, keep that primary error message
        // Otherwise, report the unterminated block error.
        free(*error_message); // Free previous non-error message if any
        // Use context-aware formatter, passing the state
        *error_message = format_preprocessor_error_with_context(&pp_state, L"كتلة شرطية غير منتهية في نهاية المعالجة (مفقود #نهاية_إذا).");
        free(final_output); // Free the potentially partially generated output
        final_output = NULL;
    }

    // Return the fully processed string (or NULL if an error occurred)
    return final_output;
}
