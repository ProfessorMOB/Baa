add_library(baa_parser
    parser.c
)

target_include_directories(baa_parser
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
