# B (باء) Programming Language

B (باء) is a programming language designed to support Arabic syntax while maintaining full compatibility with K&R C features. It allows developers to write code using Arabic keywords and identifiers while following established C programming patterns.

## Current Status (v0.1.6)

The project is currently in early development. We have implemented:

### Core Features
- Basic type system with K&R C compatibility
  - عدد_صحيح (int) - 32-bit integer
  - عدد_حقيقي (float) - 32-bit float
  - محرف (char) - 16-bit UTF-16 character
  - فراغ (void) - No value type

- Core operator system
  - Arithmetic operators (+, -, *, /, %)
  - Comparison operators (==, !=, <, >, <=, >=)
  - Assignment operator (=)
  - Type checking and validation
  - Arabic operator names

### Project Structure
```
baa/
├── src/
│   ├── types/       # Type system implementation
│   └── operators/   # Operator system implementation
├── tests/
│   ├── test_types.c    # Type system tests
│   └── test_operators.c # Operator tests
└── docs/
    ├── architecture.md  # System architecture
    ├── components.md    # Component details
    ├── development.md   # Developer guide
    ├── language.md      # Language specification
    └── roadmap.md       # Development roadmap
```

## Building from Source

### Prerequisites
- CMake 3.20 or higher
- K&R C compliant compiler
- Git for version control
- LLVM development libraries
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
# Build and run type system tests
gcc -o test_types tests/test_types.c src/types/types.c -I./src
./test_types

# Build and run operator tests
gcc -o test_operators tests/test_operators.c src/operators/operators.c src/types/types.c -I./src
./test_operators
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

## Roadmap

### Next Steps (v0.2.0)
- Control flow implementation (if/else, while, return)
- Function system
- Basic preprocessor
- Memory management

### Future Plans
- Extended type system
- Full K&R C standard library support
- Development tools and IDE integration
- Advanced error recovery

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
