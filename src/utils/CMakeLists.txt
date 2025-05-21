add_library(baa_utils
    utils.c
)

target_include_directories(baa_utils
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
# Common settings are inherited by linking Baa::CommonSettings
target_link_libraries(baa_utils INTERFACE BaaCommonSettings)
