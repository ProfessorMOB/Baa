cmake_minimum_required(VERSION 3.20)
project(baa 
    VERSION 0.1.8
    DESCRIPTION "The Baa Programming Language"
    LANGUAGES C
)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add include directory
include_directories(${CMAKE_SOURCE_DIR}/src/include)

# Set source file extensions and encoding
set(BAA_SOURCE_EXTENSIONS ".ب" ".baa")

# Configure Unicode and UTF-8 support
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8")  # MSVC UTF-8 support
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -finput-charset=UTF-8 -fexec-charset=UTF-8")
endif()

# Add include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Add component subdirectories
add_subdirectory(src/utils)
add_subdirectory(src/types)
add_subdirectory(src/operators)
add_subdirectory(src/ast)
add_subdirectory(src/control_flow)

# Add main executable
add_executable(baa src/compiler.c)
target_link_libraries(baa
    PRIVATE
    baa_utils
    baa_types
    baa_operators
    baa_ast
    baa_control_flow
)

# Enable testing
enable_testing()
add_subdirectory(tests)

# Add test file
configure_file(
    ${CMAKE_SOURCE_DIR}/examples/برنامج.ب
    ${CMAKE_BINARY_DIR}/test.ب
    COPYONLY
)