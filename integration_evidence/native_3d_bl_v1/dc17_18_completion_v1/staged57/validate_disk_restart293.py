"""Verify actual isolated adapted disk input/output and seal the scoped Euler milestone."""
from pathlib import Path
import csv
import hashlib
import io
import json
import math
import re
import sys
import time
import numpy as np

R = Path(__file__).resolve().parent
A = R / 'm6_runs/euler_build293_bounded_reference_mach_only_real_cfd50k'
D = R / 'm6_runs/euler_build293_adapted_event1_disk_restart_v1'
C = R / 'm6_inputs/euler_adapted293_event1_v1'
sys.path.insert(0, str(R.parents[3] / 'TestCases/adaptation/capability'))
import capcheck

sha = lambda p: hashlib.file_digest(Path(p).open('rb'), 'sha256').hexdigest()
helpers = {str(p): sha(p) for p in (Path(__file__), Path(capcheck.__file__))}
status = json.loads((D / 'status.json').read_text())
assert status['state'] == 'terminal' and status['exit_code'] == 0
assert status['pins_verified'] and not status['changed']
assert json.loads((D / 'terminal_handles.json').read_text())['all_owned_handles_terminal']
runtime = json.loads((D / 'loaded_runtime_startup.json').read_text())
assert runtime['all_six_executing_inodes_and_loaded_elf_pinned'] and len(runtime['ranks']) == 6
pins = json.loads((D / 'pins.json').read_text())
assert all(sha(p) == h for p, h in pins.items())
config = (D / 'run.cfg').read_text()
for line in ('ADAP_LOOP= NO', 'RESTART_SOL= YES', 'READ_BINARY_RESTART= YES',
             'MESH_FILENAME= accepted293.su2', 'SOLUTION_FILENAME= accepted293_input',
             'RESTART_FILENAME= continued293_restart', 'ITER= 1'):
    assert config.splitlines().count(line) == 1
for local, saved in [('accepted293.su2', 'adapted_mesh_adap_00001.su2'),
                     ('accepted293.su2.native_ref', 'adapted_mesh_adap_00001.su2.native_ref'),
                     ('accepted293_input.dat', 'restart_flow_adap_00001.dat')]:
    assert sha(D / local) == sha(C / saved)
log = (D / 'run.log').read_text()
assert 'Read flow solution from: accepted293_input.' in log
assert 'continued293_restart.dat' in log and 'NATIVE3D_DETAIL stage=refine' not in log
flow = re.search(r'\|\s*0\|\s*([-+.0-9eE]+)\|\s*([-+.0-9eE]+)\|\s*([-+.0-9eE]+)\|', log)
assert flow
flow = dict(zip(('rms_rho', 'CL', 'CD'), map(float, flow.groups())))
assert all(math.isfinite(v) for v in flow.values())
rows = [{k.strip(): v.strip() for k, v in row.items()}
        for row in csv.DictReader(io.StringIO((D / 'continued_history.csv').read_text()), skipinitialspace=True)]
assert rows and all(math.isfinite(float(v)) for row in rows for v in row.values())
mesh = capcheck.read_su2(D / 'accepted293.su2')
before, old, _ = capcheck.read_restart(D / 'accepted293_input.dat')
after, fields, precision = capcheck.read_restart(D / 'continued293_restart.dat')
assert len(mesh.P) == 149051 and len(mesh.E) == 736323
assert mesh.P.shape == before.shape == after.shape
coordinate_error = max(float(np.max(np.abs(p - mesh.P))) for p in (before, after))
assert coordinate_error <= 1e-12 and all(np.isfinite(v).all() for v in fields.values())
rho = fields['Density']
internal = fields['Energy'] - sum(fields['Momentum_' + s] ** 2 for s in 'xyz') / (2 * rho)
assert np.all(rho > 0) and np.all(internal > 0)
changes = {k: float(np.max(np.abs(fields[k] - old[k]))) for k in
           ('Density', 'Momentum_x', 'Momentum_y', 'Momentum_z', 'Energy')}
assert max(changes.values()) > 0 and sha(D / 'continued293_restart.dat') != sha(D / 'accepted293_input.dat')
assert all(sha(p) == h for p, h in pins.items())
assert helpers == {p: sha(p) for p in helpers}
receipt = dict(time=time.time(), disk_restart_verified=True, adapted_points=len(mesh.P),
    adapted_cells=len(mesh.E), ranks=6, standalone_qualified_cfd=True, seconds=status['seconds'],
    source_checkpoint=str(C), coordinate_maximum_error=coordinate_error,
    all_output_fields_finite=True, minimum_density=float(rho.min()),
    minimum_internal_energy_density=float(internal.min()), precision=precision,
    continued_cfd=flow, continued_history=rows, conservative_field_changes=changes,
    all_inputs_and_pins_unchanged=True, pin_count=len(pins), validation_helpers=helpers,
    artifact_sha256={name: sha(D / name) for name in
        ('accepted293.su2', 'accepted293.su2.native_ref', 'accepted293_input.dat',
         'continued293_restart.dat', 'run.cfg', 'run.log', 'continued_history.csv',
         'status.json', 'terminal_handles.json', 'loaded_runtime_startup.json')},
    scope='Actual isolated steady CFD disk restart on FIRST accepted adapted mesh and MATCHING following-CFD state. Preserved sidecar bytes unchanged; no adaptation transaction or resume-cycle numbering fix claimed. One continued CFD iteration, not convergence qualification.')
(D / 'disk_restart_validation.json').write_text(json.dumps(receipt, indent=2) + '\n')
workflow = json.loads((A / 'workflow293_independent_validation.json').read_text())
assert workflow['workflow_events_independently_verified'] == 2
assert json.loads((A / 'workflow293_prefix_verification.json').read_text())['all_12_rank_event_prefixes_verified']
resources = [json.loads(line) for line in (A / 'resources.jsonl').read_text().splitlines()]
completion = dict(time=time.time(), two_accepted_euler50k_events_verified=True,
    conservative_admissible_transfers_verified=True, finite_continued_adapted_cfd_verified=True,
    actual_isolated_adapted_disk_restart_verified=True, disk_restart_event=1,
    events=[dict(event, disk_restart_verified=event['event'] == 1) for event in workflow['events']],
    evidence={str(p): sha(p) for p in (A / 'workflow293_independent_validation.json',
              A / 'workflow293_prefix_verification.json', D / 'disk_restart_validation.json')},
    sampled_workflow_tree_peak_rss_bytes=max(x['sampled_process_tree_rss_bytes'] for x in resources),
    memory_scope='Sampled summed process RSS, not physical whole-job peak.',
    performance_cycle_implemented=False, dedicated_bl_mpi_qualified=False, rans500k_complete=False,
    full_goal_complete=False)
(A / 'euler293_completion.json').write_text(json.dumps(completion, indent=2) + '\n')
print(json.dumps(dict(disk_restart_verified=True, continued_cfd=flow,
    coordinate_maximum_error=coordinate_error, conservative_field_changes=changes,
    euler_workflow_and_actual_disk_restart_verified=True,
    sampled_workflow_tree_peak_rss_bytes=completion['sampled_workflow_tree_peak_rss_bytes'],
    performance_bl_mpi_rans_remaining=True)))
