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
    BAA_TOKEN_FOR,        // لكل
    BAA_TOKEN_RETURN,     // إرجع
    BAA_TOKEN_BREAK,      // توقف
    BAA_TOKEN_CONTINUE,   // أكمل

    // Types
    BAA_TOKEN_TYPE_INT,   // عدد_صحيح
    BAA_TOKEN_TYPE_FLOAT, // عدد_حقيقي
    BAA_TOKEN_TYPE_CHAR,  // حرف
    BAA_TOKEN_TYPE_VOID,  // فراغ
    BAA_TOKEN_TYPE_BOOL,  // منطقي

    // Operators
    BAA_TOKEN_PLUS,       // +
    BAA_TOKEN_MINUS,      // -
    BAA_TOKEN_STAR,       // *
    BAA_TOKEN_SLASH,      // /
    BAA_TOKEN_PERCENT,    // %
    BAA_TOKEN_EQUAL,      // =
    BAA_TOKEN_EQUAL_EQUAL,// ==
    BAA_TOKEN_BANG,       // !
    BAA_TOKEN_BANG_EQUAL, // !=
    BAA_TOKEN_LESS,       // <
    BAA_TOKEN_LESS_EQUAL, // <=
    BAA_TOKEN_GREATER,    // >
    BAA_TOKEN_GREATER_EQUAL, // >=
    BAA_TOKEN_AND,        // &&
    BAA_TOKEN_OR,         // ||
    
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

## Boolean Literals

The lexer supports boolean literals in both Arabic and English:

1. **Arabic Boolean Literals**:
   - `صحيح` (True)
   - `خطأ` (False)

2. **Type Name**:
   - `منطقي` (Boolean)

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

## String Handling

The lexer provides comprehensive support for string and character literals:

1. **String Literals**:
   - Delimited by double quotes (`"..."`)
   - Support for escape sequences:
     - `\n` - Newline
     - `\t` - Tab
     - `\r` - Carriage return
     - `\\` - Backslash
     - `\"` - Double quote
     - `\'` - Single quote
     - `\0` - Null character
     - `\uXXXX` - Unicode escape sequence (4 hex digits)

2. **Character Literals**:
   - Delimited by single quotes (`'...'`)
   - Support for the same escape sequences as string literals

Strings are tokenized as `BAA_TOKEN_STRING_LIT` and characters as `BAA_TOKEN_CHAR_LIT`.

## Comment Support

The lexer supports multiple comment styles:

1. **C-style Comments**:
   - Single-line comments with `//` 
   - Multi-line comments with `/* ... */`

2. **Arabic-Style Comments**:
   - Single-line comments with `#` 
   - Single-line comments with `٭` (Arabic asterisk)

Comments are tokenized as `BAA_TOKEN_COMMENT` and can be used for documentation or temporarily removing code.

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
   - `is_whitespace`: Check if a character is whitespace
   - `is_digit`: Check if a character is a digit (including Arabic digits)
   - `is_identifier_start`: Check if a character can start an identifier
   - `is_identifier_part`: Check if a character can be part of an identifier

2. **Lexer Navigation**:
   - `peek`: Look at the current character without advancing
   - `peek_next`: Look at the next character without advancing
   - `advance`: Consume the current character and advance
   - `skip_whitespace`: Skip whitespace characters

3. **Token Creation**:
   - `make_token`: Create a token of a specific type
   - `error_token`: Create an error token with a message
