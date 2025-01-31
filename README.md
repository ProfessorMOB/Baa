# B (باء) Programming Language Compiler

A compiler implementation for the B (باء) programming language, a C-like language with Arabic support.

## Current Status (v0.1.0)

The project is in its early stages, with the following features implemented:
- Basic file reading with UTF-16LE support
- Support for Arabic text in source files
- Modular architecture for future expansion
- Comprehensive test suite
- Cross-platform build system

## Features

- C-like syntax with Arabic language support
- UTF-16LE encoding support for Arabic text
- Modern CMake-based build system
- Cross-platform compatibility
- Modular design for easy extension

## Prerequisites

- CMake 3.20 or higher
- C11 compliant compiler
- C++23 compliant compiler (for some components)
- ccache (optional, for faster builds)

## Building

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

For release build:
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

## Usage

Create a source file with `.baa` extension using UTF-16LE encoding. For example:

```
// مثال برنامج بسيط بلغة باء
دالة رئيسية() {
    اطبع("مرحباً بالعالم!");
    إرجع 0;
}
```

Then compile it using:
```bash
./Baa <input_file>
```

## Project Structure

```
.
├── src/                    # Source files
│   ├── lexer/             # Lexical analysis
│   ├── parser/            # Syntactic analysis
│   ├── ast/               # Abstract Syntax Tree
│   ├── codegen/          # Code generation
│   └── utils/            # Utility functions
├── include/               # Public headers
├── tests/                # Test files
├── docs/                 # Documentation
├── examples/             # Example programs
└── tools/                # Build and development tools
```

## Current Limitations

- Parser not yet implemented
- Code generation not yet implemented
- Only file reading and lexical analysis infrastructure available
- Limited error reporting

## Roadmap

1. Implement lexical analyzer (tokenizer)
2. Develop parser for Arabic syntax
3. Build abstract syntax tree (AST)
4. Implement code generation
5. Add optimization passes
6. Enhance error reporting and recovery

## Contributing

Contributions are welcome! Please read our contributing guidelines before submitting pull requests.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

Special thanks to all contributors who have helped shape this project.
