# Create the library target first
add_library(baa_codegen)

# Add LLVM-specific files only if LLVM is available
if(LLVM_AVAILABLE)
    target_sources(baa_codegen PRIVATE
        codegen.c      # Compile the main codegen file only if LLVM is found
        llvm_codegen.c
    )

    target_compile_definitions(baa_codegen PRIVATE
    LLVM_AVAILABLE=0
    )

    # Make sure LLVM CMake modules are included
    list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
    include(AddLLVM)

    # Get required LLVM libraries
    llvm_map_components_to_libnames(llvm_libs
        core
        support
        irreader
        analysis
        target
        mcjit
        native
        executionengine
        # Add other components as needed, e.g., MC, Object, BitReader?
    )
    # Note: llvm_map_components_to_libnames is useful for older CMake/LLVM versions
    # or specific scenarios, but linking imported targets is generally preferred.

    # Print the determined LLVM libraries for debugging (optional)
    # message(STATUS "LLVM Libraries (via map): ${llvm_libs}")

    # Link against imported LLVM targets
    target_link_libraries(baa_codegen
        PRIVATE
            LLVM::Core
            LLVM::Support
            LLVM::IRReader
            LLVM::Analysis
            LLVM::Target
            LLVM::MCJIT
            LLVM::ExecutionEngine
            LLVM::NativeTarget # For LLVMInitializeNativeTarget etc.
            LLVM::NativeAsmPrinter # Often needed with NativeTarget
            LLVM::NativeAsmParser # If parsing inline asm
            # Add other LLVM:: targets corresponding to the components listed above if needed
    )
else()
    # Add stub implementation when LLVM is not available
    target_sources(baa_codegen PRIVATE
        codegen.c      # Always compile the main codegen file
        llvm_stub.c
    )

    target_compile_definitions(baa_codegen PRIVATE
        -DLLVM_AVAILABLE=0
    )
endif()

target_include_directories(baa_codegen
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include  # For baa/codegen/codegen.h and baa/codegen/llvm_codegen.h
    PRIVATE
    # ${CMAKE_SOURCE_DIR}/src/include # This path seems incorrect for this component's private headers.
    # If codegen.c or llvm_codegen.c need private headers from src/codegen, use ${CMAKE_CURRENT_SOURCE_DIR}
    ${LLVM_INCLUDE_DIRS} # Only if LLVM_AVAILABLE
)

target_compile_definitions(baa_codegen PUBLIC # Propagate if public headers depend on them
    UNICODE
    _UNICODE
    _CRT_SECURE_NO_WARNINGS
)

# baa_codegen depends on baa_utils and baa_types
# (assuming AST dependencies in llvm_codegen.c are handled/removed for now)
target_link_libraries(baa_codegen PRIVATE baa_utils baa_types)
