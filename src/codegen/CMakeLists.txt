add_library(baa_codegen
    codegen.c
)

target_include_directories(baa_codegen
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
