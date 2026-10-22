#!/usr/bin/env python3
"""DC-5 summary: per case outcome under admission A and D (identical proposal stream to first divergence)."""
import glob, hashlib, json, os, re
HERE = os.path.dirname(os.path.abspath(__file__))
cases = {}
for f in sorted(x for x in glob.glob(os.path.join(HERE, 'dc5_*.json')) if not x.endswith('dc5_result.json')):
    d = json.load(open(f))
    if 'case' not in d: continue
    c = d['case']; row = {}
    for k in ('A', 'D'):
        r = d[k]
        row[k] = {'outcome': r['outcome'], 'work': r['work'], 'accepted': r['accepted'], 'seconds': r['seconds'], 'final': r['final'],
                  'reaudit': d.get(k + '_reaudit'), 'first_row_built_refused': d.get(k + '_first_row'),
                  'top_reasons': dict(list(r['reasons_top'].items())[:6])}
    r = d['D']
    row['diverged'] = bool(r['diverged'])
    if r['diverged']:
        row['divergence'] = {'work': r['divergence']['work'], 'op': r['divergence']['op'], 'kind': r['divergence']['kind'],
                             'A': r['divergence']['A']['reason'], 'D_ok': r['divergence']['D']['ok']}
    row['proxy_field'] = c.startswith('m6_')
    row['composition_fidelity_rel'] = d.get('composition_fidelity_rel_to_max_entry')
    cases[c] = row
t = open(os.path.join(HERE, 'time_final.log')).read() if os.path.exists(os.path.join(HERE, 'time_final.log')) else ''
g = lambda k: (re.search(k + r': (.*)', t) or [None, None])[1]
out = {'check': 'DC-5 (BL_REMESH_PLAN rev 3): test-only [NativeLayer2DWay3D]; existing operators in production order (balanced split point, '
                'SplitEdge/InsertEdgeCavity, wall reconstructions, private RepairMetricConnectivity), D.3 LayerConstruct (c_v then p_f by private '
                'visibility + Cone, private repair), admission A vs D.1 on one deterministic proposal stream; T = 0.10 test-side in both; '
                'final gates absolute; independent re-audit with a fresh field',
       'limits': 'A1: 8192 work / 120 s (unchanged); fixtures 8192 work / 600 s per admission; <= 4 GiB',
       'receipt': {'elapsed': g(r'Elapsed \(wall clock\) time \(h:mm:ss or m:ss\)'), 'max_rss_kB': g(r'Maximum resident set size \(kbytes\)'), 'exit': g('Exit status')},
       'cases': cases,
       'files_sha256': {os.path.basename(f): hashlib.sha256(open(f, 'rb').read()).hexdigest() for f in sorted(glob.glob(os.path.join(HERE, 'dc5_*'))) + [os.path.join(HERE, 'provenance_final_runs.txt')]}}
json.dump(out, open(os.path.join(HERE, 'dc5_result.json'), 'w'), indent=1)
for c, r in cases.items(): print(c, r['A']['outcome'], r['D']['outcome'], r['A']['final']['long_edges'], r['D']['final']['long_edges'], r['D']['final']['q_min'], r['diverged'])
