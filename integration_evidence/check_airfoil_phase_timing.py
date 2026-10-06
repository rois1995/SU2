"""Check actual per-rank airfoil phases against their MPI maxima and mesh ownership."""
import argparse
import csv
import hashlib
import json
import math
from pathlib import Path

PHASES = ('solve_with_inner_output', 'metric', 'remesh', 'replace', 'transfer',
          'replace_without_transfer', 'mesh_output')


def validate(rows, maxima, ranks, points):
    if len(rows) != ranks or sorted(int(r['rank']) for r in rows) != list(range(ranks)):
        raise ValueError('Missing or duplicate rank timing rows')
    for row in rows:
        if int(row['ranks']) != ranks or int(row['accepted']) != 1:
            raise ValueError('Rank count or accepted state disagrees')
        if not 0 <= int(row['owned_points']) <= int(row['total_points']) or int(row['local_elements']) < 0:
            raise ValueError('Invalid local geometry counts')
        if int(row['rss_hwm_kib']) <= 0:
            raise ValueError('Missing Linux process high-water RSS')
        for phase in PHASES:
            value = float(row[phase + '_s'])
            if not math.isfinite(value) or value < 0:
                raise ValueError('Invalid local phase duration')
        if not math.isclose(float(row['replace_s']), float(row['transfer_s']) + float(row['replace_without_transfer_s']), rel_tol=1e-12, abs_tol=1e-12):
            raise ValueError('Replacement duration disagrees with its transfer/remainder')
    if sum(int(r['owned_points']) for r in rows) != points:
        raise ValueError('Owned CFD points disagree with saved adapted mesh')
    if int(maxima['ranks']) != ranks or int(maxima['accepted']) != 1:
        raise ValueError('MAX row rank count or accepted state disagrees')
    for phase in PHASES:
        expected = max(float(r[phase + '_s']) for r in rows)
        if float(maxima[phase + '_max_s']) != expected:
            raise ValueError('MPI phase maximum disagrees with local rows: ' + phase)



def validate_solution(rows, points):
    if len(rows) != len(points):
        raise ValueError('Flow snapshot point count disagrees with its mesh')
    minimum_density = float('inf'); minimum_internal = float('inf')
    for index, (row, point) in enumerate(zip(rows, points)):
        if int(row['point_id']) != index or (float(row['x']), float(row['y'])) != point:
            raise ValueError('Flow snapshot global ID/coordinates disagree with its mesh')
        rho, mx, my, energy = (float(row[k]) for k in ('density', 'momentum_x', 'momentum_y', 'energy_density'))
        if not all(math.isfinite(v) for v in (rho, mx, my, energy)) or rho <= 0:
            raise ValueError('Nonfinite or nonpositive flow snapshot')
        internal = energy - (mx*mx + my*my)/(2*rho)
        if not math.isfinite(internal) or internal <= 0:
            raise ValueError('Flow snapshot has nonpositive internal energy')
        minimum_density = min(minimum_density, rho); minimum_internal = min(minimum_internal, internal)
    return dict(points=len(rows), min_density=minimum_density, min_internal_energy_density=minimum_internal)


def self_check():
    row = dict(ranks=1, rank=0, accepted=1, owned_points=3, total_points=3, local_elements=1, rss_hwm_kib=1024)
    row.update({phase + '_s': 1. for phase in PHASES})
    row['replace_s'] = 2.
    maximum = dict(ranks=1, accepted=1, **{phase + '_max_s': row[phase + '_s'] for phase in PHASES})
    validate([row], maximum, 1, 3)
    flow = dict(point_id=0, x=0., y=0., density=1., momentum_x=1., momentum_y=0., energy_density=2.)
    validate_solution([flow], [(0., 0.)])
    bad_flow = dict(flow, x=1.)
    try:
        validate_solution([bad_flow], [(0., 0.)])
    except ValueError:
        pass
    else:
        raise AssertionError('A flow snapshot mapped to the wrong point passed')
    maximum['remesh_max_s'] = 2.
    try:
        validate([row], maximum, 1, 3)
    except ValueError:
        return
    raise AssertionError('An incorrect reported phase maximum passed')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path, nargs='?')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check(); print('Self-check PASS'); return
    if args.directory is None or args.output is None:
        parser.error('directory and --output are required for actual evidence')
    if args.output.exists():
        raise RuntimeError('Preserve existing evidence; select a fresh output')
    root = args.directory.resolve(strict=True)
    hashes = {}
    def read(path):
        data = path.read_bytes(); key = str(path.resolve()); digest = hashlib.sha256(data).hexdigest()
        if hashes.setdefault(key, digest) != digest:
            raise RuntimeError('Repeated input changed during evaluation: ' + key)
        return data.decode()
    def one(path):
        rows = list(csv.DictReader(read(path).splitlines()))
        if len(rows) != 1:
            raise ValueError('Expected one cycle timing row: ' + str(path))
        return rows[0]
    runtime = json.loads(read(root/'evidence.json'))
    if runtime.get('current_run') or not runtime.get('archived_binary') or not runtime['runs'] or not all(r['verified'] for r in runtime['runs']):
        raise RuntimeError('Timing evidence requires a finished successful runtime')
    records = []
    for run in runtime['runs']:
        ranks = run['ranks']; previous = [0]*ranks
        for cycle in range(3):
            prefix = root/f'audit_np{ranks}'/f'native_airfoil_cycle_{cycle}'
            maximum = one(Path(str(prefix)+'_timing.csv'))
            rows = [one(Path(str(prefix)+f'_timing_rank_{rank}.csv')) for rank in range(ranks)]
            point_headers = [int(line.split('=', 1)[1].split()[0]) for line in read(Path(str(prefix)+'_adapted.su2')).splitlines() if line.startswith('NPOIN=')]
            if len(point_headers) != 1:
                raise ValueError('Missing or ambiguous adapted mesh point count')
            validate(rows, maximum, ranks, point_headers[0])
            from audit_native_bl import mesh
            fields = {}
            for stage in ('donor', 'adapted'):
                mesh_path = Path(str(prefix) + '_' + stage + '.su2')
                read(mesh_path)  # Pin the exact geometry used by the paired field check.
                points, _, _ = mesh(mesh_path)
                solution_rows = list(csv.DictReader(read(Path(str(prefix) + '_' + stage + '_solution.csv')).splitlines()))
                fields[stage] = validate_solution(solution_rows, points)
            rss = [int(r['rss_hwm_kib']) for r in rows]
            if any(now < old for now, old in zip(rss, previous)):
                raise ValueError('Cumulative rank VmHWM decreased across cycles')
            previous = rss
            records.append(dict(ranks=ranks, cycle=cycle, maximum=maximum, local=rows, fields=fields,
                                owned_point_imbalance=ranks*max(int(r['owned_points']) for r in rows)/point_headers[0]))
    if any(hashlib.sha256(Path(path).read_bytes()).hexdigest() != digest for path, digest in hashes.items()):
        raise RuntimeError('Input changed during timing evaluation')
    report = dict(verified=True, cases=records, input_files_sha256=hashes,
                  checker_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  scope='phase maxima are not additive; transfer is inside replacement; solve includes inner output; cumulative rank RSS includes startup and test snapshots; local elements include overlaps')
    with args.output.open('x') as stream:
        json.dump(report, stream, indent=2); stream.write('\n')
    print(f'Timing/ownership check PASS: {len(records)} cycles')


if __name__ == '__main__':
    main()
