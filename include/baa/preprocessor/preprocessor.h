#ifndef BAA_PREPROCESSOR_H
#define BAA_PREPROCESSOR_H

#include <wchar.h>
#include <stdbool.h>
#include <stddef.h> // For size_t

// --- Add BaaMacro struct ---
// Structure to hold a macro definition
typedef struct {
    wchar_t* name;  // Macro name
    wchar_t* body;  // Macro body (replacement text)
} BaaMacro;
// --- End Add ---

// Forward declaration if needed, or include necessary headers

// Structure to hold preprocessor state (example, will expand later)
typedef struct {
    // TODO: Add fields for include paths, defined macros, file stack, etc.
    const char** include_paths; // Array of paths to search for <...>
    size_t include_path_count;
    // To detect circular includes:
    char** open_files_stack;    // Stack of full paths currently being processed
    size_t open_files_count;
    size_t open_files_capacity;
    // --- Add Macro storage ---
    // Defined macros
    BaaMacro* macros;           // Dynamically allocated array of macros
    size_t macro_count;
    size_t macro_capacity;
    // --- End Add ---
} BaaPreprocessor;

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
