/*
 * QH-SJF Scheduling Lab
 * Workload Generation & Benchmark Datasets (C11)
 *
 * Implements the 7 paper workload families, the demo mixed workload,
 * and custom CSV parsing/loading.
 */
#ifndef WORKLOAD_H
#define WORKLOAD_H

#include "sim.h"

typedef enum {
    WORKLOAD_DEMO = 0,         /* Interactive Demo: Short/Med/Long mixture */
    WORKLOAD_UNIFORM = 1,      /* Paper Family 1: Uniform / Constant Bursts */
    WORKLOAD_MULTIMODAL = 2,   /* Paper Family 2: Bimodal / Multimodal Bursts */
    WORKLOAD_CONVOY = 3,       /* Paper Family 3: Convoy Effect (Head-of-line blocking) */
    WORKLOAD_BURSTY = 4,       /* Paper Family 4: Bursty / Simultaneous Arrivals */
    WORKLOAD_CLUSTERED = 5,    /* Paper Family 5: Clustered / Batch Arrivals */
    WORKLOAD_LIGHT_LOAD = 6,   /* Paper Family 6: Light Load (Sparse Arrivals) */
    WORKLOAD_HEAVY_LOAD = 7,   /* Paper Family 7: High Variance / Heavy Load */
    N_WORKLOAD_FAMILIES = 8
} WorkloadFamily;

const char* workload_family_name(int family);
const char* workload_family_desc(int family);

/* Generates workload into array p (size at least n). Returns actual process count. */
int gen_workload_family(Proc *p, int n, int family, unsigned seed, double err);

/* Parses CSV string content into array p. Returns process count, or negative on error. */
int parse_csv_workload(const char *csv_text, Proc *p, int max_n);

/* Reads a CSV file from disk into array p. Returns process count, or negative on error. */
int load_csv_workload_file(const char *filepath, Proc *p, int max_n);

#endif /* WORKLOAD_H */
