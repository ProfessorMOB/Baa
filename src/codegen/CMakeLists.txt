# Create the library target first
add_library(baa_codegen)

# Add LLVM-specific files only if LLVM is available and found
if(USE_LLVM AND LLVM_FOUND) # Check both the option and if find_package succeeded
    target_sources(baa_codegen PRIVATE
        codegen.c
        llvm_codegen.c
    )
    target_compile_definitions(baa_codegen PRIVATE
    LLVM_AVAILABLE=1
    )

    # Make sure LLVM CMake modules are included (may not be strictly necessary if find_package in root works well)
    # list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}") # Likely not needed here
    # include(AddLLVM) # Likely not needed here

    # Get required LLVM libraries
    # Prefer linking against imported LLVM targets
    # llvm_map_components_to_libnames(llvm_libs ... )
    # Note: llvm_map_components_to_libnames is useful for older CMake/LLVM versions
    # or specific scenarios, but linking imported targets is generally preferred.

    # Print the determined LLVM libraries for debugging (optional)
    # message(STATUS "LLVM Libraries (via map): ${llvm_libs}")

    # Add LLVM include directories
    target_include_directories(baa_codegen PRIVATE ${LLVM_INCLUDE_DIRS})

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
            # ${llvm_libs} # If using llvm_map_components_to_libnames
    )
else()
    # Add stub implementation when LLVM is not available
    target_sources(baa_codegen PRIVATE
        codegen.c
        llvm_stub.c
    )
    target_compile_definitions(baa_codegen PRIVATE
    LLVM_AVAILABLE=0
    )
endif()

target_include_directories(baa_codegen
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include  # For baa/codegen/codegen.h and baa/codegen/llvm_codegen.h
)

target_compile_definitions(baa_codegen PUBLIC # Propagate if public headers depend on them
    UNICODE
    _UNICODE
    _CRT_SECURE_NO_WARNINGS
)

# baa_codegen depends on baa_utils and baa_types
# (assuming AST dependencies in llvm_codegen.c are handled/removed for now)
target_link_libraries(baa_codegen PRIVATE baa_utils baa_types)
