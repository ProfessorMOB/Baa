# Baa Language Lexer Structure Documentation

## Overview

The Baa language lexer is responsible for tokenizing source code written in the Baa programming language. It processes the raw source text and converts it into a stream of tokens that can be consumed by the parser. The lexer has special support for Arabic text and provides comprehensive token identification for the Baa language syntax.

## Lexer Components

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
    BAA_TOKEN_COMMENT,    // Comment

    // Literals
    BAA_TOKEN_IDENTIFIER, // Variable/function name
    BAA_TOKEN_INT_LIT,    // Integer literal
    BAA_TOKEN_FLOAT_LIT,  // Float literal
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

The lexer includes a specialized number parser that can handle:

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

The lexer supports various number formats:

1. **Integer Literals**:
   - Regular decimal integers: `123`, `42`, `0`
   - Binary integers (with `0b` or `0B` prefix): `0b1010`, `0B1100`
   - Hexadecimal integers (with `0x` or `0X` prefix): `0x1a3f`, `0XFF`
   - Support for Arabic-Indic digits: `١٢٣`, `٤٢`

2. **Floating-Point Literals**:
   - Regular decimal floats: `3.14`, `0.5`
   - Support for Arabic decimal separator: `3٫14`
   - Support for Arabic-Indic digits: `٣٫١٤`

3. **Scientific Notation**:
   - Standard form with `e` or `E`: `1.23e4`, `5.67e-3`, `42E2`
   - Support for Arabic-Indic digits: `١٫٢٣e٤`
   - Optional decimal point: `5e3` (equivalent to `5000`)
   - Optional sign in exponent: `1.2e+10`, `3.4e-5`

### Number Parsing Functions

The number parser provides these main functions:

- `baa_parse_number`: Parse a string as a number, detecting the format automatically
- `baa_free_number`: Free a number structure
- `baa_number_error_message`: Get error message for number parsing
- `baa_is_digit`: Check if a character is a digit (including Arabic-Indic digits)
- `baa_is_hex_digit`: Check if a character is a hexadecimal digit
- `baa_is_arabic_digit`: Check if a character is an Arabic-Indic digit

## Lexer Operations

The lexer provides the following core functionality:

1. **Lexer Initialization**:
   - `baa_create_lexer`: Create a new lexer for a source string
   - `baa_init_lexer`: Initialize an existing lexer with source code

2. **Token Scanning**:
   - `baa_scan_token`: Scan and return the next token
   - `baa_lexer_next_token`: Get the next token from the lexer

3. **Token Utilities**:
   - `baa_token_is_keyword`: Check if a token type is a keyword
   - `baa_token_is_type`: Check if a token type is a type identifier
   - `baa_token_is_operator`: Check if a token type is an operator
   - `baa_token_type_to_string`: Convert token type to string representation

4. **Number Parsing**:
   - `baa_parse_number`: Parse a string as a number
   - `baa_free_number`: Free a number structure
   - `baa_number_error_message`: Get error message for number parsing

5. **File Handling**:
   - `baa_file_size`: Get the size of a file
   - `baa_file_content`: Read the content of a file

6. **Memory Management**:
   - `baa_free_lexer`: Free lexer resources
   - `baa_free_token`: Free token resources

## Keyword, Type, and Boolean Literal Recognition

The lexer supports boolean literals in both Arabic and English:

1. **Arabic Boolean Literals**:
   - `صحيح` (True)
   - `خطأ` (False)

2. **Type Name**:
   - `منطقي` (Boolean)

The lexer now recognizes the following as distinct token types by checking against its internal keyword list after scanning a potential identifier:
**Keywords**: `متغير` (VAR), `ثابت` (CONST), in addition to control flow keywords like `دالة`, `إذا`, etc.
**Type Names**: `عدد_صحيح`, `عدد_حقيقي`, `حرف`, `فراغ`, `منطقي`. These are tokenized with their respective `BAA_TOKEN_TYPE_*` types.
**Boolean Literals**: `صحيح` (True) and `خطأ` (False). These are tokenized as `BAA_TOKEN_BOOL_LIT`.
 Boolean literals are tokenized with the `BAA_TOKEN_BOOL_LIT` token type, and they can be used in expressions, variable declarations, and other places where a boolean value is expected.

Boolean literals are tokenized with the `BAA_TOKEN_BOOL_LIT` token type, and they can be used in expressions, variable declarations, and other places where a boolean value is expected.

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

The lexer provides support for a wide range of operators:

1. **Basic Arithmetic Operators**:
   - Addition (`+`)
   - Subtraction (`-`)
   - Multiplication (`*`)
   - Division (`/`)
   - Modulo (`%`)

2. **Comparison Operators**:
   - Equal (`==`)
   - Not Equal (`!=`)
   - Less Than (`<`)
   - Less Than or Equal (`<=`)
   - Greater Than (`>`)
   - Greater Than or Equal (`>=`)

3. **Logical Operators**:
   - Logical AND (`&&`)
   - Logical OR (`||`)
   - Logical NOT (`!`)

4. **Assignment Operators**:
   - Basic Assignment (`=`)
   - Add and Assign (`+=`)
   - Subtract and Assign (`-=`)
   - Multiply and Assign (`*=`)
   - Divide and Assign (`/=`)
   - Modulo and Assign (`%=`)

5. **Increment/Decrement Operators**:
   - Increment (`++`)
   - Decrement (`--`)

These operators are tokenized with corresponding token types (e.g., `BAA_TOKEN_PLUS`, `BAA_TOKEN_INCREMENT`, etc.).

## String and Character Handling

The lexer provides comprehensive support for string and character literals:

1. **String Literals**:
   - Delimited by double quotes (`"..."`)
   - Support for basic escape sequences:
     - `\n` - Newline
     - `\t` - Tab
     - `\\` - Backslash
     - `\"` - Double quote
     - Note: Unicode escape sequences (`\uXXXX`) are not currently implemented.

2. **Character Literals**:
   - Delimited by single quotes (`'...'`), e.g., `'a'`, `'ح'`, `'\n'`.
   - Support for basic escape sequences: `\n`, `\t`, `\\`, `\'`, `\r`, `\0`.
   - Tokenized as `BAA_TOKEN_CHAR_LIT`. Note: Unicode escapes (`\uXXXX`) are not implemented. Empty (`''`) and multi-character literals (`'ab'`) are treated as errors.

Strings are tokenized as `BAA_TOKEN_STRING_LIT`, characters as `BAA_TOKEN_CHAR_LIT`.

## Comment Support

The lexer currently supports one style of single-line comments:

1. **Hash Comments**:
   - Comments start with `#` and continue to the end of the line.

The lexer skips these comments; they are not turned into `BAA_TOKEN_COMMENT` tokens. Other comment styles (`//`, `/* ... */`, `٭`) are not currently supported by the implementation.

## Arabic Language Support

The Baa lexer includes special support for Arabic text:

1. **Arabic Character Recognition**:
   - `is_arabic_letter`: Check if a character is an Arabic letter
   - `baa_is_arabic_digit`: Check if a character is an Arabic-Indic digit

2. **Arabic Number Parsing**:
   - Support for Arabic-Indic digits (٠١٢٣٤٥٦٧٨٩)
   - Support for Arabic decimal separator (٫)

3. **Arabic Keywords**:
   - Recognition of Arabic programming keywords
   - Support for Arabic identifiers

## Internal Lexer Helpers

The lexer uses several internal helper functions:

1. **Character Classification**:
   - `is_arabic_letter`: Check if a character is an Arabic letter (used in identifier scanning)
   - `is_arabic_digit`: Check if a character is an Arabic-Indic digit (used in identifier and number scanning)
   - `iswalnum`, `iswalpha`, `iswdigit`: Standard C functions used for character classification.
   - Logic for identifying identifier start/part characters is embedded within `scan_identifier`.

2. **Lexer Navigation**:
   - `peek`: Look at the current character without advancing
   - `peek_next`: Look at the next character without advancing
   - `advance`: Consume the current character and advance
   - `skip_whitespace`: Skip whitespace characters and single-line `#` comments.

3. **Token Creation**:
   - `make_token`: Create a regular token, copying the lexeme from the source.
   - `make_error_token`: Create an error token with a specific message.
