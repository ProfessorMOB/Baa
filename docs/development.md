# B (باء) Compiler Development Guide

## Development Environment Setup

### Prerequisites
- CMake 3.20 or higher
- K&R C compliant compiler
- Git for version control
- LLVM development libraries
- Unicode support libraries

### Building from Source
```bash
git clone <repository-url>
cd baa
mkdir build && cd build
cmake ..
cmake --build .
```

### Running Tests
```bash
cd build
ctest --output-on-failure
```

## Project Organization

### Source Code Structure
```
src/
├── lexer/          # Lexical analysis
│   ├── lexer.c     # Main lexer implementation
│   ├── token.c     # Token handling
│   └── utf16.c     # UTF-16 support
├── parser/         # Syntax analysis
│   ├── parser.c    # Main parser
│   ├── grammar.c   # Grammar rules
│   └── ast.c       # AST construction
├── types/          # Type system
│   ├── types.c     # Type definitions
│   ├── check.c     # Type checking
│   └── convert.c   # Type conversion
├── codegen/        # Code generation
│   ├── gen.c       # LLVM IR generation
│   ├── opt.c       # Optimizations
│   └── debug.c     # Debug info
└── runtime/        # Runtime support
    ├── memory.c    # Memory management
    └── error.c     # Error handling
```

## Coding Standards

### C Code Style
- Follow K&R style
- 4 spaces for indentation
- Maximum line length: 80 characters
- Clear function and variable names
- Comments in both English and Arabic

### Header Files
```c
#pragma once

// Include guards (alternative)
#ifndef BAA_COMPONENT_H
#define BAA_COMPONENT_H

// Includes
#include <stddef.h>
#include <stdint.h>

// Type definitions
typedef struct {
    // ...
} ComponentType;

// Function declarations
void baa_component_function(void);

#endif // BAA_COMPONENT_H
```

### Source Files
```c
#include "baa/component.h"
#include <string.h>

// Private functions
static void helper_function(void) {
    // Implementation
}

// Public functions
void baa_component_function(void) {
    // Implementation
}
```

## Implementation Guidelines

### Lexer Implementation
1. UTF-16LE handling
   ```c
   // Read UTF-16LE characters
   wchar_t baa_next_char(void);
   
   // Handle UTF-16LE strings
   wchar_t *baa_read_string(void);
   ```

2. Token handling
   ```c
   // Token structure
   typedef struct {
       BaaTokenType type;
       const wchar_t *lexeme;
       size_t length;
       size_t line;
       size_t column;
   } BaaToken;
   
   // Token scanning
   BaaToken baa_scan_token(BaaLexer* lexer);
   ```

3. Boolean and advanced operator support
   ```c
   // Boolean literals
   // صحيح (True) and خطأ (False)
   if (baa_identifier_equals(lexer, L"صحيح")) {
       return baa_make_token(lexer, BAA_TOKEN_BOOL_LIT);
   } else if (baa_identifier_equals(lexer, L"خطأ")) {
       return baa_make_token(lexer, BAA_TOKEN_BOOL_LIT);
   }
   
   // Compound assignment operators
   case L'+':
       if (baa_match(lexer, L'+')) {
           return baa_make_token(lexer, BAA_TOKEN_INCREMENT);
       } else if (baa_match(lexer, L'=')) {
           return baa_make_token(lexer, BAA_TOKEN_PLUS_EQUAL);
       }
       return baa_make_token(lexer, BAA_TOKEN_PLUS);
   ```

### Parser Implementation
1. Grammar rules
   ```c
   // Parse declarations
   BaaStmt* baa_parse_var_declaration(BaaParser* parser);
   BaaStmt* baa_parse_function(BaaParser* parser);
   
   // Parse statements
   BaaStmt* baa_parse_statement(BaaParser* parser);
   BaaStmt* baa_parse_if_statement(BaaParser* parser);
   BaaStmt* baa_parse_while_statement(BaaParser* parser);
   
   // Parse expressions
   BaaExpr* baa_parse_expression(BaaParser* parser);
   BaaExpr* baa_parse_assignment(BaaParser* parser);
   ```

2. AST construction
   ```c
   // Create AST nodes
   BaaExpr* baa_create_binary_expr(BaaBinaryOpKind op, BaaExpr* left, BaaExpr* right, BaaSourceLocation loc);
   BaaExpr* baa_create_unary_expr(BaaUnaryOpKind op, BaaExpr* operand, BaaSourceLocation loc);
   ```

### AST Implementation
1. Function parameter handling
   ```c
   // Create parameters
   BaaParameter* baa_create_parameter(const wchar_t* name, size_t name_length, BaaType* type);
   BaaParameter* baa_create_optional_parameter(const wchar_t* name, size_t name_length, BaaType* type, BaaExpr* default_value);
   BaaParameter* baa_create_rest_parameter(const wchar_t* name, size_t name_length, BaaType* element_type);
   
   // Add parameters to function
   bool baa_add_parameter_to_function(BaaFunction* function, BaaParameter* parameter);
   
   // Function validation
   bool baa_validate_function_signature(BaaFunction* function);
   ```

2. Control Flow Statements
   ```c
   // Create control flow statements
   BaaStmt* baa_create_if_stmt(BaaExpr* condition, BaaStmt* then_branch, BaaStmt* else_branch);
   BaaStmt* baa_create_while_stmt(BaaExpr* condition, BaaStmt* body);
   BaaStmt* baa_create_for_stmt(BaaStmt* initializer, BaaExpr* condition, BaaExpr* increment, BaaStmt* body);
   BaaStmt* baa_create_switch_stmt(BaaExpr* value, BaaStmt** cases, size_t case_count);
   BaaStmt* baa_create_break_stmt(int loop_depth, bool is_switch_break);
   BaaStmt* baa_create_continue_stmt(int loop_depth);
   ```

### Type System
1. Type checking
   ```c
   // Check types
   Type *check_binary_operation(Type *left, Type *right, TokenType op);
   Type *check_unary_operation(Type *operand, TokenType op);
   ```

2. Type conversion
   ```c
   // Convert types
   Type *convert_type(Type *from, Type *to);
   bool is_convertible(Type *from, Type *to);
   ```

### Code Generation
1. LLVM IR generation
   ```c
   // Generate LLVM IR
   LLVMValueRef gen_expression(Node *node);
   LLVMValueRef gen_function(Node *node);
   ```

2. Optimization
   ```c
   // Optimize code
   void optimize_function(LLVMValueRef function);
   void optimize_module(LLVMModuleRef module);
   ```

## Error Handling

### Error Types
```c
typedef enum {
    ERROR_LEXICAL,
    ERROR_SYNTAX,
    ERROR_TYPE,
    ERROR_SEMANTIC
} ErrorType;

// Report errors
void report_error(ErrorType type, const wchar_t *message);
```

### Error Recovery
```c
// Synchronize parser
void synchronize(void);

// Skip to next statement
void skip_to_statement(void);
```

## Testing

### Running Tests
```bash
# Build and run type system tests
gcc -o test_types tests/test_types.c src/types/types.c -I./src
./test_types

# Build and run operator tests
gcc -o test_operators tests/test_operators.c src/operators/operators.c src/types/types.c -I./src
./test_operators
```

### Test Organization
```
tests/
├── test_types.c     # Type system tests
└── test_operators.c # Operator tests
```

### Writing Tests
1. Create test file in `tests/` directory
2. Include necessary headers
3. Write test functions for each feature
4. Add assertions for expected behavior
5. Add the test to the build system

### Test Coverage
- Basic type creation and properties
- Type system initialization
- Type comparison and conversion
- Operator validation
- Error handling
- Arabic string handling

### Example Test
```c
void test_type_conversion(void) {
    printf("Testing type conversion...\n");
    
    // Valid conversions
    assert(baa_can_convert(type_int, type_float) == true);
    assert(baa_can_convert(type_float, type_int) == true);
    assert(baa_can_convert(type_char, type_int) == true);
    
    // Invalid conversions
    assert(baa_can_convert(type_void, type_int) == false);
    assert(baa_can_convert(type_error, type_int) == false);
    
    printf("Type conversion tests passed.\n");
}
```

## Documentation

### Code Documentation
```c
/**
 * @brief Function description in English
 * @arabic وصف الدالة بالعربية
 *
 * @param param1 Parameter description
 * @return Return value description
 */
Type function_name(Type param1);
```

### User Documentation
1. README.md
2. Installation guide
3. Language reference
4. Examples

## Build System

### CMake Configuration
```cmake
# Component libraries
add_library(baa_lexer
    src/lexer/lexer.c
    src/lexer/token.c
    src/lexer/utf16.c
)

# Main executable
add_executable(baa src/main.c)
target_link_libraries(baa
    baa_lexer
    baa_parser
    baa_codegen
)
```

## Debugging

### Tools
1. GDB/LLDB
2. Address Sanitizer
3. Memory Sanitizer
4. Undefined Behavior Sanitizer

### Debug Information
```c
// Debug logging
void debug_log(const wchar_t *format, ...);

// Memory tracking
void *debug_malloc(size_t size, const char *file, int line);
void debug_free(void *ptr, const char *file, int line);
```

## Performance Considerations

### Optimization Levels
1. -O0: No optimization
2. -O1: Basic optimization
3. -O2: Full optimization
4. -O3: Aggressive optimization

### Memory Management
1. Stack allocation preferred
2. Minimize heap allocations
3. Use memory pools
4. Cache-friendly data structures

## Security

### Guidelines
1. Input validation
2. Buffer overflow prevention
3. Memory safety
4. Error handling

### Best Practices
1. Use safe functions
2. Check return values
3. Validate array bounds
4. Handle Unicode safely
