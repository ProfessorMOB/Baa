# Baa Language Architecture

## Overview
Baa (باء) is designed with a modular architecture that separates concerns into distinct components. Each component is responsible for a specific aspect of the compilation process, making the system maintainable and extensible.

## Core Components

### 1. Lexer
The lexical analyzer responsible for tokenizing source code:
- **Token Generation**: Converts source text into tokens
- **Unicode Support**: Full support for Arabic characters and identifiers
- **Source Tracking**: Accurate line and column tracking
- **Error Detection**: Identifies lexical errors and provides diagnostics
- **Advanced Operators**: Support for compound assignments (+=, -=, etc.) and increment/decrement (++, --)
- **Boolean Literals**: Recognition of صحيح (true) and خطأ (false)
- **Number Parsing**: Support for various number formats and Arabic-Indic digits
- **Comment Handling**: Support for single-line and multi-line comments

### 2. Parser
Transforms tokens into a structured AST:
- **Recursive Descent**: Top-down parsing approach
- **Expression Parsing**: Handles precedence and associativity
- **Statement Parsing**: Converts token sequences into statement nodes
- **Declaration Parsing**: Handles variable and function declarations
- **Type Checking**: Basic type validation during parsing
- **Error Recovery**: Continues parsing after encountering errors
- **Source Location**: Tracks precise locations for error reporting

### 3. Abstract Syntax Tree (AST)
The AST module provides the foundation for representing code structure:
- **Node Types**: Comprehensive set of node types for all language constructs
- **Memory Management**: Efficient allocation and deallocation of nodes
- **Tree Operations**: Creation, traversal, and manipulation of the syntax tree
- **Visitor Pattern**: Support for tree traversal and transformation
- **Standardized Structure**: Clear hierarchy with expressions, statements, and declarations
- **Enhanced Function Parameters**: Support for optional parameters, rest parameters, and named arguments
- **Control Flow Support**: If-else, while loops, for loops, switch/case, break/continue statements
- **Array Support**: Array creation, indexing, and manipulation

### 4. Type System
A robust type system that supports both C compatibility and Arabic type names:
- **Basic Types**: عدد_صحيح (int), عدد_حقيقي (float), حرف (char)
- **Type Checking**: Static type checking with detailed error messages
- **Type Conversion**: Implicit and explicit type conversion rules
- **Type Validation**: Compile-time type validation

### 5. Operators
Operator system with full Arabic support:
- **Arithmetic**: جمع (+), طرح (-), ضرب (*), قسمة (/)
- **Comparison**: يساوي (==), أكبر_من (>), أصغر_من (<)
- **Logical**: و (&&), أو (||), ليس (!)
- **Precedence**: Clear operator precedence rules
- **Extensibility**: Easy addition of new operators

### 6. Control Flow
Control structures with Arabic keywords:
- **Conditions**: إذا (if), وإلا (else)
- **Loops**: طالما (while), من_أجل (for)
- **Functions**: دالة (function)
- **Return**: إرجع (return)

### 7. Utils
Utility functions and support features:
- **Error Handling**: Detailed Arabic error messages
- **Memory Management**: Safe memory allocation and tracking
- **String Handling**: UTF-8 string operations
- **File Operations**: Source file handling with Arabic support

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
- **Arabic Messages**: Error messages in Arabic
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
- **Code Generation**: LLVM-based code generator
- **Optimizer**: Code optimization passes
- **IDE Integration**: Support for code editors and IDEs

## Version History
- **0.1.7**: Initial project structure
- **0.1.8**: Core component implementation
  - AST implementation
  - Type system
  - Operators
  - Control flow
  - Utils and error handling
