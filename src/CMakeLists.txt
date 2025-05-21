# Add subdirectories for each component
add_subdirectory(utils)
add_subdirectory(types)
add_subdirectory(operators)
add_subdirectory(lexer)
add_subdirectory(preprocessor) # Added missing preprocessor
add_subdirectory(analysis)
add_subdirectory(codegen)
# Parser is removed, so no add_subdirectory(parser)
