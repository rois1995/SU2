#!/usr/bin/env python3
"""Extract M28 measurements from saved rank logs, including partial or rejected events."""
import argparse
from collections import defaultdict
import csv
from dataclasses import dataclass
import gzip
import hashlib
import json
from math import fsum
from pathlib import Path
import re
import runpy
import sys

NIGHT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(NIGHT / 'N16'))
from event_prefix import CANONICAL, FIELD
from report_85r import CERTIFICATION, COUNTERS, OPERATIONS

TIMERS = runpy.run_path(str(NIGHT / 'N4/section6_3_after_N11/round_8_5r_B/G5/check_timer_tree.py'))
LEGACY_CSV = NIGHT / 'N16/reports/STEP_8_5R_575.csv'
MARKER = re.compile(r'\b(?:NATIVE3D_[A-Z0-9_]+|NATIVE3D(?=\s+rank=))\b')
CENSUS = 'NATIVE3D_QUALITY'
STUCK = 'NATIVE3D_M28_STUCK'
MORE = 'NATIVE3D_M28_STUCK_MORE'
PRIVATE = 'NATIVE3D_PRIVATE_WORK'
OWNERSHIP = 'NATIVE3D_PHASE_OWNERSHIP'
GATHERED = {'NATIVE3D', 'NATIVE3D_DETAIL'}
STUDIED = GATHERED | {CENSUS, STUCK, MORE, PRIVATE, OWNERSHIP, 'NATIVE3D_PARTITION',
                      'NATIVE3D_QUALITY_LADDER', 'NATIVE3D_STAGE', 'NATIVE3D_STORAGE',
                      'NATIVE3D_TIMER', 'NATIVE3D_TIMER_SUMMARY'}
M28_COLUMNS = ('lost_own', 'lost_other', 'quality_ownership_evaluations')


@dataclass
class Row:
    tag: str
    fields: dict
    source: str
    number: int
    raw: str
    text: str

    def location(self):
        return {'source': self.source, 'line': self.number, 'raw': self.raw}


def read_rows(path, rank):
    """Read the exact complete physical lines and reuse the N16 MPI-prefix convention."""
    uncompressed = gzip.decompress(path.read_bytes()) if path.suffix == '.gz' else path.read_bytes()
    digest = hashlib.sha256(uncompressed).hexdigest()
    events = []
    for number, raw in enumerate(uncompressed.decode(errors='replace').splitlines(keepends=True), 1):
        if not raw.endswith(('\n', '\r')):
            continue
        raw = raw.rstrip('\r\n')
        text = CANONICAL['strip_mpi_prefixes'](raw)
        marks = list(MARKER.finditer(text))
        for index, marker in enumerate(marks):
            tag = marker[0]
            if tag not in STUDIED:
                continue
            end = marks[index + 1].start() if index + 1 < len(marks) else len(text)
            fields = dict(FIELD.findall(text[marker.end():end]))
            if 'rank' not in fields or (fields['rank'] != str(rank) and tag not in GATHERED):
                continue
            if tag == 'NATIVE3D_PARTITION':
                events.append(defaultdict(list))
            if events:
                events[-1][tag].append(Row(tag, fields, str(path), number, raw, text[marker.start():end]))
    return events, digest


def integer(row, key):
    if row is not None and key in row.fields:
        return int(row.fields[key])
    return None


def aggregate_rows(rows, key, operation=sum, convert=int):
    """Leave unavailable fields blank while preserving actual observed zeroes."""
    values = [convert(row.fields[key]) for row in rows if key in row.fields]
    return operation(values) if values else None


def timer_values(tags):
    ordered = sorted(tags['NATIVE3D_TIMER'] + tags['NATIVE3D_TIMER_SUMMARY'], key=lambda row: row.number)
    snapshots = TIMERS['snapshots'](''.join(row.text + '\n' for row in ordered))
    ledger = next((snapshot for snapshot in reversed(snapshots)
                   if snapshot['summary']['root'] == 'remesh'), None)
    if ledger is None:
        return {}, []
    summary = ledger['summary']
    index = next(index for index in range(len(ordered) - 1, -1, -1)
                 if ordered[index].tag == 'NATIVE3D_TIMER_SUMMARY' and ordered[index].fields == summary)
    summary_row = ordered[index]
    exclusive = {f'quality_{op}_seconds': fsum(float(row['exclusive_seconds']) for row in ledger['rows']
                 if row['phase'] == 'quality' and row['operation'] == op) for op in OPERATIONS}
    exclusive['quality_purpose_seconds'] = fsum(exclusive[f'quality_{op}_seconds'] for op in OPERATIONS[:3])
    exclusive['root_seconds'] = float(summary['root_seconds'])
    # A cumulative snapshot can repeat the same operation values at the same boundary.
    # Its own operation rows are the ones since the preceding summary for that key.
    operations = []
    for row in reversed(ordered[:index]):
        if any(row.fields[key] != summary[key] for key in ('root', 'boundary', 'rank')):
            continue
        if row.tag == 'NATIVE3D_TIMER_SUMMARY':
            break
        operations.append(row.location())
    return exclusive, [summary_row.location(), *reversed(operations)]


def rank_values(tags, gathered, name, event, rank):
    private = tags[PRIVATE]
    quality = [row for row in tags[CENSUS] if row.fields.get('complete') == '1']
    quality_exit = next((row for row in reversed(quality) if row.fields.get('stage') == 'exit'), None)
    last_census = quality[-1] if quality else None
    final = next((row for row in reversed(tags['NATIVE3D_STAGE']) if row.fields.get('stage') == 'exit'
                  and all(key in row.fields for key in CERTIFICATION)), None)
    rank_text = str(rank)
    native = next((row for row in reversed(gathered['NATIVE3D']) if row.fields['rank'] == rank_text), None)
    detail = next((row for row in reversed(gathered['NATIVE3D_DETAIL']) if row.fields['rank'] == rank_text
                   and row.fields.get('stage') == 'refine'), None)
    ownership = [row for row in tags[OWNERSHIP] if row.fields.get('quality') == '1']
    evaluations = [row for row in ownership if row.fields.get('evaluate') == '1']
    migrations = [row for row in ownership if row.fields.get('committed') == '1']
    ladder = tags['NATIVE3D_QUALITY_LADDER']
    values = dict(run=name, event=event, rank=rank, quality_rounds=len(private),
                  quality_commits=integer(quality_exit or last_census, 'commits_total'),
                  quality_trials=aggregate_rows(ladder, 'trials'),
                  private_work_used=aggregate_rows(private, 'private_work_used'),
                  private_busy_seconds=aggregate_rows(private, 'busy_seconds', fsum, float),
                  largest_round_choices=aggregate_rows(private, 'choices', max),
                  quality_windows=sum(row.fields.get('stage') == 'window' for row in quality),
                  quality_ownership_evaluations=len(evaluations), quality_ownership_migrations=len(migrations),
                  process_peak_bytes=integer(native, 'process_peak_rss'),
                  modeled_peak_bytes=integer(native, 'native_bound'),
                  engine_modeled_peak_bytes=aggregate_rows(tags['NATIVE3D_STORAGE'], 'native_peak_bytes', max))
    timers, timer_sources = timer_values(tags)
    values.update(timers)
    values.update({key: integer(detail, key) for key in COUNTERS})
    values.update({f'final_{key}': integer(final, key) for key in CERTIFICATION})
    for key in ('lost_own', 'lost_other'):
        values[key] = aggregate_rows(private, key)
    provenance = {**{key: values[key] for key in ('run', 'event', 'rank')},
                  'partition': tags['NATIVE3D_PARTITION'][0].location(),
                  'timer_snapshot': timer_sources,
                  'last_census': last_census.location() if last_census else None,
                  'quality_commit_census': (quality_exit or last_census).location() if last_census else None,
                  'refine_detail': detail.location() if detail else None,
                  'native_summary': native.location() if native else None,
                  'final_certification': final.location() if final else None,
                  'loss_rows_with_both_fields': sum('lost_own' in row.fields and 'lost_other' in row.fields
                                                    for row in private),
                  'private_work_rows': [row.location() for row in private],
                  'quality_ownership_rows': [row.location() for row in ownership]}
    return values, provenance


def gather_stuck(events_by_rank):
    first = {}
    last = {}
    for rank, events in events_by_rank.items():
        for number, tags in enumerate(events, 1):
            # The printer emits stuck lines directly after their own census and related detail rows.
            timeline = sorted(tags[CENSUS] + tags[STUCK] + tags[MORE], key=lambda row: row.number)
            for row in timeline:
                if row.tag == CENSUS and row.fields.get('complete') == '1':
                    last[rank] = (number, row, [], [])
                elif row.tag in {STUCK, MORE} and rank in last:
                    event, census, stuck, more = last[rank]
                    if event != number or any(row.fields.get(key) != census.fields.get(key)
                                              for key in ('stage', 'round')):
                        continue
                    if row.tag == MORE:
                        more.append(row)
                        continue
                    stuck.append(row)
                    key = (number, row.fields['cell'])
                    order = (int(row.fields['round']), int(census.fields['census']), rank, row.number)
                    if key not in first or order < first[key][0]:
                        first[key] = (order, row, census)
    result = []
    for rank, (event, census, stuck, more) in sorted(last.items()):
        last_rows = []
        for row in stuck:
            _, earliest, first_census = first[event, row.fields['cell']]
            last_rows.append({'cell': row.fields['cell'], 'first_rank': int(earliest.fields['rank']),
                              'first_stage': earliest.fields['stage'], 'first_round': int(earliest.fields['round']),
                              'first_line': earliest.location(), 'first_census': first_census.location(),
                              'last_line': row.location()})
        result.append({'event': event, 'rank': rank, 'stage': census.fields['stage'],
                       'round': int(census.fields['round']), 'last_census': census.location(),
                       'stuck_shape': integer(census, 'stuck_shape'), 'cells': last_rows,
                       'overflow': [{'not_printed': integer(row, 'not_printed'), **row.location()} for row in more]})
    return result


def event_sums(rank_rows, accepted_counts):
    """Sum available rank measures and preserve unavailable full-event values as blanks."""
    result = {key: rank_rows[0][key] for key in ('run', 'event')}
    result.update(rank='sum', final_mesh_points=accepted_counts.get('points'),
                  final_mesh_cells=accepted_counts.get('cells'))
    for key in rank_rows[0].keys() - result.keys():
        values = [row.get(key) for row in rank_rows]
        if any(value is None for value in values):
            continue
        if key == 'largest_round_choices':
            result[key] = max(values)
        else:
            result[key] = fsum(values) if isinstance(values[0], float) else sum(values)
    return result


def read_case(name, case):
    events_by_rank = {}
    sources = {}
    for rank_dir in (case / 'rank_output/1').glob('rank.*'):
        rank = int(rank_dir.name.removeprefix('rank.'))
        stdout = rank_dir / 'stdout'
        if not stdout.is_file():
            stdout = stdout.with_suffix('.gz')
        if stdout.is_file():
            events_by_rank[rank], sources[str(rank)] = read_rows(stdout, rank)
            sources[str(rank)] = {'path': str(stdout), 'uncompressed_sha256': sources[str(rank)]}
    if not events_by_rank:
        raise ValueError(f'{case}: no rank stdout logs found')
    workflow = case / 'full_workflow_receipt.json'
    receipt = json.loads(workflow.read_text()) if workflow.is_file() else {}
    accepted_counts = {int(event['event']): event.get('actual_mesh_counts', {}) for event in receipt.get('events', [])}
    rank_rows = defaultdict(list)
    source_rows = []
    master = events_by_rank.get(0, [])
    for rank, events in sorted(events_by_rank.items()):
        for index, tags in enumerate(events):
            gathered = master[index] if index < len(master) else tags
            row, provenance = rank_values(tags, gathered, name, index + 1, rank)
            rank_rows[index + 1].append(row)
            source_rows.append(provenance)
    values = []
    for event, ranks in sorted(rank_rows.items()):
        values.extend([*ranks, event_sums(ranks, accepted_counts.get(event, {}))])
    audit = {'run': name, 'case': str(case), 'rank_logs': sources, 'rank_events': source_rows,
             'last_censuses': gather_stuck(events_by_rank)}
    return values, audit


def stuck_table(audits):
    lines = ['## Stuck cells', '']
    for audit in audits:
        lines.extend([f"### {audit['run']}", ''])
        censuses = audit['last_censuses']
        if not any(census['cells'] for census in censuses):
            lines.extend(['no stuck cell', ''])
        else:
            lines.extend(['| Event | Rank | Last stage/round | Cell | First rank/stage/round | Raw log line |',
                          '|---|---:|---|---:|---|---|'])
            for census in censuses:
                for cell in census['cells']:
                    raw = cell['last_line']['raw'].replace('|', r'\|').replace('`', '&#96;')
                    first_location = cell['first_line']
                    first = (f"{cell['first_rank']}/{cell['first_stage']}/{cell['first_round']} "
                             f"({first_location['source']}:{first_location['line']})")
                    location = cell['last_line']
                    lines.append(f"| {census['event']} | {census['rank']} | {census['stage']}/{census['round']} | "
                                 f"{cell['cell']} | {first} | `{raw}` ({location['source']}:{location['line']}) |")
            lines.append('')
        missing = sorted(set(map(int, audit['rank_logs'])) - {census['rank'] for census in censuses})
        if missing:
            lines.extend([f"Ranks without a printed quality census: {', '.join(map(str, missing))}.", ''])
        for census in censuses:
            for row in census['overflow']:
                lines.append(f"- `{row['raw']}` ({row['source']}:{row['line']})")
            observed = len(census['cells']) + sum(row['not_printed'] for row in census['overflow'])
            if census['stuck_shape'] is not None and observed < census['stuck_shape']:
                lines.append(f"- Event {census['event']}, rank {census['rank']}, {census['stage']}/{census['round']} "
                             f"reported {census['stuck_shape']} stuck shapes and printed {observed} cell records "
                             "including overflow. See census provenance.")
        lines.append('')
    return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--run', action='append', required=True, metavar='LABEL=CASE_DIR')
    parser.add_argument('--csv', type=Path, required=True)
    parser.add_argument('--stuck', type=Path, required=True)
    parser.add_argument('--provenance', type=Path)
    args = parser.parse_args()
    outputs = [args.csv, args.stuck] + ([args.provenance] if args.provenance else [])
    if len({path.resolve() for path in outputs}) != len(outputs) or any(path.exists() for path in outputs):
        parser.error('report outputs must be distinct new files')
    records = []
    audits = []
    for spec in args.run:
        name, sep, directory = spec.partition('=')
        if not (name and sep and directory):
            parser.error('--run expects LABEL=CASE_DIR')
        rows, audit = read_case(name, Path(directory).resolve())
        records.extend(rows)
        audits.append(audit)
    with LEGACY_CSV.open(newline='') as file:
        fields = next(csv.reader(file)) + list(M28_COLUMNS)
    with args.csv.open('x', newline='') as file:
        writer = csv.DictWriter(file, fields, lineterminator='\n')
        writer.writeheader()
        writer.writerows(records)
    with args.stuck.open('x') as file:
        file.write(stuck_table(audits))
    if args.provenance:
        with args.provenance.open('x') as file:
            json.dump(audits, file, indent=2)
            file.write('\n')


if __name__ == '__main__':
    main()
