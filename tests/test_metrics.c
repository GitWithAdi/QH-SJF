/*
 * QH-SJF Scheduling Lab: Standalone Metrics Test
 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../include/sim.h"

int main(void) {
    Proc p[2] = {
        {1, 0.0, 2.0, 2.0},
        {2, 0.0, 3.0, 3.0}
    };

    Result r;
    simulate(p, 2, POL_FCFS, 2.0, &r);

    assert(fabs(r.m.wt - 1.0) < 0.01);
    assert(fabs(r.m.tat - 3.5) < 0.01);
    assert(fabs(r.m.mk - 5.0) < 0.01);

    printf("test_metrics: PASS\n");
    return 0;
}
