# Baa Language Components

## Core Components

### 1. AST (Abstract Syntax Tree)

- Full implementation
- Node creation and management
- Tree traversal utilities
- Memory management for nodes
- Support for all basic constructs
- Standardized structure with expressions, statements, and declarations
- Memory ownership model with clear allocation/deallocation rules
- Enhanced function parameter handling with optional and rest parameters
- Control flow statements (if-else, while, for, switch/case)
- Break and continue statements
- Array creation and indexing
- Full type system integration

### 2. Type System

- Complete implementation
- Basic types (int, float, char, etc.)
- Boolean type (منطقي) with literals (صحيح/خطأ)
- Type checking and conversion
- Type initialization
- Arabic type names support
- Array type support

### 3. Operators

- Full implementation
- Operator precedence table
- Binary and unary operators
- Assignment operators (=, +=, -=, *=, /=, %=)
- Increment/decrement operators (++, --)
- Comparison operators
- Arabic operator support

### 4. Control Flow

- Complete implementation
- If statements
- While loops
- For loops
- Switch/case statements
- Break and continue statements
- Return statements
- Basic blocks
- Condition handling

### 5. Utils

- Error handling with Arabic messages (planned)
- Memory management
- String utilities
- File handling
- UTF-16LE support

### 6. Parser

- Implemented with recursive descent approach
- Features:
  - Expression parsing with precedence
  - Statement parsing for basic control flow
  - Declaration parsing for variables and functions
  - Basic type annotation support
  - Error detection and reporting
  - Source location tracking
- Planned features:
  - Enhanced error recovery
  - Complete type checking

### 7. Code Generation

- Not implemented
- Planned features:
  - LLVM integration
  - Optimization passes
  - Debug information
  - Source mapping

### 8. Lexer

- Fully implemented
- Current features:
  - UTF-16LE encoding support
  - Basic token handling
  - Boolean literal support (صحيح/خطأ)
  - Advanced operators (++, --, +=, -=, *=, /=, %=)
  - Comment handling (single-line, multi-line)
  - Number parsing with Arabic-Indic digits
  - String and character literal support
  - Source position tracking
- Planned features:
  - Scientific notation support
  - Binary and hexadecimal literals
  - Enhanced error reporting and recovery
