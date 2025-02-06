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
    printf("\nTest Results:\n");
    printf("Total tests: %d\n", total_tests);
    printf("Passed: %d\n", passed_tests);
    printf("Failed: %d\n", failed_tests);
    printf("Success rate: %.2f%%\n", 
           (total_tests > 0) ? ((float)passed_tests / total_tests * 100) : 0);
}
