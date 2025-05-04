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
        - **Token Pasting (`##`):** Supports the `##` operator for concatenating tokens during macro expansion. It pastes the token preceding `##` with the token following `##`. Handles edge cases like empty arguments.
- **`#الغاء_تعريف` (Undefine):**
    - Removes a previously defined macro (both object-like and function-like).
    - Example: `#الغاء_تعريف MAX_SIZE`
- **Conditional Compilation:**
    - Supports conditional blocks:
        - `#إذا expression` (if): Evaluates a constant integer `expression`. Undefined identifiers in the expression evaluate to `0`.
        - `#إذا_عرف MACRO` (ifdef): Checks if `MACRO` is defined.
        - `#إذا_لم_يعرف MACRO` (ifndef): Checks if `MACRO` is *not* defined.
        - `#وإلا_إذا expression` (elif): Evaluates a constant integer `expression` if the preceding `#if`/`#elif` was false and no prior branch in the block was taken.
        - `#إلا` (else): Executes if the preceding `#if`/`#elif` was false and no prior branch in the block was taken.
        - `#نهاية_إذا` (endif): Ends the conditional block.
    - Lines within blocks whose conditions are false are skipped.
    - **Expression Evaluation:** Supports standard integer arithmetic (`+`, `-`, `*`, `/`, `%`), comparison (`==`, `!=`, `<`, `>`, `<=`, `>=`), logical (`&&`, `||`, `!`), parentheses `()`, and the `defined(MACRO)` operator within `#إذا` and `#وإلا_إذا` directives, respecting operator precedence. Undefined identifiers evaluate to `0`. *Note: Bitwise operators are not yet supported.*

### Macro Recursion Detection (كشف استدعاء الماكرو الذاتي)
- Detects and prevents infinite loops caused by direct or indirect recursive macro expansions using an internal expansion stack.

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
    - Macro errors (recursion, argument count mismatch, invalid ## usage).
    - Conditional expression evaluation errors (syntax errors, division by zero).
    - Memory allocation failures.
- Error messages are provided in Arabic and include file path and line number context where available.

## Preprocessor Structure (بنية المعالج المسبق)

The preprocessor implementation has been refactored into several internal source files located in `src/preprocessor/`:
- `preprocessor.c`: Main entry point (`baa_preprocess`).
- `preprocessor_core.c`: Core recursive file processing logic (`process_file`).
- `preprocessor_utils.c`: Utility functions (file I/O, path handling, error formatting, dynamic buffers, file stack).
- `preprocessor_macros.c`: Macro definition management (`#تعريف`, `#الغاء_تعريف`).
- `preprocessor_expansion.c`: Macro expansion logic (substitution, argument parsing, `#`, `##`, recursion stack).
- `preprocessor_conditionals.c`: Conditional compilation stack management (`#if`, `#ifdef`, etc.).
- `preprocessor_expr_eval.c`: Conditional expression evaluation logic.

An internal header, `src/preprocessor/preprocessor_internal.h`, defines shared structures and function prototypes used across these files.

### BaaPreprocessor (Public Interface)

The public header (`include/baa/preprocessor/preprocessor.h`) uses an **opaque pointer** pattern. Users interact with `BaaPreprocessor*` but the internal structure is hidden.

```c
// Forward declaration in public header
typedef struct BaaPreprocessor BaaPreprocessor;
```
The full definition of `struct BaaPreprocessor` resides in `src/preprocessor/preprocessor_internal.h` and contains fields for managing include paths, macro definitions, conditional state, file stacks, expansion stacks, and error context.

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

- **Rescanning Expanded Macros:** Implement proper rescanning of macro expansion results for further macro substitutions, as required by the C standard.
- **Bitwise Operators:** Add support for bitwise operators (`&`, `|`, `^`, `~`, `<<`, `>>`) in conditional expressions (`#إذا`, `#وإلا_إذا`).
- **Predefined Macros:** Implement standard predefined macros (e.g., `__FILE__`, `__LINE__`, `__DATE__`, `__TIME__`).
- **UTF-8 Input:** Add support for reading UTF-8 encoded source files in addition to UTF-16LE.
- **Input Abstraction:** Abstract the input source to allow preprocessing from strings or standard input, not just files.
- **Error Recovery:** Improve error recovery mechanisms within directives and expressions.
