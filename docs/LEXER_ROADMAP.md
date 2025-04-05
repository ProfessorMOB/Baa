# Baa Language Lexer Roadmap

This roadmap outlines the planned improvements and current status of the Baa language lexer implementation.

## Core Lexer Functionality

- [x] Basic token recognition
- [x] Lexer initialization and management
- [x] Source code navigation and tracking
- [x] Line and column tracking
- [x] Error token creation
- [x] Token memory management
- [ ] Add support for more token types for language extensions (Future)

## Token Types

- [ ] Special tokens (Comment - Defined, but comments currently skipped, not tokenized. See Comment Support section)
- [x] Literals (Identifier - Basic Arabic/ASCII support)
- [x] Literals (Int - `BAA_TOKEN_INT_LIT`)
- [ ] Literals (Float - Defined `BAA_TOKEN_FLOAT_LIT`, but lexer currently only makes `INT_LIT`; number parser handles float details post-lexing)
- [x] Literals (Char - Defined `BAA_TOKEN_CHAR_LIT`, scanned with escapes `\n, \t, \\, \', \", \r, \0, \uXXXX`)
- [x] Literals (String - `BAA_TOKEN_STRING_LIT`, scanned with escapes `\n, \t, \\, \", \uXXXX`)
- [x] Literals (Boolean - Defined `BAA_TOKEN_BOOL_LIT`, keywords `صحيح`/`خطأ` scanned)
- [x] Keywords (FUNC, RETURN, IF, ELSE, WHILE, FOR, DO, SWITCH, CASE, BREAK, CONTINUE - Present in `lexer.c` keyword list)
- [x] Keywords (VAR, CONST - Defined and added to `lexer.c` keyword list)
- [x] Types (TYPE_INT, TYPE_FLOAT, TYPE_CHAR, TYPE_VOID, TYPE_BOOL - Defined and added to `lexer.c` keyword list)
- [x] Operators (+, -, *, /, %, =, ==, !, !=, <, <=, >, >=)
- [x] Operators (&&, || - Defined and scanning logic added in `lexer.c`)
- [x] Compound assignment operators (+=, -=, *=, /=, %=)
- [x] Increment/decrement operators (++, --)
- [x] Delimiters ((, ), {, }, [, ], ,, ., ;, :)

## Number Parsing

- [x] Integer parsing
- [x] Decimal number parsing
- [x] Number error handling
- [x] Arabic-Indic digit support
- [x] Scientific notation support
- [x] Binary and hexadecimal literals
- [ ] Underscore in number literals (e.g., 1_000_000)

## String Handling

- [x] Basic string literal support (double quotes)
- [x] Character literal support (single quotes)
- [x] Basic escape sequences in strings/chars (\n, \t, \\, \", \', \r, \0)
- [x] Unicode escape sequences (\uXXXX)
- [ ] Multiline strings
- [ ] Raw string literals

## Comment Support

- [x] Single line comments (`#` style - Skipped, not tokenized)
- [x] Single line comments (`//` style - Skipped, not tokenized)
- [x] Multi-line comments (`/* ... */` - Skipped, not tokenized)
- [ ] Documentation comments (e.g., `/** ... */` or other syntax TBD)

## Arabic Language Support

- [x] Arabic letter recognition
- [x] Arabic-Indic digit support
- [x] Arabic keyword recognition
- [ ] Improve Unicode support for identifiers (beyond basic letters)
- [ ] Implement better whitespace and comment handling for RTL text
- [ ] Right-to-left text handling improvements (Visual/Editor concern mostly)
- [ ] Arabic error messages (Only number parser errors are currently Arabic)
- [ ] Visual error highlighting for Arabic text (Editor integration?)
- [ ] Arabic language directives

## Preprocessing

- [ ] Include directives
- [ ] Conditional compilation
- [ ] Macro definitions
- [ ] File inclusion

## Error Handling and Reporting

- [x] Basic error reporting
- [ ] Detailed error messages with context
- [ ] Error recovery (Improve synchronization points)
- [ ] Warning system
- [ ] Suggestions for common errors
- [ ] Error categorization and filtering

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
- [ ] UTF-8 to UTF-16 conversion (Currently assumes UTF-16LE input)

## Implementation Priorities (Revised after recent updates)

1. **Enhance Error Reporting and Recovery:** *[Done]*
    - Provide more detailed error messages with context.
    - Implement basic error recovery to continue lexing after simple errors.
2. **Add Preprocessing Directives:** *[Next]*
    - File inclusion (`#تضمين` or similar).
    - Conditional compilation (`#إذا_عرف`, etc.).
    - Macros (`#تعريف`).
3. **Testing:**
    - Develop comprehensive unit tests for lexer functionality.
    - Add integration tests with the parser.
4. **Advanced Features:**
    - Underscores in number literals (`1_000`).
    - Multiline strings / Raw strings.
    - Documentation comments.
5. **Performance Optimizations:**
    - Keyword lookup optimization.
    - String interning.

6.  **[Misc]** Implement remaining basic escape sequences (`\r`, `\0` in strings, `\"` in chars). *[Done]*
7.  **[Feature]** Implement underscores in number literals (`1_000_000`). *[Done]*

