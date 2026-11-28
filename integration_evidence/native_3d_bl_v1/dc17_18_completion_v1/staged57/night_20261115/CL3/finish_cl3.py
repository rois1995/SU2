"""Validate and report a terminal CL3 run beside the sealed CL2 evidence."""

from argparse import ArgumentParser
import json
import os

from run_cl3 import HERE, inherited, reference, standing_build
from diagnostics import PHASES, cap_coordinates, describe
from finish_cl2 import measure, rank_peaks
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
    table(lines, ('Run', 'Event', 'Construction s', 'Quality s (with cleanup)', 'Final proof s', 'Job peak bytes'),
          [(name, event['event'], *(event['phase_seconds']['max_rank'][phase] for phase in PHASES),
            event.get('job_peak_rss_bytes'))
           for name, run in named for event in run['events']])
    lines.extend(['Each phase is its maximum across the six rank ledgers. Per-rank exclusive times are in the JSON.',
                  ''])
    issues = data['tolerance_wiring']['defects']
    for row in issues:
        lines.extend([f'Event {row["cycle"]} tolerance wiring failed. {row["reason"]}', ''])
    return '\n'.join(lines).rstrip() + '\n'


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
    for cycle in result['cycles']:
        cycle['marker_faces'] = marker_faces(case / cycle['mesh'])
    cap, exact_q = cap_coordinates()
    result['tolerance_wiring'] = describe(case, result['events'], cap)
    if checked := compare_repeat(launch, result['tolerance_wiring']):
        result['runtime_repeat'] = checked
    inherited.save(case / 'validation.json', result)

    cl2 = inherited.load(paths['report'])
    cl2['receipt'] = inherited.pin(paths['report'])
    describe(paths['config'].parent, cl2['events'], cap, list_mode=False)
    measured = measure(case)
    data = make_report(case, launch, result, terminal, measured, cl2, {'points': cap.tolist(), 'event2_q': exact_q})
    stem = case.parent / 'reports' / f'CL3_{launch["build"]}'
    stem.parent.mkdir(exist_ok=True)
    path = stem.with_suffix('.md')
    text = report.render(data, path)
    text = text.replace('Native packets, timers, proof faces and native reference sidecars are not applicable to MMG2.',
                        'CL3 uses the sealed CL2 converged-loop configuration with marker-specific geometry tolerances.')
    path.write_text(text + '\n' + render_evidence(data))
    stem.with_suffix('.json').write_text(json.dumps(data, indent=2, allow_nan=False) + '\n')
    print(data['label'], flush=True)


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('--repeat', action='store_true', help='Read the one authorized R5 correction run')
    finish(parser.parse_args().repeat)
