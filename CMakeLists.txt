cmake_minimum_required(VERSION 3.20)
project(Baa 
    VERSION 0.0.1
    DESCRIPTION "A compiler for the B (باء) programming language"
    LANGUAGES C CXX
)

# Options
option(BAA_BUILD_TESTS "Build test suite" ON)
option(BAA_NATIVE_OPT "Enable native optimizations" ON)
option(BAA_BUILD_DOCS "Build documentation" OFF)

# Global settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif()

# Compiler flags
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(BAA_NATIVE_OPT)
        add_compile_options(-march=native)
    endif()
elseif(MSVC)
    add_compile_options(/W4)
endif()

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Add subdirectories for different components
add_subdirectory(src)

if(BAA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BAA_BUILD_DOCS)
    add_subdirectory(docs)
endif()

# Install rules
install(TARGETS Baa
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)