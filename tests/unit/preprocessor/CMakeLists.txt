# CMakeLists.txt for Baa Preprocessor Unit Tests

# Define the test executable
add_executable(preprocessor_unit_tests
    test_preprocessor.c
)

# Link the test executable against the Baa library and the test framework
target_link_libraries(preprocessor_unit_tests PRIVATE
    baa_lib          # Link against the main Baa library (which includes preprocessor code)
    test_framework   # Link against the test framework library
)

# Add the test executable to CTest
add_test(NAME PreprocessorUnitTests COMMAND preprocessor_unit_tests)
