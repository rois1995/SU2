"""Read D1 rank transcripts and compare the one diagnostic run with build633."""
from argparse import ArgumentParser
from collections import Counter, defaultdict
from dataclasses import dataclass
from decimal import Decimal, InvalidOperation
import gzip
import hashlib
import json
from pathlib import Path
import re
import sys

HERE = Path(__file__).resolve().parent
REFERENCE = HERE.parent / 'FP2_B/G5/build633/S1/full633_run1'
sys.dont_write_bytecode = True
sys.path.insert(0, str(HERE.parent / 'FP2_A'))
from report_c import PREFIX, fields

SHORT = 'NATIVE3D_LOCAL_SHORT'
NEAR = 'NATIVE3D_LOCAL_NEAR'
NEAR_LEFT = 'NATIVE3D_LOCAL_NEAR_LEFT'
REJECT = 'NATIVE3D_METRIC_REJECT'
SEED = 'NATIVE3D_SEED_SHORT'
LENGTH = 'NATIVE3D_LENGTH'
SURFACE = 'NATIVE3D_LOCAL_SURFACE'
TIMER = 'NATIVE3D_TIMER_SUMMARY'
MARKERS = (SHORT, NEAR, NEAR_LEFT, REJECT, SEED, LENGTH, SURFACE, TIMER,
           'NATIVE3D_LOCAL_CORE', 'NATIVE3D_LOCAL_SURFACE_STATE', 'NATIVE3D')
MARKER = re.compile(r'\b(' + '|'.join(MARKERS) + r')\s+(.+)')
INTEGER = re.compile(r'[+-]?\d+\Z')
EDGE_FLOOR = Decimal('.01')
PAYLOADS = ('adapted_mesh_adap_00001.su2', 'adapted_mesh_adap_00001.su2.native_ref',
            'restart_flow_adap_00001.dat')


@dataclass
class Row:
    kind: str
    event: int
    rank: int
    stage: int | None
    line: int
    fields: dict[str, str]

    @property
    def source(self):
        return f'r{self.rank}:{self.line}'

    def value(self, key, default='—'):
        return self.fields.get(key, default)


def number(value):
    try:
        result = Decimal(value)
        return result if result.is_finite() else None
    except (InvalidOperation, TypeError):
        return None


def scan_rank(lines, rank):
    """Use each rank's final remesh summary as its event boundary."""
    event, stage = 1, None
    for index, raw in enumerate(lines, 1):
        if not raw.endswith(('\n', '\r')) or not (match := MARKER.search(PREFIX.sub('', raw).strip())):
            continue
        marker, text = match.groups()
        row = fields(text)
        source_rank = row.get('rank')
        # The joined NATIVE3D summaries for all worker ranks appear on rank zero.
        if marker == 'NATIVE3D':
            if rank == 0 and 'complete' in row:
                yield Row(marker, event, rank, stage, index, row)
            continue
        if source_rank != str(rank):
            continue
        if 'stage' in row and row['stage'].isdigit():
            stage = int(row['stage'])
        if marker == TIMER:
            if row.get('root') == 'remesh' and row.get('boundary') == 'final':
                yield Row(marker, event, rank, stage, index, row)
                event += 1
                stage = None
        elif marker not in ('NATIVE3D_LOCAL_CORE', 'NATIVE3D_LOCAL_SURFACE_STATE'):
            yield Row(marker, event, rank, stage, index, row)


def read_case(case):
    rows = defaultdict(list)
    seen = set()
    for path in sorted(case.glob('rank_output/1/rank.*/stdout*')):
        if path.name not in ('stdout', 'stdout.gz'):
            continue
        rank = int(path.parent.name.split('.')[-1])
        if rank in seen:
            continue
        seen.add(rank)
        opener = gzip.open if path.name.endswith('.gz') else open
        with opener(path, 'rt', errors='replace') as stream:
            for row in scan_rank(stream, rank):
                rows[row.kind].append(row)
    return rows, sorted(seen)


def digest(path):
    if not path.is_file():
        return None
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def integer_fields(row):
    # Timing and distance fields may print an exact zero, but are never integer counters.
    return {key: int(value) for key, value in row.fields.items()
            if key not in ('outward_max',) and not key.startswith('seconds_') and INTEGER.fullmatch(value)}


def surface_rows(rows):
    result = defaultdict(list)
    for row in rows[SURFACE]:
        if row.event == 1:
            result[row.rank, row.stage, int(row.value('pass', '0'))].append(row)
    return result


def first_surface_difference(base, trial):
    baseline, candidate = surface_rows(base), surface_rows(trial)
    for key in sorted(baseline.keys() | candidate.keys(),
                      key=lambda k: (k[0], -1 if k[1] is None else k[1], k[2])):
        previous, observed = baseline[key], candidate[key]
        if len(previous) != len(observed):
            return f'rank/stage/pass {key}, rows {len(previous)} in build633 and {len(observed)} in D1'
        for expected_row, actual_row in zip(previous, observed):
            expected, actual = integer_fields(expected_row), integer_fields(actual_row)
            for field in dict.fromkeys((*expected.keys(), *actual.keys())):
                if expected.get(field) != actual.get(field):
                    return (f'rank/stage/pass {key}, {field}: {expected.get(field, "missing")} in build633 '
                            f'and {actual.get(field, "missing")} in D1')
    return None if baseline else 'no event-1 surface rows visible in build633'


def first_length(rows, event, rank):
    return next((row for row in rows[LENGTH] if (row.event, row.rank) == (event, rank)), None)


def sanity(base, trial, case):
    lines = ['## Reproduction sanity', '']
    difference = first_surface_difference(base, trial)
    good = difference is None
    lines.append(f'- Event-1 integer surface fields. {"Exact match." if good else "First difference: " + difference + "."}')
    files = []
    for name in PAYLOADS:
        expected, actual = digest(REFERENCE / name), digest(case / name)
        equal = expected is not None and expected == actual
        files.append([name, expected, actual, 'equal' if equal else 'missing or different'])
        good &= equal
    lines += ['', *table(('Event-1 file', 'Build633 SHA256', 'D1 SHA256', 'Result'), files)]
    expected, actual = (first_length(rows, 2, 0) for rows in (base, trial))
    expected, actual = (row.value('minimum_length') if row else None for row in (expected, actual))
    equal = number(expected) is not None and number(expected) == number(actual)
    lines += [f'- Event-2 rank-0 first LENGTH minimum. Build633 `{expected}`, D1 `{actual}`. '
              f'{"Exact numeric match." if equal else "Missing or different."}']
    if not (good and equal):
        lines.append('- A sanity difference makes event 2 a separate observed case. Its diagnostics are retained below.')
    return lines + ['']


def table(columns, rows):
    def cell(value):
        return str(value if value is not None else '—').replace('|', r'\|').replace('\n', ' ')

    entries = ['| ' + ' | '.join(cell(value) for value in row) + ' |' for row in rows]
    if not entries:
        return ['No rows observed.', '']
    return ['| ' + ' | '.join(columns) + ' |', '| ' + ' | '.join('---' for _ in columns) + ' |', *entries, '']


def scope(row):
    return row.event, row.rank, row.stage


def pass_scope(row):
    return *scope(row), row.value('pass')


def point(row, label):
    return row.value(label), row.value(f'{label}_xyz')


def vertex_key(vertex):
    point_id, xyz = vertex
    return point_id, tuple(number(part) for part in xyz.split(','))


def edge(row):
    return tuple(sorted((vertex_key(point(row, 'a')), vertex_key(point(row, 'b')))))


def point_records(rows):
    by_point = defaultdict(list)
    for row in rows[SHORT] + rows[NEAR]:
        for label in ('a', 'b', *(('m', 'nearest') if row.kind == NEAR else ())):
            if label in row.fields:
                by_point[row.event, row.value(label)].append((row, label))
    return by_point


def split_records(rows, vertex):
    return [row for row, key in rows if key == 'm' and vertex_key(point(row, key)) == vertex]


def reject_section(rows):
    lines = ['## P3 · Rejected cells', '']
    points = point_records(rows)
    base_ids = defaultdict(set)
    for row in rows[SHORT]:
        if row.stage == 0 and row.value('checkpoint') == 'c0' and row.value('pass') == '1':
            base = row.value('input_point_base')
            if base.isdigit():
                base_ids[row.event].add(int(base))
    rejects = rows[REJECT]
    if not rejects:
        return lines + ['No P3 row was observed.', '']
    for row in rejects:
        base_set = base_ids[row.event]
        base = next(iter(base_set)) if len(base_set) == 1 else None
        lines += [f'### Event {row.event}, rank {row.rank}, stage {row.stage}, cell {row.value("cell")}', '',
                  f'**Measurement reason.** {row.value("reason")}', '',
                  f'Phase `{row.value("phase")}`, centroid `{row.value("centroid")}`, '
                  f'{row.value("measured_edges")} measured edges, maximum metric length '
                  f'`{row.value("maximum_length")}`. P3 at {row.source}.', '']
        vertices = []
        for index in range(4):
            vertex = point(row, f'v{index}')
            identity = vertex_key(vertex)
            records = points[row.event, vertex[0]]
            matched = [(prior, key) for prior, key in records
                       if vertex_key(point(prior, key)) == identity and
                       (prior.stage is None or row.stage is None or prior.stage <= row.stage) and
                       (prior.rank != row.rank or prior.line < row.line)]
            splits = split_records(matched, identity)
            core_boundary = any(
                prior.kind == SHORT and prior.value(f'{key}_new') == prior.value(f'{key}_surface') == '1'
                and prior.value('checkpoint') in ('c1', 'end', 'shifted') for prior, key in matched)
            if base is None or not vertex[0].isdigit():
                origin = 'Event input provenance unavailable'
            elif int(vertex[0]) <= base:
                origin = f'Event input (stage-0 point base {base})'
            elif splits:
                origin = 'Core surface-split point from P2'
            elif core_boundary:
                origin = 'New core surface point in P1'
            else:
                origin = 'Other new ID, core surface ancestry unobserved'
            evidence = ', '.join(f'{"P1" if prior.kind == SHORT else "P2"} {prior.source}'
                                 for prior, _ in matched)
            unmatched = [prior.source for prior, _ in records if not any(prior == item for item, _ in matched)]
            if unmatched:
                evidence += f' (other rows for ID: {", ".join(dict.fromkeys(unmatched))})'
            vertices.append([*vertex, origin, evidence or 'no matching P1/P2 row'])
        lines += table(('Vertex ID', 'XYZ', 'Origin', 'Evidence'), vertices)
        edges = [[row.value(f'e{i}_a'), row.value(f'e{i}_b'), row.value(f'e{i}_space'),
                  row.value(f'e{i}_metric')] for i in range(6)]
        lines += table(('Edge A', 'Edge B', 'Space', 'Cached metric'), edges)
        spatial = min((entry for entry in edges if number(entry[2]) is not None),
                      key=lambda entry: number(entry[2]), default=None)
        metric = min((entry for entry in edges if number(entry[3]) is not None and number(entry[3]) >= 0),
                     key=lambda entry: number(entry[3]), default=None)
        for kind, index, value in (('Spatial', 2, spatial), ('Cached metric', 3, metric)):
            if value:
                lines += [f'{kind} minimum edge {value[0]}–{value[1]} at {value[index]}.', '']
    return lines


def checkpoint_groups(rows):
    groups = defaultdict(list)
    for row in rows[SHORT]:
        groups[*pass_scope(row), row.value('checkpoint')].append(row)
    return groups


def sampled_short(rows):
    return [row for row in rows if 'order' in row.fields and
            (length := number(row.value('metric'))) is not None and length < EDGE_FLOOR]


def census(rows):
    value = rows[0].value('lt_1e_2') if rows else ''
    return int(value) if value.isdigit() else None


def short_section(rows):
    lines = ['## P1 · Core short-edge observations', '',
             'The four bins count all measured edges. Each checkpoint publishes up to five identities. '
             'The hidden count is the short-edge bin minus the number of visible IDs below 0.01. '
             'Absence from a saturated list cannot establish when an edge first appeared.', '']
    groups = checkpoint_groups(rows)
    all_short, observations, counts = defaultdict(list), [], []
    for group, data in groups.items():
        first = data[0]
        sampled, total = sampled_short(data), census(data)
        counts.append([*group, first.value('input_point_base'), first.value('lt_1e_6'),
                       first.value('lt_1e_4'), total, first.value('lt_01'), len(sampled),
                       None if total is None else total - len(sampled), first.value('error')])
        event, _, stage, _, _ = group
        all_short[event, stage].extend(sampled)
        for row in data:
            observations.append([*group, row.value('order'), row.value('metric'), row.value('space'),
                                 *point(row, 'a'), '/'.join(row.value(f'a_{key}') for key in ('surface', 'border', 'new')),
                                 *point(row, 'b'), '/'.join(row.value(f'b_{key}') for key in ('surface', 'border', 'new')),
                                 row.value('physical'), row.source])
    group_columns = ('Event', 'Rank', 'Stage', 'Pass', 'Checkpoint')
    lines += table((*group_columns, 'Input point base', '<1e-6', '<1e-4', '<1e-2', '<0.1',
                    'Short identities', 'Short identities hidden', 'Error'), counts)
    lines += table((*group_columns, 'Order', 'Metric', 'Space', 'A', 'A XYZ', 'A surface/border/new',
                    'B', 'B XYZ', 'B surface/border/new', 'Physical', 'Log row'), observations)

    near_by_pass = defaultdict(list)
    for row in rows[NEAR]:
        near_by_pass[pass_scope(row)].append((row, 'm'))
    origins, matched_splits = [], {}
    seen = defaultdict(set)
    for group, data in groups.items():
        event, rank, stage, pass_number, checkpoint = group
        for row in sampled_short(data):
            key = edge(row)
            pass_key = pass_scope(row)
            if key in seen[pass_key]:
                continue
            seen[pass_key].add(key)
            prior_names = {'c0': (), 'c1': ('c0',), 'end': ('c0', 'c1'), 'shifted': ()}.get(checkpoint, ())
            prior = [groups.get((*pass_key, name), ()) for name in prior_names]
            hidden = any(census(previous) != len(sampled_short(previous)) for previous in prior)
            split_rows = [split for vertex in key for split in split_records(near_by_pass[pass_key], vertex)
                          if split.line < row.line]
            if checkpoint == 'c0':
                origin, evidence = 'Present at pass input', '—'
            elif checkpoint == 'c1':
                if split_rows:
                    origin = 'Surface work created a new endpoint and the short edge'
                else:
                    origin = 'First visible after surface work' if hidden else 'First below 0.01 after surface work'
                evidence = ', '.join(f"{s.value('step')} {s.source}" for s in split_rows) or 'no P2 row'
                matched_splits.update({(s.rank, s.line): s for s in split_rows})
            elif checkpoint == 'end':
                origin = 'First visible after volume work' if hidden else 'First below 0.01 after volume work'
                evidence = '—'
            elif checkpoint == 'shifted':
                before = [p for p in all_short[event, stage] if p.value('checkpoint') != 'shifted' and edge(p) == key]
                origin = 'Observed before shift' if before else 'First visible after shift. Shift preserves coordinates and tensors.'
                evidence = ', '.join(p.source for p in before) or '—'
            else:
                origin, evidence = 'Checkpoint unavailable', '—'
            origins.append([event, rank, stage, pass_number, checkpoint, row.value('a'), row.value('b'),
                            row.value('metric'), origin, evidence, row.source])
    lines += ['### First observed short-edge checkpoint in each pass', '',
              'The rows identify entry to the short-length bin. A matched P2 row identifies the placement of a '
              'new endpoint. Earlier censuses can still contain an edge above the bin threshold.', '',
              *table((*group_columns, 'A', 'B', 'Metric', 'Origin in this pass', 'Corroborating rows', 'P1 row'), origins)]
    fields_to_show = ('step', 'a', 'a_xyz', 'b', 'b_xyz', 'm', 'm_xyz', 'feature', 'nearest', 'nearest_xyz',
                      'nearest_surface', 'nearest_border', 'distance', 'limit', 'ab_space', 'ab_metric',
                      'min_metric', 'face0', 'facet0', 'face1', 'facet1')
    lines += ['### P2 rows cited by P1', '']
    lines += table(('Event', 'Rank', 'Stage', 'Pass', *fields_to_show, 'Log row'),
                   [[*pass_scope(row), *(row.value(key) for key in fields_to_show), row.source]
                    for row in matched_splits.values()])
    return lines


def seed_edge(row, index=0):
    return row.value(f'e{index}_a'), row.value(f'e{index}_b'), row.value(f'e{index}_metric')


def ratio(before, after):
    if before is None or after is None or after <= 0:
        return None
    return before / after


def shown_ratio(value):
    return f'{value:.6g}' if value is not None else '—'


def minimum_section(rows):
    lines = ['## P4 · Shifted core minimum, engine seed minimum and first LENGTH minimum', '',
             'A drop of at least one order of magnitude is highlighted below. Exact observed minima and '
             'ratios are shown throughout. P4 includes its measured-cell count. LENGTH reports a scalar '
             'minimum without the minimizing endpoint IDs.', '']
    shifted, seed, lengths = {}, defaultdict(list), defaultdict(list)
    for row in rows[SHORT]:
        if row.value('checkpoint') == 'shifted' and row.value('order') == '1':
            shifted[scope(row)] = row
    for row in rows[SEED]:
        seed[scope(row)].append(row)
    for row in rows[LENGTH]:
        lengths[scope(row)].append(row)

    summary, seed_rows = [], []
    for key in sorted(shifted.keys() | seed.keys() | lengths.keys(),
                      key=lambda k: (k[0], k[1], -1 if k[2] is None else k[2])):
        core = shifted.get(key)
        engine = next(iter(seed[key]), None)
        collapse = next(iter(lengths[key]), None)
        core_value = core.value('metric') if core else '—'
        engine_value = seed_edge(engine)[2] if engine else '—'
        collapse_value = collapse.value('minimum_length') if collapse else '—'
        core_to_seed = ratio(number(core_value), number(engine_value))
        seed_to_collapse = ratio(number(engine_value), number(collapse_value))
        complete = engine and engine.value('cells') == engine.value('measured') and 'measured' in engine.fields
        observations = []
        if core_to_seed is not None and core_to_seed >= 10:
            observations.append('Engine metric already gives a much shorter minimum before collapse')
        if seed_to_collapse is not None and seed_to_collapse >= 10:
            if complete and collapse.value('pass') == 'collapse':
                observations.append('A much shorter edge appears in the first collapse pass. LENGTH omits its IDs.')
            elif not complete:
                observations.append('Seed census partial. First LENGTH minimum alone cannot attribute edge creation.')
            else:
                observations.append('First LENGTH row is outside a collapse pass')
        summary.append([*key, core_value, f'{core.value("a")}–{core.value("b")}' if core else '—',
                        engine_value, f'{seed_edge(engine)[0]}–{seed_edge(engine)[1]}' if engine else '—',
                        f'{engine.value("measured")}/{engine.value("cells")}' if engine else '—',
                        collapse_value, collapse.value('pass') if collapse else '—',
                        shown_ratio(core_to_seed), shown_ratio(seed_to_collapse),
                        '. '.join(note.rstrip('.') for note in observations) or '—',
                        ' / '.join(row.source if row else '—' for row in (core, engine, collapse))])
        for ordinal, row in enumerate(seed[key], 1):
            bins = [row.value(name) for name in ('lt_1e_6', 'lt_1e_4', 'lt_1e_2')]
            bins.append(row.value('lt_0_1', row.value('lt_01')))
            for index in range(5):
                if f'e{index}_a' not in row.fields:
                    continue
                a, b, length = seed_edge(row, index)
                seed_rows.append([*key, ordinal, row.value('cells'), row.value('measured'), row.value('edges'),
                                  *bins, index + 1, a, b, length, row.value(f'e{index}_a_xyz'),
                                  row.value(f'e{index}_b_xyz'), row.source])
    lines += table(('Event', 'Rank', 'Stage', 'Shifted core', 'Core edge', 'Engine seed', 'Seed edge',
                    'Seed measured/cells', 'First LENGTH', 'LENGTH pass', 'Core/seed', 'Seed/LENGTH',
                    'Observation', 'Core / seed / LENGTH rows'), summary)
    lines += ['### P4 seed censuses and visible edges', '',
              *table(('Event', 'Rank', 'Stage', 'Seed row', 'Cells', 'Measured', 'Edges',
                       '<1e-6', '<1e-4', '<1e-2', '<0.1', 'Order', 'A', 'B', 'Metric', 'A XYZ', 'B XYZ', 'Log row'),
                      seed_rows)]
    return lines


def near_section(rows):
    lines = ['## P2 · Near-split counts', '',
             'The sheet/feature columns count printed rows. The per-pass cap is reported as an '
             'unattributed left-out count because suppressed rows carry no step or kind.', '']
    counts = defaultdict(Counter)
    for row in rows[NEAR]:
        counts[pass_scope(row)][row.value('step'), row.value('feature')] += 1
    caps = defaultdict(list)
    for row in rows[NEAR_LEFT]:
        caps[pass_scope(row)].append(row)
    bins = [('L', '0'), ('L', '1'), ('D', '0'), ('D', '1')]
    data = []
    for key in sorted(counts.keys() | caps.keys(),
                      key=lambda k: (k[0], k[1], -1 if k[2] is None else k[2], k[3])):
        selected = counts[key]
        observed = sum(selected.values())
        reported = caps[key]
        cap = reported[0] if len(reported) == 1 else None
        data.append([*key, *(selected[category] for category in bins), observed,
                     cap.value('printed') if cap else '—', cap.value('left_out') if cap else '—',
                     'equal' if cap and str(observed) == cap.value('printed') and observed <= 50
                     else ('different' if cap else f'{len(reported)} cap rows')])
    lines += table(('Event', 'Rank', 'Stage', 'Pass', 'L sheet', 'L feature', 'D sheet', 'D feature',
                    'Observed', 'Printed', 'Left out', 'Reconciliation'), data)
    return lines


def run_section(case, rows, ranks):
    path = case / 'status.json'
    status = json.loads(path.read_text()) if path.is_file() else {}
    lines = ['## Run outcome', '',
             'One diagnosis under the pinned S1 controller. Production baselines and acceptance are outside '
             'the scope of this run.', '',
             f'Controller state `{status.get("state", "unavailable")}`, '
             f'exit `{status.get("exit_code", "unavailable")}`, '
             f'stop reason `{status.get("stop_reason", "unavailable")}`. '
             f'Rank logs present for {", ".join(map(str, ranks)) or "no ranks"}.', '']
    native = defaultdict(list)
    for row in rows['NATIVE3D']:
        native[row.event].append(row)
    for event in sorted({*native, *(row.event for row in rows[TIMER])}):
        roots = [row for row in rows[TIMER] if row.event == event]
        snapshots = ', '.join(f'{row.value("rank")}={row.value("complete")}' for row in native[event])
        maximum = max((value for row in roots if (value := number(row.value('root_seconds'))) is not None), default=None)
        lines.append(f'- Event {event}. Native completion by rank {snapshots or "unavailable"}. '
                     f'Terminal remesh timer rows {len(roots)}. Maximum root seconds {maximum}.')
    if not rows[REJECT]:
        lines.append('- The requested metric-rejection branch emitted no row.')
    elif not any(row.event == 2 for row in rows[REJECT]):
        lines.append('- No event-2 metric-rejection row was observed. Other observed rows are reported.')
    return lines + ['']


def render(build, case, base, rows, ranks):
    return '\n'.join([
        f'# FP2-B diagnosis run D1 · build{build}', '',
        '[Pinned work order](../inputs/FP2_B_DIAGNOSIS_1.md) · '
        '[Whole-item source check](../source_check.md) · '
        f'[Run status](../G5/build{build}/S1/{case.name}/status.json)', '',
        'Row locators use `r<rank>:<line>` in that rank’s S1 stdout. Event numbers come from each '
        'rank’s final remesh timer summaries. Engine rows use the latest numbered local-core stage.', '',
        *run_section(case, rows, ranks),
        *sanity(base, rows, case),
        *reject_section(rows),
        *short_section(rows),
        *minimum_section(rows),
        *near_section(rows),
    ])


def self_check():
    prefix = 'Sun Nov 22 06:11:35 2026<stdout>:'
    stream = [
        'NATIVE3D_LOCAL_SURFACE rank=0 stage=0 pass=1 splits_length=3 seconds_length=0 outward_max=0',
        'NATIVE3D_LOCAL_SHORT rank=0 stage=0 pass=0 checkpoint=shifted lt_1e_2=7 order=1 '
        'metric=0.005 a=4 b=7 a_xyz=0,0,0 b_xyz=1,1,1',
        'NATIVE3D_SEED_SHORT rank=0 cells=12 measured=12 e0_a=4 e0_b=7 e0_metric=0.0001',
        'NATIVE3D_LENGTH rank=0 pass=collapse minimum_length=0.0000001',
        'NATIVE3D_TIMER_SUMMARY rank=0 root=remesh boundary=final root_seconds=1600',
        'NATIVE3D_LOCAL_SHORT rank=0 stage=0 pass=1 checkpoint=c0 input_point_base=12 lt_1e_2=0',
        'NATIVE3D_METRIC_REJECT rank=0 phase=improvement cell=8 v0=4 reason="bad metric at \\"node\\""',
    ]
    rows = defaultdict(list)
    for row in scan_rank((prefix + line + '\n' for line in stream), 0):
        rows[row.kind].append(row)
    assert (rows[REJECT][0].event, rows[REJECT][0].stage) == (2, 0)
    assert rows[REJECT][0].value('reason') == 'bad metric at "node"'
    assert integer_fields(rows[SURFACE][0]) == {'rank': 0, 'stage': 0, 'pass': 1, 'splits_length': 3}
    assert first_surface_difference(rows, rows) is None
    assert census(rows[SHORT]) - len(sampled_short(rows[SHORT])) == 6
    assert 'First visible after shift' in '\n'.join(short_section(rows))
    assert 'first collapse pass' in '\n'.join(minimum_section(rows))
    assert 'Event input' in '\n'.join(reject_section(rows))
    baseline, ranks = read_case(REFERENCE)
    assert ranks == list(range(6)) and len(surface_rows(baseline)) == 12
    assert first_length(baseline, 2, 0).value('minimum_length') == '3.4973302189904171e-09'
    baseline_report = render(633, REFERENCE, baseline, baseline, ranks)
    assert 'Exact numeric match.' in baseline_report and 'Native completion by rank 0=0' in baseline_report
    print('D1 report parser self-check passed (including build633).')


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('build', type=int, nargs='?')
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
        return
    if args.build is None:
        parser.error('a build label is required')
    work = HERE / 'G5' / f'build{args.build}' / 'S1'
    case = work / f'full{args.build}_run1'
    assert case.is_dir(), case
    base, _ = read_case(REFERENCE)
    rows, ranks = read_case(case)
    report = HERE / 'reports' / f'FP2_B_D1_{args.build}.md'
    report.parent.mkdir(parents=True, exist_ok=True)
    contents = render(args.build, case, base, rows, ranks)
    if not report.is_file():
        report.write_text(contents)
    elif not report.read_text().startswith(contents):
        raise RuntimeError(f'Existing report differs: {report}')
    print(report)


if __name__ == '__main__':
    main()
