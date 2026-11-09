"""Independently seal both actual accepted meshes and matching continued-CFD checkpoints."""
from pathlib import Path
import csv
import hashlib
import io
import json
import math
import os
import re
import shutil
import sys
import tempfile
import time
import numpy as np

R = Path(__file__).resolve().parent
A = R / 'm6_runs/euler_build293_bounded_reference_mach_only_real_cfd50k'
sys.path.insert(0, str(R.parents[3] / 'TestCases/adaptation/capability'))
import capcheck

sha = lambda p: hashlib.file_digest(Path(p).open('rb'), 'sha256').hexdigest()
helpers = {str(p): sha(p) for p in (Path(__file__), Path(capcheck.__file__))}
status = json.loads((A / 'status.json').read_text())
assert status['state'] == 'terminal' and status['solver_exit_code'] == 0
assert status['workflow_accepted'] and status['pins_verified'] and not status['changed']
assert json.loads((A / 'terminal_handles.json').read_text())['all_owned_handles_terminal']
pins = json.loads((A / 'pins.json').read_text())
assert all(sha(p) == h for p, h in pins.items())
log = (A / 'run.log').read_text()
history = [{k.strip(): v.strip() for k, v in row.items()}
           for row in csv.DictReader(io.StringIO((A / 'history.csv').read_text()), skipinitialspace=True)]
native = [json.loads((A / name).read_text())['events'][-1] for name in
          ('construction_progress_assessment.json', 'second_native_progress_assessment.json')]
results = []
for event, (points_expected, cells_expected) in enumerate(((149051, 736323), (172001, 870422)), 1):
    n = native[event - 1]
    assert n['stage'] == 'exit' and all(n['complete_flags'].values())
    assert n['totals']['cells'] == n['totals']['q_measured'] == n['totals']['j_measured'] == cells_expected
    assert all(n['totals'][k] == 0 for k in ('q_bad', 'j_bad', 'geometry_bad', 'size_bad'))
    section = log.split(f'Cycle {event - 1} of 2:', 1)[1].split(f'Cycle {event} of 2:', 1)[0]
    details = [dict(x.split('=', 1) for x in line.split()[1:] if '=' in x)
               for line in section.splitlines() if line.startswith('NATIVE3D_DETAIL stage=refine ')]
    assert len(details) == 6 and {int(x['rank']) for x in details} == set(range(6))
    assert all(x['stop'] == 'complete' and x['complete'] == '1' for x in details)
    table = section.split('Per field: S_n content and S_d content', 1)[1].split('States not admissible after the projection', 1)[0]
    residuals = {}
    for line in table.splitlines():
        if ':' not in line:
            continue
        name, values = line.split(':', 1)
        if name.strip() in ('Density', 'Momentum-x', 'Momentum-y', 'Momentum-z', 'Energy'):
            values = values.split()
            assert len(values) == 7
            value = float(values[-2])
            assert math.isfinite(value) and value <= 1e-10
            residuals[name.strip()] = value
    assert len(residuals) == 5
    assert re.search(r'States not admissible after the projection[^\n]*: 0;', section)
    assert 'Mesh replaced in ' in section
    following = log.split(f'Cycle {event} of 2:', 1)[1]
    flow = re.search(r'\|\s*0\|\s*([-+.0-9eE]+)\|\s*([-+.0-9eE]+)\|\s*([-+.0-9eE]+)\|', following)
    assert flow
    flow = dict(zip(('rms_rho', 'CL', 'CD'), map(float, flow.groups())))
    assert all(math.isfinite(v) for v in flow.values())
    rows = [row for row in history if int(row['Adap_Cycle']) == event]
    assert rows and all(math.isfinite(float(v)) for row in rows for v in row.values())
    names = [f'adapted_mesh_adap_{event:05d}.su2', f'adapted_mesh_adap_{event:05d}.su2.native_ref',
             f'restart_flow_adap_{event:05d}.dat']
    file_hashes = {name: sha(A / name) for name in names}
    mesh = capcheck.read_su2(A / names[0])
    positions, fields, precision = capcheck.read_restart(A / names[2])
    assert mesh.dim == 3 and len(mesh.P) == points_expected and len(mesh.E) == cells_expected
    assert np.isfinite(mesh.P).all() and mesh.E.min() >= 0 and mesh.E.max() < len(mesh.P)
    assert positions.shape == mesh.P.shape
    coordinate_error = float(np.max(np.abs(positions - mesh.P)))
    assert coordinate_error <= 1e-12 and all(np.isfinite(v).all() for v in fields.values())
    rho = fields['Density']
    internal = fields['Energy'] - sum(fields['Momentum_' + s] ** 2 for s in 'xyz') / (2 * rho)
    assert np.all(rho > 0) and np.all(internal > 0)
    assert file_hashes == {name: sha(A / name) for name in names}
    destination = R / f'm6_inputs/euler_adapted293_event{event}_v1'
    result = dict(event=event, native_accepted=True, points=points_expected, cells=cells_expected,
                  native_seconds=n['maximum_native_elapsed'], true_mass_residuals=residuals,
                  projected_inadmissible_states=0, continued_adapted_cfd=flow,
                  continued_history=rows, coordinate_maximum_error=coordinate_error,
                  all_restart_fields_finite=True, minimum_density=float(rho.min()),
                  minimum_internal_energy_density=float(internal.min()), precision=precision,
                  file_hashes=file_hashes, preserved_checkpoint=str(destination), disk_restart_verified=False)
    if destination.exists():
        assert event == 1
        assert all(sha(destination / name) == h for name, h in file_hashes.items())
    else:
        tmp = Path(tempfile.mkdtemp(prefix=f'.adapted293-event{event}-', dir=destination.parent))
        for name in names + ['run.cfg', 'pins.json', 'seed_input.json', f'workflow_event_{event:05d}.json']:
            shutil.copy2(A / name, tmp / name)
        assert all(sha(tmp / name) == h for name, h in file_hashes.items())
        (tmp / 'continued_history.csv').write_text((A / 'history.csv').read_text())
        (tmp / 'continued_cfd.log').write_text(log)
        (tmp / 'checkpoint.json').write_text(json.dumps(dict(result, validation_helpers=helpers,
            scope='Actual accepted mesh/reference with MATCHING following-cycle continued-CFD restart; isolated disk restart pending.'), indent=2) + '\n')
        os.rename(tmp, destination)
    results.append(result)
assert helpers == {p: sha(p) for p in helpers}
assert all(sha(p) == h for p, h in pins.items())
receipt = dict(time=time.time(), workflow_events_independently_verified=2, events=results,
    all_pins_verified=True, pin_count=len(pins), validation_helpers=helpers,
    target_integral_gate='Successful pinned transfer enforces scaled TARGET defect<=1e-12 before publication; numerical final target defects not printed.',
    metric_scope='Restart tensors are following-CFD targets, not exported construction-transport metrics.',
    disk_restart_verified=False, bl_mpi_qualified=False, rans500k_complete=False)
(A / 'workflow293_independent_validation.json').write_text(json.dumps(receipt, indent=2) + '\n')
print(json.dumps(receipt))
