add_library(baa_utils
    utils.c
)

target_include_directories(baa_utils
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(baa_utils PUBLIC # Or PRIVATE if only utils.c needs them
    UNICODE
    _UNICODE
    _CRT_SECURE_NO_WARNINGS
)
