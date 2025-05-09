# Baa Language Preprocessor Roadmap

This roadmap outlines the planned improvements and current status of the Baa language preprocessor implementation.

## Core Preprocessor Functionality

- [x] Separate processing stage before lexer
- [x] Recursive file processing
- [x] File encoding detection (UTF-8 with/without BOM, UTF-16LE)
- [x] Dynamic buffer for output generation
- [x] Error reporting via output parameter
- [x] Input source abstraction (file, string). ([ ] Add stdin support)
- [x] Support for UTF-8 input files (with conversion to internal wchar_t)
- [x] Improved error reporting (unified system with original file, line, and column numbers, including through expansions)

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
    - [ ] Implement full macro rescanning and robust substitution rules (complex edge cases).
    - [ ] Fully robust argument parsing (complex edge cases with literals/whitespace)
- **Conditional Compilation:**
    - [ ] `#إذا expression` (if - implement full macro expansion and re-evaluation for identifiers in expressions)
    - [x] `#إذا_عرف MACRO` (ifdef)
    - [x] `#إذا_لم_يعرف MACRO` (ifndef)
    - [x] `#وإلا_إذا expression` (elif - constant expression evaluation)
    - [x] `#إلا` (else)
    - [x] `#نهاية_إذا` (endif)
    - [x] Support for bitwise operators (`&`, `|`, `^`, `~`, `<<`, `>>`) in conditional expressions.
    - **Other Standard Directives:**
        - [ ] `#error message`
        - [ ] `#warning message` (Consider if distinct from `#error` or if compiler treats them similarly)
        - [ ] `#line number "filename"`
        - [ ] `#pragma directive` (Investigate common pragmas like `once`)

    ## Testing and Validation

- [ ] Unit test coverage for directive parsing
- [ ] Unit tests for include path resolution
- [ ] Unit tests for macro definition and substitution
- [ ] Unit tests for circular include detection
- [ ] Unit tests for conditional compilation logic (when implemented)
- [ ] Integration tests with the overall compiler flow

## Implementation Priorities (Excluding Testing for now)

1.  **Robustness & Advanced Features:**
    - [x] Support for UTF-8 input files.
    - [x] Input source abstraction (file, string). (*stdin not yet implemented*)
    - [x] Support for bitwise operators (`&`, `|`, `^`, `~`, `<<`, `>>`) in conditional expressions.
    - [x] Predefined macros (`__الملف__`, `__السطر__`, `__التاريخ__`, `__الوقت__`)
        - [ ] Improve error recovery mechanisms (allow continuation after some errors to find more issues).
    3.  **Macro Edge Cases:**
        - Address complex edge cases in macro substitution and argument parsing.
