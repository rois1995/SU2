#!/usr/bin/env python3
"""Collect WP-0(b) per-case results with provenance (input/script/binary/source hashes) into result.json."""
import hashlib, json, sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]

def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def main(run):
    run = (HERE / run).resolve()
    cases = {}
    for f in sorted(p for p in run.glob('wp0b_*.json') if not p.name.endswith('.receipt.json')):
        d = json.loads(f.read_text())
        entry = {'file': f.name, 'sha256': sha(f)}
        for part in ('rejected_state', 'input_state'):
            if part in d:
                entry[part] = {'summary': d[part]['summary'], 'before': d[part]['before'], 'after': d[part]['after']}
                if part == 'input_state':
                    entry[part]['first_row'] = [d[part]['first_row_built'], d[part]['first_row_refused']]
        if 'summary' in d:
            entry['summary'] = d['summary']; entry['before'] = d['before']; entry['after'] = d['after']
            entry['first_row'] = [d['first_row_built'], d['first_row_refused']]
        for key in ('fidelity', 'wp0a_native', 'repair'):
            if key in d:
                entry[key] = d[key]
        cases[d['case']] = entry
    inputs = {p.name: sha(p) for p in sorted(HERE.glob('*.txt'))}
    scripts = {p.name: sha(p) for p in sorted(HERE.glob('*.py'))}
    receipts = {p.name: json.loads(p.read_text()) for p in sorted(run.glob('*.receipt.json'))}
    sources = (HERE.parent / 'build1/source_sha256.txt').read_text().split('\n')
    result = {'scope': 'WP-0(b) test-only prototype: D2 column points and D3 single/compound insertion through existing operators '
                       'under unchanged gates; outcome counts per target; not a mesh-generation capability result',
              'binary_sha256': sha(REPO / 'integration_evidence/build-integrated-v2/UnitTests/test_driver'),
              'sources_sha256': [s for s in sources if s], 'inputs': inputs, 'scripts': scripts, 'receipts': receipts, 'cases': cases}
    (run / 'result.json').write_text(json.dumps(result, indent=1) + '\n')
    for name, c in cases.items():
        for part in ('rejected_state', 'input_state', None):
            s = (c.get(part) or {}).get('summary') if part else c.get('summary')
            if s:
                print(name, part or '', {k: s[k] for k in ('targets', 'single', 'compound', 'blocked', 'not_needed', 'unattempted')}, s['column_stops'])

if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else 'run1')
