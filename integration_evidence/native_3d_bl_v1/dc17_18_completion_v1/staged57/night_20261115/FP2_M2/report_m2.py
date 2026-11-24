"""Report the selected FP2-M2 attempts and summarize their measured CSV tables."""

from argparse import ArgumentParser
from collections import Counter
import csv
import json
import math
from pathlib import Path
import shutil

HERE = Path(__file__).resolve().parent
TRIALS = ('BASELINE', 'ANATOMY', '2E-4', '5E-4', '1E-3')
FIXTURES = ('off-serial', 'off-mpi', 'curved', 'model')
TRACE_OFF = {'BASELINE': '1E-4', **{tol: tol for tol in TRIALS[2:]}}
TURN = (1, 2, 5, 10, 15, 20, 30, 45)
KINK = (.01, .1, .5, 1, 2, 5, 15)
OUTER = ('direct_seconds', 'support_seconds', 'upper_seconds')
UPPER = ('facet_build_seconds', 'piece_selection_seconds', 'scope_build_seconds', 'traversal_seconds')
LIMITS = ('Every run uses the frozen input sensor. The pass creates tensors for inserted points as usual. '
          'The tolerance moves 0.7 tol, 1.4 tol, tol/8, the support padding by tol and the first chord ceiling '
          'tol/16 together. The remote contact margin of 3 tol belongs to MPI and is absent in N1. '
          'The angle tables describe the measured meshes. They cannot predict the mesh of an angle rule. '
          'The larger tolerances are measurement conditions. Geometry acceptance remains at 1e-4.')


def read(path):
    return json.loads(path.read_text()) if path.is_file() else None


def number(value):
    try:
        result = float(value)
    except (TypeError, ValueError):
        return None
    return result if math.isfinite(result) else None


def share(num, denom):
    return num / denom if denom else None


def finite(rows, field):
    return [value for row in rows if (value := number(row.get(field))) is not None]


def quantiles(values):
    values = sorted(values)
    n = len(values)
    return {'n': n, 'median': values[math.ceil(.5*n)-1] if n else None,
            'p90': values[math.ceil(.9*n)-1] if n else None, 'max': values[-1] if n else None}


def histogram(values, limits):
    def index(value):
        return sum(value > limit for limit in limits)

    labels = [f'<= {limits[0]}', *(f'({a}, {b}]' for a, b in zip(limits, limits[1:])), f'> {limits[-1]}']
    counts = Counter(index(value) for value in values)
    return {label: counts[i] for i, label in enumerate(labels)}


def distribution(rows, field, limits):
    values = finite(rows, field)
    return {**quantiles(values), 'missing': len(rows) - len(values), 'histogram': histogram(values, limits)}


def face_metrics(rows, tol):
    metric = finite(rows, 'm_max')
    sample = finite(rows, 'sample_max')
    longest = [max(values) for row in rows if (values := finite_edges(row))]
    return {
        'faces': len(rows), 'area': sum(finite(rows, 'area')), 'longest': quantiles(longest),
        'longest_missing': len(rows) - len(longest),
        'm_max_n': len(metric), 'm_max_missing': len(rows) - len(metric),
        **{f'm_max_below_{limit}': share(sum(x < limit for x in metric), len(metric)) for limit in (.5, .9)},
        'sample_n': len(sample), 'sample_missing': len(rows) - len(sample),
        **{f'sample_above_{limit}_tol': share(sum(x > limit*tol for x in sample), len(sample))
           for limit in (.35, .6, .7)},
        'sample_over_tol': quantiles(x/tol for x in sample),
        'some_sample_nonfinite': sum(any(row.get(f'finite{i}') == '0' for i in range(4)) for row in rows),
        'unconverged_faces': sum((number(row.get('unconverged')) or 0) > 0 for row in rows),
        'normal_deviation': distribution(rows, 'normal_dev_deg', TURN),
        'corners_ok': sum(row.get(f'eval{i}') == 'ok' for row in rows for i in range(3)),
        'corners_none': sum(row.get(f'eval{i}') == 'none' for row in rows for i in range(3)),
    }


def finite_edges(row):
    return finite([{'length': row.get(f'length{i}')} for i in range(3)], 'length')


def handover_metrics(faces, edges, tol):
    bins = {}
    for i in range(7):
        f = [row for row in faces if row.get('bin') == str(i)]
        e = [row for row in edges if row.get('bin') == str(i)]
        interior = [row for row in e if row.get('class') == 'interior']
        bins[str(i)] = {'face': face_metrics(f, tol),
                        'edge_classes': {kind: sum(row.get('class') == kind for row in e)
                                         for kind in ('open', 'feature', 'entity_border', 'interior')},
                        'interior_turn': distribution(interior, 'turn_deg', TURN)}
    return bins


def model_metrics(facets, edges):
    bins = {}
    for i in range(7):
        f = [row for row in facets if row.get('bin') == str(i)]
        e = [row for row in edges if row.get('bin') == str(i)]
        bins[str(i)] = {'facets': len(f), 'edges': len(e), 'kink': distribution(e, 'kink_max', KINK),
                        'edges_with_missing_angle': sum(any(number(row.get(f'kink_t{j}')) is None
                                                            for j in (0, 25, 50, 75, 100)) for row in e),
                        'control_deviation': quantiles(finite(f, 'control_deviation')),
                        'longest_edge': quantiles(finite(f, 'longest_edge'))}
    return bins


def stages(rows):
    result = []
    for row in rows:
        row = {key: (int(value) if str(value).isdigit() else number(value)) for key, value in row.items()}
        outer = row['seconds_forward']
        upper = row['upper_seconds']
        result.append({
            **row,
            'outer_shares': {**{key: share(row[key], outer) for key in OUTER},
                             'remainder': share(outer - sum(row[key] for key in OUTER), outer)},
            'upper_shares': {**{key: share(row[key], upper) for key in UPPER},
                             'remainder': share(upper - sum(row[key] for key in UPPER), upper)},
            'per_call_largest_bin': {key: share(value, row['calls']) for key, value in row.items()
                                     if key not in ('rank', 'pass', 'bin', 'direct_eligible', 'calls')}
            if row['bin'] == 4 else {},
        })
    return result


def forward_clocks(rows):
    result = {}
    for key in (*range(5), 'all'):
        group = [row for row in rows if key == 'all' or row['bin'] == key]
        if not group:
            continue
        calls = sum(row['calls'] for row in group)
        seconds = sum(row['seconds_forward'] for row in group)
        result[str(key)] = {'calls': calls, 'seconds': seconds, 'mean_ms': share(1000*seconds, calls)}
    return result


def store_artifact(path, output):
    if not path.is_file():
        return None
    target = output / path.name
    shutil.copyfile(path, target)
    return target.name


def load_csv(path):
    if not path.is_file():
        return []
    with path.open(newline='') as source:
        return list(csv.DictReader(source))


def first(rows, *keys):
    return next((rows[key][0] for key in keys if rows.get(key)), {})


def observed(receipt):
    raw = receipt.get('rows_by_rank', {}).get('0', {})
    measure = receipt.get('measurement', {})
    stage = stages(raw.get('STAGE', []))
    surface = first(raw, 'NATIVE3D_LOCAL_SURFACE')
    result = {
        'stage': stage, 'forwards': forward_clocks(stage), 'observer': first(raw, 'OBSERVER'),
        'table': first(raw, 'TABLE'), 'pass': first(raw, 'pass'), 'state': first(raw, 'state'),
        'proof': measure.get('proof') or first(raw, 'LAB_S_PROOF', 'proof'),
        'proof_entities': raw.get('LAB_S_ENTITY', raw.get('proof_entity', [])),
        'entity_faces': raw.get('entity_faces', []),
        'surface': {key: value for key, value in surface.items()
                    if key in ('faces_out', 'points_out', 'splits_length', 'splits_distance', 'collapses_surface',
                               'seconds_length', 'seconds_distance', 'seconds_c', 'seconds_c_bound')
                    or key.startswith(('d_', 'left_', 'c_refused_'))},
        'refused_regions': raw.get('REFUSED_REGION', []),
        'wall_seconds': receipt.get('wait', {}).get('elapsed_seconds'),
        'peak_lower_bound_bytes': receipt.get('peak_lower_bound_bytes'),
        'evaluation': {'problems': measure.get('problems'), 'failed_limits': measure.get('failed_limits')},
    }
    regions = result['refused_regions']
    result['refused_regions'] = {
        'captured_by_bin': {row['bin']: row['captured'] for row in regions},
        'total_refused': regions[0]['total_refused'] if regions else None,
    }
    return result


def collect(label, output):
    fixtures, trials = {}, {}
    for key in (*FIXTURES, *TRIALS):
        is_fixture = key in FIXTURES
        path = HERE / ('fixtures' if is_fixture else 'trials') / f'build{label}' / key / 'case_summary.json'
        summary = read(path.parent / 'r5_retry' / path.name) or read(path)
        data = dict(summary) if summary else {'status': 'missing', 'reason': 'No completed case summary.'}
        selected = data.get('used')
        if selected:
            source = Path(selected)
            directory = source.parent if source.is_file() else source
            names = []
            if key == 'model':
                model = ['model_facets.csv', 'model_edges.csv']
                names.extend(model)
                if all((directory / filename).is_file() for filename in model):
                    data['bins'] = model_metrics(*(load_csv(directory / filename) for filename in model))
            if key in TRIALS:
                data['observations'] = observed(read(source) or {})
            if key in TRACE_OFF:
                tol = TRACE_OFF[key]
                files = [f'handover_{kind}_{tol}.csv' for kind in ('faces', 'edges')]
                names.extend(files)
                if all((directory / filename).is_file() for filename in files):
                    faces, edges = (load_csv(directory / filename) for filename in files)
                    data['bins'] = handover_metrics(faces, edges, float(tol))
            if key == 'ANATOMY':
                names.extend(('anatomy_sample.log', 'anatomy_unresolved.log'))
                data['anatomy'] = read(directory / 'anatomy.json')
            data['report_artifacts'] = [name for filename in names
                                        if (name := store_artifact(directory / filename, output))]
        (fixtures if is_fixture else trials)[key] = data
    return {'build': label, 'limits': LIMITS, 'fixtures': fixtures, 'trials': trials}


def flatten(value, path=''):
    if isinstance(value, dict):
        for key, item in value.items():
            yield from flatten(item, f'{path} / {key}' if path else str(key))
    elif isinstance(value, list):
        for i, item in enumerate(value, 1):
            yield from flatten(item, f'{path} / {i}')
    else:
        yield path, value


def display(value):
    if value is None:
        return ''
    if isinstance(value, float):
        return f'{value:.8g}'
    return str(value).replace('|', '\\|').replace('\n', ' ')


def matrix(columns, values):
    ordered = dict.fromkeys(key for column in values for key in column)
    return ['| Field | ' + ' | '.join(columns) + ' |',
            '| --- |' + ' ---: |' * len(columns),
            *('| ' + display(key) + ' | ' + ' | '.join(display(column.get(key)) for column in values) + ' |'
              for key in ordered), '']


def status(data):
    fixtures = data['fixtures']
    trials = data['trials']
    if any(row['status'] != 'done' for row in fixtures.values()):
        return 'FAILED'
    if any(row['status'] in ('FAILED', 'missing') for row in trials.values()):
        return 'FAILED'
    if any(row.get('observer', {}).get('completion') == 'incomplete'
           and not row.get('observer', {}).get('reason') for row in trials.values()):
        return 'FAILED'
    for row in trials.values():
        if row['status'] != 'done':
            continue
        observation = row['observations']
        if len(observation['stage']) != 10 or observation['observer'].get('status') != 'complete':
            return 'FAILED'
    if any(row['status'] == 'OPEN' for row in trials.values()):
        return 'OPEN'
    return 'done'


def render(data):
    trials, fixtures = data['trials'], data['fixtures']
    lines = [f"# FP2-M2 build{data['build']}", '', data['label'], '', LIMITS, '',
             'Elapsed time of the traced trial is reported beside the baseline. Cost comparisons use '
             'trace-off forwards. The step clocks are instrumented. All percentile ranks use '
             'the ceiling of the requested fraction of the sample count.', '',
             'Source scope and readings are recorded in the whole-item check. [Source check](../source_check.md)', '',
             '## Part status', '']
    fields = ['status', 'evaluate_qualified', 'proof_qualified', 'proof_accepted']
    named = {**fixtures, **trials}
    lines.extend(matrix(list(named), [{**{k: row.get(k) for k in fields},
        'identity': row.get('identity', {}).get('status'), 'table completion': row.get('observer', {}).get('completion'),
        'reason': row.get('observer', {}).get('reason') or row.get('reason'),
        'selected receipt': row.get('used'), 'first difference': row.get('first_difference')}
        for row in named.values()]))
    lines.extend(['## A. Proof stages and anatomy', ''])
    base = trials['BASELINE'].get('observations', {})
    anatomy = trials['ANATOMY'].get('anatomy') or {}
    lines.extend(matrix(['Build664 N1', 'Baseline', 'Anatomy'], [
        {'elapsed seconds': data['build664_n1_seconds']},
        {'elapsed seconds': base.get('wall_seconds')},
        {'elapsed seconds': trials['ANATOMY'].get('observations', {}).get('wall_seconds')},
    ]))
    rows = base.get('stage', [])
    if rows:
        lines.extend(matrix([f"Bin {row['bin']} direct {row['direct_eligible']}" for row in rows],
                            [dict(flatten(row)) for row in rows]))
    if groups := anatomy.get('groups'):
        lines.extend(['### Step C forwards by length and eligibility', ''])
        lines.extend(matrix(list(groups), [dict(flatten(group)) for group in groups.values()]))
    if phases := anatomy.get('phases'):
        lines.extend(['### Phases and unmarked queries', ''])
        lines.extend(matrix(list(phases), [dict(flatten(phase)) for phase in phases.values()]))
    if anatomy.get('stream'):
        lines.extend(matrix(['Trace stream'], [dict(flatten(anatomy['stream']))]))
    lines.extend(['## B. Trace-off tolerances', ''])
    observations = [trials[key].get('observations', {}) for key in TRACE_OFF]
    lines.extend(matrix(list(TRACE_OFF.values()), [dict(flatten({key: value for key, value in obs.items()
                                                                if key != 'stage'})) for obs in observations]))
    for bin in range(5):
        for direct in range(2):
            lines.extend([f'### Stage shares, bin {bin}, direct eligible {direct}', ''])
            lines.extend(matrix(list(TRACE_OFF.values()), [dict(flatten(next((row for row in obs.get('stage', [])
                if row['bin'] == bin and row['direct_eligible'] == direct), {}))) for obs in observations]))
    lines.extend(['## C. Hand-over geometry', ''])
    for key, tolerance in TRACE_OFF.items():
        trial = trials[key]
        lines.extend([f'### Tolerance {tolerance}', ''])
        if bins := trial.get('bins'):
            lines.extend(matrix(list(bins), [dict(flatten(row)) for row in bins.values()]))
        else:
            lines.extend([trial.get('observer', {}).get('reason') or trial.get('reason') or 'No complete CSV pair.', ''])
        if artifacts := trial.get('report_artifacts'):
            lines.extend([' · '.join(f'[{name}]({name})' for name in artifacts), ''])
    lines.extend(['## D. Reference model', ''])
    model = fixtures['model']
    if bins := model.get('bins'):
        lines.extend(matrix(list(bins), [dict(flatten(row)) for row in bins.values()]))
    if details := model.get('details', {}).get('model'):
        lines.extend(matrix(['Model reconciliation'], [dict(flatten(details))]))
    artifacts = model.get('report_artifacts', []) + trials['ANATOMY'].get('report_artifacts', [])
    if artifacts:
        lines.extend([' · '.join(f'[{name}]({name})' for name in artifacts), ''])
    return '\n'.join(lines).rstrip() + '\n'


def report(label):
    output = HERE / 'reports'
    output.mkdir(exist_ok=True)
    data = collect(label, output)
    baseline = read(HERE.parent / 'FP2_C3/cases/build664/N1/measurement.json')
    data['build664_n1_seconds'] = baseline['wait']['elapsed_seconds']
    data['label'] = f'FP2-M2: measurement {status(data)}; default path not yet proven'
    name = output / f'FP2_M2_{label}'
    name.with_suffix('.json').write_text(json.dumps(data, indent=2, allow_nan=False) + '\n')
    name.with_suffix('.md').write_text(render(data))
    return data


def self_check():
    assert histogram([0, 1, 2, 2.01, 6], (1, 2, 5)) == {'<= 1': 2, '(1, 2]': 1, '(2, 5]': 1, '> 5': 1}
    assert quantiles([3, 2, 1, 0]) == {'n': 4, 'median': 1, 'p90': 3, 'max': 3}
    sample = face_metrics([{'m_max': '.6', 'sample_max': '.00006', 'finite0': '0', 'eval0': 'ok'}], .0001)
    assert sample['m_max_below_0.5'] == 0 and sample['m_max_below_0.9'] == 1
    assert sample['some_sample_nonfinite'] == 1 and sample['corners_ok'] == 1
    assert share(1, 0) is None
    print('FP2-M2 report histogram, nearest-rank and shares checks passed.')


if __name__ == '__main__':
    cli = ArgumentParser(description=__doc__)
    cli.add_argument('build', type=int, nargs='?')
    cli.add_argument('--self-check', action='store_true')
    opts = cli.parse_args()
    if opts.self_check:
        self_check()
    elif opts.build:
        print(report(opts.build)['label'])
    else:
        cli.error('a build number is required')
