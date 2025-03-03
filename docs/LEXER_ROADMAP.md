# Baa Language Lexer Roadmap

This roadmap outlines the planned improvements and current status of the Baa language lexer implementation.

## Core Lexer Functionality

- [x] Basic token recognition
- [x] Lexer initialization and management
- [x] Source code navigation and tracking
- [x] Line and column tracking
- [x] Error token creation
- [x] Token memory management

## Token Types

- [x] Special tokens (EOF, Error, Unknown)
- [x] Literals (Identifier, Int, Float, Char, String)
- [x] Keywords
- [x] Types
- [x] Operators
- [x] Delimiters
- [x] Advanced operators (++, --, +=, -=, etc.)
- [ ] Additional keywords (switch, case, etc.)
- [x] Boolean literals (true/false - صحيح/خطأ)

## Number Parsing

- [x] Integer parsing
- [x] Decimal number parsing
- [x] Number error handling
- [x] Arabic-Indic digit support
- [x] Scientific notation support
- [x] Binary and hexadecimal literals
- [x] Underscore in number literals (e.g., 1_000_000)

## String Handling

- [x] Basic string literal support
- [x] Character literal support
- [x] Escape sequences in strings (\n, \t, etc.)
- [x] Unicode escape sequences
- [ ] Multiline strings
- [ ] Raw string literals

## Comment Support

- [x] Single line comments
- [x] Multi-line comments
- [ ] Documentation comments

## Arabic Language Support

- [x] Arabic letter recognition
- [x] Arabic-Indic digit support
- [x] Arabic keyword recognition
- [ ] Right-to-left text handling improvements
- [ ] Arabic error messages
- [ ] Arabic language directives

## Preprocessing

- [ ] Include directives
- [ ] Conditional compilation
- [ ] Macro definitions
- [ ] File inclusion

## Error Handling and Reporting

- [x] Basic error reporting
- [ ] Detailed error messages with context
- [ ] Error recovery
- [ ] Warning system
- [ ] Suggestions for common errors

## Performance Optimizations

- [ ] Keyword lookup table optimization
- [ ] Token memory pooling
- [ ] String interning
- [ ] Buffered file reading
- [ ] Parallel lexing for large files

## Testing and Validation

- [ ] Unit test coverage
- [ ] Integration tests with parser
- [ ] Benchmark suite
- [ ] Fuzz testing
- [ ] Edge case handling

## File Handling

- [x] Basic file content reading
- [x] File size determination
- [ ] Streaming large files
- [ ] Input source abstraction (file, string, stdin)
- [ ] UTF-8 to UTF-16 conversion

## Implementation Priorities

1. ~~Complete string handling (escape sequences, Unicode)~~ ✓ Completed!
2. ~~Add comment support (single line, multi-line)~~ ✓ Completed!
3. ~~Implement advanced operators (++, --, +=, -=, etc.)~~ ✓ Completed!
4. ~~Add support for Boolean literals (true/false - صحيح/خطأ)~~ ✓ Completed!
5. ~~Add support for additional number formats:~~ ✓ Completed!
   - ~~Scientific notation for numbers~~ ✓ Completed!
   - ~~Binary and hexadecimal numbers~~ ✓ Completed!
6. Enhance error reporting and recovery:
   - Line/column information for errors
   - Improved error messages
   - Error recovery to continue lexing after errors
7. Add support for preprocessing directives
   - File inclusion
   - Conditional compilation
   - Macros
8. Implement more advanced language features:
   - Specialized documentation comments
   - Template/generic support tokens
   - Custom operators or syntax extensions
9. Performance optimizations:
   - Token interning
   - Lexer state caching
   - Optimized character handling
10. Unit and integration testing:
    - Test suite for lexer functionality
    - Benchmarking and performance testing
    - Edge case handling tests
