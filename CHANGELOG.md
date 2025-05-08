# Changelog

All notable changes to the B (باء) compiler project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.11.0] - 2025-05-08

### Added
- **Preprocessor:** Added Input Source Abstraction, allowing `baa_preprocess` to accept input from files (`BAA_PP_SOURCE_FILE`) or directly from wide character strings (`BAA_PP_SOURCE_STRING`) via the new `BaaPpSource` struct.
- **Preprocessor:** Added support for reading UTF-8 encoded input files (with or without BOM), in addition to UTF-16LE. Encoding is auto-detected.
- **Lexer:** Added support for multiline string literals (`"""..."""`). (Commit: 9b4b8a0)
- **Lexer:** Added support for raw string literals (`خ"..."` and `خ"""..."""`), including single-line newline error handling. (Commit: 5caef53)

### Fixed
- **Compiler:** Fixed use-after-free bug and potential infinite loop in lexer token processing loop (`compiler.c`) by refactoring to a standard `for` loop, ensuring correct termination on EOF or ERROR tokens. (Commit: d08a915)
- **Lexer:** Corrected handling of single-line raw strings (`خ"..."`) to properly report an error if a newline is encountered before the closing quote. (Commit: 5caef53)

### Changed
- **Lexer:** Improved error reporting for multiline strings by tracking start line/column. (Commit: 9b4b8a0)


## [0.1.10.0] - 2025-05-07

### Changed

- **Lexer Enhancements & Fixes:**
  - **Numeric Literal Tokenization:**
    - Correctly tokenizes Arabic-Indic digits (٠-٩ / U+0660-U+0669) as part of integer and float literals.
    - Implemented support for binary (`0b`/`0B`) and hexadecimal (`0x`/`0X`) integer prefixes.
    - Added support for underscores (`_`) as separators in all valid positions within numeric literals (integers, floats, including prefixed numbers and exponent parts).
    - Recognizes the Arabic decimal separator (`٫`, U+066B) as a valid decimal point in float literals. (Note: Console display of `٫` in lexemes might appear as `?` due to terminal limitations, but internal processing is correct).
  - **Dispatch Logic:** Refined character type dispatch in `baa_lexer_next_token` to correctly prioritize digit types (Arabic-Indic, then Western) before general identifier characters, resolving misidentification of Arabic-Indic digits as identifiers.
  - **Error Messaging:** Clarified the error message for invalid escape sequences (e.g., `\'`) within string literals.
  - **Modularization:** Further modularized the lexer by moving token scanning logic (`scan_identifier`, `scan_number`, `scan_string`, `scan_char_literal`) into `src/lexer/token_scanners.c` and character utilities into `src/lexer/lexer_char_utils.c`. Introduced `src/lexer/lexer_internal.h` for internal declarations. (This completes earlier modularization efforts and resolves associated build issues).
- **Documentation:**
  - Significantly updated `docs/lexer.md` to reflect the current capabilities regarding numeric literal parsing, Arabic language support, and overall lexer functionality.

### Added

- **Testing:**
  - Created a new comprehensive lexer test suite: `tests/resources/lexer_test_cases/lexer_test_suite.baa` to cover a wide range of lexer features and edge cases.

## [0.1.9.9] - 2025-05-06

### Added

- Standalone preprocessor tester executable (`tools/baa_preprocessor_tester.c`) for isolated testing.
- Placeholder implementations for missing parser functions (`baa_parse_if_statement`, `baa_parse_while_statement`, `baa_parse_for_statement`, `baa_parse_return_statement`, `baa_create_compound_assignment_expr`, `baa_create_grouping_expr`) to allow the build to complete.
- Added Arabic predefined macros to the preprocessor:
  - `__الملف__` (equivalent to `__FILE__`)
  - `__السطر__` (equivalent to `__LINE__`)
  - `__التاريخ__` (equivalent to `__DATE__`)
  - `__الوقت__` (equivalent to `__TIME__`)
- Unified preprocessor error reporting to consistently use original source location (file, line, column) for all errors.

### Fixed

- Preprocessor: Fixed bug where `#إذا` directives using macros defined as simple integers (e.g., `#تعريف DEBUG 1`) were not evaluated correctly, causing the enclosed block to be skipped erroneously. The expression evaluator now handles this case.
- Build: Corrected mismatch between `baa_parse` and `baa_parse_program` definitions/declarations in `src/parser/parser.c`.
- Build: Added missing declaration for `baa_parse_for_statement` in `include/baa/parser/parser.h`.

### Changed

- **Parser Refactoring:** Split `src/parser/parser.c` into more modular components:
  - `src/parser/parser_helper.c`: Contains core utilities like `advance`, `peek`, `match_keyword`, error helpers, etc.
  - `src/parser/statement_parser.c`: Contains statement dispatching (`baa_parse_statement`) and block parsing (`baa_parse_block`).
  - `src/parser/declaration_parser.c`: Contains declaration dispatching (`baa_parse_declaration`) and import parsing (`baa_parse_import_directive`).
- **Code Synchronization:** Aligned parser implementation files and `docs/PARSER.md` documentation. Corrected public API documentation, added missing documentation for imports, resolved conflicting function definitions.

### Fixed

- **Build Errors:**
  - Resolved duplicate symbol linker errors by consolidating function implementations (error handling, parser helpers) and removing `src/parser/error_handling.c`.
  - Fixed math library linking error (`m.lib`) on Windows/Clang by adding `m` to `target_link_libraries` in the root `CMakeLists.txt`.

### Removed

- Obsolete parser source files (`src/parser/function_parser.c`, `src/parser/error_handling.c`) after refactoring.

### Added

- LLVM integration for code generation
  - Basic LLVM IR generation infrastructure
  - Function generation with proper return types
  - Support for multiple target platforms (x86-64, ARM64, WebAssembly)
  - Integration with existing compiler pipeline
  - Arabic error messages for code generation failures
- Enhanced compiler driver
  - Integrated code generation into the compilation pipeline
  - Added support for generating LLVM IR output files
  - Improved error handling with Arabic messages
- New parser components
  - Added control_flow_parser.c for handling control structures
  - Added declaration_parser.c for variable and function declarations
  - Added expression_parser.c for expression parsing
  - Added function_parser.c for function handling
  - Added type_parser.c for type system parsing
- Enhanced AST implementation
  - Added new node types and structures
  - Added visitor interface for AST traversal
  - Added scope management system
  - Added comprehensive statement handling
  - Added literal value handling
- Improved testing infrastructure
  - Added unit tests for lexer functionality
  - Added number parsing tests
  - Added control flow tests
  - Added type and operator tests
  - Added test framework with assertion macros
- Memory management improvements
  - Added baa_malloc/baa_free for consistent memory handling
  - Added baa_strdup/baa_strndup for string operations
  - Added proper memory cleanup in all components

### Changed

- Updated build system
  - Added LLVM package detection
  - Added conditional compilation for LLVM features
  - Reorganized component subdirectories
  - Enhanced test configuration
  - Added memory leak detection
- Improved type system
  - Updated parameter handling from BaaFuncParam to BaaParameter
  - Enhanced type checking and validation
  - Added support for user-defined types
  - Improved type conversion system
- Enhanced error handling
  - Added Arabic error messages
  - Improved error recovery mechanisms
  - Added detailed error context
  - Enhanced error reporting system
- Updated project structure
  - Reorganized source files into logical components
  - Moved test files to appropriate directories
  - Enhanced documentation structure
  - Improved code organization

### Fixed

- Memory management issues
  - Fixed memory leaks in parser functions
  - Fixed string handling in parameter creation
  - Fixed resource cleanup in AST operations
  - Fixed memory management in test framework
- Type system issues
  - Fixed type declarations in parser functions
  - Fixed parameter handling in function declarations
  - Fixed type checking in expressions
  - Fixed type conversion edge cases
- Parser improvements
  - Fixed compilation errors in declaration_parser.c
  - Fixed control flow parsing issues
  - Fixed expression parsing bugs
  - Fixed function parsing problems
- Test framework issues
  - Fixed test result reporting
  - Fixed memory leak detection
  - Fixed test file organization
  - Fixed test execution flow

### Removed

- Removed deprecated components
  - Removed old control_flow.h
  - Removed old parser.h
  - Removed old tokens.h
  - Removed old types.h
- Removed redundant files
  - Removed duplicate test files
  - Removed old documentation files
  - Removed unused header files
  - Removed obsolete test framework files

## [0.1.9.8] - 2025-02-09

### Added

- Complete decimal number parsing implementation
  - Support for both Western (0-9) and Arabic-Indic (٠-٩) digits
  - Support for Western (.) and Arabic (٫) decimal separators
  - Comprehensive error handling with Arabic messages
  - Memory-safe number token management
- Enhanced lexer functionality
  - Added number validation and format checking
  - Implemented overflow protection
  - Added support for preserving original text representation

### Changed

- Updated lexer to handle both integer and decimal numbers
- Enhanced error handling system with detailed Arabic messages
- Improved memory management for number tokens
- Updated build system to include number parser module

### Fixed

- Memory leaks in number parsing
- Decimal point validation
- Overflow handling in large numbers
- Arabic numeral conversion accuracy

## [0.1.9.7] - 2025-02-08

### Added

- Restored decimal number parsing with improved implementation
- Completed control flow parsing implementation
  - Full support for إذا (if) statements
  - Integrated وإلا (else) statements
  - Implemented طالما (while) loops
- Added comprehensive test suite for control flow structures

### Changed

- Updated parser to handle decimal numbers efficiently
- Enhanced error handling for control flow statements
- Improved documentation to reflect completed features

### Fixed

- Edge cases in decimal number parsing
- Control flow statement nesting issues
- Parser error recovery in complex control structures

## [0.1.9] - 2025-02-06

### Added

- Enhanced parser debug output for better troubleshooting
- Added test files for Arabic program parsing
- Added UTF-8 support for Arabic identifiers in parser

### Changed

- Refactored parser functions for better error handling
- Simplified number parsing (temporarily removed decimal support)
- Improved error messages with more context

### Fixed

- Fixed memory leaks in parser error handling
- Fixed function declaration parsing
- Fixed identifier parsing for UTF-8 characters

### Development

- Added more debug output in parse_declaration and parse_program
- Reorganized parser code structure
- Added test cases for basic language features

## [0.1.8] - 2025-02-01

### Added

- Full implementation of AST (Abstract Syntax Tree)
- Complete type system implementation
- Operator system with precedence rules
- Control flow structures (if, while, return)
- Error handling with Arabic message support
- Memory management utilities
- Support for Arabic file extension `.ب`
- UTF-8 encoding support for source files
- Basic compiler infrastructure for Arabic source files
- Example program with Arabic file name (`برنامج.ب`)
- EditorConfig settings for Arabic source files
- `#تضمين` directive for imports
- Updated character type from `محرف` to `حرف`

### Changed

- Removed placeholder implementations
- Updated build system to exclude unimplemented components
- Improved project structure and organization
- Build system now handles Arabic file extensions
- Updated CMake configuration for UTF-8 support
- Enhanced file handling for Arabic source files
- Improved project documentation with Arabic examples
- Refined language specification with proper Arabic keywords
- Updated type system documentation to use `حرف` instead of `محرف`
- Added proper import syntax using `#تضمين`

### Fixed

- Corrected character type naming convention
- Standardized import directive syntax
- Aligned documentation with actual language specification

## [0.1.7] - 2025-02-01

### Added

- Basic control flow structures implementation:
  - إذا/وإلا (if/else) statement support
  - طالما (while) loop support
  - إرجع (return) statement support
- New control_flow.h header with comprehensive control flow API
- Memory-safe statement creation and management functions
- Unit tests for all control flow structures
- Integration with existing build system

### Changed

- Updated CMakeLists.txt to include control flow implementation
- Enhanced project structure with new control flow components
- Updated roadmap to reflect completed control flow milestone

## [0.1.6] - 2025-01-31

### Added

- Basic type system implementation with K&R C compatibility
- Core operator system with arithmetic and comparison operators
- Comprehensive test suite for type system and operators
- Type conversion and validation system
- Arabic operator names and error messages
- UTF-16 support for character type

### Changed

- Updated project structure with src/ and tests/ directories
- Enhanced type safety with strict conversion rules
- Improved error handling in type system

### Documentation

- Added type system implementation details
- Added operator system specifications
- Updated development guide with testing procedures
- Added code examples for type and operator usage

## [0.1.5] - 2025-01-31

### Added

- Comprehensive K&R C feature comparison in `docs/c_comparison.md`
- Detailed implementation roadmap for K&R C compatibility
- Arabic equivalents for all K&R C keywords and operators
- Standard library function mappings with Arabic names

### Changed

- Updated `docs/language.md` to align with K&R C specification
- Updated `docs/development.md` to follow K&R coding style
- Enhanced documentation with UTF-16LE and bidirectional text support details
- Improved error message documentation with Arabic language support

### Documentation

- Added detailed type system documentation
- Added memory model specifications
- Added preprocessor directive documentation
- Added comprehensive code examples
- Updated build system documentation

## [0.1.0] - 2025-01-31

### Added

- Initial project structure and build system
- Basic file reading with UTF-16LE support
- Support for Arabic text in source files
- Modular component architecture:
  - Lexer component with UTF-16 file handling
  - AST component with basic node structure
  - Parser component (placeholder)
  - Code generator component (placeholder)
  - Utils component with error handling and memory management
- Comprehensive test suite:
  - Lexer tests for file handling
  - Utils tests for memory and error handling
- Project documentation:
  - README with build instructions
  - API documentation
  - Example programs
- CMake-based build system with:
  - Cross-platform support
  - Test integration
  - Modular component builds
- Development tools:
  - Automated testing framework
  - Error handling system
  - Memory management utilities
- Initial project setup
- Basic file reading functionality
- UTF-16LE support for Arabic text
- CMake build system configuration
- Project documentation
- Code organization and structure

### Changed

- Organized project structure into logical components
- Improved build system with proper dependency management
- Enhanced file handling with proper Unicode support
- Improved project structure
- Enhanced build system configuration
- Updated documentation

### Fixed

- UTF-16LE file handling and BOM detection
- Memory management in AST operations
- File size calculations for wide character content
- File handling improvements
- Better error messages in Arabic

## [0.0.1] - 2025-01-31

### Added

- Initial release
- Basic file reading capabilities
- Command-line interface
- Simple error reporting

## [Unreleased]

### Planned

- Parser implementation
- Code generation
- Symbol table
- Type system
- Error recovery
- Optimization passes
- Arabic language syntax definition
- Comprehensive test suite expansion
- Documentation in both English and Arabic
