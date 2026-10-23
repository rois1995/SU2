#!/usr/bin/env python3
"""DC-8 per-case summary of runs/dc8_<case>.json (+ R2 trace classes from runs/dc8_<case>.log). Usage: summarize_dc8.py [runs-dir]"""
import json, re, sys, collections
from pathlib import Path
R = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parent / 'runs'
rows = {}
for f in sorted(R.glob('dc8_*.json')):
    d = json.loads(f.read_text()); c = d['case']
    text = (R / f'dc8_{c}.log').read_text()
    trace = collections.Counter(re.findall(r'cls=([a-z-]*)', text))
    chains = re.findall(r'R2 tx v=(\d+) \|V\|=(\d+) low=(\d) ok=(\d) old=\d+ fresh=\d+ sectors=\d+ gaps=\d+ cls=([a-z-]*)', text)
    roots, i = collections.Counter(), 0
    while i < len(chains):
        j = i + 1
        while j < len(chains) and chains[j][0] == chains[i][0] and not (chains[j][1] == '1' and chains[j][2] == '0'):
            j += 1
        seg = chains[i:j]
        if seg[-1][3] == '0':
            roots[f'{seg[0][4]} -> {seg[-1][4]}'] += 1
        i = j
    r2 = d['R2_counts']; fin = d['final']; ra = d['reaudit']; ops = d['perf']['ops']
    top = sorted(((v['seconds'], k, v['count']) for k, v in ops.items() if not k.startswith('Phase')), reverse=True)[:8]
    rows[c] = {'W': f"{d['W']['outcome']} {d['input']['wall_faces']}->{d['after_W']['wall_faces']} faces "
                    f"(stations {d['W_counts']['stations']}, levels {d['W_counts']['level_splits']}, bisections {d['W_counts']['bisections']})",
               'creases_test_side/production_segments': [d['input']['d1']['creases_in_cut_test_side'], d['input']['d1']['segments']],
               'lp_empty_full_state_after_W/R1': [d['census_after_W']['faces_full_state_lp_empty'], d['census_after_R1']['faces_full_state_lp_empty']],
               'clearance_deficient_W/R1': [d['census_after_W']['clearance_deficient_vertices'], d['census_after_R1']['clearance_deficient_vertices']],
               'R1': [d['R1']['outcome'], d['R1_counts']],
               'R2_rowed/unrowed': [r2['rowed'], r2['unrowed']], 'R2_lifted/unlifted': [r2['lifted'], r2['unlifted']],
               'R2_low_band/sector_tx/gaps/separations': [r2['low_band_transactions'], r2['sector_transactions'], r2['gap_cells'], r2['corner_separations']],
               'R2_final_witness_classes': r2['witness_classes'], 'R2_attempt_classes': dict(trace), 'R2_rejected_chains_root->final': dict(roots),
               'first_child_q/J/herr_at_admission': [r2['first_q_min'], r2['first_J_min'], r2['max_height_error']],
               'V': d['V']['outcome'], 'V_work/s': [d['V']['work'], d['V']['seconds']], 'V_counts': d['V_counts'],
               'final_complete': fin['complete'], 'final_cells': fin['cells'], 'long_edges': fin['long_edges'], 'long_by_class': fin['long_by_class'],
               'bad_shape': fin['bad_shape'], 'bad_first/column/outside': [fin['bad_first_child'], fin['bad_column_other'], fin['bad_outside_columns']],
               'first_child_q_min_final': fin['first_child_q_min'], 'height_failures': fin['height_failures'], 'unmeasurable': fin['unmeasurable_edges'],
               'reaudit': {k: ra[k] for k in ('long_edges', 'bad_shape', 'height_failures', 'unmeasurable', 'complete', 'seconds')},
               'phase_seconds': {k: d[k]['seconds'] for k in ('W', 'R1', 'R2', 'V')}, 'total_seconds': d['total_seconds'], 'peak_rss_kB': d['peak_rss_kB'],
               'phase_field': {k: d[k]['field'] for k in ('W', 'R1', 'R2', 'V')},
               'top_ops_seconds': [[round(t, 2), k, n] for t, k, n in top], 'caches': d['perf']['caches'],
               'R2_last': d['R2']['last_witness'][:240]}
json.dump(rows, sys.stdout, indent=1); print()
