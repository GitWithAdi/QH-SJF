/*
 * QH-SJF Scheduling Lab
 * Unit & Invariant Test Suite Definitions (C11)
 */
#ifndef TESTS_H
#define TESTS_H

#define MAX_TESTS 16

typedef struct {
    char name[64];
    char description[160];
    int  passed;           /* 1 for PASS, 0 for FAIL */
    char details[256];
    long duration_us;      /* Execution time in microseconds */
} TestCaseResult;

typedef struct {
    TestCaseResult tests[MAX_TESTS];
    int count;
    int passed;
    int failed;
    long total_duration_us;
} TestSuiteResult;

int  run_test_process(TestCaseResult *out);
int  run_test_policies(TestCaseResult *out);
int  run_test_simulator(TestCaseResult *out);
int  run_test_metrics(TestCaseResult *out);
int  run_test_qhsjf(TestCaseResult *out);
int  run_test_invariants(TestCaseResult *out);

void run_all_tests(TestSuiteResult *suite);

#endif /* TESTS_H */
