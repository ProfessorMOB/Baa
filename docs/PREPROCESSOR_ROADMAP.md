# Baa Language Preprocessor Roadmap

This roadmap outlines the planned improvements and current status of the Baa language preprocessor implementation.

## Core Preprocessor Functionality

- [x] Separate processing stage before lexer
- [x] Recursive file processing
- [x] UTF-16LE file reading (with BOM check)
- [x] Dynamic buffer for output generation
- [x] Error reporting via output parameter
- [ ] Input source abstraction (file, string, stdin)
- [ ] Support for UTF-8 input files (with conversion to internal wchar_t)
- [ ] Improved error reporting (track original source line/column)

## Directive Handling

- **Includes:**
    - [x] `#تضمين "..."` (Relative path resolution)
    - [x] `#تضمين <...>` (Include path searching)
    - [x] Circular include detection
- **Macros:**
    - [x] `#تعريف NAME BODY` (Parameterless macro definition)
    - [x] Simple text substitution for parameterless macros
    - [x] `#الغاء_تعريف NAME` (undef)
    - [x] Function-like macros (with parameters - *basic implementation*)
    - [ ] More robust substitution rules (recursion prevention, stringification `#`, token pasting `##`, robust argument parsing)
- **Conditional Compilation:**
    - [x] `#إذا_عرف MACRO` (ifdef)
    - [x] `#إذا_لم_يعرف MACRO` (ifndef)
    - [x] `#وإلا_إذا MACRO` (elif - *currently only checks if MACRO is defined*)
    - [x] `#إلا` (else)
    - [x] `#نهاية_إذا` (endif)
    - [ ] Basic `#if` (evaluating constant expressions - requires expression parser integration)

## Testing and Validation

- [ ] Unit test coverage for directive parsing
- [ ] Unit tests for include path resolution
- [ ] Unit tests for macro definition and substitution
- [ ] Unit tests for circular include detection
- [ ] Unit tests for conditional compilation logic (when implemented)
- [ ] Integration tests with the overall compiler flow

## Implementation Priorities (Excluding Testing for now)

1.  **Advanced Macro Features:** *[Next]*
    - Implement more robust substitution rules (stringification `#`, token pasting `##`, robust argument parsing).
2.  **Conditional Compilation Enhancements:**
    - Implement `#if` with constant expression evaluation.
    - Enhance `#elif` to support constant expression evaluation.
3.  **Error Reporting:**
    - Improve error messages to include original source line/column numbers.
4.  **Robustness & Advanced Features:**
    - Support for UTF-8 input files.
    - Input source abstraction (file, string, stdin).
