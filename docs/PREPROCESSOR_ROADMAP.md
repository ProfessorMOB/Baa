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
- [x] Improved error reporting (added column numbers to messages)
- [ ] Improved error reporting (track original source location through expansions)

## Directive Handling

- **Includes:**
    - [x] `#تضمين "..."` (Relative path resolution)
    - [x] `#تضمين <...>` (Include path searching)
    - [x] Circular include detection
- **Macros:**
    - [x] `#تعريف NAME BODY` (Parameterless macro definition)
    - [x] Simple text substitution for parameterless macros
    - [x] `#الغاء_تعريف NAME` (undef)
    - [x] Function-like macros (with parameters)
    - [x] Stringification (`#`)
    - [x] Token Pasting (`##`) (concatenation)
    - [x] Macro recursion detection
    - [ ] More robust substitution rules (handling complex edge cases, rescanning)
    - [ ] Fully robust argument parsing (complex edge cases with literals/whitespace)
- **Conditional Compilation:**
    - [x] `#إذا expression` (if - constant expression evaluation - *partially supports evaluating simple integer macros*)
    - [x] `#إذا_عرف MACRO` (ifdef)
    - [x] `#إذا_لم_يعرف MACRO` (ifndef)
    - [x] `#وإلا_إذا expression` (elif - constant expression evaluation)
    - [x] `#إلا` (else)
    - [x] `#نهاية_إذا` (endif)
    - [ ] Support for bitwise operators (`&`, `|`, `^`, `~`, `<<`, `>>`) in conditional expressions.

## Testing and Validation

- [ ] Unit test coverage for directive parsing
- [ ] Unit tests for include path resolution
- [ ] Unit tests for macro definition and substitution
- [ ] Unit tests for circular include detection
- [ ] Unit tests for conditional compilation logic (when implemented)
- [ ] Integration tests with the overall compiler flow

## Implementation Priorities (Excluding Testing for now)

1.  **Error Reporting:** *[Partially Done - Column Added; Next: Original Location Tracking]*
    - Improve error messages to include original source line/column numbers.
2.  **Robustness & Advanced Features:**
    - Support for UTF-8 input files.
    - Input source abstraction (file, string, stdin).
    - Support for bitwise operators (`&`, `|`, `^`, `~`, `<<`, `>>`) in conditional expressions.
    - Predefined macros (e.g., `__FILE__`, `__LINE__`).
3.  **Macro Edge Cases:**
    - Address complex edge cases in macro substitution and argument parsing.
