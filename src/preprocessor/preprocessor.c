#include "baa/preprocessor/preprocessor.h" // Public API
#include "preprocessor_internal.h"       // Internal definitions and function declarations
#include <time.h>                        // For __التاريخ__ and __الوقت__

// --- Public Preprocessor Function ---

wchar_t *baa_preprocess(const char *main_file_path, const char **include_paths, wchar_t **error_message)
{
    if (!main_file_path || !error_message)
    {
        if (error_message) {
            // Use the basic error formatter as full state isn't initialized
            // For this very early error, pp_state is not available.
            // We'll use a generic location.
            PpSourceLocation early_error_loc = {"(preprocessor_init)", 0, 0};
            *error_message = format_preprocessor_error_at_location(&early_error_loc, L"وسيطات غير صالحة تم تمريرها إلى المعالج المسبق (main_file_path أو error_message هو NULL).");
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
    // pp_state.current_column_number = 0;  // Will be set within process_file
    // pp_state.location_stack = NULL;      // Zero-initialized
    // pp_state.location_stack_count = 0;   // Zero-initialized
    // pp_state.location_stack_capacity = 0;// Zero-initialized

    // --- Push initial location (needed early for potential errors during macro init) ---
    PpSourceLocation initial_loc = {
        .file_path = main_file_path, // Use the original path provided
        .line = 0, // Line 0 for "compiler-defined" aspect before file processing
        .column = 0
    };
    // Note: actual file processing will push its own line 1 later.

    // --- Define __التاريخ__ and __الوقت__ macros ---
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    wchar_t date_str[100];
    wchar_t time_str[100];
    wchar_t quoted_date_str[104]; // For quotes and null terminator
    wchar_t quoted_time_str[104]; // For quotes and null terminator

    // Format date e.g., "May 06 2024" (English month names are common in __DATE__)
    // C standard format is "Mmm dd yyyy"
    // For Arabic, a direct equivalent might be complex due to month names.
    // Let's use a standard format that's easily parsable.
    // Using wcsftime for locale-independent month names is tricky.
    // Sticking to a fixed English month name format for __DATE__ as per C standard.
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    swprintf(date_str, sizeof(date_str)/sizeof(wchar_t), L"\"%hs %02d %04d\"", months[t->tm_mon], t->tm_mday, t->tm_year + 1900);

    // Format time "HH:MM:SS"
    swprintf(time_str, sizeof(time_str)/sizeof(wchar_t), L"\"%02d:%02d:%02d\"", t->tm_hour, t->tm_min, t->tm_sec);

    // Add macros to preprocessor state
    // Note: add_macro expects non-quoted body for string literals, it adds quotes if needed.
    // However, the C standard for __DATE__ and __TIME__ is that they expand *to* string literals.
    // So, the values themselves should be the quoted strings.
    if (!add_macro(&pp_state, L"__التاريخ__", date_str, false, 0, NULL)) {
        // Error handling if add_macro fails
        *error_message = format_preprocessor_error_at_location(&initial_loc, L"فشل في تعريف الماكرو المدمج __التاريخ__.");
        // Cleanup any partially initialized state if necessary before returning
        return NULL;
    }
    if (!add_macro(&pp_state, L"__الوقت__", time_str, false, 0, NULL)) {
        // Error handling if add_macro fails
        *error_message = format_preprocessor_error_at_location(&initial_loc, L"فشل في تعريف الماكرو المدمج __الوقت__.");
        // Cleanup...
        return NULL;
    }
    // --- End Define __التاريخ__ and __الوقت__ ---

    // --- End Initialize ---

    // --- Push actual starting location for file processing ---
    // The 'initial_loc' above was for pre-run definitions.
    // Now, set up the true starting point for the main file.
    PpSourceLocation file_start_loc = {
        .file_path = main_file_path,
        .line = 1,
        .column = 1
    };
    if (!push_location(&pp_state, &file_start_loc)) {
         *error_message = format_preprocessor_error_at_location(&file_start_loc, L"فشل في دفع الموقع الأولي للملف (نفاد الذاكرة؟).");
         // No need to free stacks yet as they are likely empty/null
         return NULL;
    }
    // --- End Push initial location ---


    // Start recursive processing by calling the core function
    wchar_t *final_output = process_file(&pp_state, main_file_path, error_message);

    // --- Cleanup ---
    // Free all dynamically allocated resources held by the state struct
    free_file_stack(&pp_state);
    free_macros(&pp_state);
    free_conditional_stack(&pp_state);
    free_macro_expansion_stack(&pp_state);
    free_location_stack(&pp_state); // Free the location stack
    // Note: pp_state.current_file_path is managed within process_file and its callers

    // Check for unterminated conditional block after processing is complete
    // This check needs to happen *after* cleanup of stacks but *before* returning potentially bad output
    if (pp_state.conditional_stack_count > 0 && final_output && !*error_message)
    {
        // If an error already occurred, keep that primary error message
        // Otherwise, report the unterminated block error.
        free(*error_message); // Free previous non-error message if any
        PpSourceLocation error_loc = get_current_original_location(&pp_state);
        *error_message = format_preprocessor_error_at_location(&error_loc, L"كتلة شرطية غير منتهية في نهاية المعالجة (مفقود #نهاية_إذا).");
        free(final_output); // Free the potentially partially generated output
        final_output = NULL;
    }

    // Return the fully processed string (or NULL if an error occurred)
    return final_output;
}
