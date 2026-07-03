<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>MV Drywall — Analytics</title>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/4.4.1/chart.umd.min.js"></script>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }

        :root {
            --bg: #0d0f12;
            --panel: #161a1f;
            --panel-line: #232a31;
            --ink: #f2f4f6;
            --ink-dim: #8b96a1;
            --accent: #f5a623;      /* amber — drywall/construction warmth */
            --accent-soft: rgba(245, 166, 35, 0.14);
            --blue: #4a90d9;
            --grid: rgba(255,255,255,0.05);
        }

        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif;
            background: var(--bg);
            color: var(--ink);
            min-height: 100vh;
            padding: 32px 24px 60px;
        }

        .wrap { max-width: 1100px; margin: 0 auto; }

        header {
            display: flex;
            align-items: baseline;
            justify-content: space-between;
            flex-wrap: wrap;
            gap: 12px;
            border-bottom: 1px solid var(--panel-line);
            padding-bottom: 20px;
            margin-bottom: 28px;
        }

        h1 {
            font-size: 26px;
            font-weight: 700;
            letter-spacing: -0.02em;
        }
        h1 .mark { color: var(--accent); }

        .subtitle { color: var(--ink-dim); font-size: 14px; margin-top: 4px; }

        .refresh {
            background: var(--accent-soft);
            color: var(--accent);
            border: 1px solid rgba(245,166,35,0.3);
            padding: 9px 16px;
            border-radius: 8px;
            font-size: 13px;
            font-weight: 600;
            cursor: pointer;
            transition: background 0.2s;
        }
        .refresh:hover { background: rgba(245,166,35,0.22); }

        /* KPI cards */
        .kpis {
            display: grid;
            grid-template-columns: repeat(4, 1fr);
            gap: 16px;
            margin-bottom: 28px;
        }
        .kpi {
            background: var(--panel);
            border: 1px solid var(--panel-line);
            border-radius: 12px;
            padding: 20px;
        }
        .kpi .label {
            font-size: 12px;
            text-transform: uppercase;
            letter-spacing: 0.08em;
            color: var(--ink-dim);
            margin-bottom: 10px;
        }
        .kpi .value { font-size: 28px; font-weight: 700; letter-spacing: -0.02em; }
        .kpi .value.accent { color: var(--accent); }

        /* Chart panels */
        .panel {
            background: var(--panel);
            border: 1px solid var(--panel-line);
            border-radius: 12px;
            padding: 22px;
            margin-bottom: 20px;
        }
        .panel h2 {
            font-size: 15px;
            font-weight: 600;
            margin-bottom: 18px;
            color: var(--ink);
        }
        .panel-row {
            display: grid;
            grid-template-columns: 1.6fr 1fr;
            gap: 20px;
        }
        .chart-box { position: relative; height: 300px; }
        .chart-box.tall { height: 340px; }

        .state {
            text-align: center;
            color: var(--ink-dim);
            padding: 60px 20px;
            font-size: 14px;
        }
        .state.error { color: #e0736d; }

        footer {
            color: var(--ink-dim);
            font-size: 12px;
            text-align: center;
            margin-top: 30px;
        }
        footer a { color: var(--accent); text-decoration: none; }

        @media (max-width: 820px) {
            .kpis { grid-template-columns: repeat(2, 1fr); }
            .panel-row { grid-template-columns: 1fr; }
        }
    </style>
</head>
<body>
    <div class="wrap">
        <header>
            <div>
                <h1>MV Drywall <span class="mark">Analytics</span></h1>
                <div class="subtitle">Invoice & estimate performance</div>
            </div>
            <button class="refresh" onclick="loadData()">Refresh data</button>
        </header>

        <div id="loadingState" class="state">Loading data from your sheet…</div>
        <div id="errorState" class="state error" style="display:none;"></div>

        <div id="content" style="display:none;">
            <div class="kpis">
                <div class="kpi">
                    <div class="label">Total Invoiced</div>
                    <div class="value accent" id="kpiRevenue">$0</div>
                </div>
                <div class="kpi">
                    <div class="label">Invoices</div>
                    <div class="value" id="kpiInvoices">0</div>
                </div>
                <div class="kpi">
                    <div class="label">Estimates</div>
                    <div class="value" id="kpiEstimates">0</div>
                </div>
                <div class="kpi">
                    <div class="label">Avg Invoice</div>
                    <div class="value" id="kpiAvg">$0</div>
                </div>
            </div>

            <div class="panel">
                <h2>Revenue by Month (Invoices)</h2>
                <div class="chart-box"><canvas id="revenueChart"></canvas></div>
            </div>

            <div class="panel-row">
                <div class="panel">
                    <h2>Top Customers by Revenue</h2>
                    <div class="chart-box tall"><canvas id="customerChart"></canvas></div>
                </div>
                <div class="panel">
                    <h2>Invoices vs Estimates</h2>
                    <div class="chart-box tall"><canvas id="typeChart"></canvas></div>
                </div>
            </div>
        </div>

        <footer>
            Reads live from your Google Sheet · <a href="index.html">← Back to invoice generator</a>
        </footer>
    </div>

    <script>
        // Same Apps Script URL used by the invoice generator
        const SHEET_URL = 'https://script.google.com/macros/s/AKfycbzzbctiNuqKW10e1Uzwdzv6R9SeApVNhE4silRcbL0VpSV-Za7dbJW9bJ2dYGijqgqRJw/exec';

        let charts = {};

        const money = (n) => '$' + Number(n || 0).toLocaleString('en-US', { maximumFractionDigits: 0 });

        function show(id) {
            ['loadingState', 'errorState', 'content'].forEach(s => {
                document.getElementById(s).style.display = (s === id) ? (s === 'content' ? 'block' : 'block') : 'none';
            });
        }

        async function loadData() {
            show('loadingState');
            try {
                const res = await fetch(SHEET_URL);
                if (!res.ok) throw new Error('Request failed (' + res.status + ')');
                const rows = await res.json();

                if (!Array.isArray(rows) || rows.length === 0) {
                    document.getElementById('errorState').textContent =
                        'No invoices logged yet. Generate one, then refresh.';
                    show('errorState');
                    return;
                }

                render(rows);
                show('content');
            } catch (err) {
                document.getElementById('errorState').textContent =
                    'Could not load data: ' + err.message + '. Check that the Apps Script is deployed with a doGet function.';
                show('errorState');
                console.error(err);
            }
        }

        function render(rows) {
            // Normalize
            const data = rows.map(r => ({
                type: String(r.documentType || '').toLowerCase(),
                customer: r.customerBusiness || r.customerName || 'Unknown',
                date: r.invoiceDate || '',
                amount: parseFloat(r.amount) || 0
            }));

            const invoices = data.filter(d => d.type === 'invoice');
            const estimates = data.filter(d => d.type === 'estimate');
            const totalRevenue = invoices.reduce((s, d) => s + d.amount, 0);

            // KPIs
            document.getElementById('kpiRevenue').textContent = money(totalRevenue);
            document.getElementById('kpiInvoices').textContent = invoices.length;
            document.getElementById('kpiEstimates').textContent = estimates.length;
            document.getElementById('kpiAvg').textContent =
                money(invoices.length ? totalRevenue / invoices.length : 0);

            renderRevenueByMonth(invoices);
            renderTopCustomers(invoices);
            renderTypeSplit(invoices.length, estimates.length);
        }

        function monthKey(dateStr) {
            const d = new Date(dateStr);
            if (isNaN(d)) return null;
            return d.getFullYear() + '-' + String(d.getMonth() + 1).padStart(2, '0');
        }

        function renderRevenueByMonth(invoices) {
            const byMonth = {};
            invoices.forEach(d => {
                const k = monthKey(d.date);
                if (!k) return;
                byMonth[k] = (byMonth[k] || 0) + d.amount;
            });
            const keys = Object.keys(byMonth).sort();
            const labels = keys.map(k => {
                const [y, m] = k.split('-');
                return new Date(y, m - 1).toLocaleDateString('en-US', { month: 'short', year: '2-digit' });
            });

            drawChart('revenueChart', 'line', {
                labels,
                datasets: [{
                    data: keys.map(k => byMonth[k]),
                    borderColor: '#f5a623',
                    backgroundColor: 'rgba(245,166,35,0.12)',
                    fill: true,
                    tension: 0.3,
                    pointBackgroundColor: '#f5a623',
                    pointRadius: 4
                }]
            }, moneyAxisOptions());
        }

        function renderTopCustomers(invoices) {
            const byCust = {};
            invoices.forEach(d => { byCust[d.customer] = (byCust[d.customer] || 0) + d.amount; });
            const sorted = Object.entries(byCust).sort((a, b) => b[1] - a[1]).slice(0, 6);

            drawChart('customerChart', 'bar', {
                labels: sorted.map(e => e[0]),
                datasets: [{
                    data: sorted.map(e => e[1]),
                    backgroundColor: '#4a90d9',
                    borderRadius: 6
                }]
            }, {
                indexAxis: 'y',
                ...moneyAxisOptions(),
                plugins: { legend: { display: false } }
            });
        }

        function renderTypeSplit(inv, est) {
            drawChart('typeChart', 'doughnut', {
                labels: ['Invoices', 'Estimates'],
                datasets: [{
                    data: [inv, est],
                    backgroundColor: ['#f5a623', '#4a90d9'],
                    borderColor: '#161a1f',
                    borderWidth: 3
                }]
            }, {
                plugins: {
                    legend: { labels: { color: '#8b96a1' }, position: 'bottom' }
                }
            });
        }

        function moneyAxisOptions() {
            return {
                plugins: { legend: { display: false } },
                scales: {
                    y: {
                        ticks: { color: '#8b96a1', callback: v => '$' + Number(v).toLocaleString() },
                        grid: { color: 'rgba(255,255,255,0.05)' }
                    },
                    x: {
                        ticks: { color: '#8b96a1' },
                        grid: { color: 'rgba(255,255,255,0.05)' }
                    }
                }
            };
        }

        function drawChart(id, type, data, options) {
            if (charts[id]) charts[id].destroy();
            charts[id] = new Chart(document.getElementById(id), {
                type,
                data,
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    ...options
                }
            });
        }

        loadData();
    </script>
</body>
</html>
