"""Compare RCFD2's rank-0 metric rows and adapted wing sizes against R-S."""

from argparse import ArgumentParser
import json
from pathlib import Path
import re

from report_events import PARSER, read, six_number
from report_flow import artifact_lines, table

HERE = Path(__file__).resolve().parent
RUNS = ('S', 'SM', 'SG')
CYCLE = re.compile(r'^Cycle ([012]) of 2:')
TARGET = 'Its metric makes the next mesh'
METRICS = ('Maximum density', 'Maximum cell AR', 'Sensor metric complexity before gradation',
           'Sensor-only donor complexity after gradation', 'Native metric work', 'Native sensor gradation')


def metric_rows(lines):
    cycles = {cycle: {} for cycle in range(3)}
    cycle = None
    for line in lines:
        if not line.endswith('\n'):
            continue
        clean = PARSER['timer'].strip_mpi_prefixes(line).strip()
        if match := CYCLE.match(clean):
            cycle = int(match[1])
        key, _, value = clean.partition(': ')
        if cycle is not None and key in (TARGET, *METRICS):
            cycles[cycle].setdefault(key, []).append(value)
    return cycles


def mesh_counts(mesh):
    counts = {}
    marker = None
    with artifact_lines(mesh) as lines:
        for line in lines:
            if not line.endswith('\n'):
                continue
            key, _, value = line.partition('=')
            if key == 'MARKER_TAG':
                marker = value.strip()
            elif key in ('NPOIN', 'NELEM') or (key == 'MARKER_ELEMS' and marker == 'WING3D'):
                columns = value.split()
                if columns and columns[0].isdigit():
                    counts[key] = int(columns[0])
                if len(counts) == 3:
                    break
    return dict(points=counts.get('NPOIN'), cells=counts.get('NELEM'), wing_faces=counts.get('MARKER_ELEMS'))


def collect_run(work):
    case = work / 'full647_run1'
    log = case / 'rank_output/1/rank.0/stdout'
    with artifact_lines(log) as source:
        lines = list(source)
    sealed = {row['event']: row for row in read(work / 'run_receipt.json').get('events', [])}
    connected = {row['event']: row for row in read(case / 'full_workflow_receipt.json').get('events', [])}
    terminal_blocks = PARSER['event_summary_blocks'](lines)
    events = {}
    for event in (1, 2):
        rows = terminal_blocks[event - 1].values() if len(terminal_blocks) >= event else ()
        full = connected.get(event, {})
        receipt = sealed.get(event, {})
        native = receipt.get('native_seconds', full.get('whole_event_seconds_max_rank'))
        if native is None:
            native = six_number(rows, 'total', float, max)
        mesh = case / f'adapted_mesh_adap_{event:05d}.su2'
        counts = mesh_counts(mesh)
        for key in ('points', 'cells'):
            if counts[key] is None:
                counts[key] = full.get('actual_mesh_counts', {}).get(key)
            if counts[key] is None:
                counts[key] = receipt.get('actual_mesh_counts', {}).get(key)
        events[event] = dict(native_seconds=native, **counts)
    return {'cycles': metric_rows(lines), 'events': events}


def collect(g5_directory=HERE / 'G5/build647'):
    return {run: collect_run(g5_directory / run) for run in RUNS}


def print_report(report):
    print('## Metric settings and work\n')
    print('Cycles 0 and 1 produce the metrics for events 1 and 2. Cycle 2 computes a metric after the '
          'final flow iterations. The expected R-SM and R-SG maximum density is at most 3.7037e+07. '
          'An em dash marks an unavailable row.\n')
    rows = []
    for cycle in range(3):
        for key in ((TARGET, *METRICS) if cycle < 2 else METRICS):
            values = [' / '.join(report[run]['cycles'][cycle].get(key, [])) for run in RUNS]
            rows.append((cycle, key, *values))
    table(('Cycle', 'Rank 0 row', *(f'R-{run}' for run in RUNS)), rows)

    print('## Adapted mesh sizes by event\n')
    print('Wing faces count the `WING3D` marker elements in the adapted mesh.\n')
    rows = []
    for event in (1, 2):
        for run in RUNS:
            stats = report[run]['events'][event]
            native = stats['native_seconds']
            rows.append((event, f'R-{run}', f'{native:.3f}' if native is not None else None,
                         stats['points'], stats['cells'], stats['wing_faces']))
    table(('Event', 'Run', 'Native maximum (s)', 'Points', 'Cells', 'Wing faces'), rows)


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('--json', action='store_true', help='Print report data as JSON.')
    args = parser.parse_args()
    report = collect()
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print_report(report)


if __name__ == '__main__':
    main()
