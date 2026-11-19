#!/usr/bin/env python3
"""Render FP0 from the test receipts and the pinned G5 rank logs."""

import argparse
from contextlib import redirect_stdout
import gzip
from hashlib import sha256
from io import StringIO
import json
import os
from pathlib import Path
import unittest

from fixed_parts import G5, fallback, fixed_parts, load_events

ITEM = Path(__file__).resolve().parent
CASES = ('kernels', 'optimize_10000', 'optimize_100000', 'census', 'proof', 'float_surface')
DASH = '—'


def read(path):
    return json.loads(path.read_text()) if path.is_file() else {}


def link(path, directory, label=None):
    return f'[{label or path.name}]({os.path.relpath(path, directory)})'


def number(value):
    if value is None:
        return DASH
    if isinstance(value, bool):
        return 'yes' if value else 'no'
    if isinstance(value, int):
        return f'{value:,}'
    return f'{value:.9g}' if isinstance(value, float) else str(value)


def timing(seconds, items=None):
    if seconds is None:
        return DASH
    result = f'{seconds:.9g} s'
    if items:
        result += f' / {seconds * 1e9 / items:.6g} ns'
    return result


def md(text):
    """Keep literal error messages and per-entity names in their table cell."""
    return str(text).replace('|', r'\|').replace('\n', ' ').replace('\r', ' ')


def table_row(*columns):
    return '| ' + ' | '.join(map(md, columns)) + ' |'


def fields(row, mapping):
    return ', '.join(f'{label} {number(row[key])}' for key, label in mapping if key in row) or DASH


def load_measurements():
    """Verify each retained log against its terminal fixture receipt."""
    receipts, rows = {}, {}
    for case in CASES:
        folder = ITEM / 'cases/A' / case
        path = folder / 'measurement.json'
        if not path.is_file():
            continue
        receipt = read(path)
        log = folder / 'test_outer.log'
        archive = log.with_suffix('.log.gz')
        if archive.is_file():
            log = archive
        data = gzip.decompress(log.read_bytes()) if log.suffix == '.gz' else log.read_bytes()
        if receipt['case'] != case or sha256(data).hexdigest() != receipt['log_sha256']:
            raise ValueError(f'{case}: fixture log differs from its sealed measurement receipt')
        receipts[case] = (receipt, path, log)
        for row in receipt['rows']:
            key = row['measurement']
            if key.startswith('A9'):
                key = f'{case}:{key}'
            if key in rows:
                raise ValueError(f'{case}: duplicate measurement {key}')
            rows[key] = row
    return receipts, rows


def kernel_rows(rows):
    """Put matching strict and double stages on the same item count."""
    result = []
    a1 = rows.get('A1', {})
    cells, points = a1.get('cells'), a1.get('points')
    result.extend([
        table_row('A1 ImportGeometry per cell', number(cells), timing(a1.get('import_seconds'), cells), DASH,
                  fields(a1, (('points', 'points'), ('surface_faces', 'surface faces')))),
        table_row('A1 copy points and cells, per cell', number(cells), DASH, timing(a1.get('point_cell_seconds'), cells),
                  fields(a1, (('point_bytes', 'point-array B'), ('cell_bytes', 'cell-array B')))),
        table_row('A1 face incidence, neighbors and markers', number(cells * 4 if cells is not None else None),
                  DASH, timing(a1.get('face_seconds'), cells * 4 if cells is not None else None),
                  fields(a1, (('neighbor_bytes', 'neighbor B'), ('marker_bytes', 'marker B'),
                              ('bytes', 'total B'), ('bytes_per_cell', 'B/cell')))),
    ])
    strict, double = rows.get('A2', {}), rows.get('A3', {})
    for axis, key, kind, left, right in (
        ('orientation', 'orientation_seconds', 'cells', (('filtered', 'filtered'), ('exact', 'exact')),
         (('sign_differences', 'different signs'),)),
        ('q and J', 'shape_seconds', 'cells', (),
         (('max_q_difference', 'max |Δq|'), ('max_j_difference', 'max |ΔJ|'))),
        ('closed metric length', 'edge_seconds', 'edges', (), (('max_length_difference', 'max |Δlength|'),)),
    ):
        count = strict.get(kind, double.get(kind))
        if kind in strict and kind in double and strict[kind] != double[kind]:
            raise ValueError(f'A2/A3: inconsistent {kind} denominator')
        detail = {**strict, **double}
        result.append(table_row(f'A2/A3 {axis}', number(count), timing(strict.get(key), count),
                                timing(double.get(key), count), fields(detail, left + right)))
    a4 = rows.get('A4', {})
    samples = a4.get('samples')
    result.append(table_row('A4 closed formula and TargetEdgeLength', number(samples),
                            timing(a4.get('seconds'), samples), DASH,
                            fields(a4, (('max_relative_difference', 'max relative difference'),))))
    result.append(scan_row('A5 input threshold scan', rows.get('A5', {})))
    a6 = rows.get('A6', {})
    result.extend([
        table_row('A6 point stars', number(a6.get('points', points)), DASH,
                  timing(a6.get('stars_seconds'), a6.get('points')),
                  fields(a6, (('point_cell_incidence', 'point-cell incidences'), ('max_star', 'largest star')))),
        table_row('A6 unique edge rings', number(a6.get('edges')), DASH,
                  timing(a6.get('rings_seconds'), a6.get('edges')),
                  fields(a6, (('max_ring', 'largest ring'), ('seconds', 'combined neighborhood wall s')))),
    ])
    return result


def scan_row(label, row):
    count = row.get('edges', 0) + row.get('cells', 0)
    return table_row(label, number(count) if count else DASH, DASH, timing(row.get('seconds'), count),
                     fields(row, (('points', 'points'), ('cells', 'cells'), ('edges', 'edges'))))


def edit_rows(rows):
    lines = []
    for key, count, rate, label, options in (
        ('A7', 'splits', 'microseconds_per_split', 'interior splits',
         (('refused_orientation', 'orientation refusals'), ('deferred', 'deferred'))),
        ('A8', 'collapses', 'microseconds_per_collapse', 'interior collapses',
         (('refused', 'refusals'), ('refused_orientation', 'orientation refusals'),
          ('refused_length', 'length refusals'), ('deferred', 'deferred'))),
    ):
        row = rows.get(key, {})
        accepted = row.get(count)
        detail = fields(row, (('passes', 'passes'),) + options)
        if accepted and rate in row:
            detail += f', {number(row[rate])} µs/{count[:-1]}'
        lines.extend([
            table_row(f'{key} {label}', number(accepted), DASH, timing(row.get('seconds'), accepted), detail),
            scan_row(f'{key} post-edit scan', rows.get(f'{key}_post', {})),
        ])
    return lines


def region_rows(rows):
    lines = []
    for desired in (10_000, 100_000):
        case = f'optimize_{desired}'
        row = rows.get(f'{case}:A9', {})
        setup = rows.get(f'{case}:A9_setup', {})
        count = row.get('cells', setup.get('cells'))
        timeout = row.get('stop') == 'outer_time_limit'
        setup_error = row.get('error') and row.get('place') != 'OptimizeMetricMesh' and not timeout
        clock = timing(row.get('seconds'), None if timeout or setup_error else count)
        if timeout:
            clock += ' (wall limit including setup)'
        if setup_error:
            clock += ' (setup)'
        ratio = f", {number(row['ms_per_cell'])} ms/cell" if not timeout and not setup_error and 'ms_per_cell' in row else ''
        detail = fields({**row, **{f'setup_{key}': value for key, value in setup.items()}}, (
            ('final_cells', 'final cells'), ('bad_before', 'bad before'), ('bad_after', 'bad after'),
            ('moves', 'moves'), ('reconnects', 'reconnects'), ('trials', 'trials'), ('work_limit', 'work limit'),
            ('setup_center_id', 'center point'), ('setup_center_incident_bad', 'center incident bad cells'),
            ('setup_fixed_vertices', 'fixed boundary vertices'), ('setup_radius', 'radius'),
            ('setup_seconds', 'setup s'), ('total_seconds', 'setup + optimizer s')))
        lines.append(table_row(f'A9 optimizer, target {desired:,} cells', number(count), clock, DASH, detail + ratio))
    return lines


def proof_rows(rows):
    """Report separate strict calls and the sampled double cubic evaluation."""
    a10, a11, a12 = (rows.get(key, {}) for key in ('A10', 'A11', 'A12'))
    a10_progress = rows.get('A10_progress', {})
    count = a10.get('cells', a10_progress.get('cells'))
    partial = bool(a10.get('error'))
    clock = timing(a10.get('seconds'), None if partial else count)
    reverse_faces = len(a11.get('reverse_bounds', []))
    certified_faces = a11.get('faces') if not a11.get('error') else None
    return [
        table_row('A10 strict current-nodal census, ≤128 cells per batch', number(count), clock, DASH,
                  fields(a10, (('q_bad', 'q-bad cells'), ('j_bad', 'J-bad cells'), ('long_cells', 'long-edge cells')))),
        table_row('A11 reverse surface bounds', number(a11.get('faces')),
                  timing(a11.get('reverse_seconds'), reverse_faces), DASH,
                  fields(a11, (('reverse_work', 'work'), ('reverse_direct_calls', 'direct calls'),
                               ('reverse_direct_work', 'direct work'), ('reverse_full_calls', 'full calls'),
                               ('reverse_full_work', 'full work')))),
        table_row('A11 complete geometry certificates', number(a11.get('faces')),
                  timing(a11.get('certificate_seconds') if reverse_faces else None, certified_faces),
                  DASH, fields(a11, (('certificate_work', 'work'), ('certificate_full_calls', 'full calls'),
                                    ('certificate_full_work', 'full work'), ('max_work', 'work ceiling'),
                                    ('work_exhausted', 'work exhausted'), ('peak_atlas_bytes', 'peak atlas B')))),
        table_row('A12 cubic-order comparison with EvaluateSurface', number(a12.get('order_samples')),
                  timing(a12.get('order_seconds'), a12.get('order_samples')), DASH,
                  fields(a12, (('order_mismatches', 'order mismatches'), ('order_max_difference', 'max difference')))),
        table_row('A12 four double samples per face', number(a12.get('faces')), DASH,
                  timing(a12.get('seconds'), a12.get('faces')),
                  fields(a12, (('setup_seconds', 'sample setup s'),))),
    ]


def scan_counts(rows):
    names = (('long_edges', 'Edges > 1.8'), ('short_edges_09', 'Edges < 0.9'), ('short_edges_05', 'Edges < 0.5'),
             ('q_bad', 'Cells q < 0.20'), ('j_bad', 'Cells J < 0.05'), ('shape_bad', 'Cells q or J bad'),
             ('long_cells', 'Cells with an edge > 1.8'))
    lines = ['### A5, A7 and A8 threshold scans', '',
             'Each entry is total / touches the surface / interior. Touch means at least one surface point.', '',
             '| Count | A5 input | A7 after split | A8 after collapse |', '|---|---|---|---|']
    for key, name in names:
        triplets = [rows.get(measurement, {}).get(key) for measurement in ('A5', 'A7_post', 'A8_post')]
        lines.append(table_row(name, *(' / '.join(map(number, values)) if values is not None else DASH
                                     for values in triplets)))
    strict = rows.get('A10', {})
    lines += ['', '### A10 strict versus A5 double input counts', '',
              '| Cell count | Strict A10 | Double A5 | Difference (strict − double) |',
              '|---|---:|---:|---:|']
    for key, name in names[3:5] + names[-1:]:
        left, right = strict.get(key), rows.get('A5', {}).get(key, [None])[0]
        delta = None if left is None or right is None else left - right
        lines.append(table_row(name, number(left), number(right), number(delta)))
    return lines


def surface_results(rows):
    lines = ['### A11 strict surface bounds', '']
    strict, double = rows.get('A11', {}), rows.get('A12', {})
    reverse = strict.get('reverse_bounds', [])
    finite = [value for value in reverse if value is not None]
    if strict:
        lines.append(f'Owned reverse rows {len(reverse):,}, finite {len(finite):,}. '
                     f'Maximum finite bound {number(max(finite, default=None))}. '
                     f'Total timed wall {timing(strict.get("seconds"))}.')
    else:
        lines.append('No sealed strict surface measurement row.')
    entities = strict.get('entities', [])
    if entities:
        lines += ['', '| Body / sheet / marker | Within tolerance | Outside tolerance | Mesh to model | '
                  'Model to mesh | Physical / model pieces |', '|---|---|---|---:|---:|---:|']
        for row in entities:
            names = ' / '.join(str(row[key]) for key in ('body', 'sheet', 'marker'))
            pieces = ' / '.join(number(row.get(key)) for key in ('physical_pieces', 'model_pieces'))
            lines.append(table_row(names, number(row.get('within_tolerance')), number(row.get('outside_tolerance')),
                                   number(row.get('mesh_to_model')), number(row.get('model_to_mesh')), pieces))
    else:
        lines.append('No per-entity certificate rows were emitted.')
    lines += ['', '### A12 sampled distances', '',
              '| Face group | Faces | Maximum | Mean | Faces > 0.5e-4 | Faces > 1e-4 |',
              '|---|---:|---:|---:|---:|---:|']
    for key, name in (('all', 'All'), ('anchor_forward_bindings', 'Facet bindings'),
                      ('flat_anchor_projection', 'Projected to reference anchor')):
        row = double.get(key, {})
        values = ('faces', 'max_distance', 'mean_distance', 'above_0_5e_4', 'above_1e_4')
        lines.append(table_row(name, *(number(row.get(field)) for field in values)))
    lines.append('Distance rows use four samples per face. Strict two-sided bounds are shown under A11.')
    if 'projection_retries' in double:
        lines.append(f"The thin-triangle projection was used for {double['projection_retries']} of the sampled distances.")
    return lines


def fixture_section(rows, receipts, directory):
    headline = ['## Part A — one rank', '',
                'Each timed cell shows wall seconds and nanoseconds per item. The untimed dash denotes an '
                'unavailable or inapplicable value. A4 and the A12 order check include both paths. A3 loops '
                'include the comparison to the saved A2 values.', '',
                '| Measurement | Items | Strict or comparison | Double or test-only | Observations |',
                '|---|---:|---|---|---|']
    lines = headline + kernel_rows(rows) + edit_rows(rows) + region_rows(rows) + proof_rows(rows)
    lines += ['', *scan_counts(rows), '', *surface_results(rows)]
    lines += ['', '### Fixture exits and process peaks', '',
              '| Fixture | Build | Wrapper wall s | Peak KiB | Evidence |', '|---|---:|---:|---:|---|']
    for case in CASES:
        if case in receipts:
            receipt, path, log = receipts[case]
            evidence = link(path, directory, 'measurement') + ' · ' + link(log, directory, 'raw log')
            lines.append(table_row(case, number(receipt.get('build', 593)), number(receipt['elapsed_seconds']),
                                   number(receipt['peak_kib']), evidence))
        else:
            lines.append(table_row(case, DASH, DASH, DASH, 'No sealed fixture receipt'))
    exits = [(key, row) for key, row in rows.items() if row.get('stop') or row.get('error')]
    if exits:
        lines.append('')
        for key, row in exits:
            details = fields(row, (('place', 'at'), ('stop', 'stop'), ('error', 'message')))
            lines.append(f'- {md(key)} — {md(details)}')
    return lines


def tested_bundle(build, directory, audit):
    path = ITEM / f'build{build}.json'
    state = read(path)
    source_path, reuse_path = ITEM / 'n21_source_view.json', ITEM / 'object_reuse.json'
    source, reuse = read(source_path), read(reuse_path)
    inputs = ITEM / 'case_input_manifest.json'
    manifest = read(inputs)
    counts = manifest.get('A', {}).get('counts', {})
    result = [f'# FP0, build{build}', '',
              'Test-only ONERA M6 measurements on the N21 Part P accepted event-one mesh with the event-two target. '
              + link(ITEM / 'inputs/FAST_PATH_STAGE0.md', directory, 'Pinned order')
              + ' · ' + link(inputs, directory, 'Input manifest'), '',
              f"Part A has {number(counts.get('points'))} points, {number(counts.get('cells'))} tetrahedra and "
              f"{number(counts.get('surface_faces'))} surface faces.", '',
              '## Test build and gates', '',
              'The FP0 changes are confined to UnitTests and the evidence folder.', '',
              f"Isolated accepted N21 source `{source.get('base', DASH)}`, with "
              f"{len(source.get('verified', {})):,} verified source and dependency entries and "
              f"{len(reuse.get('reused_objects', [])):,} reused objects. "
              + link(source_path, directory, 'Source view') + ' · ' + link(reuse_path, directory, 'Object reuse')]
    if state:
        warnings = len(state.get('warnings', []))
        new_warnings = len(state.get('new_warnings', state.get('warnings', [])))
        result += ['', f"Build gate {'passed' if state['passed'] else 'failed'}. Observed compiler warnings "
                   f"{warnings:,}, newly introduced warnings {new_warnings:,}. "
                   f"Test binary SHA256 `{state.get('test_binary_sha256', DASH)}`. "
                   + link(path, directory, 'Build receipt')]
    else:
        result += ['', 'No sealed FP0 build receipt.']
    result += ['', '| G1 cohort | Case records | Case-table identity | Reference / process exit | Wrapper s |',
               '|---|---:|---|---|---:|']
    for cohort in ('serial', 'mpi2', 'mpi4'):
        comparison = ITEM / 'G1' / cohort / 'comparison.json'
        gate = read(comparison)
        status = ('Passed' if gate['identity_pass'] else 'Failed') if gate else 'Unevaluated'
        result.append(table_row(cohort, number(gate.get('records')),
                                link(comparison, directory, status) if gate else status,
                                f"{number(gate.get('reference_returncode'))} / {number(gate.get('candidate_returncode'))}",
                                number(gate.get('elapsed_seconds'))))
    result += ['', 'G1 compares case status and failed assertions. The FP0 measurements are separate hidden tests.']
    if audit:
        terminal = read(audit)
        if not terminal or terminal['build'] != build:
            raise ValueError('Terminal audit is missing or corresponds to a different build')
        result += ['', f"Terminal audit verified {len(terminal['identities']):,} identities and "
                   f"{terminal['source_pins_verified']:,} source pins. All owned handles terminal "
                   f"{number(terminal['all_owned_handles_terminal'])}. "
                   f"Active production solver SHA256 `{terminal['active_production_solver_sha256']}`. "
                   + link(audit, directory, 'Final terminal audit')]
    return result


def existing_lines(directory, fallback_note):
    """Read the actual B results or an explicitly approved section 4 fallback."""
    actual = ITEM / 'cases/B/measurements.md'
    events = load_events(G5)
    output = StringIO()
    with redirect_stdout(output):
        if actual.is_file():
            print(actual.read_text().rstrip())
            print()
            print(link(actual, directory, 'Part B measurement receipt'))
        elif fallback_note:
            print('## Part B disposition')
            print()
            print(fallback_note.read_text().rstrip())
            print()
            print(link(fallback_note, directory, 'Fallback authorization and evidence'))
            print()
            fallback(events[1], G5)
        else:
            print('## Part B')
            print()
            print('No sealed replay and standalone Refine measurement receipt.')
        print()
        fixed_parts(events)
        print()
        print(link(G5 / 'full_workflow_receipt.json', directory, 'N21 Part P G5 receipt'))
    return output.getvalue().splitlines()


def render(build, path, audit, fallback_note):
    directory = path.parent
    receipts, rows = load_measurements()
    lines = tested_bundle(build, directory, audit)
    lines += ['', *fixture_section(rows, receipts, directory), '', *existing_lines(directory, fallback_note)]
    corrections = ITEM / 'r3_notes.md'
    if corrections.is_file():
        lines += ['', '## R3 corrections', '']
        notes = corrections.read_text().rstrip().splitlines()
        lines += notes[1:] if notes and notes[0].startswith('# ') else notes
        lines += ['', link(corrections, directory, 'R3 correction log')]
    return '\n'.join(lines) + '\n'


class RendererCheck(unittest.TestCase):
    def test_counts_and_unavailable_durations(self):
        rows = {'A2': {'cells': 2, 'orientation_seconds': 2e-6, 'filtered': 2, 'exact': 0},
                'A3': {'cells': 2, 'orientation_seconds': 1e-6, 'sign_differences': 0},
                'A5': {'q_bad': [3, 2, 1], 'j_bad': [2, 1, 1], 'long_cells': [5, 3, 2]},
                'A10': {'q_bad': 3, 'j_bad': 3, 'long_cells': 5},
                'optimize_100000:A9': {'cells': 100000, 'seconds': 900, 'stop': 'outer_time_limit'}}
        orientation = next(row for row in kernel_rows(rows) if 'A2/A3 orientation' in row)
        self.assertIn('2e-06 s / 1000 ns', orientation)
        self.assertIn('1e-06 s / 500 ns', orientation)
        self.assertIn('| Cells J < 0.05 | 3 | 2 | 1 |', scan_counts(rows))
        self.assertIn('900 s (wall limit including setup)', region_rows(rows)[1])
        self.assertNotIn('ns', region_rows(rows)[1])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=int, default=592)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--audit', type=Path)
    parser.add_argument('--fallback-note', type=Path)
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        result = unittest.TextTestRunner().run(unittest.defaultTestLoader.loadTestsFromTestCase(RendererCheck))
        return 0 if result.wasSuccessful() else 1
    path = (args.output or ITEM / f'reports/FP0_{args.build}.md').resolve()
    text = render(args.build, path, args.audit, args.fallback_note)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('x') as output:
        output.write(text)
    print(path)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
