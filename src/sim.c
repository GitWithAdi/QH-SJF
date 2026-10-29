/*
 * QH-SJF (Queue- and Heterogeneity-Aware Shortest Job First) Scheduling Lab
 * Core Scheduling Engine & Policy Implementation (C11)
 */
#include "sim.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

const char *POLICY_KEY[NPOL] = {
    "fcfs", "sjf", "hrrn", "lin", "quad", "lonly", "honly", "qh"
};

const char *POLICY_NAME[NPOL] = {
    "First Come First Served (FCFS)",
    "Shortest Job First (SJF)",
    "Highest Response Ratio Next (HRRN)",
    "Linear Aging SJF",
    "Fixed Quadratic Aging SJF",
    "Queue-pressure-only variant (L-only)",
    "Heterogeneity-only variant (H-only)",
    "Queue- and Heterogeneity-Aware SJF (QH-SJF)"
};

const char *POLICY_SHORT[NPOL] = {
    "FCFS", "SJF", "HRRN", "Linear", "FixedQuad", "L-only", "H-only", "QH-SJF"
};

int policy_from_key(const char *k) {
    if (!k) return -1;
    for (int i = 0; i < NPOL; i++) {
        if (strcmp(k, POLICY_KEY[i]) == 0) return i;
    }
    return -1;
}

const char* policy_name(int policy) {
    if (policy >= 0 && policy < NPOL) return POLICY_NAME[policy];
    return "Unknown Policy";
}

const char* policy_short(int policy) {
    if (policy >= 0 && policy < NPOL) return POLICY_SHORT[policy];
    return "Unknown";
}

void default_config(SimConfig *cfg) {
    if (!cfg) return;
    cfg->b0 = 0.0;          /* 0 => auto-compute from median(pb) * bm */
    cfg->bm = 4.0;          /* B0 multiplier */
    cfg->k = 5.0;           /* Saturation parameter K */
    cfg->amin = 0.10;       /* Minimum aging strength */
    cfg->amax = 1.00;       /* Maximum aging strength */
    cfg->beta_lin = 2.0;    /* Linear aging slope beta */
    cfg->alpha_quad = 0.50; /* Fixed quadratic aging alpha */
    cfg->err = 0.0;         /* Default exact prediction */
    cfg->seed = 0;          /* Auto seed */
}

static int cmp_double(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

double median_pb(const Proc *p, int n) {
    if (n <= 0) return 1.0;
    double v[MAXN];
    for (int i = 0; i < n; i++) v[i] = p[i].pb;
    qsort(v, (size_t)n, sizeof v[0], cmp_double);
    return v[n / 2];
}

static inline double round2(double x) {
    return floor(x * 100.0 + 0.5) / 100.0;
}

static void add_segment(Result *o, int idle, int id, double s, double e) {
    if (o->nseg < (2 * MAXN + 2)) {
        Seg *g = &o->seg[o->nseg++];
        g->idle = idle;
        g->id = id;
        g->s = s;
        g->e = e;
    }
}

void simulate(const Proc *P, int n, Policy pol, double b0, Result *out) {
    SimConfig cfg;
    default_config(&cfg);
    cfg.b0 = b0;
    simulate_with_config(P, n, pol, &cfg, out);
}

void simulate_with_config(const Proc *P, int n, Policy pol, const SimConfig *cfg, Result *out) {
    if (!P || n <= 0 || !out) return;
    if (n > MAXN) n = MAXN;

    memset(out, 0, sizeof(*out));
    out->policy = pol;
    out->nrows = n;

    double b0 = (cfg && cfg->b0 > 0.0) ? cfg->b0 : (median_pb(P, n) * (cfg ? cfg->bm : 4.0));
    if (b0 <= 0.001) b0 = 1.0;

    const double k_sat = (cfg && cfg->k > 0) ? cfg->k : 5.0;
    const double amin  = (cfg && cfg->amin >= 0) ? cfg->amin : 0.10;
    const double amax  = (cfg && cfg->amax >= amin) ? cfg->amax : 1.00;
    const double beta_lin = (cfg && cfg->beta_lin > 0) ? cfg->beta_lin : 2.0;
    const double alpha_quad = (cfg && cfg->alpha_quad > 0) ? cfg->alpha_quad : 0.50;

    char done[MAXN] = {0};
    int cnt = 0;

    /* Find first arrival time */
    double t = 1e300;
    for (int i = 0; i < n; i++) {
        if (P[i].at < t) t = P[i].at;
    }
    const double first_arrival = t;

    out->nseg = 0;
    if (t > 0.0) {
        add_segment(out, 1, 0, 0.0, t);
    }

    while (cnt < n) {
        int ready[MAXN];
        int nr = 0;
        double q = 0.0;

        /* Identify ready processes */
        for (int i = 0; i < n; i++) {
            if (!done[i] && P[i].at <= t) {
                ready[nr++] = i;
                q += P[i].pb;
            }
        }

        /* CPU Idle gap: advance to next arrival */
        if (nr == 0) {
            double next_arr = 1e300;
            for (int i = 0; i < n; i++) {
                if (!done[i] && P[i].at < next_arr) {
                    next_arr = P[i].at;
                }
            }
            if (next_arr >= 1e300) break;
            add_segment(out, 1, 0, t, next_arr);
            t = next_arr;
            continue;
        }

        /* Compute queue pressure L and heterogeneity H */
        double mu = q / nr;
        double var_sum = 0.0;
        for (int k = 0; k < nr; k++) {
            double diff = P[ready[k]].pb - mu;
            var_sum += diff * diff;
        }
        double sd = sqrt(var_sum / nr);

        double L = q / (q + k_sat * b0);
        double H = (mu + sd > 0.0) ? (sd / (mu + sd)) : 0.0;

        double alpha = 0.50;
        if (pol == POL_QH) {
            alpha = amin + (amax - amin) * L * H;
        } else if (pol == POL_LONLY) {
            alpha = amin + (amax - amin) * L;
        } else if (pol == POL_HONLY) {
            alpha = amin + (amax - amin) * H;
        } else if (pol == POL_QUAD) {
            alpha = alpha_quad;
        }

        /* Choose process with optimal score */
        int best = -1;
        double best_score = 0.0;

        for (int k = 0; k < nr; k++) {
            const Proc *p = &P[ready[k]];
            double wait = t - p->at;
            if (wait < 0.0) wait = 0.0;
            double score = 0.0;

            switch (pol) {
                case POL_FCFS:
                    score = p->at;
                    break;
                case POL_SJF:
                    score = p->pb;
                    break;
                case POL_HRRN:
                    /* Max (W + B)/B <=> Min -(W + B)/B */
                    score = -(wait + p->pb) / p->pb;
                    break;
                case POL_LIN:
                    score = p->pb / (1.0 + beta_lin * (wait / b0));
                    break;
                case POL_QUAD:
                case POL_LONLY:
                case POL_HONLY:
                case POL_QH:
                default: {
                    double norm_w = wait / b0;
                    score = p->pb / (1.0 + alpha * norm_w * norm_w);
                    break;
                }
            }

            /* Tie breaking: lowest score, then earliest arrival, then smallest PID */
            if (best < 0 || score < best_score ||
               (score == best_score && (p->at < P[best].at || (p->at == P[best].at && p->id < P[best].id)))) {
                best = ready[k];
                best_score = score;
            }
        }

        const Proc *p = &P[best];
        double ct = t + p->bt;

        Row *r = &out->rows[cnt];
        r->id = p->id;
        r->at = p->at;
        r->bt = p->bt;
        r->pb = p->pb;
        r->st = round2(t);
        r->ct = round2(ct);
        r->tat = round2(ct - p->at);
        r->wt = round2(t - p->at);
        r->rt = r->wt;
        r->sc = round2(best_score);

        add_segment(out, 0, p->id, t, ct);
        done[best] = 1;
        cnt++;
        t = ct;
    }

    /* Aggregate metrics */
    double w_sorted[MAXN];
    double sw = 0.0, st = 0.0, sr = 0.0, sb = 0.0, sl = 0.0, mk = 0.0;

    for (int i = 0; i < n; i++) {
        const Row *r = &out->rows[i];
        w_sorted[i] = r->wt;
        sw += r->wt;
        st += r->tat;
        sr += r->rt;
        sb += r->bt;
        sl += (r->bt > 0.0) ? (r->tat / r->bt) : 1.0;
        if (r->ct > mk) mk = r->ct;
    }

    qsort(w_sorted, (size_t)n, sizeof w_sorted[0], cmp_double);
    double span = mk - first_arrival;

    Metrics *m = &out->m;
    m->wt   = sw / n;
    m->tat  = st / n;
    m->rt   = sr / n;
    m->sl   = sl / n;
    m->mk   = mk;
    m->util = (span > 0.0) ? (100.0 * sb / span) : 100.0;
    if (m->util > 100.0) m->util = 100.0;
    m->thr  = (span > 0.0) ? (n / span) : 0.0;
    m->max  = w_sorted[n - 1];

    int idx95 = (int)ceil(0.95 * n) - 1;
    if (idx95 < 0) idx95 = 0;
    if (idx95 >= n) idx95 = n - 1;
    m->p95 = w_sorted[idx95];

    int idx99 = (int)ceil(0.99 * n) - 1;
    if (idx99 < 0) idx99 = 0;
    if (idx99 >= n) idx99 = n - 1;
    m->p99 = w_sorted[idx99];
}
