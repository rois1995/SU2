"""One isolated MPI6 disk continuation of the preserved actual first adapted M6 state."""
from pathlib import Path
import json
import os
import shutil
import subprocess
import tempfile
import time
from run_m6 import CFD, ldd_paths, sha, save
from run_cases import sample_tree

R = Path(__file__).resolve().parent
A = R / 'm6_runs/euler_build293_bounded_reference_mach_only_real_cfd50k'
checkpoint = R / 'm6_inputs/euler_adapted293_event1_v1'
folder = R / 'm6_runs/euler_build293_adapted_event1_disk_restart_v1'
assert not folder.exists(), 'Verify the existing owned handle; never overwrite/relaunch an attempt.'
assert json.loads((A / 'terminal_handles.json').read_text())['all_owned_handles_terminal']
validation = json.loads((A / 'workflow293_independent_validation.json').read_text())
assert validation['workflow_events_independently_verified'] == 2
pins = json.loads((A / 'pins.json').read_text())
assert all(sha(Path(p)) == h for p, h in pins.items())
assert all(p in pins and sha(Path(p)) == pins[p] for p in ldd_paths(CFD))
event = validation['events'][0]
for name, digest in event['file_hashes'].items():
    assert sha(checkpoint / name) == digest
available = int(next(l.split()[1] for l in Path('/proc/meminfo').read_text().splitlines()
                     if l.startswith('MemAvailable:'))) * 1024
assert available > 16 * 1024**3
text = (checkpoint / 'run.cfg').read_text()
for old, new in [('MESH_FILENAME= m6.su2', 'MESH_FILENAME= accepted293.su2'),
                 ('SOLUTION_FILENAME= baseline_euler_seed', 'SOLUTION_FILENAME= accepted293_input'),
                 ('RESTART_FILENAME= restart_flow', 'RESTART_FILENAME= continued293_restart'),
                 ('CONV_FILENAME= history', 'CONV_FILENAME= continued_history'),
                 ('ADAP_LOOP= YES', 'ADAP_LOOP= NO')]:
    assert text.splitlines().count(old) == 1
    text = text.replace(old + '\n', new + '\n')
assert 'RESTART_SOL= YES\n' in text and 'READ_BINARY_RESTART= YES\n' in text
assert 'ITER= 1\n' in text and 'ADAP_SENSOR= ( MACH )\n' in text
folder.mkdir()
for source, target in [(checkpoint / 'adapted_mesh_adap_00001.su2', folder / 'accepted293.su2'),
                       (checkpoint / 'adapted_mesh_adap_00001.su2.native_ref', folder / 'accepted293.su2.native_ref'),
                       (checkpoint / 'restart_flow_adap_00001.dat', folder / 'accepted293_input.dat')]:
    shutil.copy2(source, target)
    assert sha(source) == sha(target)
(folder / 'run.cfg').write_text(text)
for p in [Path(__file__), folder / 'run.cfg', folder / 'accepted293.su2',
          folder / 'accepted293.su2.native_ref', folder / 'accepted293_input.dat',
          A / 'workflow293_independent_validation.json', checkpoint / 'checkpoint.json']:
    pins[str(p.resolve())] = sha(p)
save(folder / 'pins.json', pins)
scratch_root = R.parents[3] / 'integration_evidence/build-integrated-v2/compile-tmp'
scratch = Path(tempfile.mkdtemp(prefix='adapted-disk293-', dir=scratch_root))
env = {k: v for k, v in os.environ.items() if not k.startswith('SU2_NATIVE_')}
env.update(OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', MKL_NUM_THREADS='1',
           OMPI_MCA_osc='pt2pt', TMPDIR=str(scratch_root), OMPI_MCA_orte_tmpdir_base=str(scratch))
argv = ['nice', '-n', '19', '/usr/bin/time', '-v', 'timeout', '600', 'mpirun',
        '--output-filename', 'rank_output', '-n', '6', str(CFD), 'run.cfg']
own = {'pid': os.getpid(), 'start_ticks': int(Path('/proc/self/stat').read_text().rsplit(')', 1)[1].split()[19])}
save(folder / 'command.json', dict(argv=argv, wrapper=own, saved_checkpoint=str(checkpoint),
     source_attempt=str(A), initial_available_bytes=available, loadavg=Path('/proc/loadavg').read_text(),
     environment={k: v for k, v in env.items() if k.startswith(('OMPI_', 'OMP_', 'OPENBLAS_', 'MKL_')) or k == 'TMPDIR'},
     scope='Actual isolated first adapted mesh and matching continued-CFD disk restart; no adaptation. One further CFD iteration; not convergence qualification.'))
started = time.time()
seen = {(own['pid'], own['start_ticks']): own}
with (folder / 'run.log').open('w') as log, (folder / 'resources.jsonl').open('w') as receipt:
    process = subprocess.Popen(argv, cwd=folder, env=env, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
    save(folder / 'status.json', dict(state='running', pid=process.pid, wrapper=own, started_at=started))
    print(json.dumps(dict(state='running', folder=str(folder), pid=process.pid, wrapper=own)), flush=True)
    while process.poll() is None:
        rows = sample_tree(process.pid)
        for row in rows:
            try:
                s = (Path('/proc') / str(row['pid']) / 'stat').read_text().rsplit(')', 1)[1].split()
                row.update(start_ticks=int(s[19]), user_cpu_ticks=int(s[11]), system_cpu_ticks=int(s[12]))
                seen[(row['pid'], row['start_ticks'])] = dict(pid=row['pid'], start_ticks=row['start_ticks'])
            except (FileNotFoundError, ProcessLookupError):
                pass
        save(folder / 'owned_handles.json', list(seen.values()))
        receipt.write(json.dumps(dict(elapsed_seconds=time.time()-started, processes=rows,
                      sampled_process_tree_rss_bytes=sum(x['rss_bytes'] for x in rows))) + '\n')
        receipt.flush()
        time.sleep(2)
    code = process.wait()
changed = [p for p, h in pins.items() if sha(Path(p)) != h]
save(folder / 'status.json', dict(state='terminal', exit_code=code, seconds=time.time()-started,
     pins_verified=not changed, changed=changed, disk_restart_validated=False))
print(json.dumps(dict(state='terminal', exit_code=code, changed=changed)), flush=True)
assert not changed, changed
raise SystemExit(code)
