/*
 * QH-SJF Scheduling Lab: Standalone Policies Test
 */
#include <assert.h>
#include <stdio.h>
#include "../include/sim.h"

int main(void) {
    Proc p[3] = {
        {1, 0.0, 8.0, 8.0},
        {2, 0.0, 2.0, 2.0},
        {3, 0.0, 4.0, 4.0}
    };

    Result r_sjf, r_fcfs, r_hrrn;
    simulate(p, 3, POL_SJF, 4.0, &r_sjf);
    simulate(p, 3, POL_FCFS, 4.0, &r_fcfs);
    simulate(p, 3, POL_HRRN, 4.0, &r_hrrn);

    assert(r_sjf.rows[0].id == 2);
    assert(r_fcfs.rows[0].id == 1);
    assert(r_hrrn.rows[0].id == 1);

    printf("test_policies: PASS\n");
    return 0;
}
