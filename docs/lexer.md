# Baa Lexer Documentation

## Overview

The Baa lexer processes source code written in the Baa language, which includes strong support for Arabic. **Note: The lexer operates on the source code *after* it has been processed by the Baa Preprocessor, which handles directives like `#تضمين`.** It reads the resulting (potentially combined) source, assumed to be encoded in UTF-16LE, and uses wide characters (`wchar_t`) internally. The lexer tokenizes Arabic identifiers, keywords, and literals, while maintaining precise source location tracking (line and column numbers). It converts the processed text into a stream of tokens that can be consumed by the parser.

## Features

### Arabic Language Support

- Recognizes Arabic letters from multiple Unicode ranges (0x0600-0x06FF, FB50-FDFF, FE70-FEFF) for identifiers.
- Recognizes Arabic-Indic digits (٠-٩) in identifiers and number literals.
- Recognizes Arabic keywords (e.g., `دالة`, `إذا`).
- Supports Arabic characters within string literals.
- Note: Bidirectional text display is dependent on the terminal/editor, not explicitly managed by the lexer logic itself.
- Note: Standard C-style comments (`//`, `/* */`) and legacy `#` comments are supported.

### Token Types

The lexer identifies the following categories of tokens:

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
- Comments (تعليقات - `//`, `/* */`, `#` style comments are skipped, not tokenized)

### Source Location Tracking

- Line and column tracking
- File position management
- Error location reporting

### Error Handling

- Returns a `BAA_TOKEN_ERROR` token when an issue is encountered (e.g., unexpected character, unterminated string/char/comment literal, invalid escape sequence).
- The `lexeme` field of the error token contains a descriptive message (currently mostly in English, except for number parsing errors).
- Invalid character detection.
- Malformed token reporting (e.g., unterminated strings, invalid escape sequences).
- Note: Detailed Arabic error messages for general lexing errors are not yet implemented. Unicode validation is limited to assuming valid UTF-16LE input.

## Lexer Structure

### BaaLexer

The main lexer structure holds the current state of the lexical analysis process:

```c
typedef struct {
    const wchar_t* source;   // Source code being lexed
    size_t start;           // Start of current token
    size_t current;         // Current position in source
    size_t line;           // Current line number
    size_t column;         // Current column number
} BaaLexer;
```

### BaaToken

Represents a single token in the source code:

```c
typedef struct {
    BaaTokenType type;       // Type of the token
    const wchar_t* lexeme;   // The actual text of the token
    size_t length;           // Length of the lexeme
    size_t line;            // Line number in source
    size_t column;          // Column number in source
} BaaToken;
```

### BaaTokenType

Enumeration of all possible token types in the Baa language:

```c
typedef enum {
    // Special tokens
    BAA_TOKEN_EOF,        // End of file
    BAA_TOKEN_ERROR,      // Error token
    BAA_TOKEN_UNKNOWN,    // Unknown token
    BAA_TOKEN_COMMENT,    // Comment (Defined, but not currently produced by scanner)

    // Literals
    BAA_TOKEN_IDENTIFIER, // Variable/function name
    BAA_TOKEN_INT_LIT,    // Integer literal
    BAA_TOKEN_FLOAT_LIT,  // Float literal (Defined, but lexer produces INT_LIT initially)
    BAA_TOKEN_CHAR_LIT,   // Character literal
    BAA_TOKEN_STRING_LIT, // String literal
    BAA_TOKEN_BOOL_LIT,   // Boolean literal (صحيح/خطأ - true/false)

    // Keywords
    BAA_TOKEN_FUNC,       // دالة
    BAA_TOKEN_VAR,        // متغير
    BAA_TOKEN_CONST,      // ثابت
    BAA_TOKEN_IF,         // إذا
    BAA_TOKEN_ELSE,       // وإلا
    BAA_TOKEN_WHILE,      // طالما
    BAA_TOKEN_FOR,        // لكل
    BAA_TOKEN_DO,         // افعل
    BAA_TOKEN_CASE,       // حالة
    BAA_TOKEN_SWITCH,     // اختر
    BAA_TOKEN_RETURN,     // إرجع
    BAA_TOKEN_BREAK,      // توقف
    BAA_TOKEN_CONTINUE,   // أكمل

    // Types
    BAA_TOKEN_TYPE_INT,   // عدد_صحيح
    BAA_TOKEN_TYPE_FLOAT, // عدد_حقيقي
    BAA_TOKEN_TYPE_CHAR,  // حرف
    BAA_TOKEN_TYPE_VOID,  // فراغ
    BAA_TOKEN_TYPE_BOOL,  // منطقي

    // Operators (Arithmetic, Comparison, Logical)
    BAA_TOKEN_PLUS,       // +
    BAA_TOKEN_MINUS,      // -
    BAA_TOKEN_STAR,       // *
    BAA_TOKEN_SLASH,      // /
    BAA_TOKEN_PERCENT,    // %
    BAA_TOKEN_EQUAL_EQUAL,// ==
    BAA_TOKEN_BANG,       // !
    BAA_TOKEN_BANG_EQUAL, // !=
    BAA_TOKEN_LESS,       // <
    BAA_TOKEN_LESS_EQUAL, // <=
    BAA_TOKEN_GREATER,    // >
    BAA_TOKEN_GREATER_EQUAL, // >=
    BAA_TOKEN_AND,        // &&
    BAA_TOKEN_OR,         // ||
    BAA_TOKEN_EQUAL,      // = (Assignment)

    // Compound assignment operators
    BAA_TOKEN_PLUS_EQUAL,  // +=
    BAA_TOKEN_MINUS_EQUAL, // -=
    BAA_TOKEN_STAR_EQUAL,  // *=
    BAA_TOKEN_SLASH_EQUAL, // /=
    BAA_TOKEN_PERCENT_EQUAL, // %=

    // Increment/decrement operators
    BAA_TOKEN_INCREMENT,   // ++
    BAA_TOKEN_DECREMENT,   // --

    // Delimiters
    BAA_TOKEN_LPAREN,     // (
    BAA_TOKEN_RPAREN,     // )
    BAA_TOKEN_LBRACE,     // {
    BAA_TOKEN_RBRACE,     // }
    BAA_TOKEN_LBRACKET,   // [
    BAA_TOKEN_RBRACKET,   // ]
    BAA_TOKEN_COMMA,      // ,
    BAA_TOKEN_DOT,        // .
    BAA_TOKEN_SEMICOLON,  // ;
    BAA_TOKEN_COLON,      // :
} BaaTokenType;
```

## Number Parsing

The lexer includes a specialized number parser (`number_parser.c`) that handles various formats after an initial `BAA_TOKEN_INT_LIT` is scanned.

### BaaNumberType

```c
typedef enum {
    BAA_NUM_INTEGER,     // عدد_صحيح - Integer number
    BAA_NUM_DECIMAL,     // عدد_عشري - Decimal number
    BAA_NUM_SCIENTIFIC   // عدد_علمي - Scientific notation
} BaaNumberType;
```

### BaaNumber

```c
typedef struct {
    BaaNumberType type;
    union {
        long long int_value;
        double decimal_value;
    };
    const wchar_t* raw_text;  // Original text representation
    size_t text_length;
} BaaNumber;
```

### BaaNumberError

```c
typedef enum {
    BAA_NUM_SUCCESS = 0,
    BAA_NUM_OVERFLOW,        // Number too large
    BAA_NUM_INVALID_CHAR,    // Invalid character in number
    BAA_NUM_MULTIPLE_DOTS,   // Multiple decimal points
    BAA_NUM_INVALID_FORMAT,  // Invalid number format
    BAA_NUM_MEMORY_ERROR     // Memory allocation error
} BaaNumberError;
```

### Number Format Support

The number parser supports:

1.  **Integer Literals**:
    -   Regular decimal integers: `123`, `42`, `0`
    -   Binary integers (with `0b` or `0B` prefix): `0b1010`, `0B1100`
    -   Hexadecimal integers (with `0x` or `0X` prefix): `0x1a3f`, `0XFF`
    -   Support for Arabic-Indic digits: `١٢٣`, `٤٢`
    -   Underscores for readability (e.g., `1_000_000`, `0xFF_FF`, `0b1010_1100`) - *[Implemented]*

2.  **Floating-Point Literals**:
    -   Regular decimal floats: `3.14`, `0.5`
    -   Support for Arabic decimal separator: `3٫14`
    -   Support for Arabic-Indic digits: `٣٫١٤`
    -   Underscores for readability (e.g., `1_234.56_78`) - *[Implemented]*

3.  **Scientific Notation**:
    -   Standard form with `e` or `E`: `1.23e4`, `5.67e-3`, `42E2`
    -   Support for Arabic-Indic digits: `١٫٢٣e٤`
    -   Optional decimal point: `5e3` (equivalent to `5000`)
    -   Optional sign in exponent: `1.2e+10`, `3.4e-5`
    -   Underscores for readability (e.g., `1.23e+1_0`) - *[Implemented]*

### Number Parsing Functions

- `baa_parse_number`: Parse a string as a number, detecting the format automatically.
- `baa_free_number`: Free a number structure.
- `baa_number_error_message`: Get error message for number parsing (provides Arabic messages).
- `baa_is_digit`: Check if a character is a digit (including Arabic-Indic digits).
- `baa_is_hex_digit`: Check if a character is a hexadecimal digit.
- `baa_is_arabic_digit`: Check if a character is an Arabic-Indic digit.

## Keyword, Type, and Boolean Literal Recognition

The lexer recognizes keywords, type names, and boolean literals by checking scanned identifiers against an internal list:

- **Keywords**: `متغير` (VAR), `ثابت` (CONST), `دالة`, `إذا`, etc. are tokenized with their specific types (e.g., `BAA_TOKEN_VAR`).
- **Type Names**: `عدد_صحيح`, `عدد_حقيقي`, `حرف`, `فراغ`, `منطقي` are tokenized with their respective `BAA_TOKEN_TYPE_*` types.
- **Boolean Literals**: `صحيح` (True) and `خطأ` (False) are tokenized as `BAA_TOKEN_BOOL_LIT`.

Example:

```baa
متغير x: منطقي = صحيح؛
إذا (x == صحيح) {
    // ...
} وإلا {
    // ...
}
```

## Operator Support

The lexer supports a wide range of operators, tokenized with corresponding types (e.g., `BAA_TOKEN_PLUS`, `BAA_TOKEN_INCREMENT`):

- **Arithmetic**: `+`, `-`, `*`, `/`, `%`
- **Comparison**: `==`, `!=`, `<`, `<=`, `>`, `>=`
- **Logical**: `&&`, `||`, `!`
- **Assignment**: `=`, `+=`, `-=`, `*=`, `/=`, `%=`
- **Increment/Decrement**: `++`, `--`

## String and Character Handling

- [x] Basic string literal support (double quotes)
- [x] Character literal support (single quotes)
- [x] Basic escape sequences in strings/chars (\n, \t, \\, \", \', \r, \0)
- [x] Unicode escape sequences (\uXXXX)
- [ ] Multiline strings
- [ ] Raw string literals

## Comment Support

- Single line comments (`#` style - Skipped, not tokenized)
- Single line comments (`//` style - Skipped, not tokenized)
- Multi-line comments (`/* ... */` - Skipped, not tokenized)
- Documentation comments (e.g., `/** ... */` or other syntax TBD)

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

### Lexer Operations

The lexer provides the following core functionality:

1.  **Initialization**: `baa_create_lexer`, `baa_init_lexer`
2.  **Token Scanning**: `baa_scan_token`, `baa_lexer_next_token`
3.  **Token Utilities**: `baa_token_is_keyword`, `baa_token_is_type`, `baa_token_is_operator`, `baa_token_type_to_string`
4.  **File Handling**: `baa_file_size`, `baa_file_content`
5.  **Memory Management**: `baa_free_lexer`, `baa_free_token`

### Internal Lexer Helpers

The lexer uses several internal helper functions:

1.  **Character Classification**: `is_arabic_letter`, `is_arabic_digit`, standard C `isw...` functions.
2.  **Lexer Navigation**: `peek`, `peek_next`, `advance`, `match`, `skip_whitespace`.
3.  **Token Creation**: `make_token`, `make_error_token`.

### State Management

- `source`, `start`, `current`, `line`, `column` fields in `BaaLexer`.

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
- Implement multi-line comments (`/* ... */`)
- Implement remaining escape sequences (`\uXXXX`)

*   **Handle Basic Escape Sequences:**
    *   String literals: `\n`, `\t`, `\"`, `\\`, `\r`, `\0`. *[Done]*
    *   Character literals: `\n`, `\t`, `\'`, `\\`, `\r`, `\0`. *[Done]*
