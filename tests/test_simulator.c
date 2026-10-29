/*
 * QH-SJF Scheduling Lab: Standalone Simulator Test
 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../include/sim.h"

int main(void) {
    Proc p[3] = {
        {1, 0.0, 3.0, 3.0},
        {2, 0.0, 1.0, 1.0},
        {3, 2.0, 2.0, 2.0}
    };

    Result r;
    simulate(p, 3, POL_SJF, 2.0, &r);

    assert(r.rows[0].id == 2);
    assert(fabs(r.rows[0].st - 0.0) < 0.01);
    assert(fabs(r.rows[1].ct - 4.0) < 0.01);
    assert(fabs(r.rows[2].st - 4.0) < 0.01);

    printf("test_simulator: PASS\n");
    return 0;
}
