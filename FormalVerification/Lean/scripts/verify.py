#!/usr/bin/env python3
"""Local reproducible checks. The user's independent evaluator must approve acceptance."""
import hashlib
import json
import os
import re
import shutil
import subprocess
import tempfile
import time
from pathlib import Path
from check_source import ROOT, check_source


def run(command, log, env, expected_success=True):
    result = subprocess.run(command, cwd=ROOT, env=env, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=1200)
    log.write('$ ' + ' '.join(command) + '\n' + result.stdout + '\n')
    log.flush()
    if expected_success and result.returncode:
        print(result.stdout)
        raise RuntimeError(f'Command failed: {command}')
    return result


def wait_for_load():
    print('One-minute load:', round(os.getloadavg()[0], 2), flush=True)
    while os.getloadavg()[0] >= 6:
        print('Waiting for workspace load < 6.', flush=True)
        time.sleep(15)


def main():
    os.chdir(ROOT)
    env = os.environ.copy()
    local_elan = ROOT / '.toolchain/elan'
    if (local_elan / 'bin/lake').exists():
        env['ELAN_HOME'] = str(local_elan)
        env['PATH'] = str(local_elan / 'bin') + ':' + env['PATH']
    if not shutil.which('lake', path=env['PATH']):
        raise RuntimeError('Install the toolchain in lean-toolchain with elan first.')
    env['LEAN_NUM_THREADS'] = '2'
    if hasattr(os, 'sched_getaffinity'):
        os.sched_setaffinity(0, sorted(os.sched_getaffinity(0))[:2])
    source = check_source()
    proof_sources = sorted((ROOT / 'SU2').glob('*.lean'))
    for file in proof_sources:
        if re.search(r'\b(sorry|admit|axiom|native_decide|unsafe)\b', file.read_text()):
            raise ValueError(f'Forbidden proof token in {file.name}')
    assert len(proof_sources) == 7, 'Review source inventory before extending this release'
    lock = json.loads((ROOT / 'lake-manifest.json').read_text())
    out = ROOT / 'artifacts'
    out.mkdir(exist_ok=True)
    with (out / 'validation.log').open('w') as log:
        version = run(['lean', '--version'], log, env).stdout.strip()
        if 'version 4.19.0,' not in version or '6caaee842e94' not in version:
            raise ValueError('Unexpected Lean compiler version')
        for package in lock['packages']:
            repo = ROOT / '.lake/packages' / package['name']
            actual = run(['git', '-C', str(repo), 'rev-parse', 'HEAD'], log, env).stdout.strip()
            if actual != package['rev']:
                raise ValueError('Dependency revision mismatch: ' + package['name'])
            run(['git', '-C', str(repo), 'diff', '--exit-code', 'HEAD', '--'], log, env)
        wait_for_load()
        # Recheck all project proofs; dependency provenance is checked above.
        build = ROOT / '.lake/build'
        if build.exists():
            shutil.rmtree(build)
        log.write('Removed project .lake/build; retained pinned dependency builds.\n')
        run(['lake', 'build'], log, env)
        wait_for_load()
        audit = run(['lake', 'env', 'lean', '-DwarningAsError=true', 'Integrity.lean'], log, env)
        summary = [line for line in audit.stdout.splitlines() if line.startswith('Audited ')]
        if len(summary) != 1:
            raise ValueError('Missing dependency-audit result')
        # Negative fixture is temporary and never an accepted proof dependency.
        with tempfile.TemporaryDirectory(prefix='audit-negative-', dir=out) as tmp:
            probe = Path(tmp) / 'Rejected.lean'
            body = (ROOT / 'Integrity.lean').read_text()
            probe.write_text(body.replace('open Lean Elab Command in',
                'axiom SU2.RejectedAuditProbe : False\n\nopen Lean Elab Command in'))
            wait_for_load()
            rejected = run(['lake', 'env', 'lean', str(probe)], log, env, expected_success=False)
            if rejected.returncode == 0 or 'Unapproved axiom SU2.RejectedAuditProbe' not in rejected.stdout:
                raise ValueError('Integrity audit did not reject the negative fixture')
        wait_for_load()
        experiment = run(['python3', 'scripts/source_experiment.py'], log, env)
        names = re.findall(r'\btheorem\s+(\w+)', '\n'.join(f.read_text() for f in proof_sources))
        report = {'status': 'local checks passed; independent acceptance pending',
                  'lean': version, 'su2_commit': source['commit'],
                  'explicit_theorems': len(names), 'audit': summary[0],
                  'negative_audit': 'custom axiom rejected', 'cpp_evidence': experiment.stdout.strip(),
                  'source_digests': {str(f.relative_to(ROOT)): hashlib.sha256(f.read_bytes()).hexdigest()
                                     for f in proof_sources},
                  'specification_approval': 'pending independent evaluator'}
        (out / 'validation.json').write_text(json.dumps(report, indent=2)+'\n')
        print(summary[0])
        print(experiment.stdout, end='')
        print('Local verification passed; independent acceptance remains pending.')


if __name__ == '__main__':
    main()
