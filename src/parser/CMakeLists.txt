if(NOT SKIP_PARSER)
    # Main parser components
    add_library(baa_parser
        parser.c
        expression_parser.c
        statement_parser.c
        declaration_parser.c
        type_parser.c
        error_handling.c
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
        baa_operators
        baa_lexer
        baa_control_flow
    )

    # Install header files
    install(FILES
        ${PROJECT_SOURCE_DIR}/include/baa/parser/parser.h
        ${PROJECT_SOURCE_DIR}/include/baa/parser/expression_parser.h
        DESTINATION include/baa/parser
    )
else()
    # When skipping parser, create a minimal stub library
    add_library(baa_parser INTERFACE)

    target_include_directories(baa_parser
        INTERFACE
        ${PROJECT_SOURCE_DIR}/include
    )

    message(STATUS "Skipping parser compilation as requested")
endif()
