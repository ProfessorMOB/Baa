# B (باء) Programming Language

B (باء) is a programming language designed to support Arabic syntax while maintaining full compatibility with K&R C features. It allows developers to write code using Arabic keywords and identifiers while following established C programming patterns.

## Current Status (v0.1.9.8)

The project now supports:

### Core Features

- **Preprocessor Directives:**
  - `#تضمين <...>` and `#تضمين "..."` (Include files)
  - `#تعريف NAME ...` (Parameterless macro definition/substitution)
  - `#الغاء_تعريف NAME` (Undefine macro)
  - Conditional compilation (`#إذا_عرف`, `#إذا_لم_يعرف`, `#إلا`, `#نهاية_إذا`)

- Basic type system with K&R C compatibility
  - عدد_صحيح (int) - 32-bit integer
  - عدد_حقيقي (float) - 32-bit float
  - حرف (char) - 16-bit UTF-16 character
  - منطقي (boolean) - Logical true/false values (`صحيح`, `خطأ`)
  - فراغ (void) - No value type

- Core operator system
  - Arithmetic operators (+, -, *, /, %)
  - Comparison operators (==, !=, <, >, <=, >=)
  - Assignment operator (=)
  - Compound assignment operators (+=, -=, *=, /=, %=)
  - Increment/decrement operators (`++`, `--`)
  - Logical operators (`&&`, `||`, `!`)
  - Type checking and validation
  - Arabic operator names

- Control Flow Structures
  - إذا/وإلا (if/else)
  - طالما (while)
  - إرجع (return)

- Function System
  - Regular function parameters
  - Optional parameters (with default values)
  - Rest parameters (variable argument lists)
  - Named arguments support
  - Method vs. function distinction

- Literals
  - Integer and Number scanning (`123`, `١٢٣`)
  - String literals (`"..."` with basic escapes)
  - Character literals (`'...'` with basic escapes)
  - Boolean literals (`صحيح`, `خطأ`)

- Arabic File Support
  - Native `.ب` extension
  - UTF-16LE encoding support
  - Arabic file naming
  - Build system integration

For detailed information about Arabic support, see [Arabic Support Documentation](docs/arabic_support.md).

### Project Status Overview

#### What's Working

- **Preprocessor**: Handles includes (`#تضمين`), basic macros (`#تعريف`), undefines (`#الغاء_تعريف`), and conditional compilation (`#إذا_عرف`, `#إذا_لم_يعرف`, `#إلا`, `#نهاية_إذا`).
- **Core Architecture**: Well-defined architecture with clear separation of concerns
- **Type System**: Basic types including Boolean, type conversion rules, and type checking
- **AST (Abstract Syntax Tree)**: Comprehensive node structure, program, function nodes, enhanced parameter handling
- **Lexer**: UTF-16LE file reading, token recognition (keywords, types, bools, operators `+ - * / % = == != < > <= >= && || ! ++ -- += -= *= /= %=`, identifiers, int/number, string, char literals), line/col tracking, basic error tokens. support, advanced operators
- **Parser**: Recursive descent implementation, expression parsing with precedence, statement parsing
- **Utils**: Memory management, string handling, error infrastructure
- **Preprocessor integration with include and basic macro support.**

#### What's Not Working

- **Code Generation**: LLVM integration is incomplete, mostly placeholder code
- **Parser Integration**: Individual components exist but full integration is incomplete
- **Standard Library**: Defined but not implemented
- **Testing Infrastructure**: Framework defined but implementation is limited

#### What Needs Improvement

- **Arabic Support**: RTL text handling, more comprehensive error messages
- **Error Handling**: More detailed messages, better recovery mechanisms
- **Documentation**: Generally updated and consolidated. Specific component details may need further review.
- **Build System**: Cross-platform support, dependency management

#### What Needs Fixing

- **Lexer Issues**: UTF-16LE handling, Arabic character recognition
- **Parser Issues**: Expression precedence, statement termination, control flow parsing
- **Memory Management**: Potential leaks, inconsistent allocation
- **Integration Issues**: Component integration, compilation pipeline gaps

### Changelog

**Added:**

- Boolean type (منطقي) with literals صحيح (true) and خطأ (false)
- Compound assignment operators (+=, -=, *=, /=, %=)
- Increment/decrement operators (++, --)
- Enhanced function parameter handling (optional, rest, named arguments)
- Method vs. function distinction in AST
- Enhanced parser debug output for better troubleshooting
- Test files for Arabic program parsing
- UTF-8 support for Arabic identifiers in the parser
- LLVM code generation infrastructure (placeholder)
- Improved error handling system

**Changed:**

- Refactored parser functions for improved error handling
- Simplified number parsing (temporarily removed decimal support)
- Improved error messages with more context
- Updated project structure to match architecture documentation
- Focus on Parser Enhancements, Code Generation, Lexer Enhancements, and AST Improvements.
- *See [docs/roadmap.md](docs/roadmap.md) for the high-level plan and component-specific roadmaps (e.g., `docs/LEXER_ROADMAP.md`, `docs/PREPROCESSOR_ROADMAP.md`) for details.*

**Fixed:**

- Memory leaks in parser error handling
- Function declaration parsing
- Identifier parsing for UTF-16LE characters
- File handling for UTF-16LE encoded files

### Roadmap

**Completed Features:**

- Core components (AST, Type System, Operators, Memory Management, Error Handling)
- Advanced operators (compound assignment, increment/decrement)
- Boolean type support
- Enhanced function parameter system
- Arabic support (UTF-16LE encoding, Arabic identifiers, Basic RTL support, Arabic error messages)
- Documentation (Language specification, Arabic support guide, C comparison, Architecture overview, Component documentation)

**In Progress:**

- Parser Development (Grammar definition, Token handling, Error recovery, Source location tracking)
- Code Generation (LLVM integration planning, Basic code emission, Debug information support)
- Standard Library (Basic I/O functions, String manipulation, Memory management, File operations)

**Short-term Goals (0.2.0):**

- Focus on Parser Enhancements, Code Generation, Lexer Enhancements, and AST Improvements.
- *See [docs/roadmap.md](docs/roadmap.md) for the high-level plan and component-specific roadmaps (e.g., `docs/LEXER_ROADMAP.md`) for details.*

### Project Structure

```
baa/
├── include/
│   └── baa/           # Public header files
│       ├── ast/       # AST definitions
│       ├── lexer/     # Lexer interface
│       ├── parser/    # Parser interface
│       ├── preprocessor/ # Preprocessor interface
│       ├── types/     # Type system interface
│       ├── utils/     # Utility functions
│       └── codegen/   # Code generation interface
├── src/
│   ├── ast/          # AST implementation
│   ├── lexer/        # Lexer implementation
│   ├── parser/       # Parser implementation
│   ├── preprocessor/ # Preprocessor implementation
│   ├── types/        # Type system implementation
│   ├── operators/    # Operator system implementation
│   ├── codegen/      # Code generation (incomplete)
│   ├── utils/        # Utility functions
│   ├── control_flow/ # Control flow implementation
│   ├── compiler.c    # Main compilation logic
│   └── main.c        # Executable entry point
├── tests/
│   ├── unit/         # Unit tests (limited)
│   └── integration/  # Integration tests (placeholder)
└── docs/             # Documentation
    ├── preprocessor.md     # Preprocessor documentation
    ├── PREPROCESSOR_ROADMAP.md # Detailed Preprocessor roadmap
    ├── architecture.md     # System architecture overview & component status
    ├── language.md         # Language specification
    ├── project_structure.md # Detailed directory layout (Arabic)
    ├── roadmap.md          # High-level development roadmap
    ├── lexer.md            # Lexer structure & usage
    ├── LEXER_ROADMAP.md    # Detailed Lexer roadmap
    ├── AST_STRUCTURE.md    # AST structure details
    ├── AST_ROADMAP.md      # Detailed AST roadmap
    ├── PARSER_STRUCTURE.md # Parser structure details
    ├── PARSER_ROADMAP.md   # Detailed Parser roadmap
    ├── arabic_support.md   # Arabic support details
    ├── c_comparison.md     # Comparison with C
    ├── development.md      # Developer guide
    └── ...                 # Other specific docs
```

## Building from Source

### Prerequisites

- CMake 3.20 or higher
- K&R C compliant compiler
- Git for version control
- LLVM development libraries (optional, for code generation)
- Unicode support libraries

### Build Steps

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

## Features

### Type Safety

- Strong type checking
- Explicit conversion rules
- Error type for invalid operations
- UTF-16 support for Arabic text

### Operator System

- Type-safe operations
- Arabic operator names
- K&R C operator precedence
- Comprehensive error handling

### Documentation

- Detailed Arabic error messages
- Comprehensive documentation in both English and Arabic
- Examples and usage guides
- Development guidelines

## File Extensions

The Baa programming language supports two file extensions:

- `.ب` - The primary Arabic file extension
- `.baa` - Alternative Latin file extension

### Example Program

```baa
#تضمين <مكتبة_طباعة>
#تعريف EXIT_SUCCESS 0

// مثال برنامج بسيط بلغة باء
دالة رئيسية() {
    اطبع("مرحباً بالعالم!").
    إرجع EXIT_SUCCESS.
}
```

## Known Issues and Limitations

- Code generation is not fully implemented yet
- Complex language constructs may not parse correctly
- Error messages may not be fully localized
- Limited testing coverage
- UTF-16LE handling has edge cases with certain characters
- Parser error recovery is limited

## Contributing

We welcome contributions! Please see our [development guide](docs/development.md) for details on:

- Code style and standards
- Testing requirements
- Documentation guidelines
- Pull request process

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

Special thanks to:

- The C language designers for their foundational work
- The Arabic programming community for their support and feedback
- All contributors who have helped shape this project
