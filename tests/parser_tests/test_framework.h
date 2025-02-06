#ifndef BAA_TEST_FRAMEWORK_H
#define BAA_TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <inttypes.h>

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("Running test: %s\n", #name); \
    test_##name(); \
    printf("Test %s passed\n", #name); \
    passed_tests++; \
    total_tests++; \
} while(0)

#define ASSERT(condition) do { \
    if (!(condition)) { \
        printf("Assertion failed: %s\n", #condition); \
        printf("File %s, line %d\n", __FILE__, __LINE__); \
        failed_tests++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(expected, actual) do { \
    __typeof__(expected) _expected = (expected); \
    __typeof__(actual) _actual = (actual); \
    if (_expected != _actual) { \
        if (sizeof(_expected) == sizeof(size_t)) { \
            printf("Assertion failed: expected %zu, got %zu\n", (size_t)_expected, (size_t)_actual); \
        } else { \
            printf("Assertion failed: expected %d, got %d\n", (int)_expected, (int)_actual); \
        } \
        printf("File %s, line %d\n", __FILE__, __LINE__); \
        failed_tests++; \
        return; \
    } \
} while(0)

#define ASSERT_STR_EQ(expected, actual) do { \
    if (strcmp((expected), (actual)) != 0) { \
        printf("Assertion failed: expected \"%s\", got \"%s\"\n", (expected), (actual)); \
        printf("File %s, line %d\n", __FILE__, __LINE__); \
        failed_tests++; \
        return; \
    } \
} while(0)

extern int total_tests;
extern int passed_tests;
extern int failed_tests;

void init_test_framework();
void print_test_results();

#endif // BAA_TEST_FRAMEWORK_H
