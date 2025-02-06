add_library(baa_parser
    parser.c
)

target_include_directories(baa_parser
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(baa_parser
    PRIVATE
    baa_ast
    baa_utils
)

# Install header files
install(FILES
    tokens.h
    DESTINATION include/baa/parser
)
