add_library(baa_control_flow
    control_flow.c
    control_flow_errors.c
)

target_include_directories(baa_control_flow
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/include
)

target_link_libraries(baa_control_flow
    PRIVATE
        baa_utils
        baa_types
        baa_ast
)
