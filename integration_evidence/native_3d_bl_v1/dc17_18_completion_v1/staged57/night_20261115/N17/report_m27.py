"""Collect the M27 measurement from the sealed six-rank phase C run."""
import argparse
from collections import defaultdict
import csv
from decimal import Decimal
import hashlib
import json
from pathlib import Path
import re
import runpy


PARTS = ('none', 'a_other', 'a_old', 'a_fresh', 'b_other', 'b_old', 'b_candidate', 'c', 'd', 'e', 'g')
STAGES = ('quality', 'other', 'final_proof', 'surface')
QUERY = ('queries', 'hits', 'miss_new', 'miss_repeat', 'miss_other_kind',
         'work_new', 'work_repeat', 'work_other_kind', 'seconds_new', 'seconds_repeat', 'seconds_other_kind',
         'faces_reached_census', 'work_reached_census')
SMOOTH = ('calls', 'checked', 'passed', 'moved', 'checked_1', 'checked_2', 'checked_3', 'checked_4',
          'checked_5plus', 'passed_0', 'passed_1', 'passed_2', 'passed_3plus')
TABLE = ('faces', 'from_earlier_event', 'observer_failed', 'estimated_bytes')
MEMO = ('memo_stores', 'memo_evictions', 'memo_union_entries', 'memo_bytes', 'memo_limit')
MARKERS = re.compile(r'\b(NATIVE3D_M27_(?:QUERY|SMOOTH|TABLE)|NATIVE3D_STAGE|D9_QUERY)\s+(?=rank=)')
FIELD = re.compile(r'(\w+)=([^\s]+)')
STRIP_MPI = runpy.run_path(str(Path(__file__).resolve().parents[2] /
                              'nobl_clean_check_20261114/extract_markers.py'))['strip_mpi_prefixes']


def read_rank(content, rank):
    """Bind rows to each event at its M27 table, the last row of the D9 root."""
    events, raw = [], []
    query, d9 = {}, {}
    smooth, last_stage = None, None
    for lineno, line in enumerate(content.splitlines(), 1):
        clean = STRIP_MPI(line)
        marker = MARKERS.search(clean)
        if marker is None:
            continue
        values = dict(FIELD.findall(clean[marker.end():]))
        if int(values['rank']) != rank:
            continue
        tag = marker[1]
        if tag == 'NATIVE3D_STAGE':
            last_stage = values
            continue
        if tag == 'D9_QUERY':
            if values['phase'] == 'quality':
                d9[values['site']] = values
            continue
        raw.append(dict(event=len(events) + 1, record=tag, line=lineno, raw=line, **values))
        if tag == 'NATIVE3D_M27_QUERY':
            key = values['stage'], values['part']
            assert key not in query and key[0] in STAGES and key[1] in PARTS, (rank, key)
            assert set((*QUERY, 'bypass')).issubset(values), (rank, key, lineno)
            query[key] = values
        elif tag == 'NATIVE3D_M27_SMOOTH':
            assert smooth is None, rank
            assert set(SMOOTH).issubset(values), (rank, lineno)
            smooth = values
        else:
            assert smooth is not None and last_stage is not None and last_stage['stage'] == 'exit'
            assert set('abcde').issubset(d9) and all(field in last_stage for field in MEMO)
            assert set(TABLE).issubset(values), (rank, lineno)
            events.append(dict(query=query, smooth=smooth, table=values, d9=d9, memo=last_stage))
            query, d9 = {}, {}
            smooth, last_stage = None, None
    assert not query and not d9 and smooth is None
    return events, raw


def number(record, field):
    if field.startswith('seconds_'):
        return Decimal(record.get(field, '0'))
    return int(record.get(field, '0'))


def table(headers, rows):
    def row(values):
        return '| ' + ' | '.join(map(str, values)) + ' |\n'
    return row(headers) + row(['---'] * len(headers)) + ''.join(row(values) for values in rows) + '\n'


def summarize(records, field, ranks):
    values = [number(record, field) for record in records]
    total = sum(values)
    if not ranks:
        return total
    return f'{total} ({min(values)}–{max(values)})'


def sum_query_parts(record, stage):
    return {field: sum(number(row, field) for (at, _), row in record['query'].items() if at == stage)
            for field in QUERY}


def render_event(event, data):
    """Report query attribution, smoothing chains, memo state and the D9 cross-check."""
    report = [f'## Event {event}\n\n']
    for stage in STAGES:
        ranks = stage == 'quality'
        title = 'sum (smallest rank–largest rank)' if ranks else 'six-rank sums'
        report.append(f'### Table Q — {stage}, {title}\n\n')
        rows = []
        for part in PARTS:
            parts = [rank['query'].get((stage, part), {}) for rank in data]
            rows.append([part, *(summarize(parts, field, ranks) for field in QUERY)])
        totals = [sum_query_parts(rank, stage) for rank in data]
        rows.append(['Total', *(summarize(totals, field, ranks) for field in QUERY)])
        report.append(table(('Part', *QUERY), rows))

    report.append('### Table W — first computations without a later Census credit\n\n')
    rows = []
    for part in PARTS:
        parts = [rank['query'].get(('quality', part), {}) for rank in data]
        faces = sum(number(row, 'miss_new') - number(row, 'faces_reached_census') for row in parts)
        work = sum(number(row, 'work_new') - number(row, 'work_reached_census') for row in parts)
        rows.append([part, faces, work])
    report.append(table(('Part', 'miss_new − faces_reached_census', 'work_new − work_reached_census'), rows))

    report.append('### Table S — quality smoothing chains\n\n')
    smooth = [rank['smooth'] for rank in data]
    rows = [[rank, *(number(row, field) for field in SMOOTH)] for rank, row in enumerate(smooth)]
    rows.append(['Total', *(summarize(smooth, field, False) for field in SMOOTH)])
    report.append(table(('Rank', *SMOOTH), rows))
    report.append('### Observation table\n\n')
    report.append(table(('Rank', *TABLE), [[rank, *(record['table'][field] for field in TABLE)]
                                            for rank, record in enumerate(data)]))
    report.append('### Quality D9 cross-check\n\n')
    rows = []
    for rank, record in enumerate(data):
        for site in 'abcde':
            parts = [values for (stage, part), values in record['query'].items()
                     if stage == 'quality' and (part == site or part.startswith(site + '_'))]
            queries = sum(number(part, 'queries') + number(part, 'bypass') for part in parts)
            hits = sum(number(part, 'hits') for part in parts)
            upper = number(record['d9'][site], 'upper_calls')
            memo = number(record['d9'][site], 'memo_union_hits')
            rows.append([rank, site, queries, upper, queries - upper, hits, memo, hits - memo])
    report.append(table(('Rank', 'Site', 'M27 queries + bypass', 'D9 upper_calls', 'Difference',
                         'M27 hits', 'D9 memo_union_hits', 'Difference'), rows))
    report.append('### Final stage memo counters\n\n')
    report.append(table(('Rank', *MEMO), [[rank, *(record['memo'][field] for field in MEMO)]
                                         for rank, record in enumerate(data)]))
    return ''.join(report)


def report(case, directory, build):
    identity = json.loads((case / 'n8_payload_identity.json').read_text())
    assert identity['passed'] and identity['workflow_gates_passed']
    ranks, all_raw = defaultdict(list), []
    for rank in range(6):
        name = f'rank_output/1/rank.{rank}/stdout'
        content = (case / name).read_bytes()
        assert hashlib.sha256(content).hexdigest() == identity['rank_stdout_sha256'][name]
        events, raw = read_rank(content.decode(errors='replace'), rank)
        assert len(events) == 2, (rank, len(events))
        for index, event in enumerate(events, 1):
            ranks[index].append(event)
        all_raw.extend(dict(stream=name, **row) for row in raw)
    stem = f'M27_build{build}'
    all_raw.sort(key=lambda row: (row['event'], int(row['rank']), row['line']))
    first = ('event', 'rank', 'record', 'stream', 'line', 'stage', 'part')
    fields = sorted(set().union(*(row.keys() for row in all_raw)) - set(first) - {'raw'})
    with (directory / f'{stem}.csv').open('x', newline='') as file:
        writer = csv.DictWriter(file, fieldnames=[*first, *fields, 'raw'])
        writer.writeheader()
        writer.writerows(all_raw)
    with (directory / f'{stem}.log').open('x') as file:
        file.writelines(row['raw'] + '\n' for row in all_raw)
    header = (f'# M27 wall-query measurements — build {build}\n\n'
              f'[CSV with original lines]({stem}.csv) · [Raw M27 lines]({stem}.log)\n\n'
              'Missing query rows contribute zero for their stage and part. Estimated bytes cover entries, '
              'node links and buckets. The D9 differences are reported as M27 minus D9.\n\n')
    with (directory / f'{stem}.md').open('x') as file:
        file.write(header + ''.join(render_event(event, data) for event, data in sorted(ranks.items())))


def self_check():
    example = ('NATIVE3D_STAGE rank=0 stage=exit ' + ' '.join(f'{field}=1' for field in MEMO) + '\n')
    example += ''.join(f'D9_QUERY rank=0 phase=quality site={site} upper_calls=1 memo_union_hits=0\n'
                       for site in 'abcde')
    numbers = dict(queries=1, miss_new=1, seconds_new='0.125', work_new=2)
    example += 'NATIVE3D_M27_QUERY rank=0 stage=quality part=a_fresh '
    example += ' '.join(f'{field}={numbers.get(field, 0)}' for field in (*QUERY, 'bypass')) + '\n'
    example += 'NATIVE3D_M27_SMOOTH rank=0 ' + ' '.join(f'{field}=0' for field in SMOOTH) + '\n'
    example += 'NATIVE3D_M27_TABLE rank=0 faces=1 from_earlier_event=0 observer_failed=0 estimated_bytes=96\n'
    fragmented = example.replace('seconds_new=0.125', 'seconds_neWed Nov 18 06:25:41 2026<stdout>:w=0.125')
    events, raw = read_rank(fragmented * 2, 0)
    assert 'seconds_neWed Nov' in raw[0]['raw']
    assert len(events) == 2 and [row['event'] for row in raw] == [1, 1, 1, 2, 2, 2]
    assert sum_query_parts(events[0], 'quality')['seconds_new'] == Decimal('0.125')
    assert '| a_fresh | 6 (1–1) |' in render_event(1, [events[0]] * 6)
    assert '| a_fresh | 6 | 12 |' in render_event(1, [events[0]] * 6)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--self-check', action='store_true')
    parser.add_argument('--case', type=Path)
    parser.add_argument('--build', type=int)
    args = parser.parse_args()
    if args.self_check:
        self_check()
    else:
        if args.case is None or args.build is None:
            parser.error('provide --case and --build')
        report(args.case, Path(__file__).resolve().parent / 'reports', args.build)
