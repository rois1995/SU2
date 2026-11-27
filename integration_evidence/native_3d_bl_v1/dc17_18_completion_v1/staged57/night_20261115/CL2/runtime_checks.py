"""Read effective runtime defaults and the signed native diagnostics."""
from datetime import datetime
from pathlib import Path
import math
import re

from validate import CYCLES, RANKS, complete_lines, gate, read_text


NUMBER = r'[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[Ee][-+]?\d+)?'
PREFIX = re.compile(r'(?:\w{3} \w{3} +\d{1,2} \d\d:\d\d:\d\d \d{4})?'
                    r'(?:\[\d+,\d+\])?<(?:stdout|stderr)>:')
STAMP = re.compile(r'^\w{3} \w{3} +\d{1,2} \d\d:\d\d:\d\d \d{4}')
CYCLE = re.compile(r'^\s*Cycle (\d+) of (\d+):')
ROOT = re.compile(r'\bNATIVE3D_TIMER_SUMMARY\b[^\n]*\broot=remesh\b[^\n]*\bboundary=final\b')
METRIC = re.compile(r'^Its metric makes the next mesh:.*?, norm ([^,]+),')
BEFORE = re.compile(rf'^Sensor metric complexity before gradation: ({NUMBER})\.$')
AFTER = re.compile(rf'^Sensor-only donor complexity after gradation: ({NUMBER})\.$')
M29 = re.compile(r'\bNATIVE3D_M29_CELL\b')
FIELDS = re.compile(r'(\w+)=([^\s]+)')


def rank_log(case, rank):
    return Path(case) / 'rank_output' / '1' / f'rank.{rank}' / 'stdout'


def log_lines(path):
    for raw in complete_lines(read_text(path)):
        yield PREFIX.sub('', raw).strip(), raw


def log_stamp(raw):
    match = STAMP.match(raw)
    return datetime.strptime(match[0], '%a %b %d %H:%M:%S %Y').timestamp() if match else None


def sections(case, total=CYCLES):
    """The donor CFD cycle's log contains the event that makes its successor."""
    master = rank_log(case, 0)
    if not master.is_file():
        master = Path(case) / 'run.log'
    cycles, current = {}, None
    for text, raw in log_lines(master):
        start = CYCLE.match(text)
        if start and int(start[2]) == total:
            current = int(start[1])
            cycles.setdefault(current, [])
        if current is not None:
            cycles[current].append((text, raw))
    return cycles


def printed_scalar(token):
    try:
        value = float(token)
    except (TypeError, ValueError):
        return None
    return value if math.isfinite(value) else token


def norm_check(cycle0):
    lines = [text for text, _ in cycle0 if METRIC.match(text)]
    values = {'expected': 2.0, 'observed': [], 'lines': lines}
    for line in lines:
        token = METRIC.match(line)[1]
        if (value := printed_scalar(token)) is not None:
            values['observed'].append(value)
    if any(value != 2.0 for value in values['observed']):
        return gate('FAILED', 'cycle 0 printed the wrong effective norm', values)
    if len(lines) != 1 or len(values['observed']) != 1:
        return gate('not evaluated', 'missing or incomplete cycle-0 norm evidence', values)
    return gate('passed', values=values)


def native_defaults(case, cycle0):
    norm = norm_check(cycle0)
    requirements = []
    for rank in sorted(RANKS):
        for text, _ in log_lines(rank_log(case, rank)):
            if ROOT.search(text):
                break
            if 'NATIVE3D_REQUIREMENTS ' not in text:
                continue
            row = dict(FIELDS.findall(text.split('NATIVE3D_REQUIREMENTS ', 1)[1]))
            value = printed_scalar(row.get('hgrad'))
            if value is not None:
                requirements.append({'file_rank': rank, 'rank': row.get('rank'), 'hgrad': value, 'line': text})
    values = {'expected': 1.6, 'rows': requirements}
    identified = [row for row in requirements if row['rank'] == str(row['file_rank'])]
    if any(row['hgrad'] != 1.6 for row in identified):
        hgrad = gate('FAILED', 'first native event printed the wrong effective hgrad', values)
    elif {row['file_rank'] for row in identified} == RANKS:
        hgrad = gate('passed', values=values)
    else:
        hgrad = gate('not evaluated', 'first-event requirements are incomplete', values)
    return {'norm': norm, 'hgrad': hgrad,
            'wiring_defect': any(check['status'] == 'FAILED' for check in (norm, hgrad))}


def sensor_rows(lines):
    fields = {'sensor_complexity_before_gradation': None, 'sensor_complexity_after_gradation': None}
    for text, _ in lines:
        for regex, key in zip((BEFORE, AFTER), fields):
            match = regex.fullmatch(text)
            if match:
                fields[key] = float(match[1])
    return fields


def rejected_cells(case, event):
    """Use the M29 seed dump, which carries shape values without marker IDs."""
    seeds = []
    for rank in sorted(RANKS):
        completed = 0
        for text, _ in log_lines(rank_log(case, rank)):
            if ROOT.search(text):
                completed += 1
                continue
            if completed == event - 1 and M29.search(text):
                fields = dict(FIELDS.findall(text.split('NATIVE3D_M29_CELL ', 1)[1]))
                if fields.get('level') == '0':
                    q, j = (fields.get(key) for key in ('q', 'j'))
                    free = fields.get('free', '').split(',')
                    # CL2 is no-BL. M29 constrains exactly vertices incident to a physical face.
                    surface = free.count('0') if len(free) == 4 and set(free) <= {'0', '1'} else 'unavailable'
                    seeds.append({'rank': rank, 'cell': fields.get('cell'),
                                  'q': float(q) if q and re.fullmatch(NUMBER, q) else 'unavailable',
                                  'J': float(j) if j and re.fullmatch(NUMBER, j) else 'unavailable',
                                  'surface_points': surface,
                                  'markers': 'unavailable', 'cell_surface': fields.get('surface'),
                                  'line': text})
    return seeds


def annotate_native(result, case):
    logs = sections(case)
    result['runtime_defaults'] = native_defaults(case, logs.get(0, []))
    last_mesh = result['cycles'][0]
    for event in result['events']:
        number = event['event']
        cycle = result['cycles'][number]
        event.update(sensor_rows(logs.get(number - 1, [])),
                     points=cycle.get('points'), cells=cycle.get('cells'))
        if event.get('native_rejection') and cycle.get('points') == last_mesh.get('points'):
            event.update(cells=last_mesh.get('cells'), retained_mesh_cycle=last_mesh['cycle'])
        elif cycle.get('cells') is not None:
            last_mesh = cycle
        if event.get('native_rejection') or event['gates']['1']['status'] == 'FAILED':
            event['rejected_cells'] = rejected_cells(case, number)
    return result


def marker_faces(path):
    """Read the published SU2 boundary count directly from each marker heading."""
    if not path.is_file():
        return None
    name, counts = None, {}
    with path.open(errors='replace') as stream:
        for line in stream:
            prefix, sep, value = line.partition('=')
            if not sep:
                continue
            if prefix.strip() == 'MARKER_TAG':
                name = value.strip()
            elif prefix.strip() == 'MARKER_ELEMS' and name is not None and value.strip().isdigit():
                counts[name] = int(value)
                name = None
    return counts or None
