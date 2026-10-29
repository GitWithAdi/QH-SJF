/*
 * QH-SJF Scheduling Lab
 * Workload Generators & Dataset Importer (C11)
 */
#include "workload.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *FAMILY_NAMES[N_WORKLOAD_FAMILIES] = {
    "Interactive Demo (Mixed Distribution)",
    "Paper Family 1: Uniform / Constant Bursts",
    "Paper Family 2: Bimodal / Multimodal Bursts",
    "Paper Family 3: Convoy Effect (Head-of-line blocking)",
    "Paper Family 4: Bursty / Simultaneous Arrivals",
    "Paper Family 5: Clustered / Batch Arrivals",
    "Paper Family 6: Light Load (Sparse Arrivals)",
    "Paper Family 7: High Variance / Heavy Load"
};

static const char *FAMILY_DESCRIPTIONS[N_WORKLOAD_FAMILIES] = {
    "Interactive synthetic mix of short (60%), medium (30%), and long (10%) bursts with log-normal prediction error.",
    "Constant burst lengths (B=8) arriving evenly; isolates queueing fairness with zero burst variance.",
    "Three distinct process classes (short, medium, long); tests differentiation across mixed execution times.",
    "Long convoy process (B=40) at head of queue followed by short bursts; tests starvation recovery under aging.",
    "Processes arrive nearly simultaneously within an initial surge; evaluates queue pressure saturation.",
    "Periodic bursts arriving in clusters of four; simulates recurring batch submission cycles.",
    "Arrivals spaced widely apart; low queue pressure where queues frequently empty.",
    "Extreme burst length variance (up to 40x); stresses heterogeneity metric H and aging resistance."
};

const char* workload_family_name(int family) {
    if (family >= 0 && family < N_WORKLOAD_FAMILIES) return FAMILY_NAMES[family];
    return "Custom Workload";
}

const char* workload_family_desc(int family) {
    if (family >= 0 && family < N_WORKLOAD_FAMILIES) return FAMILY_DESCRIPTIONS[family];
    return "Custom user-supplied process trace.";
}

/* Mulberry32 PRNG for deterministic, platform-independent random sequences */
static unsigned rng_state;
static inline double rnd(void) {
    rng_state += 0x6D2B79F5u;
    unsigned t = rng_state;
    t = (t ^ (t >> 15)) * (t | 1u);
    t ^= t + (t ^ (t >> 7)) * (t | 61u);
    return (double)(t ^ (t >> 14)) / 4294967296.0;
}

/* Box-Muller standard normal variate */
static inline double nrm(void) {
    double u1 = rnd();
    if (u1 <= 1e-15) u1 = 1e-15;
    return sqrt(-2.0 * log(u1)) * cos(6.283185307179586 * rnd());
}

static inline double r2(double x) {
    return floor(x * 100.0 + 0.5) / 100.0;
}

static int cmp_proc_arrival(const void *a, const void *b) {
    const Proc *x = (const Proc *)a;
    const Proc *y = (const Proc *)b;
    if (x->at > y->at) return 1;
    if (x->at < y->at) return -1;
    return x->id - y->id;
}

int gen_workload_family(Proc *p, int n, int family, unsigned seed, double err) {
    if (!p || n <= 0) return 0;
    if (n > MAXN) n = MAXN;

    rng_state = seed ? seed : 424242u;

    for (int i = 0; i < n; i++) {
        double r = rnd();
        double at = 0.0;
        double bt = 0.0;

        switch (family) {
            case WORKLOAD_UNIFORM: /* Family 1 */
                bt = 8.0;
                at = i * 2.0 + r;
                break;

            case WORKLOAD_MULTIMODAL: /* Family 2 */
                if (r < 0.45) {
                    bt = 1.0 + (int)(rnd() * 4);
                } else if (r < 0.85) {
                    bt = 6.0 + (int)(rnd() * 8);
                } else {
                    bt = 18.0 + (int)(rnd() * 18);
                }
                at = i * 2.0 + rnd();
                break;

            case WORKLOAD_CONVOY: /* Family 3 */
                bt = (i == 0) ? 40.0 : (1.0 + (int)(rnd() * 4));
                at = i * 2.0 + r;
                break;

            case WORKLOAD_BURSTY: /* Family 4 */
                bt = 1.0 + (int)(rnd() * 25);
                at = r * 8.0;
                break;

            case WORKLOAD_CLUSTERED: /* Family 5 */
                bt = 1.0 + (int)(rnd() * 15);
                at = (i / 4) * 20.0 + r * 2.0;
                break;

            case WORKLOAD_LIGHT_LOAD: /* Family 6 */
                bt = 1.0 + (int)(rnd() * 8);
                at = i * 8.0 + r * 2.0;
                break;

            case WORKLOAD_HEAVY_LOAD: /* Family 7 */
                bt = 1.0 + (int)(rnd() * 40);
                at = i * 2.0 + r;
                break;

            case WORKLOAD_DEMO: /* Family 0 (c-app demo) */
            default: {
                double u = rnd();
                bt = (u < 0.6) ? (1.0 + rnd() * 7.0)
                   : (u < 0.9) ? (8.0 + rnd() * 7.0)
                   : (15.0 + rnd() * 10.0);
                at = rnd() * 30.0;
                break;
            }
        }

        if (bt < 0.1) bt = 0.1;
        p[i].id = i + 1;
        p[i].at = r2(at);
        p[i].bt = r2(bt);

        /* Apply prediction error if err > 0 */
        if (err > 0.0) {
            double factor = exp(err * nrm());
            p[i].pb = r2(p[i].bt * factor);
            if (p[i].pb < 0.01) p[i].pb = 0.01;
        } else {
            p[i].pb = p[i].bt;
        }
    }

    qsort(p, (size_t)n, sizeof p[0], cmp_proc_arrival);
    return n;
}

int parse_csv_workload(const char *csv_text, Proc *p, int max_n) {
    if (!csv_text || !p || max_n <= 0) return -1;

    int count = 0;
    const char *line = csv_text;

    while (*line && count < max_n) {
        /* Skip leading whitespace */
        while (*line == ' ' || *line == '\t' || *line == '\r' || *line == '\n') line++;
        if (!*line) break;

        /* Extract line */
        char buf[256];
        size_t len = 0;
        while (*line && *line != '\n' && *line != '\r' && len + 1 < sizeof(buf)) {
            buf[len++] = *line++;
        }
        buf[len] = '\0';
        while (*line == '\r' || *line == '\n') line++;

        /* Check for header */
        if (strstr(buf, "pid") || strstr(buf, "arrival") || strstr(buf, "burst") ||
            strstr(buf, "PID") || strstr(buf, "AR") || strstr(buf, "BU")) {
            continue;
        }

        int id = 0;
        double at = 0.0, bt = 0.0, pb = 0.0;
        int parsed = sscanf(buf, "%d,%lf,%lf,%lf", &id, &at, &bt, &pb);
        if (parsed < 2) {
            /* Try 2 values: arrival,burst */
            if (sscanf(buf, "%lf,%lf", &at, &bt) == 2) {
                id = count + 1;
                pb = bt;
                parsed = 3;
            }
        }
        if (parsed >= 2) {
            if (parsed == 2) { id = count + 1; pb = bt; }
            else if (parsed == 3) { pb = bt; }
            if (id <= 0) id = count + 1;
            if (at < 0.0) at = 0.0;
            if (bt <= 0.001) bt = 0.001;
            if (pb <= 0.001) pb = bt;

            p[count].id = id;
            p[count].at = r2(at);
            p[count].bt = r2(bt);
            p[count].pb = r2(pb);
            count++;
        }
    }

    if (count > 0) {
        qsort(p, (size_t)count, sizeof p[0], cmp_proc_arrival);
    }
    return count;
}

int load_csv_workload_file(const char *filepath, Proc *p, int max_n) {
    FILE *f = fopen(filepath, "rb");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);

    if (sz <= 0 || sz > 1024 * 1024) {
        fclose(f);
        return -1;
    }

    char *buf = malloc((size_t)sz + 1);
    if (!buf) {
        fclose(f);
        return -1;
    }

    size_t rd = fread(buf, 1, (size_t)sz, f);
    buf[rd] = '\0';
    fclose(f);

    int count = parse_csv_workload(buf, p, max_n);
    free(buf);
    return count;
}
