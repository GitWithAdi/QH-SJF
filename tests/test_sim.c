/*
 * QH-SJF Scheduling Lab: Standalone Schedule Invariants & Reproducibility Test
 */
#include <stdio.h>
#include <math.h>
#include "../include/sim.h"
#include "../include/workload.h"

static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { printf("FAIL: %s\n", msg); fails++; } } while (0)

int main(void) {
    static Result a, b;
    Proc p[MAXN];
    Proc h[4] = {{1, 0, 6, 6}, {2, 1, 2, 2}, {3, 2, 4, 4}, {4, 3, 1, 1}};

    simulate(h, 4, POL_SJF, 2.0, &a);
    CHECK(a.rows[0].id == 1 && a.rows[1].id == 4 && a.rows[2].id == 2 && a.rows[3].id == 3, "SJF order 1,4,2,3");

    simulate(h, 4, POL_FCFS, 2.0, &a);
    CHECK(a.rows[1].id == 2 && a.rows[3].id == 4, "FCFS order");

    for (int n = 1; n <= MAXN; n += 99) {
        for (int pol = 0; pol < NPOL; pol++) {
            gen_workload_family(p, n, WORKLOAD_DEMO, 42u + (unsigned)n, 0.3);
            double b0 = median_pb(p, n) * 4.0;
            simulate(p, n, (Policy)pol, b0, &a);
            simulate(p, n, (Policy)pol, b0, &b);

            double end = -1.0;
            int ok = 1, seen[MAXN + 1] = {0};

            for (int i = 0; i < n; i++) {
                const Row *r = &a.rows[i];
                if (r->st + 0.006 < r->at || r->st + 0.006 < end ||
                    fabs((r->ct - r->st) - r->bt) > 0.03 || seen[r->id]++) {
                    ok = 0;
                }
                if (r->ct > end) end = r->ct;
                if (r->id != b.rows[i].id) ok = 0;
            }
            CHECK(ok, "schedule invariants (once, no early start, no overlap, reproducible)");
            CHECK(a.m.p99 <= a.m.max + 1e-6 && a.m.util <= 100.0001, "metric bounds");
        }
    }

    if (fails == 0) {
        printf("test_sim: all invariants passed\n");
        return 0;
    } else {
        printf("test_sim: %d test(s) failed\n", fails);
        return 1;
    }
}
