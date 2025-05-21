# CMakeLists.txt for the analysis module

add_library(baa_flow_analysis
    flow_analysis.c
    flow_errors.c
)

target_include_directories(baa_flow_analysis
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include # Access to baa/analysis/ headers
)

# baa_flow_analysis depends on baa_types and baa_utils
target_link_libraries(baa_flow_analysis PRIVATE baa_types baa_utils INTERFACE BaaCommonSettings)
