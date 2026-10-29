/*
 * QH-SJF Scheduling Lab
 * Cross-Platform HTTP/1.1 Server & REST API in C (C11)
 *
 * Supports native Windows (Winsock2) and POSIX (Linux/macOS/WSL).
 * Serves static assets from ./frontend and JSON simulation/test APIs.
 */
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
  #define WIN32_LEAN_AND_MEAN
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #ifdef _MSC_VER
    #pragma comment(lib, "ws2_32.lib")
  #endif
  typedef SOCKET sock_t;
  #define CLOSE_SOCK(s) closesocket(s)
  #define SOCK_VALID(s) ((s) != INVALID_SOCKET)
#else
  #define _POSIX_C_SOURCE 200809L
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <signal.h>
  #include <sys/socket.h>
  #include <sys/time.h>
  #include <unistd.h>
  typedef int sock_t;
  #define CLOSE_SOCK(s) close(s)
  #define SOCK_VALID(s) ((s) >= 0)
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sim.h"
#include "workload.h"
#include "tests.h"

/* Dynamic growth buffer for constructing HTTP responses */
typedef struct {
    char *p;
    size_t n;
    size_t cap;
} Buf;

static void bp(Buf *b, const char *fmt, ...) {
    for (;;) {
        va_list ap;
        va_start(ap, fmt);
        int k = vsnprintf(b->p + b->n, b->cap > b->n ? b->cap - b->n : 0, fmt, ap);
        va_end(ap);

        if (k >= 0 && b->n + (size_t)k < b->cap) {
            b->n += (size_t)k;
            return;
        }

        b->cap = b->cap ? b->cap * 2 : 8192;
        if (b->cap < b->n + (size_t)k + 1) b->cap = b->n + (size_t)k + 4096;
        char *q = (char *)realloc(b->p, b->cap);
        if (!q) {
            fprintf(stderr, "Out of memory in dynamic buffer\n");
            exit(1);
        }
        b->p = q;
    }
}

static void send_all(sock_t fd, const char *d, size_t n) {
    while (n > 0) {
        int k = send(fd, d, (int)n, 0);
        if (k <= 0) return;
        d += k;
        n -= (size_t)k;
    }
}

static void reply(sock_t fd, int code, const char *type, const char *body, size_t len) {
    char h[512];
    int k = snprintf(h, sizeof h,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %lu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Cache-Control: no-store\r\n"
        "X-Content-Type-Options: nosniff\r\n"
        "Connection: close\r\n\r\n",
        code,
        code == 200 ? "OK" :
        code == 400 ? "Bad Request" :
        code == 404 ? "Not Found" :
        code == 405 ? "Method Not Allowed" : "Internal Error",
        type, (unsigned long)len);

    send_all(fd, h, (size_t)k);
    if (len > 0 && body) send_all(fd, body, len);
}

static void fail_json(sock_t fd, int code, const char *msg) {
    char b[256];
    int k = snprintf(b, sizeof b, "{\"error\":\"%s\"}", msg);
    reply(fd, code, "application/json; charset=utf-8", b, (size_t)k);
}

static double qnum(const char *qs, const char *key, double def) {
    size_t kl = strlen(key);
    for (const char *s = qs; s && *s; ) {
        if (strncmp(s, key, kl) == 0 && s[kl] == '=') return atof(s + kl + 1);
        s = strchr(s, '&');
        if (s) s++;
    }
    return def;
}

static int qstr(const char *qs, const char *key, char *out, size_t cap) {
    size_t kl = strlen(key);
    for (const char *s = qs; s && *s; ) {
        if (strncmp(s, key, kl) == 0 && s[kl] == '=') {
            size_t i = 0;
            for (s += kl + 1; *s && *s != '&' && i + 1 < cap; s++) out[i++] = *s;
            out[i] = '\0';
            return 1;
        }
        s = strchr(s, '&');
        if (s) s++;
    }
    return 0;
}

static void json_append_result(Buf *b, int pol, const Result *r) {
    const Metrics *m = &r->m;
    bp(b, "{\"policy\":\"%s\",\"name\":\"%s\",\"short\":\"%s\","
          "\"m\":{\"wt\":%.4f,\"tat\":%.4f,\"rt\":%.4f,\"util\":%.4f,\"thr\":%.6f,\"mk\":%.4f,\"sl\":%.4f,\"max\":%.4f,\"p95\":%.4f,\"p99\":%.4f},"
          "\"res\":[",
       POLICY_KEY[pol], policy_name(pol), policy_short(pol),
       m->wt, m->tat, m->rt, m->util, m->thr, m->mk, m->sl, m->max, m->p95, m->p99);

    for (int i = 0; i < r->nrows; i++) {
        const Row *w = &r->rows[i];
        bp(b, "%s{\"id\":%d,\"at\":%.2f,\"bt\":%.2f,\"pb\":%.2f,\"st\":%.2f,\"ct\":%.2f,\"tat\":%.2f,\"wt\":%.2f,\"rt\":%.2f}",
           i ? "," : "", w->id, w->at, w->bt, w->pb, w->st, w->ct, w->tat, w->wt, w->rt);
    }

    bp(b, "],\"seg\":[");
    for (int i = 0; i < r->nseg; i++) {
        const Seg *g = &r->seg[i];
        if (g->idle) {
            bp(b, "%s{\"idle\":1,\"s\":%.4f,\"e\":%.4f}", i ? "," : "", g->s, g->e);
        } else {
            bp(b, "%s{\"id\":%d,\"s\":%.4f,\"e\":%.4f}", i ? "," : "", g->id, g->s, g->e);
        }
    }
    bp(b, "]}");
}

static void api_run_simulation(sock_t fd, const char *qs, int all, const char *custom_csv) {
    int n = (int)qnum(qs, "n", 50);
    int family = (int)qnum(qs, "family", 0);
    double err = qnum(qs, "err", 0.0);
    double bm  = qnum(qs, "bm", 4.0);
    unsigned seed = (unsigned)qnum(qs, "seed", 0);

    if (n < 1 || n > MAXN || err < 0.0 || err > 1.5 || bm < 1.0 || bm > 64.0) {
        fail_json(fd, 400, "Invalid parameters (n: 1-500, err: 0-1.5, bm: 1-64)");
        return;
    }

    int target_pol = -1;
    if (!all) {
        char pol_key[32] = "";
        if (!qstr(qs, "policy", pol_key, sizeof(pol_key)) || (target_pol = policy_from_key(pol_key)) < 0) {
            fail_json(fd, 400, "Unknown or missing policy parameter");
            return;
        }
    }

    Proc P[MAXN];
    int actual_n = n;

    if (custom_csv && strlen(custom_csv) > 0) {
        actual_n = parse_csv_workload(custom_csv, P, MAXN);
        if (actual_n <= 0) {
            fail_json(fd, 400, "Failed to parse custom CSV workload");
            return;
        }
        family = -1;
    } else {
        if (!seed) seed = 10000u + (unsigned)(rand() % 90000);
        actual_n = gen_workload_family(P, n, family, seed, err);
    }

    SimConfig cfg;
    default_config(&cfg);
    cfg.bm = bm;
    cfg.b0 = median_pb(P, actual_n) * bm;
    cfg.err = err;
    cfg.seed = seed;

    Result *r = (Result *)calloc(1, sizeof(Result));
    if (!r) {
        fail_json(fd, 500, "Out of memory");
        return;
    }

    Buf b = {0};
    bp(&b, "{\"seed\":%u,\"n\":%d,\"family\":%d,\"familyName\":\"%s\",\"results\":[",
       seed, actual_n, family, workload_family_name(family));

    int first = 1;
    for (int p = 0; p < NPOL; p++) {
        if (target_pol >= 0 && p != target_pol) continue;
        memset(r, 0, sizeof(*r));
        simulate_with_config(P, actual_n, (Policy)p, &cfg, r);
        bp(&b, "%s", first ? "" : ",");
        first = 0;
        json_append_result(&b, p, r);
    }
    bp(&b, "]}");

    reply(fd, 200, "application/json; charset=utf-8", b.p, b.n);
    free(b.p);
    free(r);
}

static void api_run_tests(sock_t fd) {
    TestSuiteResult suite;
    run_all_tests(&suite);

    Buf b = {0};
    bp(&b, "{\"total\":%d,\"passed\":%d,\"failed\":%d,\"total_duration_ms\":%.2f,\"tests\":[",
       suite.count, suite.passed, suite.failed, (double)suite.total_duration_us / 1000.0);

    for (int i = 0; i < suite.count; i++) {
        const TestCaseResult *t = &suite.tests[i];
        bp(&b, "%s{\"name\":\"%s\",\"description\":\"%s\",\"passed\":%s,\"details\":\"%s\",\"duration_ms\":%.2f}",
           i ? "," : "",
           t->name,
           t->description,
           t->passed ? "true" : "false",
           t->details,
           (double)t->duration_us / 1000.0);
    }
    bp(&b, "]}");

    reply(fd, 200, "application/json; charset=utf-8", b.p, b.n);
    free(b.p);
}

static const char *mime_type(const char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot) return "application/octet-stream";
    if (!strcmp(dot, ".html")) return "text/html; charset=utf-8";
    if (!strcmp(dot, ".css"))  return "text/css; charset=utf-8";
    if (!strcmp(dot, ".js"))   return "text/javascript; charset=utf-8";
    if (!strcmp(dot, ".json")) return "application/json; charset=utf-8";
    if (!strcmp(dot, ".svg"))  return "image/svg+xml";
    if (!strcmp(dot, ".ico"))  return "image/x-icon";
    if (!strcmp(dot, ".csv"))  return "text/csv; charset=utf-8";
    return "application/octet-stream";
}

static void serve_file(sock_t fd, const char *root, const char *path) {
    char filepath[1024];
    if (strstr(path, "..") || strstr(path, "//") || strlen(path) > 300) {
        fail_json(fd, 400, "Bad path");
        return;
    }

    const char *subpath = strcmp(path, "/") ? path : "/index.html";
    snprintf(filepath, sizeof(filepath), "%s%s", root, subpath);

    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        fail_json(fd, 404, "File not found");
        return;
    }

    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    rewind(fp);

    if (len < 0 || len > 10 * 1024 * 1024) {
        fclose(fp);
        fail_json(fd, 500, "File too large");
        return;
    }

    char *buf = (char *)malloc((size_t)len + 1);
    if (!buf) {
        fclose(fp);
        fail_json(fd, 500, "Memory allocation failed");
        return;
    }

    size_t rd = fread(buf, 1, (size_t)len, fp);
    fclose(fp);

    reply(fd, 200, mime_type(filepath), buf, rd);
    free(buf);
}

static void handle_request(sock_t fd, const char *root) {
    char req[65536];
    size_t got = 0;

    /* Read header */
    while (got < sizeof(req) - 1) {
        int k = recv(fd, req + got, (int)(sizeof(req) - 1 - got), 0);
        if (k <= 0) break;
        got += (size_t)k;
        req[got] = '\0';
        if (strstr(req, "\r\n\r\n")) break;
    }

    char *header_end = strstr(req, "\r\n\r\n");
    if (!header_end) {
        fail_json(fd, 400, "Incomplete request");
        return;
    }

    char method[16] = "", target[1024] = "";
    if (sscanf(req, "%15s %1023s", method, target) != 2) {
        fail_json(fd, 400, "Bad HTTP request format");
        return;
    }

    /* OPTIONS for CORS */
    if (!strcmp(method, "OPTIONS")) {
        reply(fd, 200, "text/plain", "", 0);
        return;
    }

    /* Extract body if POST */
    char *body = header_end + 4;
    size_t body_len = got - (size_t)(body - req);
    const char *cl = strstr(req, "Content-Length:");
    if (!cl) cl = strstr(req, "content-length:");
    if (cl) {
        size_t expected = (size_t)atoi(cl + 15);
        while (body_len < expected && got < sizeof(req) - 1) {
            int k = recv(fd, req + got, (int)(sizeof(req) - 1 - got), 0);
            if (k <= 0) break;
            got += (size_t)k;
            req[got] = '\0';
            body_len = got - (size_t)(body - req);
        }
    }

    char *qs = strchr(target, '?');
    if (qs) {
        *qs++ = '\0';
    } else {
        qs = target + strlen(target);
    }

    if (!strcmp(target, "/api/health")) {
        const char *resp = "{\"status\":\"ok\",\"engine\":\"c11\",\"version\":\"1.0.0\"}";
        reply(fd, 200, "application/json; charset=utf-8", resp, strlen(resp));
    } else if (!strcmp(target, "/api/simulate")) {
        api_run_simulation(fd, qs, 0, NULL);
    } else if (!strcmp(target, "/api/compare")) {
        api_run_simulation(fd, qs, 1, NULL);
    } else if (!strcmp(target, "/api/custom-simulate")) {
        api_run_simulation(fd, qs, 0, body);
    } else if (!strcmp(target, "/api/custom-compare")) {
        api_run_simulation(fd, qs, 1, body);
    } else if (!strcmp(target, "/api/tests")) {
        api_run_tests(fd);
    } else if (!strcmp(target, "/api/paper-benchmark")) {
        api_run_simulation(fd, qs, 1, NULL);
    } else if (!strncmp(target, "/api/", 5)) {
        fail_json(fd, 404, "Endpoint not found");
    } else {
        serve_file(fd, root, target);
    }
}

int main(int argc, char **argv) {
    int port = (argc > 1) ? atoi(argv[1]) : 5173;
    const char *root = (argc > 2) ? argv[2] : "frontend";

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }
#else
    signal(SIGPIPE, SIG_IGN);
#endif
    srand((unsigned)time(NULL));

    sock_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (!SOCK_VALID(s)) {
        perror("socket creation failed");
        return 1;
    }

    int one = 1;
#ifdef _WIN32
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one));
#else
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
#endif

    struct sockaddr_in a;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_port = htons((unsigned short)port);
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(s, (struct sockaddr *)&a, sizeof(a)) != 0 || listen(s, 32) != 0) {
        perror("bind / listen error");
        CLOSE_SOCK(s);
        return 1;
    }

    printf("=================================================================\n");
    printf("  QH-SJF SCHEDULING LAB: C11 HTTP BACKEND                         \n");
    printf("  Server running at: http://127.0.0.1:%d                         \n", port);
    printf("  Serving frontend:  %s                                          \n", root);
    printf("=================================================================\n");

    for (;;) {
        sock_t client = accept(s, NULL, NULL);
        if (!SOCK_VALID(client)) continue;

#ifdef _WIN32
        DWORD tv = 6000;
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
#else
        struct timeval tv = {6, 0};
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif

        handle_request(client, root);
        CLOSE_SOCK(client);
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
