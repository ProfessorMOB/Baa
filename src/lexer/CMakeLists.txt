add_library(baa_lexer
    lexer.c
)

target_include_directories(baa_lexer
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
