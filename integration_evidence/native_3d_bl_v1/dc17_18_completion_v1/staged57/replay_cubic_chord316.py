"""Bounded serial exact-chord replay captured from faithful actual M6 cell311; launch separately."""
from pathlib import Path
import json, os, re, shutil, subprocess, sys, tempfile, time
from run_m6 import ldd_paths, sha, save
from run_cases import BINARY as CFD, sample_tree
R=Path(__file__).resolve().parent
assert len(sys.argv)==3, 'usage: replay_cubic_chord316.py CAPTURE_DIR NEW_CASE_NAME'
checkpoint=Path(sys.argv[1]).resolve(strict=True)
assert re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_.-]{0,80}',sys.argv[2])
A=R/'m6_runs/rans_curved314_v1'
folder=R/'cases'/sys.argv[2]
assert not folder.exists(), 'Verify existing owned handles; never overwrite/relaunch.'
assert json.loads((A/'terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
assert json.loads((R/'build316_terminal_handles.json').read_text())['all_owned_handles_terminal']
build_status=R/'build316_status.json'; build_pins=R/'build316_source_pins.json'
status=json.loads(build_status.read_text())
assert status['state']=='terminal' and status['exit_code']==0 and status['pins_verified']
assert sha(CFD)==status['binaries'][str(CFD)]
pins=json.loads(build_pins.read_text())
runtime_manifest=R/'runtime_20261106_v2/loaded_runtime_pins.json'
runtime=json.loads(runtime_manifest.read_text())
assert not set(ldd_paths(CFD))-set(runtime)
pins.update(runtime)
prepared_path=checkpoint/'provenance.json'
prepared=json.loads(prepared_path.read_text())
for path,digest in prepared['pins'].items():
    assert sha(Path(path))==digest,path
    pins[path]=digest
for name,digest in prepared['output_sha256'].items():
    assert sha(checkpoint/name)==digest,name
    pins[str((checkpoint/name).resolve())]=digest
config_path=Path(prepared['config'])
reference=prepared['reference']
source_attempt=Path(prepared['source_attempt'])
assert json.loads((source_attempt/'terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
assert sha(Path(reference))=='2e99fb7870e2c718f110da8509e4c0f7e746ce138e7f27ab57aa0dedbb6c42de'
for path in [CFD,build_status,build_pins,runtime_manifest,prepared_path,Path(__file__),
             A/'terminal_handles_actual.json',R/'build316_terminal_handles.json',
             R/'run_rans_continuation303.py',R/'run_m6.py',R/'run_cases.py',R.parent/'diagnostic56/run.py',
             *checkpoint.iterdir(), *[Path(shutil.which(name)).resolve() for name in ('nice','timeout')],Path('/usr/bin/time')]:
    if path.is_file(): pins[str(path.resolve())]=sha(path)
assert all(sha(Path(p))==h for p,h in pins.items())
available=int(next(l.split()[1] for l in Path('/proc/meminfo').read_text().splitlines() if l.startswith('MemAvailable:')))*1024
assert available>16*1024**3
assert 'ADAP_NATIVE_MAX_TIME= 5400' in config_path.read_text().splitlines()
folder.mkdir()
save(folder/'pins.json',pins)
scratch_root = R.parents[3] / 'integration_evidence/build-integrated-v2/compile-tmp'
scratch = Path(tempfile.mkdtemp(prefix='chord-replay316-', dir=scratch_root))
env = {k: v for k, v in os.environ.items() if not k.startswith('SU2_NATIVE_')}
env.update(OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', MKL_NUM_THREADS='1',
           OMPI_MCA_osc='pt2pt', TMPDIR=str(scratch_root), OMPI_MCA_orte_tmpdir_base=str(scratch))
env.update(SU2_NATIVE_FROZEN_CONFIG=str(config_path), SU2_NATIVE_BL_REFERENCE=reference,
           SU2_NATIVE_BL_CHORD_REPLAY=str(checkpoint/'chord.txt'))
argv = ['nice', '-n', '19', '/usr/bin/time', '-v', 'timeout', '--signal=TERM', '--kill-after=20s', '1800', str(CFD), '[NativeActualCubicBLPolicy3D]', '--durations', 'yes']
own = {'pid': os.getpid(), 'start_ticks': int(Path('/proc/self/stat').read_text().rsplit(')', 1)[1].split()[19])}
save(folder / 'command.json', dict(argv=argv, wrapper=own, replay_inputs=str(checkpoint), timeout_seconds=1800,
     source_attempt=str(A), initial_available_bytes=available, loadavg=Path('/proc/loadavg').read_text(),
     environment={k: v for k, v in env.items() if k.startswith(('OMPI_', 'OMP_', 'OPENBLAS_', 'MKL_', 'SU2_NATIVE_')) or k == 'TMPDIR'},
     scope='Exact captured faithful cell311 cubic BL chord replay only. Serial 1800-second bound, unchanged native5400/config/geometry and acceptance gates; no full adaptation qualification.'))
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
