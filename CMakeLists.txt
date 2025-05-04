cmake_minimum_required(VERSION 3.20)
project(baa
    VERSION 0.1.9.8
    DESCRIPTION "لغة باء - The Baa Programming Language"
    LANGUAGES C
)

# إعداد معيار لغة C
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Option to skip parser compilation for development (No longer directly applicable this way)
# option(SKIP_PARSER "Skip compiling the parser components" OFF)

add_definitions(-DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /IGNORE:4006 /IGNORE:4099") # Ignore certain linker warnings if needed

# إضافة مجلد التضمين الرئيسي
include_directories(${CMAKE_SOURCE_DIR}/include)

# --- البرنامج الرئيسي ---
add_executable(baa
    # Core executable files
    src/main.c
    src/compiler.c

    # Utility files
    src/utils/utils.c

    # Type system files
    src/types/types.c

    # Operator files
    src/operators/operators.c

    # AST files
    src/ast/ast.c
    src/ast/expressions.c
    src/ast/statements.c
    src/ast/literals.c
    src/ast/function.c

    # Lexer files
    src/lexer/lexer.c
    src/lexer/number_parser.c

    # Preprocessor files
    src/preprocessor/preprocessor.c
    src/preprocessor/preprocessor_utils.c
    src/preprocessor/preprocessor_macros.c
    src/preprocessor/preprocessor_expansion.c
    src/preprocessor/preprocessor_conditionals.c
    src/preprocessor/preprocessor_expr_eval.c
    src/preprocessor/preprocessor_core.c

    # Parser files
    src/parser/parser.c
    src/parser/expression_parser.c
    src/parser/statement_parser.c
    src/parser/type_parser.c
    src/parser/parser_helper.c
    src/parser/declaration_parser.c
    src/parser/control_flow_parser.c
    src/parser/error_handling.c

    # Analysis files
    src/analysis/flow_analysis.c
    src/analysis/flow_errors.c

    # Codegen core file (always needed)
    src/codegen/codegen.c
)

# Include directories for the executable
target_include_directories(baa
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        # Add internal src include if necessary (e.g., src/preprocessor/preprocessor_internal.h)
        # ${CMAKE_SOURCE_DIR}/src/include # Example if such a dir exists
        ${CMAKE_SOURCE_DIR}/src/preprocessor # For preprocessor_internal.h
)

# --- Conditional LLVM Handling ---
# Check if LLVM is available (You might need to adjust how LLVM_AVAILABLE is set/found)
# For now, assume it's OFF unless explicitly enabled or found via find_package(LLVM)
option(USE_LLVM "Enable LLVM backend" OFF) # Example option

if(USE_LLVM)
    # Attempt to find LLVM (adjust paths/version as needed)
    find_package(LLVM REQUIRED CONFIG) # Or adjust version/components

    if(LLVM_FOUND)
        message(STATUS "LLVM Found: ${LLVM_PACKAGE_VERSION}")
        message(STATUS "LLVM Include Dirs: ${LLVM_INCLUDE_DIRS}")
        message(STATUS "LLVM Library Dirs: ${LLVM_LIBRARY_DIRS}")

        # Add LLVM source file
        target_sources(baa PRIVATE src/codegen/llvm_codegen.c)

        # Add LLVM include directories
        target_include_directories(baa PRIVATE ${LLVM_INCLUDE_DIRS})

        # Add LLVM compile definitions
        target_compile_definitions(baa PRIVATE LLVM_AVAILABLE=1)

        # Link LLVM libraries
        # list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}") # Usually handled by find_package
        # include(AddLLVM) # Usually handled by find_package
        llvm_map_components_to_libnames(llvm_libs
            core support irreader analysis target mcjit native executionengine
            # Add other necessary components
        )
        message(STATUS "Linking LLVM Libraries: ${llvm_libs}")
        target_link_libraries(baa PRIVATE ${llvm_libs})

    else()
        message(WARNING "USE_LLVM is ON but LLVM was not found. Using stub backend.")
        target_sources(baa PRIVATE src/codegen/llvm_stub.c)
        target_compile_definitions(baa PRIVATE LLVM_AVAILABLE=0)
    endif()
else()
    message(STATUS "LLVM backend disabled. Using stub backend.")
    # Add stub source file
    target_sources(baa PRIVATE src/codegen/llvm_stub.c)
    # Add compile definition
    target_compile_definitions(baa PRIVATE LLVM_AVAILABLE=0)
endif()

# تفعيل الاختبارات (معطل مؤقتاً)
# enable_testing()
# add_subdirectory(tests) # Keep this if tests are separate
