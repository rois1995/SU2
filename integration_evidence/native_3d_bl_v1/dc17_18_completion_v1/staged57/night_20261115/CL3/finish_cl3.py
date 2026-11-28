"""Validate and report a terminal CL3 run beside the sealed CL2 evidence."""

from argparse import ArgumentParser
import json
import os
import subprocess
import sys

from run_cl3 import HERE, inherited, reference, standing_build
from diagnostics import EXIT_COUNTS, PHASES, cap_coordinates, describe
from finish_cl2 import DEF1, ORDER, rank_peaks
import report_cl2 as report
from runtime_checks import annotate_native, marker_faces
from validate import validate


def compare_repeat(launch, result):
    old = launch.get('original_attempt')
    if old is None:
        return None
    path = inherited.checked(old['report']['path'], old['report']['sha256'])
    defects = inherited.load(path)['tolerance_wiring']['defects']
    required = {(row['cycle'], row['check']) for row in defects}
    assert required
    events = result['events']
    checks = [{'cycle': event, 'check': check,
               'status': events.get(event, {}).get(check, 'not evaluated'), 'reason': None}
              for event, check in sorted(required)]
    status = ('FAILED' if result['wiring_defect'] else
              'passed' if all(check['status'] == 'passed' for check in checks) else 'OPEN')
    return {'status': status, 'required_checks': checks}


def render_evidence(data):
    lines = ['## Marker, seam and flat-cell observations', '']
    table = report.table
    named = [('CL3', data), ('CL2', data['comparisons']['CL2'])]
    surface_rows = []
    for name, run in named:
        for event in run['events']:
            surface = event.get('surface_mesh', {})
            rows = event['marker_tolerances']['tolerance']
            tolerance = rows[0] if rows else {}
            surface_rows.append((
                name, event['event'], event['outcome'], surface.get('cycle'),
                surface.get('input_farfield_points_retained'), surface.get('circle_points'),
                *(tolerance.get(key) for key in ('seam_corners', 'seam_edges', 'refused_seam_edits'))))
    table(lines, ('Run', 'Event', 'Mesh outcome', 'Mesh cycle shown', 'Input farfield points kept', 'Circle vertices',
                  'Seam corners', 'Seam edges', 'Seam refusals'), surface_rows)
    table(lines, ('Run', 'Event', 'Marker', 'Faces', 'Flat tetrahedra', 'Estimated q below 0.2', 'Estimated minimum q',
                  'Cells without a sampled donor metric'),
          [(name, event['event'], marker, surface['marker_faces'][marker], counts['cells'], counts['q_below_0_2'],
            counts['minimum_q'], counts['unmeasured'])
           for name, run in named for event in run['events'] if (surface := event.get('surface_mesh'))
           for marker, counts in surface['flat'].items()])
    lines.extend([
        'The flat-cell census uses the accepted or retained on-disk mesh. A rejected provisional mesh has no published '
        'connectivity. Its M29 seed is shown separately.',
        'The shock measurement pairs each restart with its actual live mesh, including retained cycles.',
        'The mesh q values are estimates from linear interpolation of the archived donor VTU tensors within a containing '
        'donor tetrahedron. The native cell gate uses a frozen internal field. The M29 seed q comes directly from that gate.',
        '',
    ])
    table(lines, ('Run', 'Event', 'Cap on live mesh', 'Cap vertices kept', 'Estimated live cap q',
                  'Rejected cap q from M29'),
          [(name, event['event'], surface['cap']['present'], surface['cap']['vertices_kept'], surface['cap']['q'],
            surface.get('rejected_cap', {}).get('q'))
           for name, run in named for event in run['events'] if (surface := event.get('surface_mesh'))])
    for name, run in named:
        for event in run['events']:
            rows = event['marker_tolerances']
            if rows['tolerance']:
                lines.extend([f'{name} event {event["event"]} tolerance row', '',
                              *('    ' + row['line'] for row in rows['tolerance']), ''])
            if not rows['final_cert']:
                lines.extend([f'{name} event {event["event"]} final entity proof was not evaluated.', ''])
    table(lines, ('Run', 'Event', 'Entity', 'Marker', 'Faces', 'Mesh to model', 'Model to mesh', 'Tolerance', 'Within'),
          [(name, event['event'], *(row.get(key) for key in
                                    ('entity', 'marker', 'faces', 'mesh_to_model', 'model_to_mesh', 'tolerance', 'within')))
           for name, run in named for event in run['events'] for row in event['marker_tolerances']['final_cert']])
    table(lines, ('Run', 'Event', 'Rank', 'Phase', 'q-bad cells', 'J-bad cells', 'Size-bad cells',
                  'Geometry-bad cells', 'Size debt'),
          [(name, event['event'], rank, row['phase'], *(row[key] for key in EXIT_COUNTS), row['size_debt'])
           for name, run in named for event in run['events'] if event['outcome'] == 'rejected'
           for rank, row in event['worker_exits'].items() if any(row[key] for key in EXIT_COUNTS)])
    for name, run in named:
        for event in run['events']:
            if event['outcome'] == 'rejected' and not event.get('rejected_cells'):
                lines.extend([f'{name} event {event["event"]} logged no M29 seed cell. Individual q, J and '
                              'surface membership are unavailable for that rejection.', ''])
    table(lines, ('Run', 'Event', 'Native ledger s', 'Construction s', 'Quality s (with cleanup)', 'Final proof s', 'Job peak bytes'),
          [(name, event['event'], *(event['phase_seconds']['max_rank'][phase] for phase in ('root_seconds', *PHASES)),
            event.get('job_peak_rss_bytes'))
           for name, run in named for event in run['events']])
    lines.extend(['Each phase is its maximum across the six rank ledgers. Per-rank exclusive times are in the JSON.',
                  ''])
    issues = data['tolerance_wiring']['defects']
    for row in issues:
        lines.extend([f'Event {row["cycle"]} tolerance wiring failed. {row["reason"]}', ''])
    return '\n'.join(lines).rstrip() + '\n'


def measure_live(case, result):
    """Pair each CFD restart with the mesh it actually used, including retained cycles."""
    script = inherited.checked(DEF1 / 'inputs/shock_refinement_measure.py', ORDER['shock_refinement_measure.py'])
    live = case / 'm6.su2'
    events = {row['event']: row for row in result['events']}
    pairs = []
    for cycle in result['cycles']:
        index = cycle['cycle']
        if index:
            event = events[index]
            if event['outcome'] == 'accepted':
                live = case / f'adapted_mesh_adap_{index:05d}.su2'
            elif event['outcome'] == 'rejected':
                cycle['cells'] = event['cells']
            else:
                live = None
        restart = case / f'restart_flow_adap_{index:05d}.dat'
        if live is not None and live.is_file():
            cycle.update(live_mesh=str(live), marker_faces=marker_faces(live))
            if restart.is_file() and restart.stat().st_size:
                pairs.append((index, live, restart))
    output = case / 'shock_measure.jsonl'
    receipt = case / 'shock_measure_receipt.json'
    argv = [sys.executable, '-B', str(script), *(str(path) for _, mesh, restart in pairs for path in (mesh, restart))]
    if receipt.is_file():
        old = inherited.load(receipt)
        assert old['argv'] == argv and old['returncode'] in (0, None)
        inherited.checked(output, old['output']['sha256'])
        return output
    log = case / 'shock_measure.stderr.log'
    with output.open('w') as out, log.open('w') as err:
        code = subprocess.run(argv, cwd=case, stdout=out, stderr=err).returncode if pairs else None
    inherited.save(receipt, {'argv': argv, 'returncode': code, 'cycles': [index for index, _, _ in pairs],
                             'input_pairs': [[inherited.pin(mesh), inherited.pin(restart)]
                                             for _, mesh, restart in pairs],
                             'script': inherited.pin(script), 'output': inherited.pin(output),
                             'stderr': inherited.pin(log)})
    assert code in (0, None), log
    return output


def make_report(case, launch, result, terminal, measured, cl2, q):
    cycles = result['cycles']
    label, finding = report.label_cl1(cycles, result, terminal)
    label = 'CL3' + label[3:]
    if defect := next(iter(result['tolerance_wiring']['defects']), None):
        label = f'CL3: converged loop run FAILED at event {defect["cycle"]}, gate 1'
        finding = defect['reason']
    outputs, findings = report.file_outputs(case, cycles, native_reference=True)
    return {
        'label': label, 'finding': finding, 'launch': launch, 'terminal': terminal, 'cycles': cycles,
        'events': report.annotate_events(case, cycles, result['events']), 'whole_job_memory': result['whole_job_memory'],
        'validation': result, 'not_evaluated': report.NOT_EVALUATED, 'outputs': outputs, 'output_findings': findings,
        'shock': report.load_measure(measured, 4), 'comparisons': {'CL2': report.comparison_record(cl2)},
        'runtime_defaults': result['runtime_defaults'], 'tolerance_wiring': result['tolerance_wiring'],
        'runtime_repeat': result.get('runtime_repeat'), 'cl2_cap': q,
    }


def finish(repeat):
    case = HERE / ('repeat/run' if repeat else 'run')
    assert os.nice(0) == 19
    launch, terminal = (inherited.load(case / name) for name in ('launch.json', 'terminal.json'))
    assert launch['mode'] == 'CL3' and terminal['all_owned_handles_terminal'] and not terminal['live_handles']
    standing = inherited.checked(launch['standing']['path'], launch['standing']['sha256'])
    _, build = standing_build(standing)
    assert build['binary_sha256'] == launch['binary_sha256']
    stable = terminal['inputs_verified_after']
    for path, digest in [(case / 'run.cfg', launch['config_sha256']), (launch['binary'], launch['binary_sha256']),
                         *((case / name, launch['original_inputs'][name]['sha256']) for name in inherited.INPUT_FILES)]:
        assert stable[str(path)] and inherited.checked(path, digest)
    paths, _ = reference()
    for name, path in paths.items():
        assert inherited.pin(path) == launch['cl2_prerequisite'][name]
    complete = terminal['cause'] == 'natural' and terminal['returncode'] == 0
    result = annotate_native(validate(case, terminal_peaks=rank_peaks(case, terminal), complete=complete), case)
    cap, exact_q = cap_coordinates()
    result['tolerance_wiring'] = describe(case, result['events'], cap)
    if checked := compare_repeat(launch, result['tolerance_wiring']):
        result['runtime_repeat'] = checked
    measured = measure_live(case, result)
    inherited.save(case / 'validation.json', result)

    cl2 = inherited.load(paths['report'])
    cl2['receipt'] = inherited.pin(paths['report'])
    describe(paths['config'].parent, cl2['events'], cap, list_mode=False)
    data = make_report(case, launch, result, terminal, measured, cl2, {'points': cap.tolist(), 'event2_q': exact_q})
    stem = case.parent / 'reports' / f'CL3_{launch["build"]}'
    stem.parent.mkdir(exist_ok=True)
    path = stem.with_suffix('.md')
    text = report.render(data, path)
    text = text.replace('Native packets, timers, proof faces and native reference sidecars are not applicable to MMG2.',
                        'CL3 uses the sealed CL2 converged-loop configuration with marker-specific geometry tolerances.')
    text = text.replace('on mesh 4', 'in cycle 4').replace('All four adapted solves', 'All four later solves')
    path.write_text(text + '\n' + render_evidence(data))
    stem.with_suffix('.json').write_text(json.dumps(data, indent=2, allow_nan=False) + '\n')
    print(data['label'], flush=True)


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('--repeat', action='store_true', help='Read the one authorized R5 correction run')
    finish(parser.parse_args().repeat)
