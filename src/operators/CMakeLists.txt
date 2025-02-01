add_library(baa_operators
    operators.c
)

target_include_directories(baa_operators
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(baa_operators
    PRIVATE
    baa_utils
    baa_types
)
