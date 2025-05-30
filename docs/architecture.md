# Baa Language Architecture

## Overview

Baa (باء) is designed with a modular architecture that separates concerns into distinct components. Each component is responsible for a specific aspect of the compilation process, making the system maintainable and extensible.

## Core Components

### 0. Preprocessor (المعالج المسبق)

The initial stage that processes the source file before tokenization:

- **Directive Handling**: Processes directives like `#تضمين` (include) and `#تعريف` (define).
- **File Inclusion**: Merges included files into a single stream.
- **Macro Expansion**: Substitutes defined object-like and function-like macros. Supports stringification (`#`) and token pasting (`##`). Detects recursive expansion.
- **Input Encoding**: Expects UTF-16LE input files (checks BOM).
- **Output**: Produces a single `wchar_t*` stream for the lexer.
- **Circular Include Detection**: Prevents infinite include loops.
- **Conditional Compilation**: Handles `#إذا`, `#إذا_عرف`, `#إذا_لم_يعرف`, `#وإلا_إذا`, `#إلا`, `#نهاية_إذا`. Evaluates constant integer expressions for `#إذا`/`#وإلا_إذا` (supports arithmetic, comparison, logical operators, `معرف()`; excludes bitwise ops).
- **Error Reporting**: Reports errors with file path and line number context.
- **Features & Status:**
  - Implemented as a separate stage.
  - Handles `#تضمين` (relative and standard paths).
  - Handles `#تعريف` (object-like and function-like macros, including `#`, `##`, and **rescanning of expansion results**).
  - Handles `#الغاء_تعريف` to undefine macros.
  - Handles conditional compilation (`#إذا_عرف`, `#إذا_لم_يعرف`, `#إلا`, `#نهاية_إذا`, `#إذا`, `#وإلا_إذا`) with expression evaluation (using `معرف` for `defined`; excludes bitwise ops).
  - Detects circular includes and recursive macro expansion (expansion stack prevents direct self-recursion during a single expansion's rescan).
  - Enforces UTF-16LE input.
  - Provides error messages with file/line context.
  - Predefined macros implemented: `__الملف__`, `__السطر__` (as integer), `__التاريخ__`, `__الوقت__`.
  - *Planned:* Bitwise operators in conditional expressions, remaining predefined macros (`__الدالة__`, `__إصدار_المعيار_باء__`), Variadic Macros (`وسائط_إضافية`, `__وسائط_متغيرة__`), other standard directives (`#خطأ`, `#تحذير`, `#سطر`, `#براغما`, `أمر_براغما`), UTF-8 input support, full macro expansion in conditional expressions.

### 1. Lexer

The lexical analyzer responsible for tokenizing source code. It has a modular structure:

- `lexer.c`: Core dispatch logic (`baa_lexer_next_token`) and helper functions.
- `token_scanners.c`: Implements specific scanning functions for identifiers, numbers, strings, and characters.
- `lexer_char_utils.c`: Provides character classification utilities (e.g., for Arabic letters, digits).
- **Token Generation**: Converts source text (output from preprocessor) into a stream of tokens.
- **Unicode Support**: Full support for Arabic characters in identifiers, literals, and keywords. Recognizes Arabic-Indic digits.
- **Source Tracking**: Accurate line and column tracking for tokens and errors.
- **Error Detection**: Identifies lexical errors (e.g., unexpected characters, unterminated literals, invalid escapes) and provides diagnostic messages.
- **Whitespace and Newlines**: Tokenizes spaces/tabs (`BAA_TOKEN_WHITESPACE`) and newlines (`BAA_TOKEN_NEWLINE`) separately.
- **Comment Handling**: Tokenizes single-line (`//` -> `BAA_TOKEN_SINGLE_LINE_COMMENT`), multi-line (`/*...*/` -> `BAA_TOKEN_MULTI_LINE_COMMENT`), and documentation comments (`/**...*/` -> `BAA_TOKEN_DOC_COMMENT`). Lexemes for comments contain the content excluding delimiters.
- **Numeric Literal Recognition**:
  - Identifies various number formats: integers (decimal, binary `0b`/`0B`, hexadecimal `0x`/`0X`), floats (using `.` or `٫` as decimal separator), and scientific notation (using `أ` as exponent marker).
  - Supports Arabic-Indic digits (`٠`-`٩`) and Western digits (`0`-`9`) within all parts of numbers.
  - Supports underscores (`_`) as separators for readability in numbers.
  - (Planned: Arabic literal suffixes like `غ`, `ط`, `طط`, `ح`).
  - The lexer's `scan_number` function (in `token_scanners.c`) handles the syntactic recognition and extracts the raw lexeme. The separate `number_parser.c` utility can be used later for converting these lexemes to actual numeric values.
- **String/Char Literals**: Handles string (`"..."`) and character (`'...'`) literals. Currently implements standard C escapes. *Planned: Support for Baa-specific Arabic escape sequences (e.g., `\س` for newline, `\م` for tab, `\يXXXX` for Unicode) while retaining `\` as the escape character.*
- **Features & Status:**
  - Core lexer functionality and modular structure are implemented.
  - UTF-16LE encoding is processed (input from preprocessor).
  - Robust token handling for keywords, identifiers, operators, and various literal types.
  - String and character literal support with escapes.
  - Source position tracking.
  - Error token generation.
  - *Planned:* Advanced error recovery, Baa-specific Arabic escapes, Arabic float exponent/suffix.

### 2. Parser

Transforms tokens into a structured AST:

- **Critical:** Re-implement Parser and AST based on new designs.
- Enhance Preprocessor and Lexer with remaining planned features.
- Begin foundational Code Generation work once Parser/AST are stable.

- **Features & Status:**
  - The previous parser implementation was removed.
  - A new parser is currently under design and development.
  - Key features like expression parsing, statement handling, and declaration parsing are planned for the new implementation.

### 3. Abstract Syntax Tree (AST)

The AST module provides the foundation for representing code structure using a unified `BaaNode` approach.

- **Status**: New Design v2 (Unified `BaaNode`) - Implementation in progress. Old AST removed. (Reflects v0.1.18.0)
- **Structure**:
  - Unified `BaaNode` with `BaaNodeKind kind`, `BaaSourceSpan span`, and `void* data`.
  - Specific data structs (e.g., `BaaLiteralExprData`) for each node kind.
  - Type-safe accessor macros planned for accessing specific data.
- **Memory Management**: `baa_ast_new_node()` for base creation, `baa_ast_free_node()` for recursive freeing. Specific node types have dedicated creation functions and internal data-freeing helpers.
- **Node Types**: Being progressively defined for all language constructs (expressions, statements, declarations, types). Literals implemented.
- **Traversal**: Visitor pattern planned for AST traversal.
- *Refer to `docs/AST.md` and `docs/AST_ROADMAP.md` for details on the new design.*

### 4. Type System

A robust type system that supports both C compatibility and Arabic type names:

- **Basic Types**: عدد_صحيح (int), عدد_حقيقي (float), حرف (char)
- **Type Checking**: Static type checking with detailed error messages
- **Type Conversion**: Implicit and explicit type conversion rules
- **Type Validation**: Compile-time type validation
- **Features & Status:**
  - Complete implementation
  - Basic types (int, float, char, etc.)
  - Boolean type (منطقي) with literals (صحيح/خطأ)
  - Type checking and conversion rules
  - Type initialization support
  - Arabic type names support
  - Array type support

### 5. Operators

Operator system with full Arabic support:

- **Arithmetic**: جمع (+), طرح (-), ضرب (*), قسمة (/)
- **Comparison**: يساوي (==), أكبر_من (>), أصغر_من (<)
- **Logical**: Uses symbols `&&` (و - AND), `||` (أو - OR), `!` (ليس - NOT).
- **Precedence**: Clear operator precedence rules
- **Extensibility**: Easy addition of new operators
- **Features & Status:**
  - Full implementation
  - Operator precedence table defined
  - Binary and unary operators
  - Assignment operators (=, +=, -=, *=, /=, %=)
  - Increment/decrement operators (++, --)
  - Comparison operators
  - Arabic operator keyword support (planned/partial)

### 6. Control Flow

Control structures with Arabic keywords:

- **Conditions**: `إذا` (if), `وإلا` (else)
- **Loops**: `طالما` (while), `لكل` (for)
- **Functions**: (Uses C-style declarations)
- **Return**: `إرجع` (return)
- **Status**: Lexer supports keywords. Parser and AST support are planned for the new design.
- **Features & Status:**
- Lexer tokenizes control flow keywords.
- Parser rules and AST node definitions for control flow statements are part of the new design and are planned.
- Old AST/Parser for control flow was removed.

### 7. Utils

Utility functions and support features:

- **Error Handling**: Detailed Arabic error messages (planned)
- **Memory Management**: Safe memory allocation and tracking
- **String Handling**: UTF-16LE wide character string operations
- **File Operations**: Source file handling with Arabic support (UTF-16LE reading)
- **Features & Status:**

### 8. Code Generation (Planned)

Transforms the AST into executable code or intermediate representation.

- **Status**: Basic LLVM integration stubs exist. Blocked by Parser/AST re-implementation.
- **Features (Planned):**
  - LLVM IR generation from the new AST.
  - Optimization passes, debug information, source mapping.

## Build System

The build system is based on CMake and provides:

- **Component Management**: Each component built as a separate library
- **Test Integration**: Automated testing for each component
- **Cross-Platform**: Support for multiple operating systems
- **Configuration**: Flexible build configuration options

## Testing Framework

Comprehensive testing infrastructure:

- **Unit Tests**: Tests for individual components
- **Integration Tests**: Tests for component interaction
- **Arabic Test Cases**: Test cases with Arabic input
- **Coverage**: Test coverage tracking

## Error Handling

Robust error handling system:

- **Error Types**: Syntax, type, memory, and runtime errors
- **Arabic Messages**: Error messages in Arabic (planned for some areas)
- **Error Recovery**: Graceful error recovery where possible
- **Debug Info**: Detailed debugging information

## Memory Management

Memory management strategy:

- **Allocation Tracking**: Track all memory allocations
- **Garbage Collection**: Future support for garbage collection
- **Memory Safety**: Checks for memory leaks and buffer overflows
- **Resource Cleanup**: Automatic resource cleanup

## Future Extensions

Planned enhancements:

- **Code Generation**: LLVM-based code generator (see section above)
- **Optimizer**: Code optimization passes
- **IDE Integration**: Support for code editors and IDEs
