cmake_minimum_required(VERSION 3.20)

# Define the preprocessor library
add_library(baa_preprocessor STATIC
    preprocessor.c
)

# Ensure the library can find the main include directory
target_include_directories(baa_preprocessor
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include # Assuming main headers are in root/include
)
