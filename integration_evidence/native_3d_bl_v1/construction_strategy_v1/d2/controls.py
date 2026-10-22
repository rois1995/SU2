#!/usr/bin/env python3
"""D2: 2,000 random measured FINITE M6 band edges (seed 1) as a control set for candidate fixes (same format as
witnesses.i64: source add-edge index, point a, point b); their native lengths are stored for comparison."""
import hashlib, json, os
import numpy as np
HERE = os.path.dirname(os.path.abspath(__file__)); BASE = os.path.normpath(os.path.join(HERE, '..'))
M, S = os.path.join(BASE, 'dc2', 'm6'), os.path.join(BASE, 'dc2', 'm6_sel')
sha = lambda p: hashlib.sha256(open(p, 'rb').read()).hexdigest()
add = np.fromfile(os.path.join(M, 'input', 'add.i64'), np.int64).reshape(-1, 2)
Ln = np.full(len(add), np.nan); meas = np.zeros(len(add), bool)
Ln[:300000] = np.concatenate([np.fromfile(os.path.join(M, 'native', f'add_{k}.f64')) for k in range(3)]); meas[:300000] = True
idx = np.fromfile(os.path.join(S, 'add_index.i64'), np.int64)
Ln[idx] = np.concatenate([np.fromfile(os.path.join(S, 'native', f'add_{k}.f64')) for k in range(2)]); meas[idx] = True
rng = np.random.default_rng(1); ctl = np.sort(rng.choice(np.where(meas & np.isfinite(Ln))[0], 2000, replace=False))
np.column_stack([ctl, add[ctl, 0], add[ctl, 1]]).astype(np.int64).tofile(os.path.join(HERE, 'controls.i64'))
Ln[ctl].tofile(os.path.join(HERE, 'controls_native_length.f64'))
info = {'controls': 2000, 'seed': 1, 'script': sha(__file__), 'controls.i64': sha(os.path.join(HERE, 'controls.i64')),
        'controls_native_length.f64': sha(os.path.join(HERE, 'controls_native_length.f64'))}
json.dump(info, open(os.path.join(HERE, 'controls.json'), 'w'), indent=1); print(info)
