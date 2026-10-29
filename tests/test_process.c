/*
 * QH-SJF Scheduling Lab: Standalone Process Test
 */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../include/sim.h"

int main(void) {
    Proc p = {1, 2.0, 5.0, 5.0};
    assert(p.id == 1);
    assert(fabs(p.at - 2.0) < 1e-9);
    assert(fabs(p.bt - 5.0) < 1e-9);
    assert(fabs(p.pb - 5.0) < 1e-9);
    printf("test_process: PASS\n");
    return 0;
}
