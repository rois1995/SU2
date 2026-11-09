"""Isolated bounded MPI6 RANS/SA continuation; inputs only until explicitly launched."""
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
A = R / 'cases/mpi6_20261109T173409_3860527'
checkpoint = R / 'm6_inputs/rans_seed_continuation303_v1'
folder = R / 'm6_runs/rans_seed_continuation303_v1'
assert not folder.exists(), 'Verify existing owned handle; never overwrite/relaunch.'
assert json.loads((A / 'terminal_handles.json').read_text())['all_owned_handles_terminal']
assert json.loads((A / 'status.json').read_text())['state'] == 'terminal'
build_status = R / 'build303_status.json'
build_pins = R / 'build303_source_pins.json'
status = json.loads(build_status.read_text())
assert status['state'] == 'terminal' and status['exit_code'] == 0 and status['pins_verified']
assert json.loads((R / 'build303_terminal_handles.json').read_text())['all_owned_handles_terminal']
assert sha(CFD) == status['binaries'][str(CFD)] == '79c4d51b587f6d29ebb97815033280b4896a7568b850869c906431cb6eaa483a'
pins = json.loads(build_pins.read_text())
runtime_manifest = R / 'runtime_20261106_v2/loaded_runtime_pins.json'
runtime = json.loads(runtime_manifest.read_text())
assert not set(ldd_paths(CFD)) - set(runtime)
pins.update(runtime)
prepared = json.loads((checkpoint / 'provenance.json').read_text())
assert prepared['status'] == 'INPUTS_PREPARED_UNQUALIFIED_NOT_RUN'
for name, digest in prepared['file_sha256'].items():
    assert sha(checkpoint / name) == digest
    pins[str((checkpoint / name).resolve())] = digest
for path in [CFD, build_status, build_pins, runtime_manifest, checkpoint / 'provenance.json',
             A / 'terminal_handles.json', A / 'status.json', R / 'run_disk_restart293.py',
             R / 'run_m6.py', R / 'run_cases.py', R.parent / 'diagnostic56/run.py',
             *[Path(shutil.which(name)).resolve() for name in ('nice', 'timeout', 'mpirun')], Path('/usr/bin/time')]:
    pins[str(path.resolve())] = sha(path)
assert all(sha(Path(p)) == h for p, h in pins.items())
available = int(next(l.split()[1] for l in Path('/proc/meminfo').read_text().splitlines()
                     if l.startswith('MemAvailable:'))) * 1024
assert available > 16 * 1024**3
text = (checkpoint / 'run.cfg').read_text()
for directive in ('RESTART_SOL= YES', 'READ_BINARY_RESTART= YES', 'ITER= 80',
                  'SOLVER= RANS', 'KIND_TURB_MODEL= SA', 'COMPUTE_METRIC= NO', 'ADAP_LOOP= NO',
                  'SOLUTION_FILENAME= baseline_sa_seed', 'RESTART_FILENAME= restart_flow'):
    assert text.splitlines().count(directive) == 1, directive
folder.mkdir()
for name in ('Coarse.su2', 'baseline_sa_seed.dat', 'run.cfg'):
    shutil.copy2(checkpoint / name, folder / name)
    assert sha(checkpoint / name) == sha(folder / name)
for p in [Path(__file__), folder / 'run.cfg', folder / 'Coarse.su2', folder / 'baseline_sa_seed.dat']:
    pins[str(p.resolve())] = sha(p)
save(folder / 'pins.json', pins)
scratch_root = R.parents[3] / 'integration_evidence/build-integrated-v2/compile-tmp'
scratch = Path(tempfile.mkdtemp(prefix='rans-continuation303-', dir=scratch_root))
env = {k: v for k, v in os.environ.items() if not k.startswith('SU2_NATIVE_')}
env.update(OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', MKL_NUM_THREADS='1',
           OMPI_MCA_osc='pt2pt', TMPDIR=str(scratch_root), OMPI_MCA_orte_tmpdir_base=str(scratch))
argv = ['nice', '-n', '19', '/usr/bin/time', '-v', 'timeout', '--signal=TERM', '--kill-after=20s', '1800', 'mpirun',
        '--output-filename', 'rank_output', '-n', '6', str(CFD), 'run.cfg']
own = {'pid': os.getpid(), 'start_ticks': int(Path('/proc/self/stat').read_text().rsplit(')', 1)[1].split()[19])}
save(folder / 'command.json', dict(argv=argv, wrapper=own, saved_checkpoint=str(checkpoint),
     source_attempt=str(A), initial_available_bytes=available, loadavg=Path('/proc/loadavg').read_text(),
     environment={k: v for k, v in env.items() if k.startswith(('OMPI_', 'OMP_', 'OPENBLAS_', 'MKL_')) or k == 'TMPDIR'},
     scope='80 further steady RANS/SA iterations on exact saved original mesh/state. No adaptation or reasonable-flow qualification claim; inspect fields, forces and solver corrections separately.'))
started = time.time()
seen = {(own['pid'], own['start_ticks']): own}
mapped = {}
unknown_runtime = set()
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
                if (Path('/proc') / str(row['pid']) / 'exe').resolve() == CFD.resolve():
                    for line in (Path('/proc') / str(row['pid']) / 'maps').read_text().splitlines():
                        parts = line.split(maxsplit=5)
                        if len(parts) != 6 or not parts[5].startswith('/'):
                            continue
                        path = Path(parts[5]).resolve()
                        if str(path) in mapped or not path.is_file():
                            continue
                        with path.open('rb') as stream:
                            if stream.read(4) != b'\x7fELF':
                                continue
                        digest = sha(path)
                        mapped[str(path)] = digest
                        if str(path) not in pins or pins[str(path)] != digest:
                            unknown_runtime.add(str(path))
                    save(folder / 'mapped_runtime_pins.json', mapped)

            except (FileNotFoundError, ProcessLookupError):
                pass
        save(folder / 'owned_handles.json', list(seen.values()))
        receipt.write(json.dumps(dict(elapsed_seconds=time.time()-started, processes=rows,
                      sampled_process_tree_rss_bytes=sum(x['rss_bytes'] for x in rows))) + '\n')
        receipt.flush()
        time.sleep(2)
    code = process.wait()
live = []
for identity in seen.values():
    if identity == own:
        continue
    try:
        current = (Path('/proc') / str(identity['pid']) / 'stat').read_text().rsplit(')', 1)[1].split()
        if int(current[19]) == identity['start_ticks']:
            live.append(identity)
    except (FileNotFoundError, ProcessLookupError):
        pass
save(folder / 'terminal_handles.json', dict(all_owned_handles_terminal=not live,
     live_owned_handles=live, wrapper_excluded=own, scope='All sampled descendant PID/start identities; wrapper still finishing receipt.'))
changed = [p for p, h in pins.items() if sha(Path(p)) != h]
save(folder / 'status.json', dict(state='terminal', exit_code=code, seconds=time.time()-started,
     pins_verified=not changed, changed=changed, mapped_runtime_verified=bool(mapped) and not unknown_runtime,
     unknown_or_changed_mapped_elf=sorted(unknown_runtime), disk_restart_validated=False, reasonable_flow_qualified=False))
print(json.dumps(dict(state='terminal', exit_code=code, changed=changed)), flush=True)
assert not changed, changed
raise SystemExit(code)
