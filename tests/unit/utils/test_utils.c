#include <stdlib.h>
#include <string.h>
#include "baa/utils/errors.h"
#include "baa/utils/utils.h"
#include <assert.h>
#include <stdio.h>

void test_error_handling(void) {
    // Test error setting and getting
    baa_set_error(BAA_ERROR_MEMORY, L"Test error message");
    assert(baa_get_error() == BAA_ERROR_MEMORY);
    assert(wcscmp(baa_get_error_message(), L"Test error message") == 0);

    // Test error clearing
    baa_set_error(BAA_SUCCESS, NULL);
    assert(baa_get_error() == BAA_SUCCESS);
    assert(wcslen(baa_get_error_message()) == 0);
}

void test_memory_functions(void) {
    // Test malloc and free
    void *ptr = baa_malloc(100);
    assert(ptr != NULL);
    baa_free(ptr);

    // Test realloc
    ptr = baa_malloc(50);
    assert(ptr != NULL);
    void *new_ptr = baa_realloc(ptr, 200);
    assert(new_ptr != NULL);
    baa_free(new_ptr);
}

void test_string_functions(void) {
    // Test strdup
    const wchar_t *test_str = L"Test string";
    wchar_t *dup_str = baa_strdup(test_str);
    assert(dup_str != NULL);
    assert(wcscmp(test_str, dup_str) == 0);
    baa_free(dup_str);

    // Test strcmp
    assert(baa_strcmp(L"abc", L"abc") == 0);
    assert(baa_strcmp(L"abc", L"def") < 0);
    assert(baa_strcmp(L"def", L"abc") > 0);
    assert(baa_strcmp(NULL, L"abc") < 0);
    assert(baa_strcmp(L"abc", NULL) > 0);
    assert(baa_strcmp(NULL, NULL) == 0);
}

int main(void) {
    printf("Running utils tests...\n");

    test_error_handling();
    test_memory_functions();
    test_string_functions();

    printf("All utils tests passed!\n");
    return 0;
}
