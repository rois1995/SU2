#!/usr/bin/env python3
"""DC-10 per-case summary of <root>/dc10_<case>.json (+ R2 trace classes from the case log).
Usage: summarize_dc10.py <root> [<root> ...]; writes <root>/summary.json and prints one line per case."""
import json, re, sys, collections
from pathlib import Path

def case_row(d, log):
    f, r2 = d['final'], d['R2_counts']
    trace = collections.Counter(re.findall(r'cls=([a-z0-9-]*)', log))
    ridge_tx = re.findall(r'R2 tx v=\d+ \|V\|=\d+ low=\d ok=(\d) old=\d+ fresh=\d+ ridges=([1-9]\d*)', log)
    return {
        'W': d['W']['outcome'], 'W_counts': d['W_counts'],
        'R1w': d.get('R1w_counts'), 'R1w_reasons': d.get('R1w', {}).get('reasons'),
        'R2_rowed/unrowed': [r2['rowed'], r2['unrowed']],
        'R2_ridge_lifts': r2.get('ridge_lifts'), 'R2_fan_cells_now': r2.get('fan_cells_now'), 'R2_gap_cells': r2.get('gap_cells'),
        'R2_fan_check_failures': r2.get('fan_check_failures'), 'R2_witness_classes': r2.get('witness_classes'),
        'R2_ridge_transactions_ok/refused': [sum(1 for o, _ in ridge_tx if o == '1'), sum(1 for o, _ in ridge_tx if o == '0')],
        'R2_trace_classes': dict(trace),
        'first_q/J_at_admission': [r2['first_q_min'], r2['first_J_min']],
        'after_R_bad(first/column/outside)': [d['after_R']['bad_shape'], d['after_R']['bad_first_child'], d['after_R']['bad_column_other'], d['after_R']['bad_outside_columns']],
        'V': [d['V']['outcome'], d['V']['work'], round(d['V']['seconds'], 1), d['V_counts']['sweeps']],
        'final_cells': f['cells'], 'final_bad(first/column/outside)': [f['bad_shape'], f['bad_first_child'], f['bad_column_other'], f['bad_outside_columns']],
        'final_fan': d.get('final_fan'), 'final_long': f['long_edges'], 'final_long_by_class': f['long_by_class'],
        'final_height_failures': f['height_failures'], 'final_q_min': f['q_min'], 'complete': f['complete'],
        'reaudit_agrees': d['reaudit']['agrees_with_cached_final'], 'total_seconds': round(d['total_seconds'], 1),
        'peak_rss_kB': d['peak_rss_kB'], 'wall_distance_evaluations': d.get('wall_distance_evaluations'),
    }

for root in map(Path, sys.argv[1:]):
    rows = {}
    for jf in sorted(root.glob('dc10_*.json')):
        if jf.stem.endswith(('_W', '_R2')):
            continue
        d = json.loads(jf.read_text()); c = d['case']
        lf = root / f'dc10_{c}.log'
        rows[c] = case_row(d, lf.read_text() if lf.exists() else '')
    (root / 'summary.json').write_text(json.dumps(rows, indent=1))
    for c, r in rows.items():
        print(f"{root.name} {c}: rowed/unrowed {r['R2_rowed/unrowed']} ridge ok/refused {r['R2_ridge_transactions_ok/refused']} "
              f"bad {r['final_bad(first/column/outside)']} fan {r['final_fan']} long {r['final_long']} h {r['final_height_failures']} "
              f"complete {r['complete']} V {r['V']} t {r['total_seconds']}")
