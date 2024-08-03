cmake_minimum_required(VERSION 3.20)
project(Baa VERSION 0.0.1 LANGUAGES C CXX)

# User defined variables
set(
        NATIVE_OPT TRUE
        CACHE BOOL "Enable native optimizations"
)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
SET(CMAKE_C_STANDARD 11)
SET(CMAKE_CXX_STANDARD 23)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

add_executable(
        Baa
        src/main.c
)