# Workaround for MinGW builds picking up MSVC diaguids.lib dependency via LLVMDebugInfoPDB
if(LLVM_FOUND AND MINGW)
  if(TARGET LLVMDebugInfoPDB)
    # Get the existing link libraries for LLVMDebugInfoPDB
    get_target_property(PDB_LINK_LIBS LLVMDebugInfoPDB INTERFACE_LINK_LIBRARIES)

    # Construct the problematic library path (adjust if necessary for your VS install)
    set(DIAGUIDS_LIB "C:/Program Files (x86)/Microsoft Visual Studio/2019/Professional/DIA SDK/lib/amd64/diaguids.lib")

    # Remove it from the list
    list(REMOVE_ITEM PDB_LINK_LIBS "${DIAGUIDS_LIB}")

    # Set the modified list back
    set_target_properties(LLVMDebugInfoPDB PROPERTIES INTERFACE_LINK_LIBRARIES "${PDB_LINK_LIBS}")

    message(STATUS "Applied workaround: Removed diaguids.lib from LLVMDebugInfoPDB for MinGW build.")
  else()
     message(WARNING "LLVMDebugInfoPDB target not found, cannot apply diaguids.lib workaround.")
  endif()
endif()
# End workaround

add_executable(test_codegen
    test_codegen.c
)

# Add LLVM test
add_executable(test_llvm_codegen
    llvm_test.c
)

# Add literal system test for LLVM
add_executable(test_llvm_literals
    llvm_codegen_test.c
)

target_include_directories(test_codegen
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(test_llvm_codegen
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(test_llvm_literals
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# Add libraries for the non-LLVM test
target_link_libraries(test_codegen
    PRIVATE
        baa_utils
        baa_types
        baa_lexer
        # baa_parser # Add if needed by test_codegen.c
        baa_operators
        baa_ast
        baa_codegen
        test_framework # Restore this link
)

# Attempt to forcefully remove potentially incorrect dependencies added by LLVM's CMake
# ONLY for the LLVM-specific tests
set_property(TARGET test_llvm_codegen PROPERTY LINK_LIBRARIES "")
set_property(TARGET test_llvm_literals PROPERTY LINK_LIBRARIES "")

# Re-add the correct libraries for LLVM tests
target_link_libraries(test_llvm_codegen
    PRIVATE
        baa_utils
        baa_types
        baa_ast
        baa_codegen
)

target_link_libraries(test_llvm_literals
    PRIVATE
        baa_utils
        baa_types
        baa_ast
        baa_codegen
)

# Only run the tests if LLVM is available
if(LLVM_AVAILABLE)
    add_test(
        NAME test_codegen
        COMMAND test_codegen
    )

    add_test(
        NAME test_llvm_codegen
        COMMAND test_llvm_codegen
    )

    add_test(
        NAME test_llvm_literals
        COMMAND test_llvm_literals
    )
endif()
