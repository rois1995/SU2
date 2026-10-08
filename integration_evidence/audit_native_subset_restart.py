"""Compare current-source accepted-mesh BDF2 restarts with their uninterrupted parent."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import numpy as np
from audit_native_unsteady import read_mesh, state


def audit(case):
    origin = json.loads((case / 'restart_origin.json').read_text())
    parent = Path(origin['parent'])
    first = origin['restart_step']
    cfg = (case / 'run.cfg').read_text()
    def setting(key):
        return re.search(r'^' + key + r'\s*=\s*([^%\n]+)', cfg, re.M)[1].strip()
    assert setting('RESTART_SOL') == 'YES' and int(setting('RESTART_ITER')) == first
    assert '2ND_ORDER' in setting('TIME_MARCHING')
    mesh = read_mesh(case / setting('MESH_FILENAME'))
    original = read_mesh(parent / origin['source_mesh'])
    assert np.array_equal(mesh.P, original.P) and np.array_equal(mesh.E, original.E)
    assert (case / (setting('MESH_FILENAME') + '.native_ref')).is_file()
    assert 'Exit Success' in (case / 'solver.log').read_text()
    assert 'Native adaptation:' not in (case / 'solver.log').read_text(), 'This comparison assumes the accepted mesh stays fixed'
    rows = []
    for step in range(first - 2, int(setting('TIME_ITER'))):
        name = f'solution_{step:05d}.dat'
        expected, _ = state(original, parent / name)
        observed, positivity = state(mesh, case / name)
        scale = np.maximum(1., np.max(np.abs(expected), axis=0))
        defect = float(np.max(np.abs(observed - expected) / scale))
        assert defect <= 1e-12, (step, defect)
        rows.append(dict(step=step, scaled_max_state_defect=defect, **positivity))
    return dict(status='PASS', scope='Accepted-mesh BDF2 window-boundary restart, same mesh and current/previous histories; compares conserved states at all resumed steps and positivity. Does not exercise subsequent remeshing or mid-window restart.',
                origin=origin, states=rows, max_scaled_state_defect=max(row['scaled_max_state_defect'] for row in rows),
                checker_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('case', type=Path)
    args = p.parse_args()
    output = args.case / 'independent_restart_audit.json'
    assert not output.exists(), 'Preserve existing evidence'
    result = audit(args.case)
    output.write_text(json.dumps(result, indent=2) + '\n')
    print(args.case, result['status'], result['max_scaled_state_defect'])
