#include "test_framework.h"
#include "baa/lexer/lexer.h"
#include "baa/parser/parser.h"
#include "baa/codegen/codegen.h"
#include "baa/utils/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>

// Test fixture
typedef struct {
    BaaLexer lexer;
    BaaParser parser;
    BaaProgram* program;
    BaaCodeGen codegen;
    BaaCodeGenOptions options;
    wchar_t* output_file;
} CodegenTestFixture;

// Setup function for tests
static void setup(CodegenTestFixture* fixture) {
    // Set locale for wide character support
    setlocale(LC_ALL, "");

    // Initialize output file
    fixture->output_file = L"test_output.ll";

    // Initialize code generation options
    fixture->options.target = BAA_TARGET_X86_64;
    fixture->options.optimize = false;
    fixture->options.debug_info = false;
    fixture->options.output_file = fixture->output_file;

    // Program will be set in individual tests
    fixture->program = NULL;
}

// Teardown function for tests
static void teardown(CodegenTestFixture* fixture) {
    if (fixture->program) {
        baa_free_program(fixture->program);
        fixture->program = NULL;
    }

    baa_cleanup_codegen();
}

// Helper function to parse source code
static BaaProgram* parse_source(CodegenTestFixture* fixture, const wchar_t* source) {
    // Initialize lexer
    baa_init_lexer(&fixture->lexer, source, L"<test>");

    // Initialize parser
    baa_init_parser(&fixture->parser, &fixture->lexer);

    // Parse program
    return baa_parse_program(&fixture->parser);
}

// Test basic function generation
static void test_function_generation(void) {
    CodegenTestFixture fixture;
    setup(&fixture);

    // Simple function with return statement
    const wchar_t* source = L"دالة رئيسية() {\n"
                           L"    إرجع 0.\n"
                           L"}\n";

    fixture.program = parse_source(&fixture, source);
    ASSERT_NOT_NULL(fixture.program, "Failed to parse program");

    // Initialize code generator
    baa_init_codegen(&fixture.codegen, fixture.program, &fixture.options);

    // Generate code
    bool result = baa_generate_code(&fixture.codegen);
    ASSERT_TRUE(result, "Code generation failed");

    // Check if output file exists
    FILE* file = _wfopen(fixture.output_file, L"r");
    ASSERT_NOT_NULL(file, "Output file not created");
    if (file) {
        fclose(file);
    }

    teardown(&fixture);
}

// Test if statement generation
static void test_if_statement_generation(void) {
    CodegenTestFixture fixture;
    setup(&fixture);

    // Function with if statement
    const wchar_t* source = L"دالة رئيسية() {\n"
                           L"    إذا (1) {\n"
                           L"        إرجع 1.\n"
                           L"    } وإلا {\n"
                           L"        إرجع 0.\n"
                           L"    }\n"
                           L"}\n";

    fixture.program = parse_source(&fixture, source);
    ASSERT_NOT_NULL(fixture.program, "Failed to parse program");

    // Initialize code generator
    baa_init_codegen(&fixture.codegen, fixture.program, &fixture.options);

    // Generate code
    bool result = baa_generate_code(&fixture.codegen);
    ASSERT_TRUE(result, "Code generation failed");

    teardown(&fixture);
}

// Test while loop generation
static void test_while_loop_generation(void) {
    CodegenTestFixture fixture;
    setup(&fixture);

    // Function with while loop
    const wchar_t* source = L"دالة رئيسية() {\n"
                           L"    طالما (1) {\n"
                           L"        إرجع 1.\n"
                           L"    }\n"
                           L"    إرجع 0.\n"
                           L"}\n";

    fixture.program = parse_source(&fixture, source);
    ASSERT_NOT_NULL(fixture.program, "Failed to parse program");

    // Initialize code generator
    baa_init_codegen(&fixture.codegen, fixture.program, &fixture.options);

    // Generate code
    bool result = baa_generate_code(&fixture.codegen);
    ASSERT_TRUE(result, "Code generation failed");

    teardown(&fixture);
}

// Test expression generation
static void test_expression_generation(void) {
    CodegenTestFixture fixture;
    setup(&fixture);

    // Function with expressions
    const wchar_t* source = L"دالة رئيسية() {\n"
                           L"    إرجع 1 + 2 * 3.\n"
                           L"}\n";

    fixture.program = parse_source(&fixture, source);
    ASSERT_NOT_NULL(fixture.program, "Failed to parse program");

    // Initialize code generator
    baa_init_codegen(&fixture.codegen, fixture.program, &fixture.options);

    // Generate code
    bool result = baa_generate_code(&fixture.codegen);
    ASSERT_TRUE(result, "Code generation failed");

    teardown(&fixture);
}

// Main function
int main(void) {
    // Initialize test suite
    TEST_SUITE_BEGIN();

    // Add tests
    TEST_CASE(test_function_generation);
    TEST_CASE(test_if_statement_generation);
    TEST_CASE(test_while_loop_generation);
    TEST_CASE(test_expression_generation);

    // Run tests
    TEST_SUITE_END();

    return 0;
}
