"""Pinned, resumable build/planar/M6 compatibility receipts; reuses native campaign helpers."""
import importlib.util
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('receipts', ROOT.parent / 'run_cases.py')
receipt = importlib.util.module_from_spec(spec)
spec.loader.exec_module(receipt)
match = re.fullmatch(r'(build|planar|m6)([2-9][0-9]*)', sys.argv[1])
assert match, 'Use a new buildN, planarN or m6N receipt (N >= 2)'
mode, number = match.groups()
label = mode + number
status = ROOT / (label + '_status.json')
assert not status.exists(), 'Inspect existing owned handle; never overwrite its receipt'
environment = json.loads((ROOT / 'build_command.json').read_text())['environment']
env = os.environ.copy()
env.update(environment)
if mode == 'build':
    pins = json.loads((ROOT / 'build_pins.json').read_text())
    for path, digest in pins.items():
        if Path(path) not in (ROOT / 'gc_probe.cpp', ROOT / 'CMakeLists.txt'):
            assert receipt.sha(path) == digest, 'Upstream/toolchain/helper changed: ' + path
    for path in (ROOT / 'gc_probe.cpp', ROOT / 'CMakeLists.txt'):
        pins[str(path)] = receipt.sha(path)
        (ROOT / (label + '_' + path.name)).write_bytes(path.read_bytes())
    argv = ['nice', '-n', '19', 'timeout', '600', 'cmake', '--build', str(ROOT / 'build'), '--parallel', '1', '--target', 'gc_probe']
else:
    build = json.loads((ROOT / ('build' + number + '_status.json')).read_text())
    assert build['exit_code'] == 0 and build['pins_verified']
    pins = json.loads((ROOT / ('build' + number + '_pins.json')).read_text())
    binary = ROOT / 'build/gc_probe'
    pins[str(binary)] = receipt.sha(binary)
    ldd = subprocess.run(['ldd', str(binary)], capture_output=True, text=True, check=True)
    (ROOT / (label + '_ldd.txt')).write_text(ldd.stdout)
    for line in ldd.stdout.splitlines():
        found = re.search(r'(/[^\s]+)', line)
        if found and Path(found[1]).is_file():
            path = Path(found[1]).resolve()
            pins[str(path)] = receipt.sha(path)
    argv = ['nice', '-n', '19', '/usr/bin/time', '-v', 'timeout', '60', str(binary)]
    if mode == 'm6':
        argv.append(str(ROOT / 'm6_faceted_star.txt'))
        for path in (ROOT / 'm6_faceted_star.txt', ROOT / 'm6_faceted_star.json'):
            pins[str(path)] = receipt.sha(path)
pins[str(Path(__file__).resolve())] = receipt.sha(Path(__file__).resolve())
assert all(receipt.sha(path) == digest for path, digest in pins.items()), 'Input pins changed'
receipt.save(ROOT / (label + '_pins.json'), pins)
receipt.save(ROOT / (label + '_command.json'), dict(argv=argv, environment=environment))
start = time.time()
peak = 0
with (ROOT / (label + '.log')).open('w') as log, (ROOT / (label + '_resources.jsonl')).open('w') as resources:
    child = subprocess.Popen(argv, env=env, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
    receipt.save(status, dict(state='running', pid=child.pid, start=start))
    print(label, 'RUNNING', child.pid, flush=True)
    while child.poll() is None:
        rows = receipt.sample_tree(child.pid)
        rss = sum(row['rss_bytes'] for row in rows)
        peak = max(peak, rss)
        resources.write(json.dumps(dict(seconds=time.time() - start, processes=rows, sampled_process_tree_rss_bytes=rss)) + '\n')
        resources.flush()
        time.sleep(1 if mode == 'build' else .05)
    code = child.wait()
changed = [path for path, digest in pins.items() if receipt.sha(path) != digest]
result = dict(state='terminal', exit_code=code, seconds=time.time() - start, pins_verified=not changed, changed=changed,
              largest_sampled_process_tree_rss_bytes=peak, rss_note='sampled owned process tree; not a whole-job peak')
if mode != 'build':
    events = [json.loads(line) for line in (ROOT / (label + '.log')).read_text().splitlines() if line.startswith('{')]
    receipt.save(ROOT / (label + '_events.json'), events)
    result['complete_16_events'] = [row.get('event') for row in events] == list(range(16))
    result['scope'] = 'representation/API only; no cubic/BL/MPI/volume/transfer/joint-network qualification'
    if not result['complete_16_events'] and not code:
        result['exit_code'] = code = 2
receipt.save(status, result)
print(json.dumps(result), flush=True)
sys.exit(code or bool(changed))
