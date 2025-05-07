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
- Note: Standard C-style comments (`//`, `/* */`) are supported and skipped. Legacy `#` style comments are no longer supported by the lexer (preprocessor handles `#` for directives).

### Token Types

The lexer identifies the following categories of tokens:

- Keywords (كلمات مفتاحية)
- Identifiers (معرفات)
- Literals (قيم حرفية)
  - Numbers (أرقام - Scanned as `BAA_TOKEN_INT_LIT` or `BAA_TOKEN_FLOAT_LIT` based on syntax. Value parsing may occur later.)
  - Strings (نصوص - `BAA_TOKEN_STRING_LIT`)
  - Boolean (قيم منطقية - `BAA_TOKEN_BOOL_LIT`, scanned via `صحيح`/`خطأ` keywords)
  - Characters (حروف - `BAA_TOKEN_CHAR_LIT`, scanned via `'c'` syntax)
- Type Names (أسماء الأنواع - e.g., `عدد_صحيح`, `منطقي` scanned as specific `BAA_TOKEN_TYPE_*`)
- Operators (عوامل - e.g., `+`, `==`, `+=`, `&&`, `++`)
- Delimiters (فواصل - e.g., `(`, `;`, `{`)
- Comments (تعليقات - `//`, `/* */` style comments are skipped, not tokenized. `#` is handled by the preprocessor.)

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
    BAA_TOKEN_FLOAT_LIT,  // Float literal (Identified by lexer based on syntax like '.' or 'e')
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

## Numeric Literals

The lexer's `scan_number` function is responsible for identifying numeric literals and tokenizing them as either `BAA_TOKEN_INT_LIT` or `BAA_TOKEN_FLOAT_LIT`. It extracts the raw text of the number. The actual conversion of this text to a C numeric type (e.g., `long long`, `double`) is typically handled later by the parser or a dedicated utility like `baa_parse_number`.

The lexer supports the following formats for numeric literals:

### 1. Integer Literals (`BAA_TOKEN_INT_LIT`)
    - **Decimal Integers**: Sequences of Western digits (`0`-`9`) and/or Arabic-Indic digits (`٠`-`٩` / U+0660-U+0669).
        - Examples: `123`, `٤٢`, `0`, `٠`, `12٠3٤`
    - **Binary Integers**: Must start with `0b` or `0B`, followed by binary digits (`0` or `1`).
        - Examples: `0b1010`, `0B11001`
    - **Hexadecimal Integers**: Must start with `0x` or `0X`, followed by hexadecimal digits (`0`-`9`, `a`-`f`, `A`-`F`).
        - Examples: `0x1a3f`, `0XFF`, `0xDeadBeef`
    - **Underscores for Readability**: Single underscores (`_`) can be used as separators within the digits of any integer literal type. They cannot be consecutive, at the very beginning of the digit sequence (immediately after a prefix like `0x_`), or at the end of the number.
        - Examples: `1_000_000`, `0xFF_EC`, `0b1010_0101`, `١_٢٣٤_٥٦٧`

### 2. Floating-Point Literals (`BAA_TOKEN_FLOAT_LIT`)
    Float literals are identified if they contain a decimal point (`.` or `٫`) or an exponent part (`e` or `E`).
    - **Decimal Representation**:
        - Consist of an integer part, a decimal point, and a fractional part.
        - The decimal point can be a period `.` (U+002E) or an Arabic Decimal Separator `٫` (U+066B).
        - Digits in the integer and fractional parts can be Western (`0`-`9`) or Arabic-Indic (`٠`-`٩`).
        - Examples: `3.14`, `0.5`, `٣٫١٤`, `123.456`, `٠.٥`
        - Note: Literals starting with a dot (e.g., `.5`) or ending with a dot without a fractional part (e.g., `123.`) are tokenized differently by the lexer (e.g., `BAA_TOKEN_DOT` followed by `BAA_TOKEN_INT_LIT`, or `BAA_TOKEN_INT_LIT` followed by `BAA_TOKEN_DOT`). The parser would determine their validity as float literals.
    - **Scientific Notation**:
        - Can be appended to a decimal number (integer or fractional part).
        - Introduced by `e` or `E`, followed by an optional sign (`+` or `-`), and then one or more decimal digits (Western or Arabic-Indic).
        - Examples: `1.23e4`, `5.67E-3`, `42e+2`, `1e10`, `٣٫١٤e-2`, `12E+٠٥`
    - **Underscores for Readability**: Single underscores (`_`) can be used as separators within the integer part, fractional part, and exponent part of float literals, with the same restrictions as for integer literals.
        - Examples: `1_234.567_890`, `3.141_592e+1_0`

*(The following sections describe a utility `baa_parse_number` which is separate from the lexer's `scan_number` but complements it by performing the actual string-to-value conversion. The lexer's `scan_number` primarily focuses on identifying the token type and boundaries.)*

### BaaNumberType (Utility)

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

### Number Parsing Utility Functions (e.g., `baa_parse_number`)

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
- [x] Multiline strings (`"""..."""`): Newlines are preserved, escape sequences are processed.
- [x] Raw string literals (`خ"..."`, `خ"""..."""`): Newlines are preserved in multiline raw strings; no escape sequences are processed in any raw string.

## Comment Support

- Single line comments (`//` style - Skipped, not tokenized)
- Multi-line comments (`/* ... */` - Skipped, not tokenized)
- Note: Legacy `#` style comments are no longer supported by the lexer. The preprocessor handles lines starting with `#` for directives.
- Documentation comments (e.g., `/** ... */` or other syntax TBD)

## Usage

### Initialization

The primary way to initialize the lexer is using `baa_init_lexer`:

```c
#include "baa/lexer/lexer.h"
#include "baa/utils/utils.h" // For file reading

// ...

const wchar_t* filename = L"your_file.baa";
// Use a utility function (e.g., from utils.h) to read the file content
// Assuming baa_file_content (moved to utils) or similar exists:
wchar_t* source_code = baa_file_content(filename); // Or baa_read_file
if (!source_code) { /* Handle file reading error */ }

BaaLexer lexer; // Allocate on stack or heap as needed
baa_init_lexer(&lexer, source_code, filename); // Initialize the lexer state

// If using baa_create_lexer (allocates lexer struct on heap):
// BaaLexer* lexer_ptr = baa_create_lexer(source_code);
// if (!lexer_ptr) { /* Handle allocation error */ }
// // Use lexer_ptr->...
// baa_free_lexer(lexer_ptr); // Remember to free later

// Remember to free source_code when done
// free(source_code);

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
            baa_token_type_to_string(token->type), // Function exists
            token->lexeme ? token->lexeme : L"<NULL>", // Handle potential NULL lexeme
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

*Note: Error reporting relies on the `BAA_TOKEN_ERROR` token type; the `lexeme` field of this token contains the error message.*

## Implementation Details

### Lexer Operations

The lexer provides the following core functionality (declared in `baa/lexer/lexer.h`):

1.  **Initialization**:
    - `baa_init_lexer(BaaLexer* lexer, const wchar_t* source, const wchar_t* filename)`: Initializes a pre-allocated `BaaLexer` struct.
    - `baa_create_lexer(const wchar_t* source)`: Allocates and initializes a `BaaLexer` struct (caller must free with `baa_free_lexer`).
2.  **Token Scanning**:
    - `baa_lexer_next_token(BaaLexer* lexer)`: Gets the next token from the source. Returns a dynamically allocated `BaaToken` (caller must free with `baa_free_token`).
3.  **Token Utilities**:
    - `baa_token_type_to_string(BaaTokenType type)`: Converts a token type enum to its string representation.
    - `baa_token_is_keyword(BaaTokenType type)`: Checks if a token type is a keyword.
    - `baa_token_is_type(BaaTokenType type)`: Checks if a token type is a type name.
    - `baa_token_is_operator(BaaTokenType type)`: Checks if a token type is an operator.
4.  **Memory Management**:
    - `baa_free_lexer(BaaLexer* lexer)`: Frees a lexer allocated by `baa_create_lexer`.
    - `baa_free_token(BaaToken* token)`: Frees a token allocated by `baa_lexer_next_token` (including its lexeme if dynamically allocated).

*(Note: General file handling utilities like `baa_file_size` and `baa_file_content` have been moved to `baa/utils/utils.h`)*

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
- Implement documentation comments
