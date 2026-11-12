#!/usr/bin/env python3
"""Run the selected upstream path; observed positivity is not a preservation theorem."""
import argparse
import csv
import json
import math
from pathlib import Path
import shutil
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT.parent / 'EvaluationKit'))
from common import before_work, limits, require, run, sha


def table(path):
    with path.open() as f:
        reader = csv.DictReader(f, skipinitialspace=True)
        return [{k.strip().strip('"'): float(v) for k, v in row.items()} for row in reader]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    limits()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    records = []
    for attempt in range(2):
        case = out / str(attempt)
        shutil.copytree(ROOT / 'tests/integration', case)
        before_work()
        run([args.binary.resolve(), 'euler.cfg'], case, out / f'run-{attempt}.log', timeout=120)
        require('Exit Success' in (out / f'run-{attempt}.log').read_text(), 'Missing successful solver completion')
        restart = case / 'restart.csv'
        rows = table(restart)
        require(len(rows) == 20, 'Unexpected mesh state count')
        require(all(math.isfinite(v) for row in rows for v in row.values()), 'Nonfinite restart state')
        densities = [row['Density'] for row in rows]
        internals = [row['Energy'] - (row['Momentum_x']**2 + row['Momentum_y']**2) / (2 * row['Density']) for row in rows]
        require(min(densities) > 0 and min(internals) > 0, 'Observed final state not admissible')
        history = table(case / 'history.csv')
        require(len(history) == 20, 'Expected twenty executed iterations')
        require(all(math.isfinite(v) for row in history for v in row.values()), 'Nonfinite history')
        require(len({row['Momentum_y'] for row in rows}) > 1, 'Trivial unchanged uniform result')
        records.append({'restart_sha256': sha(restart), 'history_sha256': sha(case / 'history.csv'),
                        'iterations': len(history), 'points': len(rows),
                        'min_density': min(densities), 'min_internal_energy_density': min(internals)})
    require(records[0]['restart_sha256'] == records[1]['restart_sha256'], 'Serial final states are not reproducible')
    result = {'status': 'PASS native integration only', 'source_commit': 'd162016382f0c122433000c114e88d808b48f7a7',
              'binary_sha256': sha(args.binary), 'config_sha256': sha(ROOT / 'tests/integration/euler.cfg'),
              'mesh_sha256': sha(ROOT / 'tests/integration/mesh.su2'), 'runs': records,
              'limitation': 'No physical accuracy, convergence, general positivity or IEEE semantics proof; mathematical boundary/flux correspondence remains open.'}
    (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print('Two selected-path runs passed, with identical final conservative states.', flush=True)


if __name__ == '__main__':
    main()
