add_library(baa_operators
    operators.c
)

target_include_directories(baa_operators
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Removed redundant linking of utils, types as they are linked in the main executable
# target_link_libraries(baa_operators
#     PRIVATE
#     baa_utils
#     baa_types
# )
