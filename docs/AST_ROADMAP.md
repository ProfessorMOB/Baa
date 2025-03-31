# Baa Language AST Implementation Roadmap

This document outlines planned improvements to the Abstract Syntax Tree (AST) implementation
for the Baa programming language compiler. Each improvement is categorized and its current status
is indicated.

## Memory Management

- [x] Consistent memory ownership across all AST nodes
- [x] Proper cleanup functions for all node types
- [x] String duplication for identifiers and literals
- [x] Unified memory management strategy with clear ownership rules

## AST Structural Improvements

- [x] Standardize naming conventions (BaaExprKind, BaaStmtKind, BaaNodeKind instead of BaaExprType, etc.)
- [x] Remove embedding of data in union; use void* data pointer approach with specific type structs
- [x] Standardize expression structure in expressions.h and expressions.c
- [x] Standardize statement structure in statements.h and statements.c
- [x] Remove redundant expression types (BAA_EXPR_BOOL, BAA_EXPR_INT, etc)
- [x] Consolidate AST node types and expression types to eliminate overlap
- [x] Standardize AST node types
- [x] Add documentation for AST structures and memory ownership

## Code Generation

- [x] Update LLVM code generator for standardized AST structure
- [x] Support for basic literal types in code generator
- [x] Support for control flow statements (if/while)
- [ ] Improve type checking in code generation
- [ ] Support for all statement types in code generator
- [ ] Function and parameter code generation
- [ ] Module-level code generation
- [ ] Optimization pass integration

## Type System

- [x] Basic type system with size and alignment information
- [x] Boolean type support (منطقي)
- [ ] Type compatibility checking
- [ ] Type conversion operations
- [ ] User-defined types support (structs, unions, enums)
- [ ] Type inference
- [ ] Generics/templates support
- [ ] Union types
- [ ] Intersection types

## Function and Parameter Handling

- [x] Basic function declaration parsing
- [x] Complete function parameter lists
- [x] Function signature validation
- [x] Optional parameter support
- [x] Rest parameter support
- [x] Method vs. function distinction
- [x] Named argument support
- [ ] Parameter type checking
- [x] Function overloading
- [ ] Return type validation
- [ ] Method call handling
- [x] Named parameter argument passing
- [ ] Default parameter value evaluation
- [x] Rest parameter handling
- [ ] Anonymous function expressions (Lambdas)
- [ ] Closure capture analysis

## Control Flow Statements

- [x] If-else statements
- [x] While loops
- [x] Block statements with proper scope
- [x] For loops
- [x] Switch/case statements
- [x] Break/continue statements

## Expression Handling

- [x] Binary operations
- [x] Unary operations
- [x] Literal values (including Boolean literals صحيح/خطأ)
- [x] Variable references
- [x] Function calls
- [x] Compound assignment operators (+=, -=, *=, /=, %=)
- [x] Increment/decrement operators (++, --)
- [ ] Ternary conditional operator
- [x] Array indexing
- [x] Member access (structs/classes)
- [ ] Pattern matching / Destructuring assignments

## Arrays and Collections
- [x] Array type definition
- [x] Array creation expression
- [x] Array indexing
- [ ] Array methods (length, push, pop, etc.)
- [ ] Array iteration
- [ ] Slice operations
- [ ] Exception handling constructs

## Error Handling

- [x] Basic error reporting
- [x] Source location tracking
- [ ] Comprehensive error reporting
- [ ] Recoverable parsing errors
- [ ] Type error reporting
- [ ] Runtime error handling
- [ ] Add robust error state tracking within AST nodes

## Unicode and Internationalization

- [x] Basic wide character support
- [ ] Full Unicode support for identifiers
- [ ] Complete support for right-to-left text
- [ ] Localized error messages
- [ ] Advanced character encoding handling
- [ ] AST support for async/await constructs

## AST Modularity and Transformations

- [x] Basic block management with add_statement functionality
- [ ] AST visitors for traversal
- [ ] AST transformation framework
- [ ] AST serialization and deserialization
- [ ] AST validation passes
- [ ] Visitor pattern enhancements for complex traversals

## Scope Management

- [x] Basic variable declaration and scoping
- [ ] Complete block-level scoping
- [ ] Variable shadowing rules
- [ ] Lexical closure support
- [ ] Module-level scope
- [ ] Class/method implementation (OOP support)
- [ ] Module system enhancements (import/export, namespaces)

## Optimization Infrastructure

- [ ] Constant folding
- [ ] Dead code elimination
- [ ] Expression simplification
- [ ] Common subexpression elimination
- [ ] AST support for metaprogramming features

## Documentation

- [x] AST structure documentation
- [x] Memory ownership documentation
- [ ] AST structure diagrams
- [ ] Code generation process documentation
- [ ] API documentation for library users
- [ ] Add comprehensive tests for all AST node types
- [ ] Add tests for error conditions and recovery related to AST
- [ ] Add tests for Unicode and RTL text handling in AST nodes
- [ ] Add tests for memory management and resource leaks in AST

## Implementation Priorities
1. ✓ ~~Function parameter handling~~
2. ✓ ~~For loop implementation~~
3. ✓ ~~Switch/case statements~~
4. ✓ ~~Break/continue statements~~
5. ✓ ~~Array creation and indexing~~
6. ✓ Boolean literal support
7. ✓ Advanced operator support
8. Class/method implementation
9. Anonymous function expressions
10. Pattern matching
11. Module system enhancements
12. Type inference
13. Generics/templates
14. Exception handling
15. Anonymous functions / Closures
16. Pattern matching
17. Async/await support
18. Metaprogramming support
