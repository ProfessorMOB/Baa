#include "tests/framework/test_framework.h"
#include "baa/preprocessor/preprocessor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#ifdef _WIN32
#include <windows.h> // For CP_UTF8, MultiByteToWideChar, _wfopen_s
#endif

// Helper function to create temporary files for testing includes/directives
static char *create_temp_file(const char *name_prefix, const wchar_t *content)
{
    // TODO: Implement platform-specific temporary file creation
    // For now, just return a placeholder name. Actual file creation needed.
    static char temp_path[256];
    // Use sprintf_s on Windows if available, otherwise sprintf
#ifdef _WIN32
    sprintf_s(temp_path, sizeof(temp_path), "%s_temp.baa", name_prefix);
#else
    sprintf(temp_path, "%s_temp.baa", name_prefix);
#endif

    // Convert wchar_t content to UTF-16LE bytes with BOM
    FILE *fp = NULL;
#ifdef _WIN32
    wchar_t w_temp_path[256];
    if (MultiByteToWideChar(CP_UTF8, 0, temp_path, -1, w_temp_path, 256) == 0)
    {
        perror("Failed to convert temp path to wide char");
        return NULL;
    }
    // Use _wfopen_s on Windows if available, otherwise _wfopen
    errno_t err = _wfopen_s(&fp, w_temp_path, L"wb");
    if (err != 0 || fp == NULL)
    {
        perror("Failed to create temp file (Windows)");
        return NULL;
    }
#else
    fp = fopen(temp_path, "wb");
    if (!fp)
    {
        perror("Failed to create temp file");
        return NULL;
    }
#endif

    // Write BOM
    unsigned char bom[] = {0xFF, 0xFE};
    fwrite(bom, sizeof(unsigned char), 2, fp);

    // Write content (assuming wchar_t is 2 bytes on target platform)
    fwrite(content, sizeof(wchar_t), wcslen(content), fp);

    fclose(fp);

    // Return allocated path (caller should free if dynamically allocated)
    // For this static buffer version, caller doesn't free.
    // A better implementation would allocate memory for the path.
    char *allocated_path = strdup(temp_path); // Allocate memory for the path
    return allocated_path;
}

// --- Test Functions ---

// Note: Test functions are just standard C functions.
// The TEST_CASE macro in the suite definition runs them.

void PreprocessorConditional_IfdefDefined()
{
    const char *main_content_path = NULL;
    wchar_t *error_msg = NULL;
    wchar_t *result = NULL;

    // Create main file content
    const wchar_t *main_content = L"#تعريف MY_MACRO\n"
                                  L"#إذا_عرف MY_MACRO\n"
                                  L"هذا يجب أن يظهر\n"
                                  L"#نهاية_إذا\n"
                                  L"#إذا_عرف OTHER_MACRO\n"
                                  L"هذا لا يجب أن يظهر\n"
                                  L"#نهاية_إذا\n";
    main_content_path = create_temp_file("ifdef_defined", main_content);
    ASSERT_NOT_NULL(main_content_path, L"Failed to create temp file for IfdefDefined"); // Add message

    result = baa_preprocess(main_content_path, NULL, &error_msg);

    ASSERT_NULL(error_msg, error_msg ? error_msg : L"No error expected"); // Use ASSERT_NULL, add message
    ASSERT_NOT_NULL(result, L"Preprocessor result should not be NULL"); // Use ASSERT_NOT_NULL, add message

    const wchar_t *expected = L"هذا يجب أن يظهر\n";
    ASSERT_STR_EQ(expected, result); // Use ASSERT_STR_EQ(expected, actual)

    free(result);
    free(error_msg);
    remove(main_content_path);       // Clean up temp file
    free((void *)main_content_path); // Free allocated path
}

void PreprocessorConditional_IfdefNotDefined()
{
    const char *main_content_path = NULL;
    wchar_t *error_msg = NULL;
    wchar_t *result = NULL;

    const wchar_t *main_content = L"#إذا_عرف MY_MACRO\n"
                                  L"هذا لا يجب أن يظهر\n"
                                  L"#نهاية_إذا\n"
                                  L"هذا يجب أن يظهر دائما\n";
    main_content_path = create_temp_file("ifdef_notdef", main_content);
    ASSERT_NOT_NULL(main_content_path, L"Failed to create temp file for IfdefNotDefined"); // Add message

    result = baa_preprocess(main_content_path, NULL, &error_msg);

    ASSERT_NULL(error_msg, error_msg ? error_msg : L"No error expected"); // Use ASSERT_NULL, add message
    ASSERT_NOT_NULL(result, L"Preprocessor result should not be NULL"); // Use ASSERT_NOT_NULL, add message

    const wchar_t *expected = L"هذا يجب أن يظهر دائما\n";
    ASSERT_STR_EQ(expected, result); // Use ASSERT_STR_EQ(expected, actual)

    free(result);
    free(error_msg);
    remove(main_content_path);
    free((void *)main_content_path);
}

void PreprocessorConditional_IfndefDefined()
{
    const char *main_content_path = NULL;
    wchar_t *error_msg = NULL;
    wchar_t *result = NULL;

    const wchar_t *main_content = L"#تعريف MY_MACRO\n"
                                  L"#إذا_لم_يعرف MY_MACRO\n"
                                  L"هذا لا يجب أن يظهر\n"
                                  L"#نهاية_إذا\n"
                                  L"هذا يجب أن يظهر دائما\n";
    main_content_path = create_temp_file("ifndef_def", main_content);
    ASSERT_NOT_NULL(main_content_path, L"Failed to create temp file for IfndefDefined"); // Add message

    result = baa_preprocess(main_content_path, NULL, &error_msg);

    ASSERT_NULL(error_msg, error_msg ? error_msg : L"No error expected"); // Use ASSERT_NULL, add message
    ASSERT_NOT_NULL(result, L"Preprocessor result should not be NULL"); // Use ASSERT_NOT_NULL, add message

    const wchar_t *expected = L"هذا يجب أن يظهر دائما\n";
    ASSERT_STR_EQ(expected, result); // Use ASSERT_STR_EQ(expected, actual)

    free(result);
    free(error_msg);
    remove(main_content_path);
    free((void *)main_content_path);
}

void PreprocessorConditional_IfndefNotDefined()
{
    const char *main_content_path = NULL;
    wchar_t *error_msg = NULL;
    wchar_t *result = NULL;

    const wchar_t *main_content = L"#إذا_لم_يعرف MY_MACRO\n"
                                  L"هذا يجب أن يظهر\n"
                                  L"#نهاية_إذا\n";
    main_content_path = create_temp_file("ifndef_notdef", main_content);
    ASSERT_NOT_NULL(main_content_path, L"Failed to create temp file for IfndefNotDefined"); // Add message

    result = baa_preprocess(main_content_path, NULL, &error_msg);

    ASSERT_NULL(error_msg, error_msg ? error_msg : L"No error expected"); // Use ASSERT_NULL, add message
    ASSERT_NOT_NULL(result, L"Preprocessor result should not be NULL"); // Use ASSERT_NOT_NULL, add message

    const wchar_t *expected = L"هذا يجب أن يظهر\n";
    ASSERT_STR_EQ(expected, result); // Use ASSERT_STR_EQ(expected, actual)

    free(result);
    free(error_msg);
    remove(main_content_path);
    free((void *)main_content_path);
}

void PreprocessorConditional_IfdefElse()
{
    const char *main_content_path = NULL;
    wchar_t *error_msg = NULL;
    wchar_t *result = NULL;

    // Case 1: Defined
    const wchar_t *main_content1 = L"#تعريف FLAG\n"
                                   L"#إذا_عرف FLAG\n"
                                   L"معرف\n"
                                   L"#إلا\n"
                                   L"غير معرف\n"
                                   L"#نهاية_إذا\n";
    main_content_path = create_temp_file("ifdef_else1", main_content1);
    ASSERT_NOT_NULL(main_content_path, L"Failed to create temp file for IfdefElse (Case 1)"); // Add message
    result = baa_preprocess(main_content_path, NULL, &error_msg);
    ASSERT_NULL(error_msg, error_msg ? error_msg : L"No error expected (Case 1)"); // Use ASSERT_NULL, add message
    ASSERT_NOT_NULL(result, L"Preprocessor result should not be NULL (Case 1)"); // Use ASSERT_NOT_NULL, add message
    ASSERT_STR_EQ(L"معرف\n", result); // Use ASSERT_STR_EQ(expected, actual)
    free(result);
    free(error_msg);
    error_msg = NULL;
    remove(main_content_path);
    free((void *)main_content_path);

    // Case 2: Not Defined
    const wchar_t *main_content2 = L"#إذا_عرف FLAG\n"
                                   L"معرف\n"
                                   L"#إلا\n"
                                   L"غير معرف\n"
                                   L"#نهاية_إذا\n";
    main_content_path = create_temp_file("ifdef_else2", main_content2);
    ASSERT_NOT_NULL(main_content_path, L"Failed to create temp file for IfdefElse (Case 2)"); // Add message
    result = baa_preprocess(main_content_path, NULL, &error_msg);
    ASSERT_NULL(error_msg, error_msg ? error_msg : L"No error expected (Case 2)"); // Use ASSERT_NULL, add message
    ASSERT_NOT_NULL(result, L"Preprocessor result should not be NULL (Case 2)"); // Use ASSERT_NOT_NULL, add message
    ASSERT_STR_EQ(L"غير معرف\n", result); // Use ASSERT_STR_EQ(expected, actual)
    free(result);
    free(error_msg);
    error_msg = NULL;
    remove(main_content_path);
    free((void *)main_content_path);
}

void PreprocessorConditional_NestedConditionals()
{
    const char *main_content_path = NULL;
    wchar_t *error_msg = NULL;
    wchar_t *result = NULL;

    const wchar_t *main_content = L"#تعريف OUTER\n"
                                  L"#تعريف INNER\n"
                                  L"#إذا_عرف OUTER\n" // True
                                  L"خارجي صحيح\n"
                                  L"  #إذا_عرف INNER\n" // True
                                  L"  داخلي صحيح\n"
                                  L"  #نهاية_إذا\n"
                                  L"  #إذا_لم_يعرف INNER_OTHER\n" // True
                                  L"  داخلي آخر صحيح\n"
                                  L"  #نهاية_إذا\n"
                                  L"#إلا\n" // Skipped
                                  L"خارجي خطأ\n"
                                  L"#نهاية_إذا\n"
                                  L"#إذا_لم_يعرف OUTER_OTHER\n" // True
                                  L"خارجي آخر صحيح\n"
                                  L"  #إذا_عرف INNER_FAKE\n" // False
                                  L"  داخلي مزيف خطأ\n"
                                  L"  #إلا\n" // True
                                  L"  داخلي مزيف صحيح\n"
                                  L"  #نهاية_إذا\n"
                                  L"#نهاية_إذا\n";

    main_content_path = create_temp_file("nested", main_content);
    ASSERT_NOT_NULL(main_content_path, L"Failed to create temp file for NestedConditionals"); // Add message

    result = baa_preprocess(main_content_path, NULL, &error_msg);

    ASSERT_NULL(error_msg, error_msg ? error_msg : L"No error expected"); // Use ASSERT_NULL, add message
    ASSERT_NOT_NULL(result, L"Preprocessor result should not be NULL"); // Use ASSERT_NOT_NULL, add message

    const wchar_t *expected = L"خارجي صحيح\n"
                              L"  داخلي صحيح\n"
                              L"  داخلي آخر صحيح\n"
                              L"خارجي آخر صحيح\n"
                              L"  داخلي مزيف صحيح\n";
    ASSERT_STR_EQ(expected, result); // Use ASSERT_STR_EQ(expected, actual)

    free(result);
    free(error_msg);
    remove(main_content_path);
    free((void *)main_content_path);
}

void PreprocessorConditional_UnterminatedIfdef()
{
    const char *main_content_path = NULL;
    wchar_t *error_msg = NULL;
    wchar_t *result = NULL;

    const wchar_t *main_content = L"#إذا_عرف MY_MACRO\n"
                                  L"محتوى\n";
    // Missing #نهاية_إذا
    main_content_path = create_temp_file("unterminated", main_content);
    ASSERT_NOT_NULL(main_content_path, L"Failed to create temp file for UnterminatedIfdef"); // Add message

    result = baa_preprocess(main_content_path, NULL, &error_msg);

    ASSERT_NOT_NULL(error_msg, L"Expected an error for unterminated #ifdef"); // Use ASSERT_NOT_NULL, add message
    ASSERT_NULL(result, L"Result should be NULL on error"); // Use ASSERT_NULL, add message
    // TODO: Add a check for the specific error message content if needed (e.g., using wcsstr)
    // ASSERT_WCS_CONTAINS(error_msg, L"كتلة شرطية غير منتهية"); // Macro not defined in framework

    free(result); // Should be NULL already
    free(error_msg);
    remove(main_content_path);
    free((void *)main_content_path);
}

void PreprocessorConditional_MismatchedEndif()
{
    const char *main_content_path = NULL;
    wchar_t *error_msg = NULL;
    wchar_t *result = NULL;

    const wchar_t *main_content = L"#نهاية_إذا\n"; // #endif without #if...
    main_content_path = create_temp_file("mismatched", main_content);
    ASSERT_NOT_NULL(main_content_path, L"Failed to create temp file for MismatchedEndif"); // Add message

    result = baa_preprocess(main_content_path, NULL, &error_msg);

    ASSERT_NOT_NULL(error_msg, L"Expected an error for mismatched #endif"); // Use ASSERT_NOT_NULL, add message
    ASSERT_NULL(result, L"Result should be NULL on error"); // Use ASSERT_NULL, add message
    // TODO: Add a check for the specific error message content if needed (e.g., using wcsstr)
    // ASSERT_WCS_CONTAINS(error_msg, L"#نهاية_إذا بدون #إذا_عرف مطابق"); // Macro not defined in framework

    free(result);
    free(error_msg);
    remove(main_content_path);
    free((void *)main_content_path);
}

// TODO: Add tests for #elif when implemented
// TODO: Add tests for #include within conditional blocks
// TODO: Add tests for #define/#undef within conditional blocks

// --- Test Suite Main Function ---

TEST_SUITE_BEGIN()
    TEST_CASE(PreprocessorConditional_IfdefDefined);
    TEST_CASE(PreprocessorConditional_IfdefNotDefined);
    TEST_CASE(PreprocessorConditional_IfndefDefined);
    TEST_CASE(PreprocessorConditional_IfndefNotDefined);
    TEST_CASE(PreprocessorConditional_IfdefElse);
    TEST_CASE(PreprocessorConditional_NestedConditionals);
    TEST_CASE(PreprocessorConditional_UnterminatedIfdef);
    TEST_CASE(PreprocessorConditional_MismatchedEndif);
TEST_SUITE_END()
