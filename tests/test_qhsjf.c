/*
 * QH-SJF Scheduling Lab: Standalone QH-SJF Aging Test
 */
#include <assert.h>
#include <stdio.h>
#include "../include/sim.h"

int main(void) {
    Proc p[3] = {
        {1, 0.0, 2.0, 2.0},
        {2, 0.0, 8.0, 8.0},
        {3, 0.0, 4.0, 4.0}
    };

    Result r;
    simulate(p, 3, POL_QH, 4.0, &r);

    assert(r.nrows == 3);
    for (int i = 0; i < 3; i++) {
        assert(r.rows[i].ct > 0.0);
    }

    printf("test_qhsjf: PASS\n");
    return 0;
}
