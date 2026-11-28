"""Check SC1's per-rank diagnostics and derive the signed coverage prediction."""

from argparse import ArgumentParser
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import re
from tempfile import TemporaryDirectory

import run_full  # Load the sealed FP3-E log parser through the standing runner.
from collect_fp3_full import PREFIX, FIELDS

CERT = 'NATIVE3D_GEOMETRY_CERTIFICATES'
DIFFER = 'NATIVE3D_GEOMETRY_CERTIFICATE_DISAGREE'
GAP = 'NATIVE3D_UNATTRIBUTED_SPANS'
SUMMARY = 'NATIVE3D_TIMER_SUMMARY'
TIMER = 'NATIVE3D_TIMER'
PARTITION = 'NATIVE3D_PARTITION'
ASSIGNMENTS = 'NATIVE3D_CERTIFICATE_PARTITION'
STAGE = 'NATIVE3D_STAGE'
OWNERSHIP = 'NATIVE3D_PHASE_OWNERSHIP'
DETAIL = 'NATIVE3D_DETAIL'
KINDS = (CERT, DIFFER, GAP, SUMMARY, TIMER, PARTITION, ASSIGNMENTS, STAGE, OWNERSHIP, DETAIL)
KIND = re.compile(r'\b(?:' + '|'.join(KINDS) + r')(?=\s|$)')
BOUNDARY = re.compile(r'(?:push|pop|root_start|root_finish):[^:\s]+:\d+')
PHASES = ('preparation', 'metric', 'construction', 'surface', 'quality', 'final_proof', 'transfer', 'output')
ORIGINS = ('hit_step_c', 'hit_proof', 'hit_commit')
COUNTS = ('workers', 'eligible_stages', 'stage', 'reader_calls', 'cert_hit', 'cert_miss', *ORIGINS, 'records', 'bytes')
COUNT_FIELDS = ('cert_disagree', 'disagree_finite', 'disagree_nonfinite', 'skip_calls')
WEIGHTS = ('parts', 'uniform_routing_weights', 'cells_before', 'cells_after', 'metric_before', 'metric_after',
           'geometry_before', 'geometry_after', 'geometry_positive_cells', 'geometry_unit_cells',
           'geometry_maximum_cells', 'geometry_weight_scale')
FACET_FIELDS = ('distinct_facets', 'prepared_facet_builds', 'prepared_facet_mean_bytes', 'facet_misses',
                'facet_build_seconds', 'retention_refusals', 'atlas_work')


def number(fields, key):
    value = fields[key]
    if not value.isdecimal():
        raise ValueError(f'{key} must be a nonnegative integer, got {value!r}')
    return int(value)


def seconds(fields, key):
    value = float(fields[key])
    if not math.isfinite(value) or value < 0:
        raise ValueError(f'{key} must be a finite nonnegative number, got {value!r}')
    return value


def near(actual, expected, label, *, absolute=1e-6):
    if not math.isclose(actual, expected, rel_tol=1e-9, abs_tol=absolute):
        raise ValueError(f'{label} differs: {actual} versus {expected}')


def ensure(condition, error):
    if not condition:
        raise ValueError(error)


def scan(case):
    logs = {int(path.parent.name.rsplit('.', 1)[-1]): path
            for path in case.glob('rank_output/*/rank.*/stdout')}
    ensure(set(logs) == set(range(6)), f'{case}: expected six rank stdout streams')
    packets = {(event, rank): {} for event in (1, 2) for rank in range(6)}
    for rank, log in sorted(logs.items()):
        event = 0
        with log.open(errors='replace') as stream:
            for line, raw in enumerate(stream, 1):
                text = PREFIX.sub('', raw)
                matches = list(KIND.finditer(text))
                for i, match in enumerate(matches):
                    kind = match.group()
                    fragment = text[match.end():matches[i + 1].start() if i + 1 < len(matches) else None]
                    pairs = FIELDS.findall(fragment)
                    fields = dict(pairs)
                    if kind in (CERT, GAP, DIFFER):
                        ensure(len(pairs) == len(fields) and len(fragment.split()) == len(fields)
                               and fields.get('rank') == str(rank), f'{log}:{line}: malformed {kind}')
                        ensure(event in (1, 2), f'{log}:{line}: {kind} outside a native event')
                    if kind == DETAIL:
                        # Rank zero prints all worker details in the parent's ordered report.
                        if rank != 0 or fields.get('stage') != 'refine' or fields.get('rank') not in ('0', '1', '2', '3', '4', '5'):
                            continue
                        target = int(fields['rank'])
                    else:
                        if fields.get('rank') != str(rank):
                            continue
                        target = rank
                    if kind == PARTITION:
                        event += 1
                    if event not in (1, 2):
                        continue
                    if kind in (TIMER, SUMMARY) and not (
                            fields.get('root') == 'remesh' and fields.get('boundary') == 'final'):
                        continue
                    if kind == TIMER and fields.get('operation') != 'unattributed':
                        continue
                    fields.update(file=str(log), line=line)
                    packets[event, target].setdefault(kind, []).append(fields)
        ensure(event == 2, f'{log}: expected two partition boundaries; got {event}')
    return packets


def once(packet, kind):
    rows = packet.get(kind, [])
    ensure(len(rows) == 1, f'{kind} expected once, got {len(rows)}')
    return rows[0]


def ledger(packet):
    timer = once(packet, SUMMARY)
    root, other = seconds(timer, 'root_seconds'), seconds(timer, 'unattributed')
    ensure(root > 0 and 0 <= other < root, f'invalid root/unattributed values: {root}/{other}')
    ensure(number(timer, 'fixedstorage') == 4120, 'timer storage changed')
    near(seconds(timer, 'sumexclusive'), root, 'exclusive sum vs root')
    coverage = 1 - other / root
    near(seconds(timer, 'coverage'), coverage, 'final timer coverage')
    phases = dict.fromkeys(PHASES, 0.0)
    seen = set()
    for row in packet.get(TIMER, []):
        phase = row['phase']
        ensure(phase in phases and phase not in seen, f'bad unattributed phase row {row}')
        seen.add(phase)
        phases[phase] = seconds(row, 'exclusive_seconds')
    remainder = other - sum(phases.values())
    ensure(remainder >= -1e-6, f'phase unattributed exceeds final unattributed by {-remainder}')
    # The root owns preparation time until its first scope. The timer rows omit this remainder.
    phases['preparation'] += max(remainder, 0)
    return {'root_seconds': root, 'unattributed_seconds': other, 'attributed_seconds': root - other,
            'coverage': coverage, 'unattributed_by_phase': phases, 'timer': timer}


def measured_gaps(packet, final, skip):
    row = once(packet, GAP)
    root, attributed = final['root_seconds'], final['attributed_seconds']
    ensure(skip < root and skip <= attributed, 'skip duration exceeds attributed/root duration')
    prediction = (attributed - skip) / (root - skip)
    for key, value in (('root_seconds', root), ('attributed_seconds', attributed),
                       ('unattributed_seconds', final['unattributed_seconds']), ('skip_seconds', skip)):
        near(seconds(row, key), value, key)
    near(seconds(row, 'predicted_coverage'), prediction, 'predicted coverage', absolute=1e-8)
    spans = []
    for phase, duration in final['unattributed_by_phase'].items():
        near(seconds(row, f'{phase}_unattributed_seconds'), duration, f'{phase} unattributed')
        phase_spans = []
        for index in range(1, 4):
            prefix = f'{phase}_span{index}_'
            keys = [prefix + suffix for suffix in ('start', 'end', 'open', 'close')]
            if not any(key in row for key in keys):
                continue
            ensure(len(phase_spans) + 1 == index, f'{phase}: incomplete span order')
            first, end = (seconds(row, key) for key in keys[:2])
            opening, closing = (row[key] for key in keys[2:])
            ensure(first < end <= root + 1e-6 and BOUNDARY.fullmatch(opening) and BOUNDARY.fullmatch(closing),
                   f'{phase}: malformed span {index} {first}/{end} {opening}/{closing}')
            phase_spans.append({'phase': phase, 'start': first, 'end': end, 'seconds': end - first,
                                'open': opening, 'close': closing})
        ensure(duration <= 1e-6 or phase_spans, f'{phase}: no span for {duration:.9g}s unattributed')
        ensure(sum(span['seconds'] for span in phase_spans) <= duration + 1e-6,
               f'{phase}: longest gaps exceed phase unattributed time')
        for position, span in enumerate(phase_spans[:-1]):
            ensure(span['seconds'] + 1e-9 >= phase_spans[position + 1]['seconds'],
                   f'{phase}: gaps must be longest first')
        spans.extend(phase_spans)
    chronological = sorted(spans, key=lambda span: span['start'])
    for left, right in zip(chronological, chronological[1:]):
        ensure(left['end'] <= right['start'] + 1e-9, f'unattributed spans overlap: {left}, {right}')
    return prediction, spans


def certificates(packet, probe, final):
    row = once(packet, CERT)
    ensure(number(row, 'probe') == probe, f'expected probe {probe}')
    counts = {key: number(row, key) for key in COUNTS}
    counts['gather_seconds'] = seconds(row, 'gather_seconds')
    ensure(0 < counts['workers'] <= 6 and 0 <= counts['eligible_stages'] <= 2, 'invalid worker/stage count')
    ensure(counts['cert_hit'] + counts['cert_miss'] == counts['reader_calls'], 'hits + misses differs from reader calls')
    ensure(sum(counts[key] for key in ORIGINS) == counts['cert_hit'], 'hit origins do not sum to hits')
    if probe != 6:
        ensure(not any(key in row for key in (*COUNT_FIELDS, 'skip_seconds')), 'count-only fields on default row')
        ensure(GAP not in packet and DIFFER not in packet, 'count-only rows in default log')
        return counts
    counts.update({key: number(row, key) for key in COUNT_FIELDS})
    counts['skip_seconds'] = seconds(row, 'skip_seconds')
    ensure(counts['cert_disagree'] <= counts['cert_hit'] and counts['skip_calls'] <= counts['cert_hit'],
           'disagreement or skip total exceeds hit count')
    ensure(counts['skip_calls'] or counts['skip_seconds'] == 0, 'skip duration without skip calls')
    differences = packet.get(DIFFER, [])
    classified = Counter(face['kind'] for face in differences)
    ensure(len(differences) == counts['cert_disagree'] and
           classified == {kind: counts[f'disagree_{kind}'] for kind in ('finite', 'nonfinite')
                          if counts[f'disagree_{kind}']}, 'individual differences disagree with the counts')
    for face in differences:
        for key in ('stage', 'facet', 'entity_body', 'entity_sheet'):
            number(face, key)
        int(face['marker'])
        ensure(number(face, 'origin') in range(3), f'unknown disagreement origin {face}')
        for vertex in ('v0', 'v1', 'v2'):
            words = face[vertex].split(',')
            ensure(len(words) == 8 and all(word.isdecimal() and int(word) < 2**64 for word in words),
                   f'incomplete disagreement vertex {face}')
        ensure(math.isfinite(float(face['legacy'])) == (face['kind'] == 'finite'), face)
    prediction, spans = measured_gaps(packet, final, counts['skip_seconds'])
    counts.update(predicted_coverage=prediction, spans=spans, disagreements=differences)
    return counts


def ownership_digest(weights):
    canonical = json.dumps(weights, sort_keys=True, separators=(',', ':'))
    return hashlib.sha256(canonical.encode()).hexdigest()


def optional_uint64(fields, key):
    value = fields.get(key, '')
    if not value.isascii() or not value.isdecimal() or len(value) > 20:
        return None
    parsed = int(value)
    return parsed if parsed < 2**64 else None


def assignment_digests(packet):
    for row in packet.get(ASSIGNMENTS, []):
        site = row.get('site')
        cells = optional_uint64(row, 'cells')
        digest = optional_uint64(row, 'assignments_digest')
        if site in ('initial', 'ownership') and cells is not None and digest is not None:
            yield {'site': site, 'round': optional_uint64(row, 'round'), 'cells': cells,
                   'assignments_digest': digest, 'file': row['file'], 'line': row['line']}


def collect(case, probe=None):
    packets = scan(case)
    rows = []
    for (event, rank), packet in packets.items():
        try:
            final = ledger(packet)
            row = {'event': event, 'rank': rank, **final}
            routed = once(packet, PARTITION)
            weights = {key: routed[key] for key in WEIGHTS if key in routed}
            row['event_partition_weights'] = weights
            row['partition_summary_sha256'] = ownership_digest(weights)
            row['partition_assignments'] = list(assignment_digests(packet))
            row['ownership'] = packet.get(OWNERSHIP, [])
            row['engine'] = [{key: fields.get(key) for key in ('seed', 'atlas_arithmetic_work', 'work_limit',
                                                              'atlas_work_limit')}
                             for fields in packet.get(DETAIL, [])]
            seed_stages = [fields for fields in packet.get(STAGE, []) if fields.get('stage') == 'length_collapse_exit']
            row['seed_facet_pool'] = [{key: fields.get(key) for key in FACET_FIELDS} for fields in seed_stages]
            if probe in (0, 6):
                row['certificates'] = certificates(packet, probe, final)
                row['atlas_work_limit'] = optional_uint64(once(packet, CERT), 'atlas_work_limit') or None
            else:
                ensure(not any(kind in packet for kind in (CERT, GAP, DIFFER)),
                       'certificate diagnostics appeared in an OFF/reference run')
            rows.append(row)
        except (KeyError, ValueError) as exc:
            raise ValueError(f'{case}: event {event}, rank {rank}: {exc}') from exc
    for event in (1, 2):
        selected = [row['certificates'] for row in rows if row['event'] == event and 'certificates' in row]
        declarations = {(row['workers'], row['stage']) for row in selected}
        ensure(len(declarations) <= 1, f'event {event}: worker stages differ across ranks {declarations}')
    return rows


def count_measurements(case):
    rows = collect(case, 6)
    minimum = min(row['certificates']['predicted_coverage'] for row in rows)
    return {'ranks': rows, 'minimum_predicted_coverage': minimum, 'repeat_required': minimum < .96,
            'below_96_percent': [
                {'event': row['event'], 'rank': row['rank'], 'prediction': row['certificates']['predicted_coverage'],
                 'longest_spans': row['certificates']['spans']}
                for row in rows if row['certificates']['predicted_coverage'] < .96]}


def self_check():
    with TemporaryDirectory(prefix='sc1-collect-') as temp:
        case = Path(temp)
        for rank in range(6):
            log = case / f'rank_output/1/rank.{rank}/stdout'
            log.parent.mkdir(parents=True)
            event = (
                f'{PARTITION} rank={rank} parts=6\n'
                f'{CERT} rank={rank} probe=6 workers=6 eligible_stages=1 stage=0 '
                'reader_calls=5 cert_hit=2 cert_miss=3 hit_step_c=1 hit_proof=1 hit_commit=0 records=4 bytes=512 '
                'gather_seconds=0.2 cert_disagree=1 disagree_finite=0 disagree_nonfinite=1 skip_calls=2 skip_seconds=10\n'
                f'{DIFFER} rank={rank} stage=0 facet=1 entity_body=0 entity_sheet=1 marker=0 '
                'kind=nonfinite legacy=nan origin=1 v0=0,0,0,1,1,0,0,0 v1=1,0,0,1,1,0,0,0 v2=0,1,0,1,1,0,0,0\n'
                f'{TIMER} root=remesh boundary=final rank={rank} phase=preparation operation=unattributed '
                'exclusive_seconds=2\n'
                f'{SUMMARY} root=remesh boundary=final rank={rank} root_seconds=100 sumexclusive=100 '
                'unattributed=4 coverage=0.96 fixedstorage=4120\n'
                f'{GAP} rank={rank} root_seconds=100 attributed_seconds=96 unattributed_seconds=4 skip_seconds=10 '
                'predicted_coverage=0.9555555555555556 preparation_span1_start=0 preparation_span1_end=4 '
                'preparation_span1_open=root_start:timer.cpp:5 preparation_span1_close=push:timer.cpp:9 ' +
                ' '.join(f'{phase}_unattributed_seconds={4 if phase == "preparation" else 0}' for phase in PHASES) + '\n')
            with_digests = event.replace(
                f'{PARTITION} rank={rank} parts=6\n',
                f'{PARTITION} rank={rank} parts=6\n'
                f'{ASSIGNMENTS} rank={rank} site=initial cells=4 assignments_digest={rank}\n'
                f'{ASSIGNMENTS} rank={rank} site=ownership round=7 cells=5 assignments_digest={2**64 - 1}\n'
                f'{ASSIGNMENTS} rank={rank} site=ownership cells=7 assignments_digest=unfinished\n')
            log.write_text(with_digests.replace('gather_seconds=', 'atlas_work_limit=8192 gather_seconds=') + event)
        report = count_measurements(case)
        assert report['repeat_required'] and len(report['below_96_percent']) == 12
        for row in report['ranks']:
            if row['event'] == 1:
                initial, ownership = row['partition_assignments']
                assert initial['assignments_digest'] == row['rank'] and initial['round'] is None
                assert ownership['round'] == 7 and ownership['assignments_digest'] == 2**64 - 1
                assert row['atlas_work_limit'] == 8192
            else:
                assert row['partition_assignments'] == [] and row['atlas_work_limit'] is None
        log.write_text(log.read_text().replace('hit_step_c=1', 'hit_step_c=0', 1))
        try:
            count_measurements(case)
        except ValueError as exc:
            assert 'hit origins' in str(exc), exc
        else:
            raise AssertionError('collector accepted incorrect hit origins')


if __name__ == '__main__':
    parser = ArgumentParser(description=__doc__)
    parser.add_argument('case', type=Path, nargs='?')
    parser.add_argument('--self-check', action='store_true')
    args = parser.parse_args()
    if args.self_check:
        self_check()
    elif args.case:
        print(json.dumps(count_measurements(args.case), indent=2, allow_nan=False))
    else:
        parser.error('choose a case or --self-check')
