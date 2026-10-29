/*
 * QH-SJF Scheduling Lab
 * Interactive Web Application (Vanilla JS)
 */
const $ = s => document.querySelector(s);
const $$ = s => document.querySelectorAll(s);

const POL = [
    ['fcfs', 'First Come First Served (FCFS)', 'FCFS', '#9bbf6f'],
    ['sjf', 'Shortest Job First (SJF)', 'SJF', '#9db8a4'],
    ['hrrn', 'Highest Response Ratio Next (HRRN)', 'HRRN', '#c9d19a'],
    ['lin', 'Linear Aging SJF', 'Linear Aging', '#d9c49a'],
    ['quad', 'Fixed Quadratic Aging SJF', 'Fixed Quadratic', '#a9bfd6'],
    ['lonly', 'Queue-pressure-only variant (L-only)', 'L-only', '#bfaedd'],
    ['honly', 'Heterogeneity-only variant (H-only)', 'H-only', '#d9a99f'],
    ['qh', 'Queue- and Heterogeneity-Aware Shortest Job First (QH-SJF)', 'QH-SJF', '#bdf05a']
];

const FAM_DESC = {
    '0': 'Interactive synthetic mix of short (60%), medium (30%), and long (10%) bursts with log-normal prediction error.',
    '1': 'Constant burst lengths (B=8) arriving evenly; isolates queueing fairness with zero burst variance.',
    '2': 'Three distinct process classes (short, medium, long); tests differentiation across mixed execution times.',
    '3': 'Long convoy process (B=40) at head of queue followed by short bursts; tests starvation recovery under aging.',
    '4': 'Processes arrive nearly simultaneously within an initial surge; evaluates queue pressure saturation.',
    '5': 'Periodic bursts arriving in clusters of four; simulates recurring batch submission cycles.',
    '6': 'Arrivals spaced widely apart; low queue pressure where queues frequently empty.',
    '7': 'Extreme burst length variance (up to 40x); stresses heterogeneity metric H and aging resistance.',
    '-1': 'Custom user-supplied process trace. Upload a CSV file or paste lines in pid,arrival,burst format.'
};

const PC = ['#bdf05a', '#c9b8e0', '#e0c8a0', '#a9c0d4', '#d4dca0', '#d9aaa0', '#a8c4a8', '#b8c4bb'];
const f = x => (typeof x === 'number' ? +x.toFixed(2) : x);

let mode = 'single';
let lastSimulationData = null;

/* Populate policy selector */
$('#pol').innerHTML = POL.map(p => `<option value="${p[0]}">${p[1]}</option>`).join('');
$('#pol').value = 'qh';

const pname = () => {
    const found = POL.find(p => p[0] === $('#pol').value);
    return found ? found[1] : 'QH-SJF';
};
$('#pn').textContent = pname();

/* Handle Workload Family selection */
$('#fam').onchange = () => {
    const val = $('#fam').value;
    $('#famdesc').textContent = FAM_DESC[val] || '';
    if (val === '-1') {
        $('#csvbox').classList.remove('hide');
        $('#nControlGroup').classList.add('hide');
    } else {
        $('#csvbox').classList.add('hide');
        $('#nControlGroup').classList.remove('hide');
    }
};

/* Example CSV loader */
$('#loadEgCsv').onclick = () => {
    $('#csvText').value = "pid,arrival,burst\n1,0,6\n2,1,2\n3,2,4\n4,3,1";
};

/* CSV file upload */
$('#uploadCsvBtn').onclick = () => $('#csvFile').click();
$('#csvFile').onchange = e => {
    const file = e.target.files[0];
    if (!file) return;
    const reader = new FileReader();
    reader.onload = ev => {
        $('#csvText').value = ev.target.result;
    };
    reader.readAsText(file);
};

/* Searchable, Sortable Data Table Component */
function mkTable(host, cols, rows) {
    let s = null, d = 1, q = '';
    const draw = () => {
        let r = rows.filter(x => !q || Object.values(x).join(' ').toLowerCase().includes(q));
        if (s) {
            r = [...r].sort((a, b) => (a[s] > b[s] ? 1 : -1) * d);
        }
        host.innerHTML = `
            <div class="tb">
                <span class="mono">${r.length} rows</span>
                <input placeholder="Search rows" aria-label="Search rows" value="${q}">
            </div>
            <div class="tw">
                <table>
                    <thead>
                        <tr>
                            ${cols.map(c => `<th data-k="${c.k}" class="${c.n ? 'r' : ''}">${c.l} ⇅</th>`).join('')}
                        </tr>
                    </thead>
                    <tbody>
                        ${r.map(x => `
                            <tr>
                                ${cols.map(c => `<td class="${c.n ? 'r' : ''}">${c.f ? c.f(x) : x[c.k]}</td>`).join('')}
                            </tr>
                        `).join('')}
                    </tbody>
                </table>
            </div>`;

        host.querySelectorAll('th').forEach(h => {
            h.onclick = () => {
                d = (s === h.dataset.k) ? -d : 1;
                s = h.dataset.k;
                draw();
            };
        });

        const input = host.querySelector('input');
        input.oninput = e => {
            q = e.target.value.toLowerCase();
            draw();
            const j = host.querySelector('input');
            j.focus();
            j.setSelectionRange(q.length, q.length);
        };
    };
    draw();
}

const pcols = [
    ['id', 'PID'],
    ['at', 'AT'],
    ['bt', 'BT'],
    ['pb', 'Pred. BT'],
    ['st', 'Start'],
    ['ct', 'CT'],
    ['tat', 'TAT'],
    ['wt', 'WT'],
    ['rt', 'RT']
].map(c => ({ k: c[0], l: c[1], n: c[0] !== 'id' }));

/* Gantt Chart Component */
function gantt(seg, mkn) {
    const n = seg.length;
    const density = n > 120 ? 'compact' : (n > 40 ? 'dense' : '');
    return `
        <div class="gantt-wrap">
            <div class="gantt-header-meta">
                <span class="mono">TIMELINE · ${n} INTERVALS</span>
                <span class="gantt-scroll-hint">⇄ Mousewheel / drag to scroll</span>
            </div>
            <div class="gantt-scroll-pane">
                <div class="gantt ${density}">
                    ${seg.map(g => g.idle
                        ? `<i class="idle" title="Idle: ${f(g.s)} → ${f(g.e)}"></i>`
                        : `<i style="background:${PC[g.id % 8]}" title="P${g.id}: ${f(g.s)} → ${f(g.e)}" data-t="P${g.id} ran from ${f(g.s)} to ${f(g.e)}">P${g.id}</i>`
                    ).join('')}
                </div>
            </div>
            <div class="axis">
                <span>Start: 0.00</span>
                <span>Timeline increases →</span>
                <span>Makespan: ${f(mkn)}</span>
            </div>
            <div class="leg">
                <span><s></s>Process execution</span>
                <span><s style="background:#555"></s>Idle time</span>
                <span class="gi">Click or hover over a segment for its exact interval.</span>
            </div>
        </div>`;
}

/* 6 Primary KPIs */
const K6 = m => [
    ['Average waiting time', f(m.wt), 'WT · time units'],
    ['Average turnaround', f(m.tat), 'TAT · time units'],
    ['Average response', f(m.rt), 'RT · time units'],
    ['CPU utilization', f(m.util) + '%', 'Busy time / elapsed time'],
    ['Throughput', f(m.thr), 'Processes / time unit'],
    ['Makespan', f(m.mk), 'Last completion timestamp']
].map((k, i) => `
    <div class="k ${i === 0 ? 'hi' : ''}">
        <span class="mu">${k[0]}</span>
        <b>${k[1]}</b>
        <small>${k[2]}</small>
    </div>
`).join('');

/* Simulation Runner */
async function run() {
    const n = Math.max(1, Math.min(500, +$('#n').value || 50));
    const er = +$('#er').value || 0;
    const bm = +$('#bm').value || 4;
    const fam = +$('#fam').value;
    const out = $('#out');
    const id = $('#pol').value;

    const isCustom = (fam === -1);
    const customCsv = $('#csvText').value.trim();

    if (isCustom && !customCsv) {
        alert('Please enter or upload CSV data first.');
        return;
    }

    const q = `n=${n}&family=${fam}&seed=${+$('#sd').value || 0}&err=${er}&bm=${bm}`;
    let D = {}, seed, actualN, familyName;

    $('#run').disabled = true;
    $('#run').textContent = 'Simulating…';

    try {
        let r, d;
        if (isCustom) {
            const url = (mode === 'single')
                ? `/api/custom-simulate?policy=${id}&bm=${bm}`
                : `/api/custom-compare?bm=${bm}`;
            r = await fetch(url, {
                method: 'POST',
                headers: { 'Content-Type': 'text/plain' },
                body: customCsv
            });
        } else {
            const url = (mode === 'single')
                ? `/api/simulate?policy=${id}&${q}`
                : `/api/compare?${q}`;
            r = await fetch(url);
        }

        d = await r.json();
        if (d.error) throw new Error(d.error);

        seed = d.seed;
        actualN = d.n;
        familyName = d.familyName || 'Standard Workload';
        d.results.forEach(x => { D[x.policy] = x; });
        lastSimulationData = d;
    } catch (e) {
        out.innerHTML = `
            <section class="card">
                <b style="color:var(--fail)">Backend error:</b> ${e.message}.
                <br><small class="sub">Ensure the C server is running on port 5173.</small>
            </section>`;
        $('#run').disabled = false;
        $('#run').textContent = '▷ Run simulation →';
        return;
    }

    $('#run').disabled = false;
    $('#run').textContent = '▷ Run simulation →';

    const sim = p => D[p];

    if (mode === 'single') {
        const pl = POL.find(p => p[0] === id);
        const r = sim(id);
        const m = r.m;

        out.innerHTML = `
            <section class="card" style="display:flex;gap:18px;align-items:center;justify-content:space-between">
                <div style="display:flex;gap:18px;align-items:center">
                    <span class="ic" style="background:var(--lime);color:var(--on);width:56px;height:56px">▣</span>
                    <div>
                        <span class="mono">POLICY SELECTED · ${familyName}</span>
                        <h2 style="font-size:22px;margin:2px 0">${pl[1]}</h2>
                        <span class="sub">${actualN} processes scheduled · seed #${seed}</span>
                    </div>
                </div>
                <span class="tag">${pl[2]}</span>
            </section>

            <div class="kpis">${K6(m)}</div>

            <section class="card">
                <span class="mono">02 / TIMELINE EXECUTION</span>
                <div class="sec">
                    <h2>Gantt chart</h2>
                    <span class="pill sub">◷ Time units</span>
                </div>
                <p class="sub">Bars show process execution intervals. Hover or click for details.</p>
                ${gantt(r.seg, m.mk)}
            </section>

            <section class="card">
                <span class="mono">03 / PROCESS DETAIL & TAIL METRICS</span>
                <div class="sec">
                    <h2>Per-process metrics</h2>
                    <span class="tag">${pl[2]}</span>
                </div>
                <p class="sub">AT, BT, Predicted BT, Start, CT, TAT, WT, and response time.</p>
                <div class="st4">
                    ${[
                        ['Average slowdown', m.sl],
                        ['Maximum waiting', m.max],
                        ['95th percentile WT', m.p95],
                        ['99th percentile WT', m.p99]
                    ].map(x => `
                        <div>
                            <span class="mu" style="font-size:15px">${x[0]}</span>
                            <b>${f(x[1])}</b>
                        </div>
                    `).join('')}
                </div>
                <div id="pt" style="margin-top:16px"></div>
            </section>`;

        mkTable($('#pt'), pcols, r.res);
    } else {
        /* Compare all mode */
        const R = POL.map(p => ({ p, ...sim(p[0]) }));
        const best = (k, dir) => R.reduce((a, b) => (dir * (b.m[k] - a.m[k]) < 0 ? b : a));

        const I = [
            ['Lowest avg. waiting', 'wt', 1, 'time units'],
            ['Lowest avg. turnaround', 'tat', 1, 'time units'],
            ['Lowest avg. response', 'rt', 1, 'time units'],
            ['Highest throughput', 'thr', -1, '/ unit']
        ].map((x, i) => {
            const b = best(x[1], x[2]);
            return `
                <div class="k">
                    <span class="mono">0${i + 1}</span><br>
                    <span class="mu">${x[0]}</span>
                    <b>${b.p[2]}</b>
                    <small class="mono">${f(b.m[x[1]])} ${x[3]}</small>
                </div>`;
        }).join('');

        const maxWt = Math.max(...R.map(r => r.m.wt));

        out.innerHTML = `
            <section class="card">
                <span class="mono">01 / COMPARISON BENCHMARK · ${familyName}</span>
                <h2>Same workload, eight policies</h2>
                <span class="sub">${actualN} processes evaluated · seed #${seed}</span>
            </section>

            <div class="ins">${I}</div>

            <section class="card">
                <span class="mono">02 / BENCHMARK SCORECARD</span>
                <div class="sec">
                    <h2>Algorithm comparison table</h2>
                    <span class="pill sub">▥ 8 policies</span>
                </div>
                <p class="sub">Lower is better for waiting, turnaround, and slowdown. Higher is better for throughput and utilization.</p>
                <div id="sc"></div>
            </section>

            <section class="card bars">
                <span class="mono">AVERAGE WAITING TIME (WT)</span>
                <div class="sec">
                    <p class="sub">Direct comparison of process queue delay before execution.</p>
                    <span class="tag" style="color:var(--mu)">LOWER IS BETTER</span>
                </div>
                ${R.map(r => `
                    <div class="b">
                        <span class="mono" style="color:var(--tx)">${r.p[2]}</span>
                        <span class="t">
                            <u style="width:${Math.max(2, 100 * r.m.wt / maxWt)}%;background:${r.p[3]}"></u>
                        </span>
                    </div>
                `).join('')}
            </section>

            <div style="margin-top:28px">
                <span class="mono">03 / TIMELINES</span>
                <div class="sec">
                    <h2>Gantt charts across all policies</h2>
                    <span class="pill sub">Same workload</span>
                </div>
                <p class="sub">All charts share the identical arrival sequence. Expand any algorithm to view its individual rows.</p>
            </div>

            <div class="gg">
                ${R.map((r, i) => `
                    <section class="card">
                        <div class="sec">
                            <div style="display:flex;gap:12px;align-items:center">
                                <span class="ic" style="background:${r.p[3]};color:var(--on);width:36px;height:36px;border-radius:10px">0${i + 1}</span>
                                <div>
                                    <b>${r.p[1]}</b>
                                    <div class="sub" style="font-size:15.5px;margin-top:2px">${actualN} jobs · ${r.seg.length} intervals</div>
                                </div>
                            </div>
                        </div>
                        ${gantt(r.seg, r.m.mk)}
                        <div class="m4">
                            ${[
                                ['Avg WT', r.m.wt],
                                ['Avg TAT', r.m.tat],
                                ['Avg RT', r.m.rt],
                                ['CPU util.', r.m.util]
                            ].map(x => `
                                <span class="mu" style="font-size:15px">${x[0]}
                                    <b style="color:var(--tx);font-size:20px">${f(x[1])}</b>
                                </span>
                            `).join('')}
                        </div>
                        <details style="margin-top:12px">
                            <summary class="pill">View process rows ▾</summary>
                            <div id="gr${i}"></div>
                        </details>
                    </section>
                `).join('')}
            </div>`;

        mkTable($('#sc'), [
            { k: 'a', l: 'Algorithm', f: x => `<span class="dot" style="background:${x.c}"></span>${x.a}` },
            ...[
                ['wt', 'Avg WT'],
                ['tat', 'Avg TAT'],
                ['rt', 'Avg RT'],
                ['sl', 'Avg slowdown'],
                ['max', 'Max WT'],
                ['p95', 'P95 WT'],
                ['p99', 'P99 WT'],
                ['thr', 'Throughput'],
                ['util', 'CPU util.'],
                ['mk', 'Makespan']
            ].map(c => ({ k: c[0], l: c[1], n: 1 }))
        ], R.map(r => ({
            a: r.p[1],
            c: r.p[3],
            ...Object.fromEntries(Object.entries(r.m).map(([k, v]) => [k, f(v)]))
        })));

        R.forEach((r, i) => mkTable($('#gr' + i), pcols, r.res));
    }

    /* Gantt tooltip handling */
    out.querySelectorAll('.gantt i[data-t]').forEach(e => {
        e.onclick = () => {
            const card = e.closest('.card');
            card.querySelectorAll('i.sel').forEach(x => x.classList.remove('sel'));
            e.classList.add('sel');
            card.querySelector('.gi').textContent = e.dataset.t;
        };
    });

    /* Enable horizontal mousewheel and drag-to-scroll on all Gantt panes */
    out.querySelectorAll('.gantt-scroll-pane').forEach(pane => {
        pane.addEventListener('wheel', e => {
            if (e.deltaY !== 0 && pane.scrollWidth > pane.clientWidth) {
                e.preventDefault();
                pane.scrollLeft += e.deltaY * 1.5;
            }
        }, { passive: false });

        let isDown = false, startX, scrollLeft;
        pane.addEventListener('mousedown', e => {
            if (e.target.tagName.toLowerCase() === 'i') return;
            isDown = true;
            startX = e.pageX - pane.offsetLeft;
            scrollLeft = pane.scrollLeft;
        });
        pane.addEventListener('mouseleave', () => { isDown = false; });
        pane.addEventListener('mouseup', () => { isDown = false; });
        pane.addEventListener('mousemove', e => {
            if (!isDown) return;
            e.preventDefault();
            const x = e.pageX - pane.offsetLeft;
            const walk = (x - startX) * 1.5;
            pane.scrollLeft = scrollLeft - walk;
        });
    });

    $('#rq').textContent = actualN;
    out.scrollIntoView({ behavior: matchMedia('(prefers-reduced-motion:reduce)').matches ? 'auto' : 'smooth' });
}

/* Paper Test Suite Runner */
async function runTests() {
    const box = $('#testResultsBox');
    const btn = $('#runTestsBtn');
    btn.disabled = true;
    btn.textContent = 'Running tests in C…';

    try {
        const res = await fetch('/api/tests');
        const data = await res.json();

        btn.disabled = false;
        btn.textContent = '🧪 Re-run Paper Tests →';

        const allPass = data.failed === 0;

        box.innerHTML = `
            <div class="test-suite-header">
                <div>
                    <span class="mono">STATUS</span>
                    <h3 style="margin:4px 0 2px;font-size:20px;color:${allPass ? 'var(--pass)' : 'var(--fail)'}">
                        ${allPass ? '✓ ALL PAPER TESTS PASSED' : `✕ ${data.failed} TEST(S) FAILED`}
                    </h3>
                    <span class="sub">${data.passed} of ${data.total} test suites verified in ${f(data.total_duration_ms)} ms</span>
                </div>
                <span class="pill mono" style="font-size:13px">${f(data.total_duration_ms)} ms</span>
            </div>

            <div class="test-grid">
                ${data.tests.map(t => `
                    <div class="test-card ${t.passed ? 'passed' : 'failed'}">
                        <div class="test-info">
                            <div style="display:flex;gap:10px;align-items:center;margin-bottom:6px">
                                <span class="test-badge ${t.passed ? 'pass' : 'fail'}">${t.passed ? 'PASS' : 'FAIL'}</span>
                                <h3 style="margin:0">${t.name}</h3>
                            </div>
                            <p>${t.description}</p>
                            <span class="detail">${t.details}</span>
                        </div>
                        <span class="mono">${f(t.duration_ms)} ms</span>
                    </div>
                `).join('')}
            </div>`;
    } catch (e) {
        btn.disabled = false;
        btn.textContent = '🧪 Run Paper Tests →';
        box.innerHTML = `<div class="card"><b style="color:var(--fail)">Failed to run tests:</b> ${e.message}</div>`;
    }
}

/* CSV Export Functionality */
function exportResultsCsv() {
    if (!lastSimulationData || !lastSimulationData.results) {
        alert('Please run a simulation first before exporting CSV.');
        return;
    }

    let csv = "policy,pid,arrival,burst,predicted,start,completion,waiting,turnaround,response\n";
    lastSimulationData.results.forEach(res => {
        const pol = res.policy;
        res.res.forEach(r => {
            csv += `${pol},${r.id},${r.at},${r.bt},${r.pb},${r.st},${r.ct},${r.wt},${r.tat},${r.rt}\n`;
        });
    });

    csv += "\npolicy,avg_wait,avg_turnaround,avg_response,avg_slowdown,max_wait,p95_wait,p99_wait,throughput,cpu_utilization,makespan\n";
    lastSimulationData.results.forEach(res => {
        const m = res.m;
        csv += `${res.policy},${m.wt},${m.tat},${m.rt},${m.sl},${m.max},${m.p95},${m.p99},${m.thr},${m.util},${m.mk}\n`;
    });

    const blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' });
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.setAttribute('href', url);
    link.setAttribute('download', `qhsjf_experiment_seed${lastSimulationData.seed}_n${lastSimulationData.n}.csv`);
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
}

/* Process count N helper */
function setN(v) {
    v = Math.max(1, Math.min(500, Math.round(v) || 1));
    $('#n').value = v;
    $('#nr').value = v;
    $('#rq').textContent = v;
}

$('#n').oninput = e => setN(+e.target.value);
$('#nr').oninput = e => setN(+e.target.value);
$$('.step button').forEach(b => b.onclick = () => setN(+$('#n').value + +b.dataset.d));

$('#pol').onchange = () => { $('#pn').textContent = pname(); };

/* Mode Switcher */
$$('.mode').forEach(b => {
    b.onclick = () => {
        mode = b.dataset.m;
        $$('.mode').forEach(x => x.classList.toggle('on', x === b));
        $('#out').innerHTML = '';

        if (mode === 'tests') {
            $('#cfgCard').classList.add('hide');
            $('#testsCard').classList.remove('hide');
            $('#modeTip').textContent = '🧪 Paper unit tests & invariant verification';
            runTests();
        } else {
            $('#cfgCard').classList.remove('hide');
            $('#testsCard').classList.add('hide');
            $('#pbox').classList.toggle('hide', mode === 'compare');
            $('#rm').textContent = (mode === 'single') ? '01 / SELECTIVE' : '02 / COMPARE ALL';
            $('#modeTip').textContent = '? Same workload, fair comparison';
        }
    };
});

$('#run').onclick = run;
$('#runTestsBtn').onclick = runTests;
$('#exportCsv').onclick = exportResultsCsv;

/* Check Backend Engine Health */
fetch('/api/health')
    .then(r => r.json())
    .then(d => {
        $('#eng').innerHTML = '<span class="dot"></span>C ENGINE CONNECTED';
    })
    .catch(() => {
        $('#eng').innerHTML = '<span class="dot" style="background:#d9695f"></span>ENGINE OFFLINE';
    });

/* Theme toggle */
$('#th').onclick = () => {
    const root = document.documentElement;
    const isLight = root.dataset.theme === 'light';
    root.dataset.theme = isLight ? 'dark' : 'light';
    $('#th').textContent = isLight ? '☀' : '☾';
};
