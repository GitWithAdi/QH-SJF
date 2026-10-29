# QH-SJF: Queue- and Heterogeneity-Aware Shortest Job First Scheduling Lab

[![Language: C11](https://img.shields.io/badge/Language-C11-00599C?logo=c)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Platform: Cross-Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-4EAA25)](https://en.wikipedia.org/wiki/Cross-platform_software)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Status: Tests Passing](https://img.shields.io/badge/Tests-6%2F6%20Passed-brightgreen)](tests/)

An end-to-end CPU scheduling laboratory combining a high-performance **C11 simulation engine**, an ultra-lightweight **C HTTP server**, an **interactive browser frontend**, and the complete **experimental benchmark & verification test suite** used in the QH-SJF research paper.

No Node.js, Python, or external runtime dependencies required—just standard C11.

---

## Table of Contents
- [1. Overview & Theoretical Background](#1-overview--theoretical-background)
- [2. Scheduling Policies](#2-scheduling-policies)
- [3. Workload Families](#3-workload-families)
- [4. Verification Test Suite](#4-verification-test-suite)
- [5. Project Architecture & Layout](#5-project-architecture--layout)
- [6. Quick Start Guide](#6-quick-start-guide)
  - [Windows (Native MinGW / PowerShell)](#windows-native-mingw--powershell)
  - [Linux / macOS / WSL](#linux--macos--wsl)
  - [Using CMake](#using-cmake)
- [7. Web Interface Features](#7-web-interface-features)
- [8. REST API Reference](#8-rest-api-reference)
- [9. Citation & License](#9-citation--license)

---

## 1. Overview & Theoretical Background

Standard **Shortest Job First (SJF)** minimizes average waiting time but suffers from starvation of long jobs under heavy load. Classical aging heuristics (e.g., HRRN or linear aging) apply a constant aging slope that either starves long processes during high traffic or fails to prioritize short tasks when queue congestion is low.

**QH-SJF** solves this dilemma by dynamically modulating quadratic aging based on two instant ready-queue properties:
1. **Workload Pressure ($L$)**: The total queued work relative to system saturation.
2. **Heterogeneity ($H$)**: The relative dispersion (coefficient of variation proxy) of queued burst lengths.

### Mathematical Formulation

Given a ready queue with $n_r$ arrived processes and predicted bursts $\hat{B}_j$:

1. **Total Queued Work ($Q$) & Mean ($\mu$)**:

   $$Q = \sum_{j \in \text{Ready}} \hat{B}_j, \quad \mu = \frac{Q}{n_r}$$

2. **Burst Standard Deviation ($\sigma$)**:

   $$\sigma = \sqrt{\frac{1}{n_r} \sum_{j \in \text{Ready}} (\hat{B}_j - \mu)^2}$$

3. **Workload Pressure ($L \in [0, 1)$)**:

   $$L = \frac{Q}{Q + K \cdot B_0}$$

   where $K = 5.0$ and $B_0$ is the baseline reference burst (median predicted burst $\times$ multiplier $M_{B_0}$, default $M_{B_0} = 4$).

4. **Heterogeneity Factor ($H \in [0, 1)$)**:

   $$H = \begin{cases} \frac{\sigma}{\mu + \sigma} & \text{if } \mu + \sigma > 0 \\ 0 & \text{otherwise} \end{cases}$$

5. **Dynamic Aging Strength ($\alpha \in [\alpha_{\min}, \alpha_{\max}]$)**:

   $$\alpha = \alpha_{\min} + (\alpha_{\max} - \alpha_{\min}) \cdot L \cdot H$$

   where $\alpha_{\min} = 0.10$ and $\alpha_{\max} = 1.00$.

6. **Priority Score ($S_j$)**:

   $$S_j = \frac{\hat{B}_j}{1 + \alpha \left(\frac{W_j}{B_0}\right)^2}$$

   where $W_j = t - A_j$ is the waiting time. The ready process with the **lowest score $S_j$** is dispatched next.

7. **Burst Prediction Noise Model**:

   In practical operating systems, CPU burst durations are predicted from past execution history rather than known in advance. To evaluate robustness against estimation uncertainty, the benchmark applies a multiplicative log-normal error model:

   $$b_i = B_i \exp(s \cdot z_i), \quad z_i \sim \mathcal{N}(0, 1)$$

   where $B_i$ is the actual ground-truth CPU burst, $b_i$ (or $\hat{B}_i$) is the predicted burst passed to the scheduler, $s$ is the error intensity parameter (default $s = 0.5$), and $z_i$ is a standard normal variate generated via the Box-Muller transform:

   $$z_i = \sqrt{-2\ln(u_1)}\cos(2\pi u_2), \quad u_1, u_2 \sim \mathcal{U}(0, 1)$$

   Because $\mathbb{E}[\ln(b_i)] = \ln(B_i)$, the median predicted burst equals the true burst ($\text{median}(b_i) = B_i$), ensuring unbiased central tendency while faithfully replicating real-world estimation variance.

---

## 2. Scheduling Policies

The engine implements and compares all 8 policies from the paper:

| Key | Algorithm | Score / Selection Metric | Characteristics |
|---|---|---|---|
| `fcfs` | First-Come, First-Served | $\min(A_j)$ | Strict arrival order; simple, vulnerable to convoy effect. |
| `sjf` | Shortest Job First | $\min(\hat{B}_j)$ | Minimizes mean wait time; starves long jobs under load. |
| `hrrn` | Highest Response Ratio Next | $\max\left(\frac{W_j + \hat{B}_j}{\hat{B}_j}\right)$ | Linear aging ratio; balances wait and burst duration. |
| `lin` | Linear Aging SJF | $\min\left(\frac{\hat{B}_j}{1 + \beta (W_j / B_0)}\right)$ | Constant linear aging slope ($\beta = 2.0$). |
| `quad` | Fixed Quadratic Aging | $\min\left(\frac{\hat{B}_j}{1 + \alpha (W_j / B_0)^2}\right)$ | Constant quadratic aging parameter ($\alpha = 0.50$). |
| `lonly` | Queue Pressure Variant | $\alpha = \alpha_{\min} + (\alpha_{\max} - \alpha_{\min}) \cdot L$ | Adapts aging solely to queued load volume. |
| `honly` | Heterogeneity Variant | $\alpha = \alpha_{\min} + (\alpha_{\max} - \alpha_{\min}) \cdot H$ | Adapts aging solely to burst dispersion. |
| `qh` | **QH-SJF (Proposed)** | $\alpha = \alpha_{\min} + (\alpha_{\max} - \alpha_{\min}) \cdot L \cdot H$ | Full adaptive queue-pressure and heterogeneity-aware scheduling. |

---

## 3. Workload Families

The system includes synthetic generators for the **7 workload families** studied in the paper, plus an interactive demo generator and a custom CSV loader:

- **Interactive Demo (Family 0)**: Synthetic mix of short (60%, 1–8), medium (30%, 8–15), and long (10%, 15–25) bursts with optional log-normal prediction error.
- **Family 1 (Uniform / Constant Bursts)**: $B = 8$ for all tasks; tests fairness under identical task sizes.
- **Family 2 (Multimodal / Bimodal)**: Three distinct classes (short, medium, long) to evaluate differentiation.
- **Family 3 (Convoy Effect)**: Long initial process ($B = 40$) followed by short tasks ($B \in [1, 4]$); demonstrates head-of-line blocking recovery.
- **Family 4 (Bursty / Simultaneous Arrivals)**: All jobs arrive in an initial surge ($A_i \in [0, 8]$); tests queue saturation.
- **Family 5 (Clustered / Batch Arrivals)**: Recurring periodic clusters arriving in batches of four ($A_i \approx \lfloor i/4 \rfloor \cdot 20$).
- **Family 6 (Light Load / Sparse Arrivals)**: Arrivals spaced widely apart ($A_i = i \cdot 8 + \dots$); queues frequently empty.
- **Family 7 (High Variance / Heavy Load)**: Wide burst distribution ($B \in [1, 40]$); stress tests heterogeneity metric $H$.
- **Custom CSV Trace**: Provide custom workload files or paste rows in `pid,arrival,burst` format.

---

## 4. Verification Test Suite

The laboratory includes both a **CLI test runner** and an **interactive in-browser test suite** verifying the six core verification suites:

1. **Process Initialization & State (`test_process`)**: Checks memory layout, PID assignment, default predicted burst, and state tracking.
2. **Policy Selection Logic (`test_policies`)**: Validates initial dispatch choices for SJF, FCFS, and HRRN on identical ready queues.
3. **Non-preemptive Event Loop (`test_simulator`)**: Verifies non-preemptive ordering, mid-run arrivals, timeline intervals, and CPU idle gap handling.
4. **Metrics & Statistical Calculations (`test_metrics`)**: Validates analytical ground truth for mean WT, mean TAT, slowdown, makespan, CPU utilization, and throughput.
5. **QH-SJF Adaptive Aging (`test_qhsjf`)**: Verifies queue pressure $L \in [0, 1)$, heterogeneity $H \in [0, 1)$, and dynamic aging bound $\alpha \in [\alpha_{\min}, \alpha_{\max}]$.
6. **Paper Invariants & Reproducibility (`test_invariants`)**: Stress-tests over 4,000 schedule invariants across all 8 policies:
   - Each process runs exactly once.
   - No early start ($S_i \ge A_i$).
   - Non-preemption / zero overlap ($S_{i+1} \ge C_i$).
   - Exact run duration ($C_i - S_i = B_i$).
   - $P_{99} \le \text{Max WT}$ and CPU utilization $\le 100\%$.
   - Deterministic reproducibility given the same seed.

---

## 5. Project Architecture & Layout

```
QH-SJF/
├── .gitignore              # Clean gitignore for builds and temporary files
├── LICENSE                 # MIT License
├── README.md               # Documentation and research paper guide
├── Makefile                # POSIX Makefile (Linux, macOS, WSL)
├── build.bat               # Windows build script (MinGW / CMD / PowerShell)
├── CMakeLists.txt          # Cross-platform CMake configuration
├── include/                # C11 Header Files
│   ├── sim.h               # Core structures, policies, simulation prototypes
│   ├── workload.h          # 7 workload families and CSV parser definitions
│   └── tests.h             # Unit test suite definitions and test runner API
├── src/                    # C11 Source Files
│   ├── sim.c               # Simulator event loop, 8 policies, metrics
│   ├── workload.c          # Workload family generators and CSV loaders
│   ├── tests.c             # Implementation of all 6 test suites
│   ├── server.c            # Cross-platform C HTTP/1.1 server (Winsock2 & POSIX)
│   └── main_test.c         # CLI test suite entry point
├── frontend/               # Web Application
│   ├── index.html          # Clean HTML5 user interface
│   ├── style.css           # Modern dark/light theme CSS design system
│   └── app.js              # Interactive Gantt charts, comparisons, and test runner
├── tests/                  # Standalone Modular Test Sources
│   ├── test_process.c      # Standalone process unit test
│   ├── test_policies.c     # Standalone policy decision unit test
│   ├── test_simulator.c    # Standalone simulator event loop test
│   ├── test_metrics.c      # Standalone metric calculation test
│   ├── test_qhsjf.c        # Standalone QH-SJF aging test
│   └── test_sim.c          # Standalone schedule invariant test
├── workloads/              # Workload Datasets
│   ├── manual/
│   │   └── example.csv     # Sample CSV workload
│   └── generated/          # Storage for generated experiment traces
└── results/                # Output directory for exported benchmark CSVs
```

---

## 6. Quick Start Guide

### Windows (Native MinGW / PowerShell)

1. **Build all binaries**:
   ```cmd
   build.bat
   ```

2. **Run the paper verification test suite**:
   ```cmd
   build.bat test
   ```

3. **Start the web application**:
   ```cmd
   build.bat run
   ```
   Open **http://127.0.0.1:5173** in your browser.

---

### Linux / macOS / WSL

1. **Build all binaries**:
   ```bash
   make
   ```

2. **Run the paper test suite**:
   ```bash
   make test
   ```

3. **Start the web server**:
   ```bash
   make run
   ```
   Open **http://127.0.0.1:5173** in your browser.

---

### Using CMake

```bash
mkdir build && cd build
cmake ..
cmake --build .
ctest --output-on-failure
./qhsjf-server 5173
```

---

## 7. Web Interface Features

- **Single Algorithm Inspection**:
  - Full interactive Gantt chart with hover/click inspection of exact execution timeframes and CPU idle segments.
  - 6 Key Performance Indicators (Average WT, Turnaround, Response Time, CPU Utilization, Throughput, Makespan).
  - Tail latency distribution (Average Slowdown, Max Wait, P95 WT, P99 WT).
  - Live searchable and sortable per-process metrics table.
- **Compare All Mode**:
  - Benchmark all 8 policies against the identical workload input.
  - Best-in-class highlights (lowest waiting time, turnaround, response, and maximum throughput).
  - Side-by-side Gantt charts and horizontal waiting time comparison bars.
  - Complete algorithm scorecard table.
- **Paper Tests & Suite Runner**:
  - Run all 6 unit and invariant test suites directly from the browser with live microsecond timings, assertions, and pass/fail badges.
- **Workload Selection**:
  - Choose from the 7 paper families or input custom workloads via CSV paste/upload.
- **Data Export**:
  - **Export CSV** button downloads paper-ready per-process traces and summary statistics compatible with Python, R, and Pandas.
- **Theme Support**:
  - Toggle between dark mode and light mode with a single click.

---

## 8. REST API Reference

The C HTTP backend exposes the following endpoints:

| Endpoint | Method | Parameters | Description |
|---|---|---|---|
| `/api/health` | `GET` | None | Engine status and version check. |
| `/api/simulate` | `GET` | `policy`, `n`, `family`, `seed`, `err`, `bm` | Simulates a single policy. |
| `/api/compare` | `GET` | `n`, `family`, `seed`, `err`, `bm` | Simulates all 8 policies on the same workload. |
| `/api/custom-simulate` | `POST` | `policy`, `bm` (CSV body) | Simulates single policy on custom CSV input. |
| `/api/custom-compare` | `POST` | `bm` (CSV body) | Simulates all 8 policies on custom CSV input. |
| `/api/tests` | `GET` | None | Executes the complete test suite and returns JSON results. |
| `/api/paper-benchmark` | `GET` | `family`, `n`, `seed`, `err`, `bm` | Runs benchmark on specific paper family. |

---

## 9. Citation & License

### Citation
If you use this simulator or the QH-SJF algorithm in your research or coursework, please cite:

```bibtex
@article{qhsjf2026,
  title={Queue- and Heterogeneity-Aware Shortest Job First Scheduling with Dynamic Quadratic Aging},
  author={QH-SJF Research Team},
  year={2026}
}
```

### License
This project is licensed under the [MIT License](LICENSE) - see the [LICENSE](LICENSE) file for full details.
