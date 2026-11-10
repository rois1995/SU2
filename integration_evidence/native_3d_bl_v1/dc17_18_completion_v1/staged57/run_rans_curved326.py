"""Actual saved-state RANS/SA curved BL two-event attempt; no qualification inferred."""
from pathlib import Path
import json
import os
import shutil
import subprocess
import tempfile
import time
from run_m6 import CFD as PRODUCTION_CFD, ldd_paths, sha, save
from run_cases import BINARY
CFD = BINARY
from run_cases import sample_tree

R = Path(__file__).resolve().parent
A = R / 'cases/mpi6_20261109T173409_3860527'
checkpoint = R / 'm6_bl_inputs/rans_curved303_v1'
folder = R / 'm6_runs/rans_curved326_v1'
assert not folder.exists(), 'Verify existing owned handle; never overwrite/relaunch.'
assert json.loads((A / 'terminal_handles.json').read_text())['all_owned_handles_terminal']
assert json.loads((A / 'status.json').read_text())['state'] == 'terminal'
build_status = R / 'build326_status.json'
build_pins = R / 'build326_source_pins.json'
status = json.loads(build_status.read_text())
assert status['state'] == 'terminal' and status['exit_code'] == 0 and status['pins_verified']
assert json.loads((R / 'build326_terminal_handles.json').read_text())['all_owned_handles_terminal']
assert sha(CFD) == status['binaries'][str(CFD)]
assert sha(PRODUCTION_CFD) == status['binaries'][str(PRODUCTION_CFD)]
pins = json.loads(build_pins.read_text())
runtime_manifest = R / 'runtime_20261106_v2/loaded_runtime_pins.json'
runtime = json.loads(runtime_manifest.read_text())
assert not (set(ldd_paths(CFD)) | set(ldd_paths(PRODUCTION_CFD))) - set(runtime)
pins.update(runtime)
qualification_path=R/'qualification326.json'
qualification=json.loads(qualification_path.read_text())
assert qualification['build']=='build326' and qualification['state']=='terminal'
assert qualification['passed'] is True
pins[str(qualification_path.resolve())]=sha(qualification_path)
for stage in ('focused',):
    entry=qualification[stage]
    assert entry['passed'] is True,stage
    attempt=Path(entry['attempt']).resolve()
    outcome_path=attempt/'status.json'
    outcome=json.loads(outcome_path.read_text())
    assert outcome['state']=='terminal' and outcome['pins_verified'] and outcome['exit_code']==0,stage
    terminal_path=attempt/'terminal_handles_actual.json'
    assert json.loads(terminal_path.read_text())['all_owned_handles_terminal'],stage
    for file in (outcome_path,terminal_path,attempt/'pins.json',attempt/'command.json'):
        pins[str(file)]=sha(file)
    for path,digest in json.loads((attempt/'pins.json').read_text()).items():
        assert sha(Path(path))==digest,path
        pins[path]=digest
reused=R/'qualification325.json'
assert sha(reused)==qualification['reused_qualification325_sha256']
assert json.loads(reused.read_text())['passed'] is True
pins[str(reused.resolve())]=sha(reused)
previous325=R/'m6_runs/rans_curved325_v1'
assert json.loads((previous325/'terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
assert json.loads((previous325/'status.json').read_text())['state']=='terminal'
for name in ('status.json','terminal_handles_actual.json','assessment.json'):
    path=previous325/name;pins[str(path.resolve())]=sha(path)
previous319=R/'m6_runs/rans_curved319_v1'
for name in ('status.json','terminal_handles_actual.json','assessment.json'):
    path=previous319/name
    pins[str(path.resolve())]=sha(path)
assert json.loads((previous319/'terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
assert json.loads((previous319/'status.json').read_text())['state']=='terminal'
prepared = json.loads((checkpoint / 'provenance.json').read_text())
assert prepared['status'] == 'INPUTS_PREPARED_UNQUALIFIED_NOT_RUN'
for name, digest in prepared['file_sha256'].items():
    assert sha(checkpoint / name) == digest
    pins[str((checkpoint / name).resolve())] = digest
for path in [CFD, PRODUCTION_CFD, build_status, build_pins, runtime_manifest, checkpoint / 'provenance.json',
             A / 'terminal_handles.json', A / 'status.json', R / 'run_disk_restart293.py',
             R / 'run_m6.py', R / 'run_cases.py', R.parent / 'diagnostic56/run.py',
             *[Path(shutil.which(name)).resolve() for name in ('nice', 'timeout', 'mpirun')], Path('/usr/bin/time')]:
    pins[str(path.resolve())] = sha(path)
assert all(sha(Path(p)) == h for p, h in pins.items())
available = int(next(l.split()[1] for l in Path('/proc/meminfo').read_text().splitlines()
                     if l.startswith('MemAvailable:'))) * 1024
assert available > 16 * 1024**3
text = (checkpoint / 'run.cfg').read_text()
for directive in ('RESTART_SOL= YES', 'READ_BINARY_RESTART= YES', 'ITER= 1',
                  'SOLVER= RANS', 'KIND_TURB_MODEL= SA', 'COMPUTE_METRIC= YES', 'ADAP_LOOP= YES',
                  'SOLUTION_FILENAME= baseline_sa_seed', 'ADAP_SENSOR= ( MACH )', 'ADAP_NORM= 1',
                  'ADAP_SUBITER= ( 2 )', 'ADAP_FLOW_ITER= ( 1 )', 'ADAP_SIZES= ( 500000 )',
                  'ADAP_NATIVE_MAX_TIME= 5400', 'ADAP_NATIVE_MEMORY_LIMIT= 12288',
                  'ADAP_NATIVE_RANKS= 1', 'ADAP_BL_METHOD= FIRST_ROW', 'ADAP_COMPLEXITY= 500000',
                  'ADAP_HAUSD= 1E-4', 'ADAP_BL_FIRST_HEIGHT= ( 2E-6 )',
                  'ADAP_BL_GROWTH= ( 1.2 )', 'ADAP_BL_THICKNESS= ( 0.01 )'):
    assert text.splitlines().count(directive) == 1, directive
prerequisites = [R/'cases'/name for name in (
    'mpi6_20261109T173409_3860527', 'mpi6_resume303_20261109T174937_3895940',
    'archived296_failures303_mpi6_20261109T174641_3889081',
    'archived296_distributed303_mpi6_20261109T175217_3902095')]
prerequisites.extend([R/'m6_runs/rans_seed_continuation303_v1', R/'m6_runs/rans_curved303_v1', R/'m6_runs/rans_curved314_v1'])
for previous in prerequisites:
    terminal = previous/'terminal_handles.json'
    actual = previous/'terminal_handles_actual.json'
    if actual.exists(): terminal = actual
    receipt = json.loads(terminal.read_text())
    assert receipt['all_owned_handles_terminal'], previous
    previous_status = previous/'status.json'
    observed = json.loads(previous_status.read_text())
    assert observed['state']=='terminal' and observed['pins_verified'], previous
    pins[str(terminal.resolve())]=sha(terminal)
    pins[str(previous_status.resolve())]=sha(previous_status)
assert sha(checkpoint/'baseline_sa_seed.dat')=='c90000ea804c7e08a54dbec93f218b5bd3ae3dce3db8c287db6783fd138f1c78'
assert sha(checkpoint/'m6.su2.native_ref')=='2e99fb7870e2c718f110da8509e4c0f7e746ce138e7f27ab57aa0dedbb6c42de'

decision_path=R/'rans_seed303_use_decision.json'
decision=json.loads(decision_path.read_text())
assert decision['qualified_for_initial_adaptation'] is True
pins[str(decision_path.resolve())]=sha(decision_path)
folder.mkdir()
for name in ('m6.su2', 'm6.su2.native_ref', 'baseline_sa_seed.dat', 'run.cfg'):
    shutil.copy2(checkpoint / name, folder / name)
    assert sha(checkpoint / name) == sha(folder / name)
for p in [Path(__file__), folder / 'run.cfg', folder / 'm6.su2', folder / 'm6.su2.native_ref', folder / 'baseline_sa_seed.dat']:
    pins[str(p.resolve())] = sha(p)
save(folder / 'pins.json', pins)
scratch_root = R.parents[3] / 'integration_evidence/build-integrated-v2/compile-tmp'
scratch = Path(tempfile.mkdtemp(prefix='rans-curved326-', dir=scratch_root))
env = {k: v for k, v in os.environ.items() if not k.startswith('SU2_NATIVE_')}
env.update(OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', MKL_NUM_THREADS='1',
           OMPI_MCA_osc='pt2pt', TMPDIR=str(scratch_root), OMPI_MCA_orte_tmpdir_base=str(scratch))
env['SU2_NATIVE_FROZEN_CONFIG']=str((folder/'run.cfg').resolve())
argv = ['nice', '-n', '19', '/usr/bin/time', '-v', 'timeout', '--signal=TERM', '--kill-after=20s', '14400', 'mpirun',
        '--output-filename', 'rank_output', '-n', '6', str(CFD), '[NativeSelectedFailureWorkflow3D]', '--durations', 'yes']
own = {'pid': os.getpid(), 'start_ticks': int(Path('/proc/self/stat').read_text().rsplit(')', 1)[1].split()[19])}
save(folder / 'command.json', dict(argv=argv, wrapper=own, saved_checkpoint=str(checkpoint),
     source_attempt=str(R/'m6_runs/rans_curved314_v1'), initial_available_bytes=available, loadavg=Path('/proc/loadavg').read_text(),
     environment={k: v for k, v in env.items() if k.startswith(('OMPI_', 'OMP_', 'OPENBLAS_', 'MKL_', 'SU2_NATIVE_')) or k == 'TMPDIR'},
     expected_events=2, outer_seconds_total=14400, qualification='325 conservative discovery behavior qualified;326 query-local work attribution focused passed; fullBL target unproven', scope='Same303 seed/config after qualified325 geometry discovery and326 failure-only local work attribution. No traversal/cache policy or decisions changed. Empirical uncertainty remains explicit; native5400 and total14400 are unchanged. Actual RANS/SA CFD to curved BL mesh to finite CFD to second mesh, outer14400 total. Unchanged strict gates; conservative flow/turbulence transfer and disk restart require independent assessment. Rejected-state capture enabled.'))
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
                row['executable']=str((Path('/proc') / str(row['pid']) / 'exe').resolve())
                if Path(row['executable']) in (CFD.resolve(),PRODUCTION_CFD.resolve()):
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
     unknown_or_changed_mapped_elf=sorted(unknown_runtime), disk_restart_validated=False, reasonable_flow_qualified=False, expected_events=2, workflow_accepted=False, qualification='Requires independent event/transfer/turbulence/CFD/restart assessment; exit zero alone is insufficient'))
print(json.dumps(dict(state='terminal', exit_code=code, changed=changed)), flush=True)
assert not changed, changed
raise SystemExit(code)
