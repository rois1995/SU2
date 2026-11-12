#!/usr/bin/env python3
"""Restore unchanged initialization sources and select the separately pinned replay correction."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('handoff',type=Path)
    p.add_argument('output',type=Path)
    p.add_argument('--historical-kit',type=Path,required=True)
    p.add_argument('--cfdlean-source',type=Path,required=True)
    p.add_argument('--expected-candidate-sha256',required=True)
    p.add_argument('--expected-replay-sha256',required=True)
    args = p.parse_args()
    manifest = json.loads((args.handoff/'manifest.json').read_text())
    if manifest['replay_lock_sha256']!=args.expected_replay_sha256: raise ValueError('Wrong replay identity')
    original = Path(__file__).resolve().parent.parent/'Initialization/scripts/restore.py'
    subprocess.run([sys.executable,original,args.handoff,args.output,
        '--historical-kit',args.historical_kit,'--cfdlean-source',args.cfdlean_source,
        '--expected-candidate-sha256',args.expected_candidate_sha256],check=True)
    project = args.output.resolve()/'SU2/FormalVerification/InitializationReplay'
    digest = hashlib.sha256((project/'candidate.json').read_bytes()).hexdigest()
    if digest!=args.expected_replay_sha256: raise ValueError('Restored replay identity differs')
    commands = args.output/'commands.txt'
    text = commands.read_text()
    if text.count('python3 scripts/check.py ')!=1: raise ValueError('Unexpected prior command format')
    text = text.replace('python3 scripts/check.py ','python3 ../InitializationReplay/verify.py ')
    commands.write_text(text)
    result = args.output/'restore-result.json'
    record = json.loads(result.read_text())
    record['replay_lock_sha256'] = args.expected_replay_sha256
    record['checker_selection'] = 'InitializationReplay/verify.py; original source and failed checker preserved'
    result.write_text(json.dumps(record,indent=2)+'\n')
    print('Use the CORRECTED commands.txt below; original checker failure is documented.\n'+text)


if __name__=='__main__':
    main()
