add_library(baa_operators
    operators.c
)

target_include_directories(baa_operators
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# baa_operators depends on baa_types
target_link_libraries(baa_operators PRIVATE baa_types INTERFACE BaaCommonSettings)
# It does not seem to directly depend on baa_utils, but baa_types does.
