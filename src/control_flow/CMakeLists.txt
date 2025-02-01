add_library(baa_control_flow
    control_flow.c
)

target_include_directories(baa_control_flow
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(baa_control_flow
    PRIVATE
        baa_utils
)
