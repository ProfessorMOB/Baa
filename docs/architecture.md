# B (باء) Compiler Architecture

## Overview

The B (باء) compiler is designed as a modular system that transforms Arabic source code into executable programs. The compiler follows the traditional K&R C compiler architecture while supporting Arabic syntax and modern features.

## Components

### 1. Lexical Analysis (src/lexer)
- **Purpose**: Converts source text into tokens
- **Key Features**:
  - UTF-16LE support for Arabic text
  - K&R C token compatibility
  - Arabic identifier support
  - Error reporting in Arabic
- **Key Files**:
  - `lexer.c`: Main lexer implementation
  - `lexer.h`: Public interface

### 2. Parser (src/parser)
- **Purpose**: Builds syntax tree from tokens
- **Features**:
  - K&R C grammar support
  - Arabic syntax extensions
  - Type checking
  - Symbol table management
- **Components**:
  - Expression parser
  - Declaration parser
  - Statement parser
  - Type system

### 3. Abstract Syntax Tree (src/ast)
- **Purpose**: Internal representation of program structure
- **Node Types**:
  - Declarations (functions, variables)
  - Statements (if, while, return)
  - Expressions (arithmetic, logical)
  - Types (basic types, structs, unions)
- **Features**:
  - Memory management
  - Tree traversal
  - Optimization support

### 4. Type System (src/types)
- **Purpose**: Type checking and management
- **Features**:
  - Basic types (عدد_صحيح, عدد_حقيقي, محرف)
  - Derived types (مصفوفة, مؤشر)
  - User-defined types (بنية, اتحاد)
  - Type conversion

### 5. Code Generation (src/codegen)
- **Purpose**: Generates target code from AST
- **Features**:
  - LLVM IR generation
  - Basic optimizations
  - Debug information
  - Memory management

### 6. Preprocessor (src/preproc)
- **Purpose**: Source code preprocessing
- **Features**:
  - File inclusion (تضمين#)
  - Macro definition (تعريف#)
  - Conditional compilation
  - Token manipulation

### 7. Symbol Table (src/symtab)
- **Purpose**: Manages identifiers and scopes
- **Features**:
  - Scope management
  - Symbol resolution
  - Type information
  - Linkage handling

### 8. Error Handling (src/error)
- **Purpose**: Error detection and reporting
- **Features**:
  - Arabic error messages
  - Source location tracking
  - Error recovery
  - Warning system

## Build System

### CMake Configuration
- Modern CMake practices
- Component-based structure
- Test integration
- Cross-platform support

### Directory Structure
```
.
├── src/                 # Source code
│   ├── lexer/          # Lexical analysis
│   ├── parser/         # Syntax analysis
│   ├── ast/            # Abstract Syntax Tree
│   ├── types/          # Type system
│   ├── codegen/        # Code generation
│   ├── preproc/        # Preprocessor
│   ├── symtab/         # Symbol table
│   └── error/          # Error handling
├── include/            # Public headers
├── tests/             # Test suite
├── docs/              # Documentation
├── examples/          # Example programs
└── tools/             # Build tools
```

## Testing Strategy

### Unit Tests
- Component-level testing
- K&R C compatibility tests
- Arabic syntax tests
- Memory management tests

### Integration Tests
- End-to-end compilation
- Standard library tests
- Error handling tests
- Performance benchmarks

## Memory Management

### Stack Allocation
- Local variables
- Function parameters
- Temporary objects

### Heap Allocation
- Dynamic memory
- String literals
- Compound objects

### Register Allocation
- Expression evaluation
- Variable storage
- Optimization support

## Error Handling

### Compile-time Errors
- Syntax errors
- Type errors
- Symbol resolution
- Preprocessor errors

### Runtime Errors
- Memory access
- Division by zero
- Stack overflow
- Array bounds

## Future Enhancements

### Language Extensions
1. Modern Features
   - Enhanced type safety
   - Better error messages
   - Memory protection

2. Development Tools
   - Integrated debugger
   - Profile generation
   - Code analysis

3. Standard Library
   - I/O functions
   - String handling
   - Math operations
   - Memory management
