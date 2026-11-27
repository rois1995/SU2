"""Check the serial MMG path using CL1's physical, transfer and CFD criteria."""
from datetime import datetime, timezone
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'CL2'))
from runtime_checks import FIELDS, NUMBER, log_stamp, marker_faces, norm_check, printed_scalar, sections, sensor_rows
from validate import (CYCLES, MAX_EVENT_SECONDS, REPO, gate, module, nearest_sample, physical,
                      read_samples, transfer, whole_job_memory)
from report_cl2 import read_cycles

# CMMGInterface::GetMarkerReference indexes the sorted unique config marker names from one.
EXPECTED = {'FARFIELD': (1, 1.0), 'SYMMETRY': (2, 0.001), 'WING3D': (3, 0.001)}
SUCCESS = re.compile(rf'^MMG3D status (\w+), ({NUMBER}) s .*?, extraction ({NUMBER}) s'
                     rf'(?: \(gathered from (\d+) ranks\), slices sent ({NUMBER}) s)?'
                     rf'(?:[, ]+total_seconds=({NUMBER}))?\.$')
DECLARED_STATUS = re.compile(r'^MMG3D status (\w+)\b')
REJECTED = ('MMG failed (MMG5_STRONGFAILURE,', 'MMG returned an invalid element.',
            'MMG returned an invalid boundary element.', 'MMG returned non-simplex elements.',
            'Could not read the mesh size from MMG.', 'Could not read the mesh from MMG.')


def mmg_status(lines):
    rows = [(text, raw) for text, raw in lines if DECLARED_STATUS.match(text)]
    errors = [text for text, _ in lines if text.startswith(REJECTED)]
    values = {'lines': [text for text, _ in rows], 'errors': errors}
    if errors:
        return gate('FAILED', errors[0], values)
    for text, _ in rows:
        if DECLARED_STATUS.match(text)[1] != 'SUCCESS':
            return gate('FAILED', text, values)
    if len(rows) != 1 or not (match := SUCCESS.fullmatch(rows[0][0])) or match[6] is None:
        return gate('not evaluated', 'missing complete MMG status or whole remesh time', values)
    whole = printed_scalar(match[6])
    values.update(mmg_seconds=printed_scalar(match[2]), extraction_seconds=printed_scalar(match[3]),
                  gathered_ranks=int(match[4]) if match[4] else 1,
                  distribution_seconds=printed_scalar(match[5]), total_seconds=whole,
                  end_unix=log_stamp(rows[0][1]))
    if not isinstance(whole, float) or not 0 <= whole <= MAX_EVENT_SECONDS:
        return gate('FAILED', f'whole MMG remesh took {whole} s, limit {MAX_EVENT_SECONDS} s', values)
    return gate('passed', values=values)


def param_fields(text):
    return dict(FIELDS.findall(text))


def numeric(row, key):
    return printed_scalar(row.get(key))


def mmg_parameters(lines):
    global_rows = [param_fields(text) for text, _ in lines if text.startswith('MMG_PARAMETERS ')]
    local_rows = [param_fields(text) for text, _ in lines if text.startswith('MMG_LOCAL_PARAMETER ')]
    values = {'expected_global': {'dim': 3, 'hmin': 0.003, 'hgrad': 1.6, 'hausd': 0.01, 'local_rows': 3},
              'expected_local': {name: {'ref': ref, 'hausd': hausd} for name, (ref, hausd) in EXPECTED.items()},
              'global_rows': global_rows, 'local_rows': local_rows}
    found = set()
    for row in global_rows:
        for key, expected in values['expected_global'].items():
            if (given := numeric(row, key)) is not None and given != expected:
                return gate('FAILED', f'global {key} {given}, expected {expected}', values)
    for row in local_rows:
        name = row.get('marker')
        if not name:
            continue
        if name not in EXPECTED or name in found:
            return gate('FAILED', f'unexpected or repeated local marker {name}', values)
        found.add(name)
        visit = dict(zip(('ref', 'hausd'), EXPECTED[name]))
        if len(global_rows) == 1:
            visit.update((key, numeric(global_rows[0], key)) for key in ('hmin', 'hmax'))
        for key, expected in visit.items():
            given = numeric(row, key)
            if given is not None and expected is not None and given != expected:
                return gate('FAILED', f'{name} {key} {given}, expected {expected}', values)
    if (len(global_rows) != 1 or found != EXPECTED.keys()
            or any(numeric(global_rows[0], key) is None for key in (*values['expected_global'], 'hmax'))
            or any(numeric(row, key) is None for row in local_rows for key in ('ref', 'hmin', 'hmax', 'hausd'))):
        return gate('not evaluated', 'missing complete MMG parameter evidence', values)
    return gate('passed', values=values)


def validate_mmg2(case, terminal_peaks=None):
    case = Path(case)
    logs = sections(case)
    cycles = read_cycles(case)
    norm = norm_check(logs.get(0, []))
    capcheck = module('mmg2_capcheck', REPO / 'TestCases/adaptation/capability/capcheck.py')
    samples = read_samples(case)
    for cycle in cycles:
        cycle['marker_faces'] = marker_faces(Path(cycle['mesh']))
    events, defects, failures = [], [], []
    if norm['status'] == 'FAILED':
        defects.append({'cycle': 0, 'check': 'norm', 'reason': norm['reason']})
    for index in range(1, CYCLES + 1):
        lines = logs.get(index - 1, [])
        started = any(text.startswith(('MMG', 'Remeshed ', '------------------------------ Remesh (MMG)'))
                      for text, _ in lines)
        if started:
            remesh = mmg_status(lines)
            parameters = mmg_parameters(lines)
            moved = transfer('\n'.join(text for text, _ in lines), index, native=False)
            cfd = cycles[index]['history_gate']
            if divergence := cycles[index].get('log_divergence'):
                cfd = gate('FAILED', divergence[0], cfd['values'])
            checks = {'remesh': remesh, 'parameters': parameters,
                      'physical': physical(capcheck, case, index, gate(), native_reference=False),
                      'transfer': moved, 'continued CFD': cfd}
            if parameters['status'] == 'FAILED':
                defects.append({'cycle': index, 'check': 'parameters', 'reason': parameters['reason']})
            for name, result in checks.items():
                if result['status'] == 'FAILED':
                    failures.append({'cycle': index, 'check': name, 'reason': result['reason']})
        else:
            checks = {name: gate() for name in ('remesh', 'parameters', 'physical', 'transfer', 'continued CFD')}
            remesh = checks['remesh']
        mmg = remesh['values']
        end = mmg.get('end_unix')
        seconds = mmg.get('total_seconds')
        start = end - seconds if end is not None and isinstance(seconds, float) else None
        results = {check['status'] for check in checks.values()}
        status = ('not evaluated' if not started else 'FAILED' if 'FAILED' in results
                  else 'passed' if results == {'passed'} else 'OPEN')
        event = {'event': index, 'cycle': index, 'status': status, 'checks': checks, 'mmg': mmg,
                 'driver_adapt_seconds': cycles[index - 1].get('adapt_next_seconds'),
                 'host_load_start': nearest_sample(samples, start), 'host_load_end': nearest_sample(samples, end),
                 'remesh_start_utc': datetime.fromtimestamp(start, timezone.utc).isoformat() if start is not None else None,
                 'remesh_end_utc': datetime.fromtimestamp(end, timezone.utc).isoformat() if end is not None else None,
                 'points': cycles[index].get('points'), 'cells': cycles[index].get('cells'),
                 'marker_faces': cycles[index]['marker_faces'], **sensor_rows(lines)}
        events.append(event)
    return {'events': events, 'cycles': cycles, 'whole_job_memory': whole_job_memory(terminal_peaks),
            'runtime_defaults': {'norm': norm, 'wiring_defect': bool(defects), 'defects': defects},
            'failures': failures,
            'not_applicable': ['native packets', 'native timers', 'native proof faces', 'native reference sidecars']}


def self_check():
    rows = [('MMG_PARAMETERS dim=3 hmin=0.0030000000000000001 hmax=100 hgrad=1.6000000000000001 '
             'hausd=0.01 local_rows=3', '')]
    rows.extend((f'MMG_LOCAL_PARAMETER marker={name} ref={ref} hmin=0.003 hmax=100 hausd={value}', '')
                for name, (ref, value) in EXPECTED.items())
    assert mmg_parameters(rows)['status'] == 'passed'
    assert mmg_parameters(rows[:-1])['status'] == 'not evaluated'
    assert mmg_parameters([(rows[0][0].replace('hgrad=1.6000000000000001', 'hgrad=nan'), '')])['status'] == 'FAILED'
    assert mmg_parameters(rows[:-1] + [(rows[-1][0].replace('ref=3', 'ref=1'), '')])['status'] == 'FAILED'
    sample = [('MMG3D status SUCCESS, 120.936 s (with the validation of input and output), extraction 0.131491 s '
               '(gathered from 6 ranks), slices sent 0.179399 s total_seconds=121.24789999999999.', '')]
    assert mmg_status(sample)['status'] == 'passed'
    assert mmg_status([(sample[0][0].replace('121.24789999999999', '5400.1'), '')])['status'] == 'FAILED'
    assert mmg_status([('MMG3D status LOWFAILURE, 1 s.', '')])['status'] == 'FAILED'
    assert mmg_status([('MMG failed (MMG5_STRONGFAILURE, status 2), no valid mesh.', '')])['status'] == 'FAILED'
    assert mmg_status([])['status'] == 'not evaluated'


if __name__ == '__main__':
    self_check()
