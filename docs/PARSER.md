# Baa Language Parser Structure Documentation

This document provides a comprehensive reference for the parser implementation in the Baa programming language compiler. The parser's role is to transform token sequences produced by the lexer into an Abstract Syntax Tree (AST) representation.

## Core Design Principles

1. **Modularity**: The parser is organized into specialized modules for different language constructs.
2. **Recursive Descent Approach**: The parser uses a top-down, predictive recursive descent parsing technique.
3. **Error Handling**: Robust error detection, reporting, and recovery mechanisms.
4. **Integration with AST**: Direct generation of AST nodes during parsing.
5. **Type Checking**: Basic type annotation support during parsing.

## Parser Architecture

The parser is structured into the following key components:

```
BaaParser (Main Parser)
├── Expression Parser
├── Statement Parser
├── Declaration Parser
└── Type Parser
```

## Parser State

The parser maintains its state in the `BaaParser` structure:

```c
typedef struct {
    BaaLexer* lexer;           // Lexer
    BaaToken current_token;    // Current token
    BaaToken previous_token;   // Previous token
    bool had_error;            // Error flag
    bool panic_mode;           // Error recovery flag
    const wchar_t* error_message;  // Error message if had_error is true
    BaaSourceLocation location;    // Current source location
} BaaParser;
```

## Module Organization

### Main Parser (parser.c/h)
- Entry point for parsing a complete program
- Manages parser state and token advancement
- Delegates to specialized parsers for expressions, statements, etc.

### Expression Parser (expression_parser.c/h)
- Parses all types of expressions
- Handles operator precedence
- Supports literals, identifiers, calls, operators, etc.
- Implements support for:
  - Boolean literals (صحيح/خطأ)
  - Compound assignment operators (+=, -=, *=, /=, %=)
  - Increment/decrement operators (++, --)
  - Array indexing expressions

### Statement Parser (statement_parser.c)
- Parses control flow statements (if, while, for, switch, case, break, continue, return, etc.)
- Handles blocks and expression statements
- Implements support for:
  - If-else statements with conditions and multiple branches
  - While loops with condition and body
  - For loops with initializer, condition, increment, and body
  - Return statements with optional return value
  - Blocks of statements

### Declaration Parser (declaration_parser.c)
- Parses variable declarations
- Parses function declarations with parameters

### Type Parser (type_parser.c)
- Parses type annotations
- Handles primitive types and arrays

### Helper Modules (e.g., parser_helper.c)
- Provides utility functions used across different parser modules (e.g., error synchronization, token consumption).
- Other specialized modules (like `control_flow_parser.c`) might exist to handle specific statement types.

## Parsing Process

The parsing process follows this general flow:
1. Initialize parser with lexer
2. Parse program (entry point)
3. Parse top-level declarations (currently only functions are supported at the top level)
4. For each function, parse parameters and body
5. Parse statements and expressions recursively within function bodies or blocks
6. Construct AST nodes during parsing
7. Handle errors and recover when possible

## Expression Parsing

Expression parsing uses precedence climbing to handle operator precedence:

1. **Primary Expressions**: Literals (numbers, strings, booleans), identifiers, grouped expressions
2. **Member Access**: Dot notation for accessing object members
3. **Call Expressions**: Function calls with arguments
4. **Unary Expressions**: Prefix operators (!, -) and prefix increment/decrement (++x, --x)
5. **Postfix Expressions**: Postfix increment/decrement (x++, x--)
6. **Factor Expressions**: Multiplication, division, modulo
7. **Term Expressions**: Addition, subtraction
8. **Comparison Expressions**: <, <=, >, >=
9. **Equality Expressions**: ==, !=
10. **Logical AND Expressions**: &&
11. **Logical OR Expressions**: ||
12. **Assignment Expressions**: Simple (=) and compound (+=, -=, *=, /=, %=)

### Boolean Literal Support

The parser recognizes and processes Arabic boolean literals:
- `صحيح` (True)
- `خطأ` (False)

These are parsed in the `parse_primary` function and generate a boolean literal expression with the appropriate value.

### Compound Assignment Support

Compound assignment operators are handled in the `parse_assignment` function:
- `+=` (Add and assign)
- `-=` (Subtract and assign)
- `*=` (Multiply and assign)
- `/=` (Divide and assign)
- `%=` (Modulo and assign)

These operators create a special compound assignment expression that combines the operation with assignment.

### Increment/Decrement Support

Increment and decrement operators are handled in both prefix and postfix positions:
- Prefix: `++x`, `--x` (increment/decrement before use)
- Postfix: `x++`, `x--` (increment/decrement after use)

Prefix operators are parsed in the `parse_unary` function, while postfix operators are checked after parsing the operand.

## Statement Parsing

Statements are parsed based on their starting token:

- **If Statements**: Condition, then branch, optional else branch
- **While Statements**: Condition and body
- **Block Statements**: Sequence of statements in braces
- **Return Statements**: Optional return value
- **Expression Statements**: Expressions followed by semicolon

## Declaration Parsing

Declarations are recognized and parsed:

- **Variable Declarations**: Type annotation and optional initializer
- **Function Declarations**: Name, parameters, return type, and body
- **Import Directives**: Parses `#تضمين` directives for including other source files, supporting both system (`<...>`) and local (`"..."`) paths.

## Type System Integration

Type parsing includes:
- Basic types (int, float, etc.)
- Boolean type (منطقي)
- Array types with dimensions
- User-defined types (placeholder implementation)

## Error Handling

The parser includes error handling mechanisms:
- Error detection during parsing
- Error reporting with meaningful messages
- Basic error recovery to continue parsing

## Helper Functions

Various helper functions support the parsing process:
- Token matching and consumption
- Error reporting
- AST node creation
- Source location tracking

## Public API

The parser primarily exposes functions for creating a parser instance and parsing a complete source input. Statement and expression parsing are generally handled internally during the program parsing process.

```c
// Parser creation
BaaParser* baa_create_parser(const wchar_t* source, size_t source_len);
// Note: baa_init_parser is typically called internally by baa_create_parser

// Program parsing (Main entry point)
BaaProgram* baa_parse(const wchar_t* input, const wchar_t* filename);

// Error handling
bool baa_parser_had_error(const BaaParser* parser);
const wchar_t* baa_parser_error_message(const BaaParser* parser);
void baa_clear_parser_error(BaaParser* parser);

// Note: While functions like baa_parse_expression exist, they are often
// intended for internal use by other parser modules or for specific
// scenarios like REPLs, rather than direct use when parsing a full program.
```
