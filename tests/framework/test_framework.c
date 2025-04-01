#include "test_framework.h"
#include <wchar.h> // Include for wprintf

int total_tests = 0;
int passed_tests = 0;
int failed_tests = 0;
int current_failed_count = 0; // Add this

void init_test_framework() {
    total_tests = 0;
    passed_tests = 0;
    failed_tests = 0;
    current_failed_count = 0; // Initialize
}

void print_test_results() {
    wprintf(L"\n--- Test Results ---\n"); // Use wprintf
    wprintf(L"Total tests: %d\n", total_tests);
    wprintf(L"Passed: %d\n", passed_tests);
    wprintf(L"Failed: %d\n", failed_tests);
    wprintf(L"Success rate: %.2f%%\n",
           (total_tests > 0) ? ((double)passed_tests / total_tests * 100.0) : 0.0); // Use double for precision
}
