#!/usr/bin/env python3
"""Bounded, resumable focused-test runner (one owned process at a time, nice 19, single-threaded).
Usage: run_focused.py <label> <workdir> <ranks> <catch filter> [ENV=VALUE ...]
Runs immediately (no load-based waiting; one own job at a time), records a receipt with binary hash, timing and peak RSS."""
import hashlib, json, os, subprocess, sys, time
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
BINARY = REPO / 'integration_evidence/build-integrated-v2/UnitTests/test_driver'

def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def main():
    label, workdir, ranks, test = sys.argv[1], Path(sys.argv[2]).resolve(), int(sys.argv[3]), sys.argv[4]
    extra = dict(a.split('=', 1) for a in sys.argv[5:])
    workdir.mkdir(parents=True, exist_ok=True)
    receipt = workdir / f'{label}.receipt.json'
    if receipt.exists() and json.loads(receipt.read_text()).get('exit_code') == 0 and os.environ.get('FORCE') != '1':
        print(f'{label}: reusing passing receipt'); return
    # User rule 2026-10-21: no load-based waiting; one own job at a time.
    env = dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1', MKL_NUM_THREADS='1', MALLOC_ARENA_MAX='2')
    env.update(extra)
    command = ['/usr/bin/time', '-v', 'nice', '-n', '19']
    if ranks > 1:
        command += ['mpirun', '--oversubscribe', '-np', str(ranks)]
    command += [str(BINARY), test, '--reporter', 'compact', '--durations', 'yes']
    started = time.time(); load = os.getloadavg()
    with open(workdir / f'{label}.log', 'w') as log:
        proc = subprocess.run(command, cwd=workdir, env=env, stdout=log, stderr=subprocess.STDOUT)
    text = (workdir / f'{label}.log').read_text(errors='replace')
    rss = [int(l.split(':')[1]) for l in text.splitlines() if 'Maximum resident set size' in l]
    summary = [l for l in text.splitlines() if l.startswith('Passed') or l.startswith('Failed') or 'test cases' in l][-3:]
    data = {'label': label, 'command': command, 'environment': extra, 'cwd': str(workdir), 'exit_code': proc.returncode,
            'seconds': time.time() - started, 'load_at_start': load, 'max_rss_kb_of_one_process': max(rss) if rss else None,
            'binary_sha256': sha(BINARY), 'summary': summary}
    receipt.write_text(json.dumps(data, indent=1) + '\n')
    print(json.dumps(data, indent=1))

if __name__ == '__main__':
    main()
