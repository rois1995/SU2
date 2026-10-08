"""Check and save native remeshing timing scopes without adding rank maxima together."""
import argparse
import hashlib
import json
from pathlib import Path
import re

STAGES = ('round protocol', 'round dependency import', 'round donor import and IDs',
          'round reconstruction', 'round validation', 'round commit')


def inspect(log):
    profiles = []
    current = {}
    for name, values in re.findall(r'^Native rank cost (.*?) seconds min/mean/max: ([^\n]+)', log, re.M):
        if name == 'selection' and current:
            profiles.append(current)
            current = {}
        numbers = list(map(float, values.split()))
        assert len(numbers) == 3 and all(v >= -1e-8 for v in numbers), (name, numbers)
        assert numbers[0] <= numbers[1] + 1e-6 and numbers[1] <= numbers[2] + 1e-6, (name, numbers)
        assert name not in current
        current[name] = dict(zip(('min', 'mean', 'max'), numbers))
    if current:
        profiles.append(current)
    assert profiles, 'No native timing records'
    closures = []
    for profile in profiles:
        assert all(key in profile for key in (*STAGES, 'selection', 'engine adapt', 'adapt unclassified'))
        # Means are additive for exclusive scopes over the same active ranks; maxima are not.
        accounted = sum(profile[key]['mean'] for key in STAGES) + profile['selection']['mean']
        reconstructed = accounted + profile['adapt unclassified']['mean']
        elapsed = profile['engine adapt']['mean']
        # Logs currently use six significant digits. Allow their rounding, not missing stages.
        tolerance = max(1e-6, 1e-5 * elapsed)
        assert abs(reconstructed - elapsed) <= tolerance, (reconstructed, elapsed)
        assert profile['round reconstruction']['mean'] + tolerance >= profile['private reconstruction']['mean']
        closures.append(dict(engine_adapt_mean_seconds=elapsed, accounted_mean_seconds=accounted,
                             unclassified_mean_seconds=profile['adapt unclassified']['mean'],
                             printed_closure_defect_seconds=abs(reconstructed - elapsed)))
    def series(label):
        return [list(map(float, values.split(';', 1)[0].split()))
                for values in re.findall(r'^' + re.escape(label) + r'([^\n]+)', log, re.M)]
    return dict(status='PASS', worker_profiles=profiles, accounting_closure=closures,
                setup=series('Native setup seconds import-reference/target-policy/working-partition (maximum across ranks):'),
                execution=series('Native execution seconds workers/return-to-CFD/idle-wall/idle-CPU (maximum across ranks):'),
                working_partition=series('Native working partition seconds estimate/graph/ParMETIS/migration (maximum across ranks):'),
                cfd_geometry=series('CFD geometry seconds partition/migration/preprocess (maximum across ranks):'),
                replacement=series('Mesh replacement seconds geometry-build/check-wall-distance/solver-init/transfer/finalize (maximum across ranks, exclusive scopes):'),
                scope='Exclusive transaction means account for engine adaptation plus explicit unclassified overhead. All wall scopes include MPI waits. Rank maxima cannot be summed into critical-path cost. CFD geometry rows include initial geometry; transfer is nested in replacement. No performance or conservation certificate.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('case', nargs='?', type=Path)
    parser.add_argument('--selftest', action='store_true')
    args = parser.parse_args()
    if args.selftest:
        log = 'Native rank cost selection seconds min/mean/max: 1 1 1\n'
        log += ''.join(f'Native rank cost {name} seconds min/mean/max: 1 1 1\n' for name in STAGES)
        log += 'Native rank cost private reconstruction seconds min/mean/max: 1 1 1\n'
        log += 'Native rank cost engine adapt seconds min/mean/max: 8 8 8\n'
        log += 'Native rank cost adapt unclassified seconds min/mean/max: 1 1 1\n'
        assert inspect(log)['accounting_closure'][0]['printed_closure_defect_seconds'] == 0
        try:
            inspect(log.replace('engine adapt seconds min/mean/max: 8 8 8', 'engine adapt seconds min/mean/max: 9 9 9'))
        except AssertionError:
            pass
        else:
            raise AssertionError('Broken accounting was accepted')
        print('Profile accounting selftest PASS')
    else:
        if args.case is None:
            parser.error('Supply a case folder or --selftest')
        output = args.case / 'independent_profile_accounting.json'
        assert not output.exists(), 'Preserve existing evidence'
        source = args.case / 'solver.log'
        result = inspect(source.read_text())
        result.update(log_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                      checker_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
        output.write_text(json.dumps(result, indent=2) + '\n')
        print(output, result['status'], result['accounting_closure'])
