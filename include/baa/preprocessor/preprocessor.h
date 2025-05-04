#ifndef BAA_PREPROCESSOR_H
#define BAA_PREPROCESSOR_H

#include <wchar.h>
#include <stdbool.h>
#include <stddef.h> // For size_t

// Structure to hold a macro definition
typedef struct {
    wchar_t* name;          // Macro name
    wchar_t* body;          // Macro body (replacement text)
    bool is_function_like;  // True if defined with (...)
    size_t param_count;     // Number of parameters
    wchar_t** param_names;  // Array of parameter names (NULL if not function-like)
} BaaMacro;

// Forward declaration for the preprocessor state structure (Opaque Pointer)
// The actual definition is internal to the preprocessor library.
typedef struct BaaPreprocessor BaaPreprocessor;

/**
 * @brief Processes a Baa source file, handling preprocessor directives.
 *
 * This function reads the specified main source file, processes directives
 * like #تضمين (include), and returns a dynamically allocated wide character
 * string containing the fully processed source code.
 *
 * The caller is responsible for freeing the returned string using free().
 *
 * @param main_file_path Path to the main Baa source file.
 * @param include_paths Null-terminated array of strings representing standard include directories.
 * @param error_message Pointer to a wchar_t* that will be set to an allocated error message on failure (caller must free).
 * @return A dynamically allocated wchar_t* containing the processed source code, or NULL on failure.
 */
wchar_t* baa_preprocess(const char* main_file_path, const char** include_paths, wchar_t** error_message);

#endif // BAA_PREPROCESSOR_H
