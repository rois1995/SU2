"""Run three serial proposals against the captured final private state; no MPI initialization."""
import hashlib
import json
import os
import subprocess
import sys
import time
from pathlib import Path
from run_cases import sample_tree

ROOT = Path(__file__).resolve().parent
BUILD = ROOT.parents[2] / 'build-integrated-v2'
OUT = ROOT / 'serial_replay80'
SOURCE = ROOT / 'serial_operator_replay80.cpp'

def sha(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''): h.update(block)
    return h.hexdigest()

def main():
    if OUT.exists(): raise RuntimeError('Receipt directory already exists')
    build_status = json.loads((ROOT / 'build79_status.json').read_text())
    if build_status.get('state') != 'terminal' or build_status.get('exit_code') != 0 or not build_status.get('pins_verified'):
        raise RuntimeError('Build79 must be terminal, successful and pin-verified')
    prior = json.loads((ROOT / 'serial_replay76/command.json').read_text())
    if Path(prior['cwd']).resolve() != BUILD.resolve(): raise RuntimeError('Unexpected build directory')
    old = str(ROOT / 'serial_replay76'); new = str(OUT)
    def replace(args):
        return [str(SOURCE) if x == str(ROOT / 'serial_edge_replay74.cpp') else x.replace(old, new) for x in args]
    compile_cmd = replace(prior['phases']['compile']); link_cmd = replace(prior['phases']['link'])
    inputs = [Path(x) for x in prior['phases']['run_success'][1:4]]
    previous_pins = json.loads((ROOT / 'serial_replay76/pins.json').read_text())
    pins = [Path(p) for p in previous_pins if p.endswith('.a')]
    pins += inputs + [SOURCE, Path(__file__).resolve(), ROOT / 'run_cases.py', ROOT / 'serial_replay76/command.json', ROOT / 'serial_replay76/pins.json', ROOT / 'build79_source_pins.json', ROOT / 'build79_status.json']
    before = json.loads((ROOT / 'build79_source_pins.json').read_text())
    if any(sha(Path(path)) != digest for path, digest in before.items()): raise RuntimeError('Build79 source pins changed')
    before.update({str(p): sha(p) for p in pins})
    OUT.mkdir()
    runs = {'split': ['16','6496','split'], 'collapse_6496_1': ['6496','1','collapse'], 'collapse_1_6496': ['1','6496','collapse']}
    commands = {'compile': compile_cmd, 'link': link_cmd}
    for label, (a,b,op) in runs.items(): commands[label] = [str(OUT/'replay'), *(str(p) for p in inputs), a,b,'67207168','.001','16777216',op]
    (OUT/'pins.json').write_text(json.dumps(before, indent=2)+'\n')
    (OUT/'command.json').write_text(json.dumps({'cwd':str(BUILD),'phases':commands,'scope':'serial final private-state proposals; not earlier transaction replay'}, indent=2)+'\n')
    env = os.environ.copy(); env.update({'OMP_NUM_THREADS':'1','OPENBLAS_NUM_THREADS':'1','MKL_NUM_THREADS':'1','TMPDIR':str(BUILD/'compile-tmp')})
    status = {}; start = time.monotonic(); status_path = OUT/'status.json'
    for label, command in commands.items():
        with (OUT/(label+'.log')).open('w') as log, (OUT/'resources.jsonl').open('a') as resources:
            proc = subprocess.Popen(['nice','-n','19','/usr/bin/time','-v','timeout','180',*command], cwd=BUILD, env=env, stdout=log, stderr=subprocess.STDOUT)
            status_path.write_text(json.dumps({'state':'running','pid':proc.pid,'phase':label,'phase_exit_codes':status},indent=2)+'\n')
            while proc.poll() is None:
                rows = sample_tree(proc.pid)
                resources.write(json.dumps({'phase':label,'elapsed_seconds':time.monotonic()-start,'processes':rows,
                    'sampled_process_tree_rss_bytes':sum(row['rss_bytes'] for row in rows)})+'\n');resources.flush()
                time.sleep(2)
            status[label] = proc.returncode
        if label in ('compile','link') and proc.returncode: break
    unchanged = all(sha(Path(p)) == digest for p,digest in before.items())
    (OUT/'status.json').write_text(json.dumps({'state':'terminal','phase_exit_codes':status,'pins_verified':unchanged,'seconds':time.monotonic()-start,'binary_sha256':sha(OUT/'replay') if (OUT/'replay').exists() else None,'scope':'private final-state operator diagnostics; not acceptance','rss_note':'sampled process tree; not allocation peak'},indent=2)+'\n')
    if not unchanged or status.get('compile') != 0 or status.get('link') != 0: sys.exit(1)

if __name__ == '__main__': main()
