# cmake/BaaCompilerSettings.cmake

if(NOT TARGET BaaCommonSettings)
    add_library(BaaCommonSettings INTERFACE)
    target_compile_definitions(BaaCommonSettings INTERFACE
        UNICODE
        _UNICODE
        _CRT_SECURE_NO_WARNINGS
    )
    # Add other common project-wide compile options or definitions here if needed
    # For example:
    # target_compile_options(BaaCommonSettings INTERFACE
    #   $<$<COMPILE_LANGUAGE:C>:-Wall;-Wextra> # Example for GCC/Clang
    #   $<$<COMPILE_LANGUAGE:C>:$<$<CXX_COMPILER_ID:MSVC>:/W4>> # Example for MSVC
    # )
endif()
