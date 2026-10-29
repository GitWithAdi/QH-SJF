/*
 * QH-SJF Scheduling Lab
 * Command-Line Unit & Invariant Test Runner (C11)
 */
#include <stdio.h>
#include "tests.h"

int main(void) {
    printf("=================================================================\n");
    printf("  QH-SJF SCHEDULING LAB: UNIT & PAPER INVARIANT TEST RUNNER       \n");
    printf("=================================================================\n\n");

    TestSuiteResult suite;
    run_all_tests(&suite);

    for (int i = 0; i < suite.count; i++) {
        const TestCaseResult *t = &suite.tests[i];
        printf("[%s] %s (%.2f ms)\n",
               t->passed ? "PASS" : "FAIL",
               t->name,
               (double)t->duration_us / 1000.0);
        printf("       %s\n", t->description);
        printf("       -> %s\n\n", t->details);
    }

    printf("-----------------------------------------------------------------\n");
    printf("Summary: %d / %d tests passed (%.2f ms total)\n",
           suite.passed, suite.count, (double)suite.total_duration_us / 1000.0);
    printf("=================================================================\n");

    return suite.failed == 0 ? 0 : 1;
}
