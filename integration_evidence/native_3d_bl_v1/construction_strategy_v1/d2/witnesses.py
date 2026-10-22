#!/usr/bin/env python3
"""D2: collect the 971 measured non-integrable M6 band edges (NaN native TargetEdgeLength) from the DC-2 rev-2 runs
(dc2/m6/native add_0..2 = first 300k added edges, dc2/m6_sel/native add_0..1 at add_index) and write them as int64
(source add-edge index, point a, point b) for the native D2 replay. Cross-checks rev3/diag/nonintegrable_samples.npz."""
import hashlib, json, os
import numpy as np
HERE = os.path.dirname(os.path.abspath(__file__)); BASE = os.path.normpath(os.path.join(HERE, '..'))
M, S = os.path.join(BASE, 'dc2', 'm6'), os.path.join(BASE, 'dc2', 'm6_sel')
sha = lambda p: hashlib.sha256(open(p, 'rb').read()).hexdigest()
add = np.fromfile(os.path.join(M, 'input', 'add.i64'), np.int64).reshape(-1, 2)
Ln = np.full(len(add), np.nan); meas = np.zeros(len(add), bool)
first = [os.path.join(M, 'native', f'add_{k}.f64') for k in range(3)]
Ln[:300000] = np.concatenate([np.fromfile(f) for f in first]); meas[:300000] = True
idx = np.fromfile(os.path.join(S, 'add_index.i64'), np.int64)
sel = [os.path.join(S, 'native', f'add_{k}.f64') for k in range(2)]
Ln[idx] = np.concatenate([np.fromfile(f) for f in sel]); meas[idx] = True
bad = np.where(meas & ~np.isfinite(Ln))[0]
out = np.column_stack([bad, add[bad, 0], add[bad, 1]]).astype(np.int64)
out.tofile(os.path.join(HERE, 'witnesses.i64'))
npz = np.load(os.path.join(BASE, 'rev3', 'diag', 'nonintegrable_samples.npz'))
same = bool(np.array_equal(npz['edges'], add[bad]))
info = {'witnesses': int(len(bad)), 'measured': int(meas.sum()), 'equal_to_rev3_npz_edges': same,
        'inputs': {os.path.relpath(p, BASE): sha(p) for p in first + sel + [os.path.join(M, 'input', 'add.i64'),
                   os.path.join(S, 'add_index.i64'), os.path.join(BASE, 'rev3', 'diag', 'nonintegrable_samples.npz')]},
        'output': {'witnesses.i64': sha(os.path.join(HERE, 'witnesses.i64'))}, 'script': sha(__file__)}
json.dump(info, open(os.path.join(HERE, 'witnesses.json'), 'w'), indent=1); print(json.dumps(info, indent=1))
