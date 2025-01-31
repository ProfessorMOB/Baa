# Changelog

All notable changes to the B (باء) compiler project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.6] - 2025-01-31

### Added
- Basic type system implementation with K&R C compatibility
- Core operator system with arithmetic and comparison operators
- Comprehensive test suite for type system and operators
- Type conversion and validation system
- Arabic operator names and error messages
- UTF-16 support for character type

### Changed
- Updated project structure with src/ and tests/ directories
- Enhanced type safety with strict conversion rules
- Improved error handling in type system

### Documentation
- Added type system implementation details
- Added operator system specifications
- Updated development guide with testing procedures
- Added code examples for type and operator usage

## [0.1.5] - 2025-01-31

### Added
- Comprehensive K&R C feature comparison in `docs/c_comparison.md`
- Detailed implementation roadmap for K&R C compatibility
- Arabic equivalents for all K&R C keywords and operators
- Standard library function mappings with Arabic names

### Changed
- Updated `docs/language.md` to align with K&R C specification
- Updated `docs/development.md` to follow K&R coding style
- Enhanced documentation with UTF-16LE and bidirectional text support details
- Improved error message documentation with Arabic language support

### Documentation
- Added detailed type system documentation
- Added memory model specifications
- Added preprocessor directive documentation
- Added comprehensive code examples
- Updated build system documentation

## [0.1.0] - 2025-01-31

### Added
- Initial project structure and build system
- Basic file reading with UTF-16LE support
- Support for Arabic text in source files
- Modular component architecture:
  - Lexer component with UTF-16 file handling
  - AST component with basic node structure
  - Parser component (placeholder)
  - Code generator component (placeholder)
  - Utils component with error handling and memory management
- Comprehensive test suite:
  - Lexer tests for file handling
  - Utils tests for memory and error handling
- Project documentation:
  - README with build instructions
  - API documentation
  - Example programs
- CMake-based build system with:
  - Cross-platform support
  - Test integration
  - Modular component builds
- Development tools:
  - Automated testing framework
  - Error handling system
  - Memory management utilities
- Initial project setup
- Basic file reading functionality
- UTF-16LE support for Arabic text
- CMake build system configuration
- Project documentation
- Code organization and structure

### Changed
- Organized project structure into logical components
- Improved build system with proper dependency management
- Enhanced file handling with proper Unicode support
- Improved project structure
- Enhanced build system configuration
- Updated documentation

### Fixed
- UTF-16LE file handling and BOM detection
- Memory management in AST operations
- File size calculations for wide character content
- File handling improvements
- Better error messages in Arabic

## [0.0.1] - 2025-01-31

### Added
- Initial release
- Basic file reading capabilities
- Command-line interface
- Simple error reporting

## [Unreleased]

### Planned
- Parser implementation
- Code generation
- Symbol table
- Type system
- Error recovery
- Optimization passes
- Arabic language syntax definition
- Comprehensive test suite expansion
- Documentation in both English and Arabic
