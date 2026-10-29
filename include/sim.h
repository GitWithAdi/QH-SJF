/*
 * QH-SJF (Queue- and Heterogeneity-Aware Shortest Job First) Scheduling Lab
 * Core Scheduling Engine & Policy Definitions (C11)
 */
#ifndef SIM_H
#define SIM_H

#include <stddef.h>

#define MAXN 500

/* 8 Evaluated Policies */
typedef enum {
    POL_FCFS = 0,
    POL_SJF,
    POL_HRRN,
    POL_LIN,
    POL_QUAD,
    POL_LONLY,
    POL_HONLY,
    POL_QH,
    NPOL
} Policy;

/* Per-process input */
typedef struct {
    int id;
    double at;   /* Arrival time */
    double bt;   /* Actual burst time */
    double pb;   /* Predicted burst time */
} Proc;

/* Per-process execution result */
typedef struct {
    int id;
    double at;   /* Arrival time */
    double bt;   /* Actual burst time */
    double pb;   /* Predicted burst time */
    double st;   /* Start time */
    double ct;   /* Completion time */
    double tat;  /* Turnaround time (ct - at) */
    double wt;   /* Waiting time (st - at) */
    double rt;   /* Response time */
    double sc;   /* Policy priority score */
} Row;

/* Timeline Gantt segment */
typedef struct {
    int idle;    /* 1 if CPU is idle, 0 if executing process */
    int id;      /* Process ID if !idle */
    double s;    /* Start time */
    double e;    /* End time */
} Seg;

/* Aggregate metrics */
typedef struct {
    double wt;   /* Average waiting time */
    double tat;  /* Average turnaround time */
    double rt;   /* Average response time */
    double util; /* CPU utilization percentage (0 - 100%) */
    double thr;  /* Throughput (processes / time unit) */
    double mk;   /* Makespan (finish timestamp) */
    double sl;   /* Average slowdown (tat / bt) */
    double max;  /* Maximum waiting time */
    double p95;  /* 95th percentile waiting time */
    double p99;  /* 99th percentile waiting time */
} Metrics;

/* Complete simulation outcome for one policy */
typedef struct {
    Row rows[MAXN];
    Seg seg[2 * MAXN + 2];
    int nseg;
    int nrows;
    Metrics m;
    Policy policy;
} Result;

/* Simulator hyperparameters */
typedef struct {
    double b0;         /* Reference burst B0 (<=0 for automatic median(pb)*bm) */
    double bm;         /* B0 multiplier on median(pb) */
    double k;          /* Saturation parameter K (default 5.0) */
    double amin;       /* Minimum aging strength (default 0.10) */
    double amax;       /* Maximum aging strength (default 1.00) */
    double beta_lin;   /* Linear aging slope beta (default 2.0) */
    double alpha_quad; /* Fixed quadratic aging alpha (default 0.50) */
    double err;        /* Log-normal error sigma (default 0.0) */
    unsigned seed;     /* Workload random seed */
} SimConfig;

extern const char *POLICY_KEY[NPOL];
extern const char *POLICY_NAME[NPOL];
extern const char *POLICY_SHORT[NPOL];

int         policy_from_key(const char *key);
const char* policy_name(int policy);
const char* policy_short(int policy);
void        default_config(SimConfig *cfg);
double      median_pb(const Proc *p, int n);
void        simulate(const Proc *p, int n, Policy pol, double b0, Result *out);
void        simulate_with_config(const Proc *p, int n, Policy pol, const SimConfig *cfg, Result *out);

#endif /* SIM_H */
