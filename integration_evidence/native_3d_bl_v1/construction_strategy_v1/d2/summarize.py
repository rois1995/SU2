#!/usr/bin/env python3
"""D2 summary: native replay classes + candidate fix (tie-canonical ownership, test-only copy) on witnesses/controls."""
import hashlib, json, os, subprocess
import numpy as np
H = os.path.dirname(os.path.abspath(__file__)); sha = lambda p: hashlib.sha256(open(os.path.join(H, p), 'rb').read()).hexdigest()
L = lambda f: {r['index']: r for r in map(json.loads, open(os.path.join(H, f)))}
nat, w0, w1, c0, c1, pw, pc, c3 = map(L, ['diagnose.jsonl', 'copy_tie0_witnesses.jsonl', 'copy_tie1_witnesses.jsonl',
                              'copy_tie0_controls.jsonl', 'copy_tie1_controls.jsonl', 'pin_witnesses.jsonl', 'pin_controls.jsonl',
                              'run3_copy_tie0_controls.jsonl'])
cls = json.loads(subprocess.run(['python3', os.path.join(H, 'classify.py'), os.path.join(H, 'diagnose.jsonl')],
                                capture_output=True, text=True, check=True).stdout)
q = lambda v: np.percentile(v, [0, 50, 90, 99, 100]).tolist() if len(v) else None
# Copy faithfulness: tie 0 copy reproduces the native replica outcome (failure, total, error) on every witness.
same = sum(1 for k, r in nat.items() if (w0[k]['replica_ok'], w0[k]['replica_failure'], w0[k]['replica_total'], w0[k]['replica_error'])
           == (r['replica_ok'], r['replica_failure'], r['replica_total'], r['replica_error']))
ctl = np.fromfile(os.path.join(H, 'controls.i64'), np.int64).reshape(-1, 3)[:, 0]
Lnat = dict(zip(ctl.tolist(), np.fromfile(os.path.join(H, 'controls_native_length.f64')).tolist()))
c0eq = sum(1 for k in ctl.tolist() if c0[k]['replica_ok'] and c0[k]['replica_total'] == Lnat[k])
fixed = [w1[k] for k in nat]; ok1 = [r for r in fixed if r['replica_ok']]
# Branch ambiguity: tie-1 length vs the native (non-converged) estimate on witnesses; tie-1 vs native on controls.
dw = [abs(w1[k]['replica_total'] / nat[k]['replica_total'] - 1) for k in nat if w1[k]['replica_ok']]
dc = [abs(c1[k]['replica_total'] / Lnat[k] - 1) for k in ctl.tolist() if c1[k]['replica_ok']]
out = {'native_replay': cls,
       'copy_tie0_reproduces_native_replica_on_witnesses': same, 'witnesses': len(nat),
       'copy_tie0_controls_converged': sum(r['replica_ok'] for r in c0.values()), 'copy_tie0_controls_equal_native_length': c0eq,
       'tie1_witnesses_converged': len(ok1), 'tie1_witness_evaluations_p0_p50_p90_p99_max': q([r['replica_evaluations'] for r in ok1]),
       'tie1_witness_rel_change_vs_native_unconverged_estimate_p0_p50_p90_p99_max': q(dw),
       'tie1_controls_converged': sum(r['replica_ok'] for r in c1.values()), 'controls': len(c1),
       'tie0_control_evaluations_p50_p99_max': np.percentile([r['replica_evaluations'] for r in c0.values()], [50, 99, 100]).tolist(),
       'tie1_control_evaluations_p50_p99_max': np.percentile([r['replica_evaluations'] for r in c1.values()], [50, 99, 100]).tolist(),
       'tie1_controls_bitwise_equal_native': sum(1 for x in dc if x == 0),
       'tie1_controls_rel_change_vs_native_p0_p50_p90_p99_max': q(dc),
       'run3_copy_tie0_controls_equal_native_length': sum(1 for k in ctl.tolist() if c3[k]['replica_ok'] and c3[k]['replica_total'] == Lnat[k]),
       'pin_witnesses_converged': sum(r['replica_ok'] for r in pw.values()), 'pin_witness_cut_replica_equal': sum(r['cut_replica_equal'] for r in pw.values()),
       'pin_witness_evaluations_p0_p50_p90_p99_max': q([r['replica_evaluations'] for r in pw.values() if r['replica_ok']]),
       'pin_witness_rel_diff_vs_tie1_p0_p50_p90_p99_max': q([abs(pw[k]['replica_total'] / w1[k]['replica_total'] - 1) for k in pw if pw[k]['replica_ok'] and w1[k]['replica_ok']]),
       'pin_controls_converged': sum(r['replica_ok'] for r in pc.values()),
       'pin_control_evaluations_p50_p99_max': np.percentile([r['replica_evaluations'] for r in pc.values()], [50, 99, 100]).tolist(),
       'pin_controls_bitwise_equal_native': sum(1 for k in ctl.tolist() if pc[k]['replica_total'] == Lnat[k]),
       'pin_controls_rel_change_vs_native_p0_p50_p90_p99_max': q([abs(pc[k]['replica_total'] / Lnat[k] - 1) for k in ctl.tolist() if pc[k]['replica_ok']]),
       'budget': 65536, 'tolerance': '1e-9 relative + 1e-12 absolute (unchanged)',
       'files': {f: sha(f) for f in ['witnesses.i64', 'controls.i64', 'controls_native_length.f64', 'diagnose.jsonl',
                 'copy_tie0_witnesses.jsonl', 'copy_tie1_witnesses.jsonl', 'copy_tie0_controls.jsonl', 'copy_tie1_controls.jsonl',
                 'classify.py', 'make_patched_copy.py', 'native_d2_replay_run1.inc', 'native_d2_replay_run2.inc',
                 'provenance_run1.txt', 'provenance_run2.txt', 'test_cpp_run2.diff',
                 'pin_witnesses.jsonl', 'pin_controls.jsonl', 'run3_copy_tie0_controls.jsonl', 'native_d2_replay_run3.inc',
                 'provenance_run3.txt', 'test_cpp_run3.diff', 'make_patched_copy.json', 'witnesses.json', 'controls.json', 'summary_tie_only.json']}, 'script': sha('summarize.py')}
json.dump(out, open(os.path.join(H, 'summary.json'), 'w'), indent=1); print(json.dumps({k: v for k, v in out.items() if k != 'files'}, indent=1))
