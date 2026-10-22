#!/usr/bin/env python3
"""DC-7 per-case summary of runs/dc7_<case>.json (+ R2 trace classes from runs/dc7_<case>.log)."""
import json, re, sys, collections
from pathlib import Path
R = Path(__file__).resolve().parent / 'runs'
rows = {}
for f in sorted(R.glob('dc7_*.json')):
    d = json.loads(f.read_text()); c = d['case']
    text = (R / f'dc7_{c}.log').read_text()
    trace = collections.Counter(re.findall(r'cls=([a-z-]*)', text))
    # root cause of each rejected chain: class of its |V|=1 attempt (growth then ends in that class or another)
    roots, chain = collections.Counter(), None
    for m in re.finditer(r'R2 tx v=(\d+) \|V\|=(\d+) ok=(\d) old=\d+ fresh=\d+ cls=([a-z-]*)', text):
        v, n, ok, cls = m.groups()
        if n == '1':
            chain = cls
        if ok == '1':
            chain = None
    for m in re.finditer(r'R2 tx v=(\d+) \|V\|=1 ok=0 old=\d+ fresh=\d+ cls=([a-z-]*)', text):
        pass
    chains = re.findall(r'R2 tx v=(\d+) \|V\|=(\d+) ok=(\d) old=\d+ fresh=\d+ cls=([a-z-]*)', text)
    i = 0
    while i < len(chains):
        j = i + 1
        while j < len(chains) and chains[j][1] != '1':
            j += 1
        seg = chains[i:j]
        if seg[-1][2] == '0':
            roots[f'{seg[0][3]} -> {seg[-1][3]}'] += 1
        i = j
    r2 = d['R2_counts']; fin = d['final']
    rows[c] = {'W': f"{d['W']['outcome']} {d['input']['wall_faces']}->{d['after_W']['wall_faces']} faces",
               'lp_empty_full_state_after_W/R1': [d['census_after_W']['faces_full_state_lp_empty'], d['census_after_R1']['faces_full_state_lp_empty']],
               'clearance_deficient_W/R1': [d['census_after_W']['clearance_deficient_vertices'], d['census_after_R1']['clearance_deficient_vertices']],
               'R2_rowed/unrowed': [r2['rowed'], r2['unrowed']], 'R2_lifted/unlifted': [r2['lifted'], r2['unlifted']],
               'R2_final_witness_classes': r2['witness_classes'], 'R2_attempt_classes': dict(trace), 'R2_rejected_chains_root->final': dict(roots),
               'first_child_q/J/herr_at_admission': [r2['first_q_min'], r2['first_J_min'], r2['max_height_error']],
               'V': d['V']['outcome'], 'V_work/s': [d['V'].get('work'), d['V'].get('seconds')], 'V_column_demand': d.get('V_column_demand'),
               'final_complete': fin['complete'], 'final_cells': fin['cells'], 'long_edges': fin['long_edges'], 'long_by_class': fin['long_by_class'],
               'bad_shape': fin['bad_shape'], 'bad_first/column/outside': [fin['bad_first_child'], fin['bad_column_other'], fin['bad_outside_columns']],
               'height_failures': fin['height_failures'], 'reaudit_complete': d['reaudit']['complete'], 'seconds': d['total_seconds'],
               'R2_last': d['R2']['last_witness'][:200]}
json.dump(rows, sys.stdout, indent=1); print()
