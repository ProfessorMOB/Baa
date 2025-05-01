# Baa Language CMake Build System Roadmap

This document outlines potential future improvements for the Baa project's CMake build system, aiming for increased modularity, maintainability, and adherence to modern CMake practices.

**Note:** This is a planning document; these changes are not yet implemented.

## Proposed Refactoring Plan

1.  **Create `cmake/` Directory:**
    *   Establish a `cmake/` directory at the project root.
    *   Move LLVM finding logic from the root `CMakeLists.txt` into a dedicated module (e.g., `cmake/FindLLVM.cmake`).
    *   Consider creating modules for common compiler settings (e.g., C standard, UTF-8 flags) to be included where needed (e.g., `cmake/BaaCompilerSettings.cmake`).

2.  **Enforce Out-of-Source Builds:**
    *   Add a check to the root `CMakeLists.txt` to prevent in-source builds and guide users to create a separate build directory.

3.  **Adopt Target-Centric Approach:**
    *   **Remove Global Settings:** Eliminate global `include_directories()`, `add_definitions()`, and `set(CMAKE_C_FLAGS ...)` from the root `CMakeLists.txt`.
    *   **Apply Per-Target:** Ensure all component libraries (`baa_utils`, `baa_lexer`, etc.) use `target_include_directories()`, `target_compile_definitions()`, and `target_compile_options()` with appropriate visibility (`PUBLIC`, `PRIVATE`, `INTERFACE`).

4.  **Declare Dependencies Explicitly:**
    *   Modify each component's `CMakeLists.txt` (in `src/*`) to explicitly link against its direct dependencies using `target_link_libraries()`.
        *   Example: `target_link_libraries(baa_lexer PRIVATE baa_utils)`
        *   Example: `target_link_libraries(baa_parser PRIVATE baa_lexer baa_ast baa_utils baa_operators)` (already partially done)
    *   This allows CMake to manage the build order and transitive dependencies correctly.

5.  **Refine Executable Linking:**
    *   Consider creating a `baa_compiler` library target in `src/` containing `compiler.c`.
    *   Modify the root `CMakeLists.txt` to link the final `baa` executable primarily against `baa_compiler` (and potentially `baa_preprocessor` if used directly by `main.c`), relying on transitive linking for other components.

6.  **CMake Policies:**
    *   Add relevant `cmake_policy(SET ...)` commands at the top of the root `CMakeLists.txt` to ensure consistent behavior across CMake versions (e.g., CMP0074).

7.  **Conditional Component Logic:**
    *   Move conditional logic (like `SKIP_PARSER`) from component `CMakeLists.txt` files to the root file, wrapping the corresponding `add_subdirectory()` calls.

## Benefits

*   Improved modularity and separation of concerns.
*   Clearer dependency graph.
*   More robust handling of include paths and compile options.
*   Easier maintenance and extension of the build system.
*   Better alignment with modern CMake best practices.
