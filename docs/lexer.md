# Baa Lexer Documentation

## Overview

The Baa lexer processes source code written in the Baa language, which includes strong support for Arabic. It reads source files encoded in UTF-16LE and uses wide characters (`wchar_t`) internally, which inherently supports bidirectional text rendering if the display environment handles it. The lexer tokenizes Arabic identifiers, keywords, and literals, while maintaining precise source location tracking (line and column numbers).

## Features

### Arabic Language Support

- Recognizes Arabic letters from multiple Unicode ranges (0x0600-0x06FF, FB50-FDFF, FE70-FEFF) for identifiers.
- Recognizes Arabic-Indic digits (٠-٩) in identifiers and number literals.
- Recognizes Arabic keywords (e.g., `دالة`, `إذا`).
- Supports Arabic characters within string literals.
- Note: Bidirectional text display is dependent on the terminal/editor, not explicitly managed by the lexer logic itself.
- Note: Only `#` style comments are currently supported.

### Token Types

- Keywords (كلمات مفتاحية)
- Identifiers (معرفات)
- Literals (قيم حرفية)
  - Numbers (أرقام - Scanned as `BAA_TOKEN_INT_LIT`, specific type determined by parser)
  - Strings (نصوص - `BAA_TOKEN_STRING_LIT`)
  - Boolean (قيم منطقية - `BAA_TOKEN_BOOL_LIT`, scanned via `صحيح`/`خطأ` keywords)
  - Characters (حروف - `BAA_TOKEN_CHAR_LIT`, scanned via `'c'` syntax)
- Type Names (أسماء الأنواع - e.g., `عدد_صحيح`, `منطقي` scanned as specific `BAA_TOKEN_TYPE_*`)
- Operators (عوامل - e.g., `+`, `==`, `+=`, `&&`, `++`)
- Delimiters (فواصل - e.g., `(`, `;`, `{`)
- Comments (تعليقات - `#` style comments are skipped, not tokenized)

### Source Location Tracking

- Line and column tracking
- File position management
- Error location reporting

### Error Handling

- Returns a `BAA_TOKEN_ERROR` token when an issue is encountered (e.g., unexpected character, unterminated string/char literal, invalid escape sequence).
- The `lexeme` field of the error token contains a descriptive message (currently mostly in English, except for number parsing errors).
- Invalid character detection.
- Malformed token reporting (e.g., unterminated strings, invalid escape sequences).
- Note: Detailed Arabic error messages for general lexing errors are not yet implemented. Unicode validation is limited to assuming valid UTF-16LE input.

## Usage

### Initialization

To lex a string directly:

```c
// Assuming 'source_code' is a null-terminated wchar_t* string
BaaLexer* lexer = baa_create_lexer(source_code);
if (!lexer) { /* Handle allocation error */ }
```

To lex file content (Helper function provided):

```c
const wchar_t* file_path = L"your_file.baa";
wchar_t* file_content = baa_file_content(file_path);
if (!file_content) { /* Handle file reading error */ }

BaaLexer* lexer = baa_create_lexer(file_content);
if (!lexer) { /* Handle allocation error */ }

// Remember to free file_content later
// free(file_content);
```

### Token Processing

```c
BaaToken* token;
do {
    token = baa_lexer_next_token(lexer);
    if (!token) {
        // Handle potential memory allocation error during token creation
        fprintf(stderr, "Failed to create token.\n");
        break;
    }

    // Process the token based on its type
    wprintf(L"Token: %ls, Lexeme: '%ls', Line: %zu, Col: %zu\n",
            baa_token_type_to_string(token->type), // Assuming this function exists
            token->lexeme,
            token->line,
            token->column);

    if (token->type == BAA_TOKEN_ERROR) {
        // Handle lexer error (message is in token->lexeme)
        fwprintf(stderr, L"Lexer Error: %ls at Line %zu, Col %zu\n",
                 token->lexeme, token->line, token->column);
        // Depending on desired behavior, you might stop or try to continue
    }

    // Free the current token before getting the next one
    baa_free_token(token);

} while (token->type != BAA_TOKEN_EOF); // Stop when EOF is reached

// Clean up the lexer
baa_free_lexer(lexer);

// Free file content if loaded from file
// free(file_content);
```

*Note: The example assumes a `baa_token_type_to_string` function exists for printing token types.*
*Note: The functions `baa_get_lexer_error` and `baa_clear_lexer_error` are declared in the header but not implemented in `lexer.c`. Error reporting currently relies on `BAA_TOKEN_ERROR` tokens.*

## Implementation Details

### Character Classification

- `is_arabic_letter`, `is_arabic_digit`: Custom functions for Arabic characters.
- Standard C `isw...` functions (`iswalpha`, `iswdigit`, etc.).
- Whitespace and `#` comment skipping logic.

### State Management

- `source`, `start`, `current`, `line`, `column` fields in `BaaLexer`.
- Helper functions like `peek`, `peek_next`, `advance`, `match`.

### Memory Management

- `baa_create_lexer`, `baa_free_lexer`: Manage the `BaaLexer` struct.
- `make_token`, `scan_string`, `make_error_token`: Allocate `BaaToken` structs and potentially their `lexeme` buffers.
- `baa_free_token`: Frees `BaaToken` and its dynamically allocated `lexeme`.
- `baa_file_content`: Allocates buffer for file content (caller must free).
- `baa_parse_number`, `baa_free_number`: Manage `BaaNumber` struct and its `raw_text`.

## Future Improvements

- Enhanced support for Arabic dialects
- Performance optimizations for large files
- Advanced error recovery mechanisms
- Extended Unicode range support
