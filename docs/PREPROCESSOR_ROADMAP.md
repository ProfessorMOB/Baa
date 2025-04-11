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
    - [ ] Function-like macros (with parameters)
    - [ ] `#undef` directive
    - [ ] More robust substitution rules (recursion prevention, stringification `#`, token pasting `##`)
- **Conditional Compilation:**
    - [ ] `#إذا_عرف MACRO` (ifdef)
    - [ ] `#إذا_لم_يعرف MACRO` (ifndef)
    - [ ] `#إلا` (else)
    - [ ] `#نهاية_إذا` (endif)
    - [ ] Basic `#if` (evaluating constant expressions - requires expression parser integration)

## Testing and Validation

- [ ] Unit test coverage for directive parsing
- [ ] Unit tests for include path resolution
- [ ] Unit tests for macro definition and substitution
- [ ] Unit tests for circular include detection
- [ ] Unit tests for conditional compilation logic (when implemented)
- [ ] Integration tests with the overall compiler flow

## Implementation Priorities

1.  **Conditional Compilation:** *[Next]*
    - Implement `#إذا_عرف`, `#إذا_لم_يعرف`, `#إلا`, `#نهاية_إذا`.
2.  **Testing:**
    - Develop comprehensive unit tests for existing functionality (`#تضمين`, `#تعريف`).
    - Add tests for conditional compilation as it's implemented.
3.  **Advanced Macro Features:**
    - Function-like macros.
    - `#undef` directive.
4.  **Error Reporting:**
    - Improve error messages to include original source line/column numbers.
5.  **Robustness & Advanced Features:**
    - More robust macro substitution rules.
    - Support for `#if` expressions.
    - UTF-8 input support.
