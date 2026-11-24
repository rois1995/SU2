"""Verify FP2-M3 receipts and assemble the report without collecting raw provenance."""

from argparse import ArgumentParser
from collections import Counter
import csv
import hashlib
import json
from pathlib import Path
import shutil
import sys

HERE = Path(__file__).resolve().parent
M2 = HERE.parent / 'FP2_M2'
sys.path.insert(0, str(M2))
from report_m2 import flatten, matrix, read  # noqa: E402

FIXTURES = ('G1', 'off-serial', 'off-mpi', 'curved')
TRIALS = ('TRACE-OFF', 'TRACE-ON')
DEFERRED = ('No-BL first-collapse work and count', 'Exact saved-state C492 BL repeat',
            'C481 and the current BL target', 'Native 2D with BL at 1, 2 and 4 ranks',
            'Native 2D without BL at 1, 2 and 4 ranks', 'Full two-event no-BL workflow milestone',
            'G2', 'GQ', 'G3', 'G5', 'S')


def sha(path):
    with path.open('rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()


def copy(path, output, digest=None):
    if digest is not None:
        assert sha(path) == digest, path
    target = output / path.name
    if path.resolve() != target.resolve():
        shutil.copyfile(path, target)
    return target.name


def selected(path):
    return read(path.parent / 'r5_retry' / path.name) or read(path) or {
        'status': 'not evaluated', 'reason': 'No sealed result.'}


def read_parts(build, output):
    parts = {}
    for mode in (*FIXTURES, *TRIALS):
        branch = 'trials' if mode in TRIALS else 'fixtures'
        record = selected(HERE / branch / f'build{build}' / mode / 'case_summary.json')
        parts[mode] = record
        if not record.get('used'):
            continue
        path = Path(record['used'])
        assert sha(path) == record['attempts'][str(path)], path
        measure = read(path)
        for relative, digest in measure['artifacts'].items():
            present = path.parent / relative
            assert sha(present) == digest, present
        if mode not in TRIALS:
            continue
        record['elapsed_seconds'] = measure['wait']['elapsed_seconds']
        record['peak_lower_bound_bytes'] = measure['peak_lower_bound_bytes']
        record['proof'] = measure['measurement']['proof']
        record['forward_rows'] = measure['rows_by_rank'].get('0', {}).get('STAGE', [])
        if mode == 'TRACE-OFF':
            present = path.with_name('facet_vertices.csv')
            if present.is_file():
                record['facet_export'] = copy(present, output, record['observer']['artifacts'][str(present)])
                counters = {str(i): Counter() for i in range(7)}
                with present.open(newline='') as source:
                    for row in csv.DictReader(source):
                        counters[row['bin']]['positive' if int(row['retained_pieces']) else 'zero'] += 1
                record['retained_piece_snapshot'] = {
                    k: {'facets': sum(c.values()), 'zero': c['zero'], 'positive': c['positive']}
                    for k, c in counters.items()}
        if mode == 'TRACE-ON':
            record['anatomy'] = read(path.with_name('anatomy.json'))
    return parts


def m2_compare(new, old):
    if not new or not old:
        return {}
    result = {}
    for bin, group in new['groups'].items():
        result[bin] = {}
        for label, dataset in (('M2', old), ('M3', new)):
            row = dataset['groups'][bin]
            keys = row['miss_bypass_denominators']
            result[bin][label] = {'forwards': keys['forwards'], 'prepared_nodes': keys['prepared_nodes'],
                                  'root_enough': row['root_enough'], 'result_work': keys['result_work']}
    return result


def status(parts, analysis):
    states = {part['status'] for part in parts.values()} | {analysis}
    if 'FAILED' in states:
        return 'FAILED'
    if states & {'OPEN', 'not evaluated'}:
        return 'OPEN'
    return 'done'


def render(data):
    parts = data['parts']
    lines = [f"# FP2-M3 build{data['build']}", '', data['label'], '',
             f"Code base {data['base']}. The production trace patch has SHA256 {data['production_diff_sha256']}. "
             'The new rows run only with the geometry work trace enabled. '
             'The measured classes describe the assembled witness. They do not establish a counterfactual bound. '
             '[Production diff](../production_trace.diff) · [Source check](../source_check.md)', '',
             '## Result gates and measurement completion', '']
    lines.extend(matrix(list(parts), [{k: row.get(k) for k in ('status', 'result_status', 'evaluate_qualified',
        'elapsed_seconds', 'peak_lower_bound_bytes', 'first_difference', 'used')}
        | {'identity': row.get('identity', {}).get('status'),
           'measurement': row.get('observer', {}).get('completion'),
           'reason': row.get('reason') or row.get('observer', {}).get('reason')}
        for row in parts.values()]))
    if g1 := parts['G1'].get('cohorts'):
        lines.extend(matrix(list(g1), [dict(flatten(row)) for row in g1.values()]))
    if curved := parts['curved'].get('details', {}).get('leaf_pin', {}).get('result'):
        lines.extend(matrix(['Curved fixture leaf pin'], [dict(flatten(curved))]))
    lines.extend(['## Deferred checks', ''])
    lines.extend(f'- {name} — not evaluated' for name in data['deferred'])
    lines.extend(['', '## Observer effect', ''])
    lines.extend(matrix(['FP2-M2 baseline', 'FP2-M3 trace-off', 'FP2-M2 anatomy', 'FP2-M3 trace-on'],
        [{'trial seconds': value} for value in data['observer_seconds']]))
    if provenance := parts['TRACE-ON'].get('observer', {}).get('provenance'):
        lines.extend(matrix(['Compressed provenance'], [dict(flatten(provenance))]))
    if comparison := data['m2_comparison']:
        lines.extend(['### M2 and M3 anatomy cohorts', ''])
        lines.extend(matrix(list(comparison), [dict(flatten(row)) for row in comparison.values()]))
    if anatomy := parts['TRACE-ON'].get('anatomy'):
        lines.extend(['### M2-format anatomy tables', ''])
        for key in ('groups', 'phases'):
            table = anatomy[key]
            lines.extend(matrix(list(table), [dict(flatten(row)) for row in table.values()]))
        lines.extend(matrix(['Anatomy stream'], [dict(flatten(anatomy['stream']))]))
    lines.extend(['## Facet export', ''])
    if facets := parts['TRACE-OFF'].get('retained_piece_snapshot'):
        lines.extend(matrix(list(facets), list(facets.values())))
        lines.extend(['These counts are the retained cache snapshot after the digest. '
                      '[Facet vertices](facet_vertices.csv)', ''])
    lines.extend(['## Witness analysis', ''])
    lines.extend([f"Status {data['analysis_status']}.", ''])
    if data.get('analysis_reason'):
        lines.extend([data['analysis_reason'], ''])
    if info := data.get('analysis'):
        for key, rows in info.items():
            lines.extend([f'### {key.replace("_", " ").capitalize()}', ''])
            if isinstance(rows, dict) and rows and all(isinstance(v, dict) for v in rows.values()):
                lines.extend(matrix(list(rows), [dict(flatten(row)) for row in rows.values()]))
            else:
                lines.extend(matrix(['Value'], [dict(flatten(rows))]))
    if data.get('examples'):
        lines.extend([f"[Witness examples]({data['examples']})", ''])
    return '\n'.join(lines).rstrip() + '\n'


def self_check():
    assert status({'one': {'status': 'done'}}, 'done') == 'done'
    assert status({'one': {'status': 'not evaluated'}}, 'done') == 'OPEN'
    assert status({'one': {'status': 'FAILED'}}, 'OPEN') == 'FAILED'
    print('FP2-M3 report label precedence verified.')


def report(build, analysis_path=None, examples_path=None):
    output = HERE / 'reports'
    output.mkdir(exist_ok=True)
    parts = read_parts(build, output)
    old = read(M2 / 'reports/FP2_M2_667.json')['trials']
    off, on = (parts[mode] for mode in TRIALS)
    analysis = read(analysis_path) if analysis_path else None
    reason = None
    if analysis:
        provenance = on['observer']['provenance']
        assert analysis['stream']['sha256'] == provenance['sha256']
        assert analysis['stream']['bytes'] == provenance['bytes']
        analysis_status = 'done' if analysis['completion'] == 'complete' else 'FAILED'
    elif not off.get('facet_export'):
        analysis_status, reason = 'not evaluated', 'The trace-off facet export is unavailable.'
    elif not on.get('observer', {}).get('provenance'):
        analysis_status, reason = 'not evaluated', 'The trace-on provenance stream is unavailable.'
    else:
        analysis_status, reason = 'FAILED', 'No validated witness analysis is available.'
    data = {'build': build, 'base': 'd9357fcb73', 'production_diff_sha256': sha(HERE / 'production_trace.diff'),
            'parts': parts, 'analysis': analysis, 'analysis_status': analysis_status, 'analysis_reason': reason,
            'deferred': DEFERRED,
            'm2_comparison': m2_compare(on.get('anatomy'), old['ANATOMY'].get('anatomy')),
            'observer_seconds': [old['BASELINE']['observations']['wall_seconds'], off.get('elapsed_seconds'),
                                 old['ANATOMY']['observations']['wall_seconds'], on.get('elapsed_seconds')],
            'examples': copy(examples_path, output) if examples_path and examples_path.is_file() else None}
    data['label'] = f'FP2-M3: measurement {status(parts, analysis_status)}; default path not yet proven'
    path = output / f'FP2_M3_{build}'
    path.with_suffix('.json').write_text(json.dumps(data, indent=2, allow_nan=False) + '\n')
    path.with_suffix('.md').write_text(render(data))
    return data


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int, nargs='?')
    parser.add_argument('--analysis', type=Path)
    parser.add_argument('--examples', type=Path)
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
    elif args.build:
        print(report(args.build, args.analysis, args.examples)['label'])
    else:
        parser.error('a build number is required')
