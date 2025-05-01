# Baa Preprocessor Documentation (توثيق المعالج المسبق)

## Overview (نظرة عامة)

The Baa preprocessor is a crucial initial stage in the compilation pipeline. It runs **before** the lexer and is responsible for handling preprocessor directives within the Baa source code. Its primary function is to process the source text, resolving includes and expanding macros, to produce a single, unified translation unit (as a wide character string) that is then passed to the lexer.

It operates on files assumed to be encoded in **UTF-16LE**.

## Features (الميزات)

### Directive Handling (معالجة التوجيهات)

- **`#تضمين` (Include):**
    - Processes file inclusion directives.
    - Supports standard include paths using angle brackets (`#تضمين <ملف.ب>`). Searches paths provided during initialization.
    - Supports relative include paths using double quotes (`#تضمين "ملف.ب"`). Paths are resolved relative to the directory of the file containing the directive.
- **`#تعريف` (Define):**
    - **Object-like Macros:** Defines simple text replacement macros.
        - Example: `#تعريف MAX_SIZE 1024`
    - **Function-like Macros:** Defines macros that take arguments.
        - Example: `#تعريف ADD(a, b) a + b`
        - Parses parameter names within the parentheses.
        - Performs substitution of parameter names in the macro body with the provided arguments during invocation (`ADD(1, 2)` becomes `1 + 2`).
        - Argument parsing correctly handles nested parentheses and basic string/character literals.
        - **Stringification (`#`):** Supports the `#` operator before a parameter name (e.g., `#param`) to convert the corresponding argument into a string literal (quoted and escaped).
        - **Token Pasting (`##`):** Includes basic support for the `##` operator. It currently works by suppressing whitespace between the tokens adjacent to `##`. *Note: This is a simplified implementation and does not perform true token concatenation to form new identifiers/numbers.*
- **`#الغاء_تعريف` (Undefine):**
    - Removes a previously defined macro (both object-like and function-like).
    - Example: `#الغاء_تعريف MAX_SIZE`
- **Conditional Compilation:**
    - Supports basic conditional blocks:
        - `#إذا_عرف MACRO` (ifdef): Checks if `MACRO` is defined.
        - `#إذا_لم_يعرف MACRO` (ifndef): Checks if `MACRO` is *not* defined.
        - `#وإلا_إذا MACRO` (elif): **Currently** checks if `MACRO` is defined (similar to `#ifdef`). Executes if the preceding `#if`/`#elif` was false, no prior branch in the block was taken, *and* `MACRO` is defined. *Note: Does not yet support general expression evaluation.*
        - `#إلا` (else): Executes if the preceding `#if`/`#elif` was false and no prior branch in the block was taken.
        - `#نهاية_إذا` (endif): Ends the conditional block.
    - Lines within blocks whose conditions are false are skipped.
- **Other Directives:** Directives not explicitly handled (e.g., `#if` with general expressions) are currently passed through unchanged to the output (intended for future implementation).

### Circular Include Detection (كشف التضمين الدائري)

- Tracks the stack of currently open files using their absolute paths.
- Prevents infinite loops by detecting and reporting circular `#تضمين` directives.

### UTF-16LE Support (دعم UTF-16LE)

- Reads source files assuming UTF-16 Little Endian encoding.
- Verifies the presence of the correct Byte Order Mark (BOM).
- Outputs the processed source code as a UTF-16 `wchar_t*` string.

### Error Handling (معالجة الأخطاء)

- Reports errors via an output parameter (`wchar_t** error_message`) in the main `baa_preprocess` function.
- Detects and reports errors such as:
    - File not found (for main file or includes).
    - Invalid file encoding (not UTF-16LE or BOM mismatch).
    - Circular includes.
    - Invalid directive syntax (e.g., malformed `#تضمين` or `#تعريف`).
    - Memory allocation failures.
- Error messages are provided in Arabic.

## Preprocessor Structure (بنية المعالج المسبق)

### BaaPreprocessor

The main structure holding the preprocessor's state during execution:

```c
typedef struct {
    // Include paths
    const char** include_paths;     // Array of paths to search for <...>
    size_t include_path_count;
    // Circular include detection
    char** open_files_stack;        // Stack of full paths currently being processed
    size_t open_files_count;
    size_t open_files_capacity;
    // Defined macros
    BaaMacro* macros;               // Dynamically allocated array of macros
    size_t macro_count;
    size_t macro_capacity;
    // Conditional compilation state
    bool* conditional_stack;        // Stack: true if current block's condition was met
    size_t conditional_stack_count;
    size_t conditional_stack_capacity;
    bool* conditional_branch_taken_stack; // Stack: true if a branch (#if, #elif, #else) has been taken
    size_t conditional_branch_taken_stack_count;
    size_t conditional_branch_taken_stack_capacity;
    bool skipping_lines;            // True if currently skipping lines
} BaaPreprocessor;
```

### BaaMacro

Represents a single macro definition:

```c
typedef struct {
    wchar_t* name;          // Macro name
    wchar_t* body;          // Macro body (replacement text)
    bool is_function_like;  // True if defined with (...)
    size_t param_count;     // Number of parameters
    wchar_t** param_names;  // Array of parameter names (NULL if not function-like)
} BaaMacro;
```

## Usage (الاستخدام)

The preprocessor is typically invoked by the main compiler driver before the lexer stage.

```c
#include "baa/preprocessor/preprocessor.h"

// ... inside compiler logic ...

const char* main_file = "path/to/your/program.ب";
const char* std_includes[] = {"/usr/local/include/baa", NULL}; // Example include paths
wchar_t* error_msg = NULL;

// Call the preprocessor
wchar_t* processed_source = baa_preprocess(main_file, std_includes, &error_msg);

if (!processed_source) {
    // Handle error
    if (error_msg) {
        fwprintf(stderr, L"Preprocessor Error: %ls\n", error_msg);
        free(error_msg);
    }
    // Abort compilation
} else {
    // Pass processed_source to the lexer
    // baa_init_lexer(..., processed_source, ...);

    // Remember to free the processed source later
    free(processed_source);
}
```

## Future Enhancements (تحسينات مستقبلية)

- Conditional compilation (`#if`, `#elif` with constant expressions).
- More robust macro substitution rules (recursion prevention, true token pasting `##`).
- Fully robust argument parsing for function-like macros (edge cases with literals/whitespace).
- Improved error reporting with original source line numbers.
- Support for UTF-8 input files.
- Input source abstraction (file, string, stdin).
