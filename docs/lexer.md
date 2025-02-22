# Baa Lexer Documentation

## Overview
The Baa lexer is designed to process source code written in Arabic, supporting UTF-16LE encoding and bidirectional text handling. It provides robust tokenization for Arabic identifiers, keywords, and comments while maintaining precise source location tracking.

## Features

### Arabic Language Support
- Full support for Arabic characters (Unicode range 0x0600-0x06FF)
- Bidirectional text handling
- Arabic identifier validation
- Support for Arabic comments and string literals

### Token Types
- Keywords (كلمات مفتاحية)
- Identifiers (معرفات)
- Literals (قيم حرفية)
  - Numbers (أرقام)
  - Strings (نصوص)
  - Boolean (قيم منطقية)
- Operators (عوامل)
- Delimiters (فواصل)
- Comments (تعليقات)

### Source Location Tracking
- Line and column tracking
- File position management
- Error location reporting

### Error Handling
- Detailed error messages in Arabic
- Invalid character detection
- Malformed token reporting
- Unicode validation

## Usage

### Initialization
```c
Lexer* lexer = baa_create_lexer(source_code, file_name);
```

### Token Processing
```c
Token* token = baa_next_token(lexer);
```

### Error Handling
```c
BaaError error = baa_get_lexer_error(lexer);
if (error != BAA_ERROR_NONE) {
    const wchar_t* error_message = baa_get_error_message(error);
    // Handle error
}
```

## Implementation Details

### Character Classification
- Arabic letter detection
- Number detection
- Whitespace handling
- Special character processing

### State Management
- Current position tracking
- Line and column counting
- Token start position recording

### Memory Management
- Token allocation and deallocation
- String literal buffer management
- Error message handling

## Future Improvements
- Enhanced support for Arabic dialects
- Performance optimizations for large files
- Advanced error recovery mechanisms
- Extended Unicode range support