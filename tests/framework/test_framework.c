#include "test_framework.h"

int total_tests = 0;
int passed_tests = 0;
int failed_tests = 0;

void init_test_framework() {
    total_tests = 0;
    passed_tests = 0;
    failed_tests = 0;
}

void print_test_results() {
    wprintf(L"\nTest Results:\n");
    wprintf(L"Total tests: %d\n", total_tests);
    wprintf(L"Passed: %d\n", passed_tests);
    wprintf(L"Failed: %d\n", failed_tests);
    wprintf(L"Success rate: %.2f%%\n",
           (total_tests > 0) ? ((float)passed_tests / total_tests * 100) : 0);
}
