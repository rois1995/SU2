"""Render the CL1 capability observation from saved run evidence."""

import csv
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import re


HERE = Path(__file__).resolve().parent
REPO = next(path for path in HERE.parents if (path / '.git').exists())
D_CASE = HERE.parent / 'FP3_D/G5/build678/S1/trial1/full678_run1'
D_MEASURE_PATH = REPO / 'reviewer_handoff/scripts/s6/out/measure_fp3d.jsonl'
D_MEASURE_SHA = '5c464abf56962271cf502cced7c04f4d3171490ed2fec765d1c4d628cd747576'
NOT_EVALUATED = ('six-file identity', 'first-collapse identity', 'two-run rule', 'isolated restart')
PREFIX = re.compile(r'(?:(?P<stamp>\w{3} \w{3} +\d{1,2} \d\d:\d\d:\d\d \d{4})\s*)?'
                    r'(?:\[\d+,\d+\])?<(?:stdout|stderr)>:')
CYCLE = re.compile(r'^Cycle (\d+) of (\d+): (\d+) points, at most (\d+) iterations,')
CAP = re.compile(r'^Maximum number of iterations reached \(ITER = (\d+)\) before convergence\.$')
BOOL_EXIT = {'All convergence criteria satisfied.': 'converged',
             'Interrupt signal received, exiting before the convergence criteria were satisfied.': 'interrupted'}
DIVERGED = re.compile(r'SU2 has diverged\.?\s*\(NaN detected\)', re.I)
NATIVE_BEGIN = ('Native3D configured working-storage ceiling:', 'NATIVE3D_PARTITION ',
                'NATIVE3D_LOCAL_CORE ', 'NATIVE3D_LOCAL_SURFACE ')
LOCAL_SURFACE = re.compile(r'\bNATIVE3D_LOCAL_(?:SURFACE rank=(\d+) (?:stage=(\d+) pass=(\d+) )?'
                           r'applicable=([01])|CORE rank=(\d+) applicable=(0))\b')
PAYLOADS = ('flow_{tag}.vtu', 'surface_{tag}.vtu', 'restart_flow_{tag}.dat',
            'adapted_mesh_{tag}.su2', 'adapted_mesh_{tag}.su2.native_ref')
HISTORY_REQUIRED = ('Inner_Iter', 'Adap_Cycle', 'Adap_Iter', 'rms[Rho]')


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def pin(path):
    return {'path': str(path), 'bytes': path.stat().st_size, 'sha256': sha(path)}


def number(value):
    try:
        value = float(value)
        return value if math.isfinite(value) else None
    except (ValueError, TypeError):
        return None


def timestamp(line):
    match = PREFIX.search(line)
    if match and match['stamp']:
        return datetime.strptime(match['stamp'], '%a %b %d %H:%M:%S %Y').astimezone(timezone.utc).isoformat()
    return None


def log_rows(path):
    with path.open(errors='replace') as stream:
        for line in stream:
            yield PREFIX.sub('', line).strip(), line.rstrip('\n')


def table_fields(text):
    return [field.strip() for field in text[1:-1].split('|')] if text.startswith('|') and text.endswith('|') else []


def mesh_counts(path):
    counts = {}
    if path.is_file():
        with path.open(errors='replace') as stream:
            for line in stream:
                match = re.match(r'^(NELEM|NPOIN)\s*=\s*(\d+)', line)
                if match:
                    counts[{'NELEM': 'cells', 'NPOIN': 'points'}[match[1]]] = int(match[2])
                    if len(counts) == 2:
                        break
    return counts


def read_history(path):
    grouped, issues, shared = {}, {}, []
    if not path.is_file() or not path.stat().st_size:
        return grouped, issues, ['history.csv is missing or empty']
    with path.open(newline='') as stream:
        reader = csv.reader(stream, skipinitialspace=True)
        fields = [field.strip() for field in next(reader, [])]
        if len(set(fields)) != len(fields) or not all(key in fields for key in HISTORY_REQUIRED):
            return grouped, issues, ['named history columns are missing or duplicated']
        for idx, line in enumerate(reader, 2):
            if not line or all(not value.strip() for value in line):
                continue
            row = dict(zip(fields, line))
            raw = row.get('Adap_Cycle', '').strip()
            if not raw.isdecimal():
                shared.append(f'history row {idx} lacks a complete Adap_Cycle')
                continue
            cycle = int(raw)
            if len(line) != len(fields) or any(not val.strip() for val in line):
                issues.setdefault(cycle, []).append(('OPEN', f'history row {idx} is cut or malformed'))
                continue
            parsed = {key: number(value) for key, value in row.items()}
            bad = [key for key, value in parsed.items() if value is None]
            if bad:
                issues.setdefault(cycle, []).append(
                    ('FAILED', f'history row {idx} has nonfinite or nonnumeric fields: {", ".join(bad)}'))
                continue
            grouped.setdefault(cycle, []).append(parsed)
    return grouped, issues, shared


def history_result(cycle, rows, issues, shared, offset):
    expected = cycle['iterations']
    values = {'rows': len(rows), 'expected_rows': expected, 'adap_iter_offset': offset}
    for status, reason in issues:
        if status == 'FAILED':
            return {'status': status, 'reason': reason, 'values': values}
    actual_inner = [row['Inner_Iter'] for row in rows]
    actual_adap = [row['Adap_Iter'] for row in rows]
    if any(actual != i for i, actual in enumerate(actual_inner)):
        i, actual = next((i, actual) for i, actual in enumerate(actual_inner) if actual != i)
        return {'status': 'FAILED', 'reason': f'Inner_Iter expected {i}, saw {actual:g}', 'values': values}
    if rows:
        start = actual_adap[0] if offset is None else offset
        if any(actual != start + i for i, actual in enumerate(actual_adap)):
            i, actual = next((i, actual) for i, actual in enumerate(actual_adap) if actual != start + i)
            return {'status': 'FAILED', 'reason': f'Adap_Iter expected {start + i:g}, saw {actual:g}', 'values': values}
    if cycle['status'] == 'not evaluated':
        reason = 'cycle start is absent'
        if rows:
            return {'status': 'OPEN', 'reason': 'history rows without a cycle start', 'values': values}
        return {'status': 'not evaluated', 'reason': reason, 'values': values}
    if cycle['status'] not in ('converged', 'cap reached') or expected is None or offset is None:
        return {'status': 'OPEN', 'reason': 'finished cycle, driver iterations or prior offset is missing', 'values': values}
    if issues or shared:
        return {'status': 'OPEN', 'reason': (issues[0][1] if issues else shared[0]), 'values': values}
    if not rows or len(rows) < expected:
        return {'status': 'OPEN', 'reason': f'expected {expected} finite rows, found {len(rows)}', 'values': values}
    if len(rows) > expected:
        return {'status': 'FAILED', 'reason': f'expected {expected} rows, found {len(rows)}', 'values': values}
    if cycle['status'] == 'cap reached' and expected != cycle['cap']:
        return {'status': 'FAILED', 'reason': f'cap exit expected {cycle["cap"]} iterations, found {expected}',
                'values': values}
    return {'status': 'passed', 'reason': None, 'values': values}


def read_cycles(case, total=4):
    """Read convergence and named history within the actual cycle boundaries."""
    case = Path(case)
    master = sorted(case.glob('rank_output/*/rank.0/stdout'))
    log = master[0] if len(master) == 1 else case / 'run.log'
    cycles = [{'cycle': idx, 'status': 'not evaluated', 'iterations': None, 'cap': None}
              for idx in range(total + 1)]
    current, summary, screen_table, convergence_table, writing_table = None, False, {}, False, False
    if log.is_file():
        for text, raw in log_rows(log):
            if match := CYCLE.match(text):
                idx, count, points, cap = map(int, match.groups())
                if count != total or not 0 <= idx <= total:
                    continue
                current = cycles[idx]
                current.update(status='OPEN', cycle_start_utc=timestamp(raw), points=points, cap=cap,
                               log_divergence=[], output_log=[], convergence_summary={}, exit_messages=[],
                               screen_coefficients={}, adaptation_start_utc=None, surface_core_evidence=[])
                screen_table, convergence_table, writing_table = {}, False, False
                continue
            if 'Mesh Adaptation Summary' in text and text.startswith('-'):
                summary = True
                current = None
                continue
            fields = table_fields(text)
            if summary:
                if len(fields) == 6 and all(fields[i].isdecimal() for i in (0, 2, 3)):
                    idx = int(fields[0])
                    if 0 <= idx <= total:
                        cycles[idx].update(iterations=int(fields[3]), summary_points=int(fields[2]),
                                           solve_seconds=number(fields[4]), adapt_next_seconds=number(fields[5]))
                continue
            if current is None:
                continue
            if DIVERGED.search(text):
                current['log_divergence'].append(text)
            cap = CAP.fullmatch(text)
            if text in BOOL_EXIT or cap:
                current['exit_messages'].append(text)
                current['solver_exit_utc'] = timestamp(raw)
                continue
            if fields and fields[0] == 'Convergence Field':
                convergence_table = True
                continue
            if convergence_table:
                if len(fields) == 4 and fields[0] in ('Cauchy[CL]', 'Cauchy[CD]'):
                    current['convergence_summary'][fields[0]] = {'value': number(fields[1]), 'criterion': fields[2],
                                                                 'converged': fields[3], 'line': raw}
                    continue
                if text.startswith('---'):
                    current['convergence_summary_complete'] = True
                    convergence_table = False
            if fields and fields[0] == 'File Writing Summary':
                current['output_log'].append(raw)
                writing_table = True
                continue
            if writing_table and fields and (fields[0] or fields[-1]):
                current['output_log'].append(raw)
            elif writing_table and text and not text.startswith('+'):
                writing_table = False
            if current['adaptation_start_utc'] is None and text.startswith(NATIVE_BEGIN):
                current['adaptation_start_utc'] = timestamp(raw)
            if match := LOCAL_SURFACE.search(text):
                rank, stage, step, applicable, core_rank, core_inactive = match.groups()
                rank, applicable = rank or core_rank, applicable or core_inactive
                current['surface_core_evidence'].append({'rank': int(rank), 'worker_stage': int(stage) if stage else None,
                                                         'pass': int(step) if step else None,
                                                         'applicable': applicable == '1'})
            if 'Inner_Iter' in fields:
                screen_table = {key: fields.index(key) for key in ('CL', 'CD') if key in fields}
            elif fields and fields[0].isdecimal():
                current['screen_coefficients'] = {key: number(fields[i]) for key, i in screen_table.items() if i < len(fields)}
    history, issues, shared = read_history(case / 'history.csv')
    shared.extend(f'out-of-range Adap_Cycle {idx}' for idx in (history.keys() | issues.keys()) - set(range(total + 1)))
    offset = 0
    for idx, cycle in enumerate(cycles):
        tag = f'adap_{idx:05d}'
        cycle['mesh'] = str(case / ('m6.su2' if idx == 0 else f'adapted_mesh_{tag}.su2'))
        cycle.update(mesh_counts(Path(cycle['mesh'])))
        messages = list(dict.fromkeys(cycle.get('exit_messages', [])))
        if len(messages) == 1:
            message = messages[0]
            match = CAP.fullmatch(message)
            cycle['status'] = 'cap reached' if match and int(match[1]) == cycle['cap'] else BOOL_EXIT.get(message, 'OPEN')
            cycle['exit_message'] = message
        rows = history.get(idx, [])
        first = rows[0].get('rms[Rho]') if rows else None
        last = rows[-1].get('rms[Rho]') if rows else None
        cycle['rms_rho_first'] = first
        cycle['rms_rho_last'] = last
        cycle['rms_rho_drop'] = first - last if first is not None and last is not None else None
        final = rows[-1] if rows else {}
        for key in ('CL', 'CD'):
            cycle[key] = final.get(key, cycle.get('screen_coefficients', {}).get(key))
            val = final.get(f'Cauchy[{key}]')
            cycle[f'cauchy_{key}'] = val if val is not None else cycle.get('convergence_summary', {}).get(
                f'Cauchy[{key}]', {}).get('value')
        cycle['history_gate'] = history_result(cycle, rows, issues.get(idx, []), shared, offset)
        cycle['observed_iterations'] = len(rows) if rows else None
        offset = None if offset is None or cycle['iterations'] is None else offset + cycle['iterations']
    combined = case / 'run.log'
    if combined != log and combined.is_file():
        cycle = cycles[0]
        for text, raw in log_rows(combined):
            if match := CYCLE.match(text):
                idx = int(match[1])
                if int(match[2]) == total and 0 <= idx <= total:
                    cycle = cycles[idx]
            elif DIVERGED.search(text) and text not in cycle.setdefault('log_divergence', []):
                cycle['log_divergence'].append(text)
    return cycles


def load_measure(path, total):
    cycles = {idx: {'status': 'not evaluated'} for idx in range(total + 1)}
    if not path or not Path(path).is_file():
        return {'cycles': cycles, 'raw_json_lines': [], 'issues': []}
    path = Path(path)
    raw, issues = [], []
    for idx, line in enumerate(path.read_text().splitlines(), 1):
        if not line.strip():
            continue
        raw.append(line)
        try:
            def unavailable(constant):
                issues.append({'line': idx, 'reason': f'nonfinite measure value {constant}'})
                return None

            data = json.loads(line, parse_constant=unavailable)
            match = re.fullmatch(r'restart_flow_adap_(\d{5})\.dat', Path(data['restart']).name)
            cycle = int(match[1]) if match else -1
            if cycle not in cycles or cycles[cycle]['status'] != 'not evaluated':
                raise ValueError('unknown or duplicate restart cycle')
            for station in data.get('C_cp_stations', []):
                station['lambda_flag'] = station.get('second_strong_rise_xc') is not None
            cycles[cycle] = {'status': 'measured', **data}
        except (TypeError, ValueError, KeyError) as error:
            issues.append({'line': idx, 'reason': str(error)})
    return {'receipt': pin(path), 'cycles': cycles, 'raw_json_lines': raw, 'issues': issues}


def file_outputs(case, cycles):
    inventory, findings = [], []
    for cycle in cycles:
        idx = cycle['cycle']
        tag = f'adap_{idx:05d}'
        names = PAYLOADS if idx else PAYLOADS[:3]
        paths = [case / name.format(tag=tag) for name in names]
        records = [dict(pin(path), status='present' if path.stat().st_size else 'empty')
                   if path.is_file() else {'path': str(path), 'bytes': None, 'sha256': None, 'status': 'missing'}
                   for path in paths]
        inventory.append({'cycle': idx, 'files': records})
        accepted = idx and all(row['status'] == 'present' for row in records[-2:])
        if accepted:
            for row in records[:2]:
                if row['status'] != 'present':
                    findings.append({'cycle': idx, **row, 'output_log': cycle.get('output_log', [])})
    return inventory, findings


def epoch(iso):
    if iso is None:
        return None
    try:
        return datetime.fromisoformat(iso).timestamp()
    except (ValueError, TypeError):
        return None


def nearest(samples, stamp):
    target = epoch(stamp)
    if target is None or not samples:
        return None
    sample = min(samples, key=lambda row: abs(row['unix'] - target))
    return {**sample, 'sample_offset_seconds': sample['unix'] - target}


def read_samples(case):
    samples = []
    path = case / 'resources.jsonl'
    if path.is_file():
        with path.open() as stream:
            for line in stream:
                try:
                    data = json.loads(line)
                    if isinstance(data, dict) and number(data.get('unix')) is not None and data.get('loadavg'):
                        samples.append(data)
                except ValueError:
                    continue
    return samples


def annotate_events(case, cycles, events):
    samples = read_samples(case)
    results = []
    for event in events:
        idx = event['event']
        donor = cycles[idx - 1]
        recipient = cycles[idx]
        evidence = donor.get('surface_core_evidence', [])
        result = {**event, 'driver_adapt_seconds': donor.get('adapt_next_seconds'),
                  'job_peak_rss_bytes': event.get('job_peak_rss_bytes', event.get('job_peak_bytes')),
                  'surface_core': {'ran': event.get('local_surface_core_ran',
                                                    any(row['applicable'] for row in evidence) if evidence else None),
                                   'ranks': event.get('local_surface_core_ranks'),
                                   'evidence': evidence}}
        start = result.get('native_start_utc') or donor.get('adaptation_start_utc')
        end = result.get('native_end_utc') or recipient.get('cycle_start_utc')
        result.setdefault('host_load_start', nearest(samples, start))
        result.setdefault('host_load_end', nearest(samples, end))
        if not result.get('native_start_utc') and start:
            result['load_start_boundary'] = 'first native marker'
        if not result.get('native_end_utc') and end:
            result['load_end_boundary'] = 'next CFD cycle start'
        results.append(result)
    return results


def historical():
    receipt = D_CASE.parent / 'run_receipt.json'
    collection = D_CASE.parent / 'collection.json'
    sealed = json.loads(receipt.read_text())
    assert sha(collection) == sealed['artifacts']['collection.json'], collection
    assert sha(D_MEASURE_PATH) == D_MEASURE_SHA, D_MEASURE_PATH
    cycles = read_cycles(D_CASE, 2)
    events = annotate_events(D_CASE, cycles, json.loads(collection.read_text())['events'])
    outputs, _ = file_outputs(D_CASE, cycles)
    return {'cycles': cycles, 'events': events, 'outputs': outputs, 'shock': load_measure(D_MEASURE_PATH, 2),
            'run_receipt': pin(receipt), 'collection': pin(collection)}


def label_for(cycles, validation, terminal):
    events = validation['events']
    for event in events:
        for idx in range(1, 7):
            result = event.get('gates', {}).get(str(idx), {})
            if result.get('status') == 'FAILED':
                return (f'CL1: converged loop run FAILED at event {event["event"]}, gate {idx}',
                        result.get('reason'))
        rejection = event.get('native_rejection') or event.get('rejection')
        if rejection:
            return f'CL1: converged loop run FAILED at event {event["event"]}: native rejection', rejection
    for cycle in cycles:
        if cycle.get('log_divergence'):
            return f'CL1: converged loop run FAILED at cycle {cycle["cycle"]}: solver divergence', cycle['log_divergence'][0]
    completion = validation.get('whole_job_memory', {})
    if completion.get('status') == 'FAILED':
        return ('CL1: converged loop run FAILED at final CFD and output: whole-job memory limit',
                completion.get('reason'))
    extra = validation.get('failures', [])
    if extra:
        first = extra[0]
        if isinstance(first, dict):
            stage, reason = first.get('stage', 'validation'), first.get('reason', str(first))
        else:
            stage, reason = 'validation', str(first)
        return f'CL1: converged loop run FAILED at {stage}: {reason}', reason
    start = cycles[0]
    seen = set(start.get('convergence_summary', {}))
    if (start.get('convergence_summary_complete') and seen != {'Cauchy[CL]', 'Cauchy[CD]'}
            and start['status'] in ('converged', 'cap reached')):
        reason = 'the first convergence summary did not list both CL and CD'
        return f'CL1: converged loop run FAILED at cycle 0: {reason}', reason
    if terminal.get('cause') == 'natural' and terminal.get('returncode') not in (0, None):
        visited = [cycle['cycle'] for cycle in cycles if cycle['status'] != 'not evaluated']
        stage = f'cycle {visited[-1]}' if visited else 'startup'
        reason = f'solver exit code {terminal["returncode"]}'
        return f'CL1: converged loop run FAILED at {stage}: {reason}', reason
    ready = (terminal.get('cause') == 'natural' and terminal.get('returncode') == 0
             and terminal.get('all_owned_handles_terminal') and completion.get('status') == 'passed'
             and len(events) == 4 and seen == {'Cauchy[CL]', 'Cauchy[CD]'}
             and start['status'] == 'cap reached' and start['iterations'] == 1
             and all(event.get('gates', {}).get(str(idx), {}).get('status') == 'passed'
                     for event in events for idx in range(1, 7))
             and all(cycle['status'] in ('converged', 'cap reached') for cycle in cycles[1:]))
    if not ready:
        return 'CL1: converged loop run OPEN', 'watchdog, interruption or required completion evidence missing'
    caps = ', '.join(str(cycle['cycle']) for cycle in cycles[1:] if cycle['status'] == 'cap reached')
    if caps:
        return f'CL1: loop run DONE; convergence cap reached on cycles {caps}', None
    return 'CL1: converged loop run DONE', None


def cell(value):
    if value is None:
        return 'unavailable'
    if isinstance(value, bool):
        return 'yes' if value else 'no'
    if isinstance(value, float):
        return f'{value:.10g}'
    return str(value).replace('|', '\\|').replace('\n', '<br>')


def table(lines, headings, rows):
    lines.extend(['| ' + ' | '.join(headings) + ' |', '| ' + ' | '.join(['---'] * len(headings)) + ' |'])
    lines.extend('| ' + ' | '.join(cell(value) for value in row) + ' |' for row in rows)
    lines.append('')


B_FIELDS = ('shock_edges', 'edge_extent_along_shock_normal_median', 'edge_mach_jump_median',
            'metric_h_along_shock_normal_median', 'metric_h_along_wall_normal_median_at_shock',
            'le_points', 'metric_h_chordwise_median_at_le', 'metric_h_along_wall_normal_median_at_le',
            'metric_hmin_any_direction_median_at_le')


def render_shock(lines, named):
    lines.extend(['## Shock measurements', '',
                  'The metric on cycle k requests sizes for mesh k+1. The realized size is the median edge '
                  'extent along the in-surface shock normal. A second strong rise sets the lambda flag.', ''])
    b_rows, c_rows, pairs = [], [], []
    for name, record in named:
        measured = record['shock']['cycles']
        for cycle, measure in measured.items():
            metrics = measure.get('B_direction_aware', {})
            b_rows.append((name, cycle, measure['status'], *(metrics.get(key) for key in B_FIELDS)))
            for station in measure.get('C_cp_stations', ()):
                c_rows.append((name, cycle, *(station.get(key) for key in (
                    'eta_y', 'shock_xc', 'cp_jump', 'width_10_90_over_chord', 'cut_points_in_10_90',
                    'lambda_flag', 'second_strong_rise_xc'))))
            if cycle + 1 in measured:
                next_metrics = measured[cycle + 1].get('B_direction_aware', {})
                pairs.append((name, f'{cycle} → {cycle + 1}',
                              metrics.get('metric_h_along_shock_normal_median'),
                              metrics.get('metric_h_along_wall_normal_median_at_shock'),
                              next_metrics.get('edge_extent_along_shock_normal_median')))
    table(lines, ('Run', 'Cycle', 'Status', 'Shock edges', 'Realized shock-normal size', 'Mach jump per edge',
                  'Metric shock-normal size', 'Metric wall-normal size', 'Leading-edge points',
                  'Leading-edge chordwise metric', 'Leading-edge wall-normal metric', 'Leading-edge minimum metric'),
          b_rows)
    table(lines, ('Run', 'Requested → realized cycle', 'Requested shock-normal metric',
                  'Requested wall-normal metric', 'Realized shock-normal size'), pairs)
    table(lines, ('Run', 'Cycle', 'Span station y', 'Shock x/c', 'Cp jump', '10–90% width/chord', 'Points in width',
                  'Lambda flag', 'Second rise x/c'), c_rows)
    for name, record in named:
        shock = record['shock']
        if shock['issues']:
            lines.extend([f'{name} measure evidence includes unreadable rows. See the JSON report.', ''])
        if shock['raw_json_lines']:
            lines.extend([f'### {name} measure JSON lines', '', '```jsonl', *shock['raw_json_lines'], '```', ''])


def render(record, report_path):
    lines = [f'# CL1 build{record["launch"]["build"]}', '', f'**{record["label"]}**', '']
    if record['finding']:
        lines.extend([f'First finding — {cell(record["finding"])}.', ''])
    launch = record['launch']
    terminal = record['terminal']
    lines.extend([
        f'Checkout commit `{launch["checkout_commit"]}`. Solver source commit `{launch["binary_source_commit"]}`.',
        f'Solver `{launch["binary"]}` · SHA256 `{launch["binary_sha256"]}`.',
        f'Launch {cell(launch.get("started_utc"))}. Terminal time {cell(terminal.get("finished_utc"))}.',
        f'Termination cause **{cell(terminal.get("cause"))}**. Wait exit status `{cell(terminal.get("returncode"))}`. '
        f'Elapsed {cell(terminal.get("elapsed_seconds"))} s. '
        f'All owned handles terminal **{cell(terminal.get("all_owned_handles_terminal"))}**.', '',
        'Unevaluated by order — six-file identity, first-collapse identity, the two-run rule and isolated restart.',
        'The first setup supervisor stopped on an incorrect host-preflight path before any solver launch. '
        'The corrected helper passed the host check, then the sole solver invocation started. '
        '[Setup log](setup_host_preflight_path.log)', '',
        '## Configuration change', '', '```diff', launch['config_diff'].rstrip(), '```', '',
        '## CFD cycles', '',
        'The driver solve interval includes flow iterations, metric construction and cycle output. '
        'FP3-D S1 printed CL and CD on the screen and recorded no CL/CD Cauchy history.', '',
    ])
    named = [('CL1', record), ('FP3-D S1', record['comparison_FP3_D_S1'])]
    table(lines, ('Run', 'Cycle', 'Status', 'Driver iterations', 'Observed history rows', 'Cauchy CL', 'Cauchy CD', 'First rms[Rho]',
                  'Last rms[Rho]', 'rms[Rho] drop', 'CL', 'CD', 'Points', 'Cells', 'Solve s'),
          [(name, *(row.get(key) for key in (
              'cycle', 'status', 'iterations', 'observed_iterations', 'cauchy_CL', 'cauchy_CD', 'rms_rho_first', 'rms_rho_last',
              'rms_rho_drop', 'CL', 'CD', 'points', 'cells', 'solve_seconds')))
           for name, each in named for row in each['cycles']])
    for name, each in named:
        for row in each['cycles']:
            if message := row.get('exit_message'):
                lines.extend([f'{name} cycle {row["cycle"]} — “{message}”', ''])
    lines.extend(['CL1 cycle 0 uses the configured cap of one iteration.', ''])
    if conv := record['cycles'][0].get('convergence_summary'):
        lines.extend(['CL1 cycle 0 convergence summary', '', '```text',
                      *(entry['line'] for entry in conv.values()), '```', ''])
    lines.extend(['## Adaptation events', '',
                  'Host loads are the nearest recorded samples to the stated boundaries. '
                  'Load was uncontrolled. The detailed receipts preserve sample timestamps and offsets.', ''])
    table(lines, ('Run', 'Event', 'Native s', 'Driver adapt s', 'Job peak bytes', 'Start load', 'End load',
                  'Local surface core ran'),
          [(name, event['event'], event.get('native_seconds'), event.get('driver_adapt_seconds'),
            event.get('job_peak_rss_bytes'), (event.get('host_load_start') or {}).get('loadavg'),
            (event.get('host_load_end') or {}).get('loadavg'), event.get('surface_core', {}).get('ran'))
           for name, each in named for event in each['events']])
    table(lines, ('Event', 'Gate', 'Status', 'Finding'),
          [(event['event'], idx, result.get('status'), result.get('reason'))
           for event in record['events'] for idx, result in event.get('gates', {}).items()])
    transfer_rows = [
        (event['event'], field, values['residual'], values['bounds_relaxed'] or 'none')
        for event in record['events']
        for field, values in (event['gates']['3'].get('values') or {}).get('fields', {}).items()
    ]
    if transfer_rows:
        table(lines, ('Event', 'Transfer field', 'Conservative residual', 'Bounds suffix'), transfer_rows)
    mem = record['whole_job_memory']
    lines.extend([f'Whole-job memory through the final CFD and output — **{cell(mem.get("status"))}**. '
                  f'{cell(mem.get("reason"))}' if mem.get('reason') else
                  f'Whole-job memory through the final CFD and output — **{cell(mem.get("status"))}**.', ''])
    memory_values = mem.get('values') or {}
    table(lines, ('Terminal peak sum bytes', 'Limit bytes'),
          [(memory_values.get('sum_bytes'), memory_values.get('limit_bytes'))])
    table(lines, ('Rank', 'PID', 'Terminal peak bytes', 'Wait return code', 'Raw wait status'),
          [(row.get('rank'), row.get('pid'), row.get('peak_rss_bytes'), row.get('returncode'), row.get('wait_status'))
           for row in terminal.get('rank_waits', [])])
    lines.extend(['## Output files', '',
                  'The listed files remain in the run directories. FP3-D S1 did not request ParaView files.', ''])
    rows = []
    for name, each in named:
        for cycle in each['outputs']:
            for item in cycle['files']:
                path = Path(item['path'])
                relative = os.path.relpath(path, report_path.parent)
                link = f'[{path.name}]({relative})' if item['status'] == 'present' else f'`{path.name}`'
                rows.append((name, cycle['cycle'], link, item['bytes'], item['status'], item['sha256']))
    table(lines, ('Run', 'Cycle', 'File', 'Bytes', 'Status', 'SHA256'), rows)
    for finding in record['output_findings']:
        lines.extend([f'Cycle {finding["cycle"]} output finding — `{finding["path"]}` is {finding["status"]} '
                      'after accepted mesh replacement.', '', '```text',
                      *(finding['output_log'] or ['No writing-summary lines were captured for this cycle.']), '```', ''])
    render_shock(lines, named)
    return '\n'.join(lines).rstrip() + '\n'


def write_report(case, *, launch, events, terminal, measure):
    """Write the Markdown and JSON receipts after solver termination."""
    case = Path(case).resolve()
    cycles = events.get('cycles') or read_cycles(case)
    label, finding = label_for(cycles, events, terminal)
    annotated = annotate_events(case, cycles, events['events'])
    outputs, output_findings = file_outputs(case, cycles)
    historical_record = historical()
    report = {
        'label': label, 'finding': finding, 'launch': launch, 'terminal': terminal,
        'cycles': cycles, 'events': annotated, 'whole_job_memory': events.get('whole_job_memory', {}),
        'validation': events, 'not_evaluated': NOT_EVALUATED, 'outputs': outputs,
        'output_findings': output_findings, 'shock': load_measure(measure, 4),
        'comparison_FP3_D_S1': historical_record,
    }
    stem = HERE / 'reports' / f'CL1_{launch["build"]}'
    stem.parent.mkdir(parents=True, exist_ok=True)
    stem.with_suffix('.md').write_text(render(report, stem.with_suffix('.md')))
    stem.with_suffix('.json').write_text(json.dumps(report, indent=2, allow_nan=False) + '\n')
    return report


def self_check():
    baseline = historical()
    assert [row['iterations'] for row in baseline['cycles']] == [1, 1, 1]
    assert all(row['status'] == 'cap reached' and row['history_gate']['status'] == 'passed'
               for row in baseline['cycles'])
    assert all(row['surface_core']['ran'] for row in baseline['events'])
    assert len(baseline['shock']['raw_json_lines']) == 3
    cycles = [{'cycle': idx, 'status': 'converged', 'iterations': 300} for idx in range(5)]
    cycles[0].update(status='cap reached', iterations=1,
                     convergence_summary=dict.fromkeys(('Cauchy[CL]', 'Cauchy[CD]')))
    events = [{'event': idx, 'gates': {str(gate): {'status': 'passed'} for gate in range(1, 7)}}
              for idx in range(1, 5)]
    validation = {'events': events, 'whole_job_memory': {'status': 'passed'}}
    terminal = {'cause': 'natural', 'returncode': 0, 'all_owned_handles_terminal': True}
    assert label_for(cycles, validation, terminal)[0] == 'CL1: converged loop run DONE'
    cycles[3]['status'] = 'cap reached'
    assert label_for(cycles, validation, terminal)[0].endswith('cap reached on cycles 3')
    assert label_for(cycles, validation, {**terminal, 'cause': 'watchdog'})[0].endswith('OPEN')
    events[1]['gates']['5']['status'] = 'FAILED'
    assert label_for(cycles, validation, {**terminal, 'cause': 'watchdog'})[0].endswith('FAILED at event 2, gate 5')
    print('CL1 report classification and authenticated FP3-D comparison self-check passed.')


if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--self-check', action='store_true', required=True)
    parser.parse_args()
    self_check()
