#ifndef BAA_TEST_FRAMEWORK_H
#define BAA_TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <inttypes.h>
#include <wchar.h> // Include for wprintf
#include <math.h>  // Include for fabs in double equal

// --- Test Suite Macros ---
#define TEST_SUITE_BEGIN()     \
    int main(void)             \
    {                          \
        init_test_framework(); \
        wprintf(L"Starting Test Suite...\n\n");

#define TEST_CASE(name)                                                           \
    do                                                                            \
    {                                                                             \
        wprintf(L"--- Running test: %ls ---\n", L"" #name);                       \
        total_tests++;                                                            \
        name();                                                                   \
        if (failed_tests == current_failed_count)                                 \
        { /* Check if test function incremented failed_tests */                   \
            wprintf(L"--- Test PASSED: %ls ---\n\n", L"" #name);                  \
            passed_tests++;                                                       \
        }                                                                         \
        else                                                                      \
        {                                                                         \
            wprintf(L"--- Test FAILED: %ls ---\n\n", L"" #name);                  \
            current_failed_count = failed_tests; /* Update count for next test */ \
        }                                                                         \
    } while (0)

#define TEST_SUITE_END()       \
    print_test_results();      \
    return (failed_tests > 0); \
    }

// --- Assertion Macros ---
#define ASSERT(condition)                                                          \
    do                                                                             \
    {                                                                              \
        if (!(condition))                                                          \
        {                                                                          \
            wprintf(L"[ASSERT FAILED] Condition: %ls\n", L"" #condition);          \
            wprintf(L"                File: %hs, Line: %d\n", __FILE__, __LINE__); \
            failed_tests++;                                                        \
            /* return; // Don't return from main, just record failure */           \
        }                                                                          \
    } while (0)

#define ASSERT_TRUE(condition, message)                                                   \
    do                                                                                    \
    {                                                                                     \
        if (!(condition))                                                                 \
        {                                                                                 \
            wprintf(L"[ASSERT FAILED] Expected true: %ls\n", (const wchar_t *)(message)); \
            wprintf(L"                File: %hs, Line: %d\n", __FILE__, __LINE__);        \
            failed_tests++;                                                               \
        }                                                                                 \
    } while (0)

// Generic ASSERT_EQUAL using _Generic (C11) if possible, otherwise needs specific types
// This is a simplified version. A real framework might have type-specific ones.
#define ASSERT_EQUAL(expected, actual, message)                                                                                                     \
    do                                                                                                                                              \
    {                                                                                                                                               \
        if ((expected) != (actual))                                                                                                                 \
        {                                                                                                                                           \
            /* Use format specifiers appropriate for common types */                                                                                \
            if (sizeof(expected) == sizeof(long long))                                                                                              \
            {                                                                                                                                       \
                wprintf(L"[ASSERT FAILED] %ls: Expected %lld, Got %lld\n", (const wchar_t *)(message), (long long)(expected), (long long)(actual)); \
            }                                                                                                                                       \
            else if (sizeof(expected) == sizeof(int))                                                                                               \
            {                                                                                                                                       \
                wprintf(L"[ASSERT FAILED] %ls: Expected %d, Got %d\n", (const wchar_t *)(message), (int)(expected), (int)(actual));                 \
            }                                                                                                                                       \
            else                                                                                                                                    \
            {                                                                                                                                       \
                wprintf(L"[ASSERT FAILED] %ls: Values not equal (non-standard size)\n", (const wchar_t *)(message));                                \
            }                                                                                                                                       \
            wprintf(L"                File: %hs, Line: %d\n", __FILE__, __LINE__);                                                                  \
            failed_tests++;                                                                                                                         \
        }                                                                                                                                           \
    } while (0)

// Macro similar to the original ASSERT_EQ for specific integer types if needed
#define ASSERT_EQ(expected, actual) ASSERT_EQUAL(expected, actual, L"Values not equal")

#define ASSERT_STR_EQ(expected, actual)                                                                                                   \
    do                                                                                                                                    \
    {                                                                                                                                     \
        if (wcscmp((expected), (actual)) != 0)                                                                                            \
        {                                                                                                                                 \
            wprintf(L"[ASSERT FAILED] Expected String: \"%ls\", Got: \"%ls\"\n", (const wchar_t *)(expected), (const wchar_t *)(actual)); \
            wprintf(L"                File: %hs, Line: %d\n", __FILE__, __LINE__);                                                        \
            failed_tests++;                                                                                                               \
        }                                                                                                                                 \
    } while (0)

#define ASSERT_NOT_NULL(ptr, message)                                                         \
    do                                                                                        \
    {                                                                                         \
        if ((ptr) == NULL)                                                                    \
        {                                                                                     \
            wprintf(L"[ASSERT FAILED] Expected non-NULL: %ls\n", (const wchar_t *)(message)); \
            wprintf(L"                File: %hs, Line: %d\n", __FILE__, __LINE__);            \
            failed_tests++;                                                                   \
        }                                                                                     \
    } while (0)

#define ASSERT_NULL(ptr, message)                                                         \
    do                                                                                    \
    {                                                                                     \
        if ((ptr) != NULL)                                                                \
        {                                                                                 \
            wprintf(L"[ASSERT FAILED] Expected NULL: %ls\n", (const wchar_t *)(message)); \
            wprintf(L"                File: %hs, Line: %d\n", __FILE__, __LINE__);        \
            failed_tests++;                                                               \
        }                                                                                 \
    } while (0)

#define ASSERT_DOUBLE_EQUAL(expected, actual, tolerance, message)                                                                                                            \
    do                                                                                                                                                                       \
    {                                                                                                                                                                        \
        if (fabs((double)(expected) - (double)(actual)) > (double)(tolerance))                                                                                               \
        {                                                                                                                                                                    \
            wprintf(L"[ASSERT FAILED] %ls: Expected %lf, Got %lf (Tolerance %lf)\n", (const wchar_t *)(message), (double)(expected), (double)(actual), (double)(tolerance)); \
            wprintf(L"                File: %hs, Line: %d\n", __FILE__, __LINE__);                                                                                           \
            failed_tests++;                                                                                                                                                  \
        }                                                                                                                                                                    \
    } while (0)

// --- Framework Globals ---
extern int total_tests;
extern int passed_tests;
extern int failed_tests;
// Helper variable for TEST_CASE
extern int current_failed_count;

// --- Framework Functions ---
void init_test_framework();
void print_test_results();

#endif // BAA_TEST_FRAMEWORK_H
