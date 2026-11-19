"""Verify both final surface proofs and the Part P identity allowances."""
from dataclasses import replace
from decimal import Decimal
from pathlib import Path
import re
import runpy

NIGHT = Path(__file__).resolve().parent.parent
PHASE = NIGHT / 'N4/section6_3_after_N11'
STRICT = PHASE / 'candidate_prefix_preparation/compare_candidate_prefix.py'
base = runpy.run_path(str(STRICT))['base']
N4 = base.N4

RANK_COUNT = 6
CERT_LIMIT = Decimal('53.4')
MEASURED = ('full_calls', 'full_work', 'full_seconds', 'traversal_work', 'traversal_seconds',
            'atlas_work', 'atlas_arithmetic_work', 'collective_calls', 'collective_seconds',
            'election_calls', 'election_seconds', 'tracked_calls', 'tracked_worker_calls',
            'communication_calls', 'nested_tracked_collectives', 'nested_tracked_worker')
MODELED = ('atlas_bytes', 'atlas_peak_bytes', 'native_bound', 'maximum_native_bytes', 'native_peak_bytes')
PROOF_MARKS = re.compile(r'\b(NATIVE3D_FINAL_PROOF|NATIVE3D_FINAL_CERT|NATIVE3D_STORAGE) ')


def proof_snapshots(path, rank):
    return [snapshot for snapshot in base.parse_timer_stream(path)
            if snapshot['key'] == ('remesh', 'exit', str(rank)) and proof_operations(snapshot)]


def proof_operations(snapshot):
    return {row['fields']['operation']: row['fields'] for row in snapshot['rows']
            if row['fields']['phase'] == 'final_proof'}


def lines(path):
    """Read complete records after repairing repeated MPI stamps within long lines."""
    with path.open(errors='replace') as source:
        for number, raw in enumerate(source, 1):
            line = N4.strip_mpi_prefixes(raw)
            marks = list(PROOF_MARKS.finditer(line))
            for i, mark in enumerate(marks):
                end = marks[i + 1].start() if i + 1 < len(marks) else len(line)
                yield number, mark[1], dict(N4.FIELD.findall(line[mark.end():end]))


def campaign_proofs(case, shared):
    """Collect per-rank proof and certificate rows without mixing events or timer roots."""
    events = [{'event': number, 'ranks': [], 'certificates': []} for number in (1, 2)]
    for rank in range(RANK_COUNT):
        path = case / f'rank_output/1/rank.{rank}/stdout'
        snapshots = proof_snapshots(path, rank)
        assert len(snapshots) == 2, (path, 'remesh/exit proof timer snapshots', len(snapshots))
        stderr = path.with_name('stderr')
        raw = [(number, fields, mark) for number, mark, fields in lines(stderr)]
        records = [(number, fields) for number, fields, mark in raw if mark == 'NATIVE3D_FINAL_PROOF']
        certificates = [(number, fields) for number, fields, mark in raw if mark == 'NATIVE3D_FINAL_CERT']
        assert len(records) == len(snapshots) * int(shared), (stderr, 'final proof row count', len(records))
        assert rank == 0 or not certificates, (stderr, 'final certificate rows must be on rank 0')
        for index, (event, snapshot) in enumerate(zip(events, snapshots)):
            number, proof = records[index] if shared else (None, None)
            op = proof_operations(snapshot)
            event['ranks'].append({
                'rank': rank, 'proof': proof, 'proof_line': number, 'proof_log': str(stderr),
                'certification_exclusive_seconds': op['certification']['exclusive_seconds'],
                'certification_calls': int(op['certification']['calls']),
                'communication_calls': int(op['communication']['calls']),
                'communication_exclusive_seconds': op['communication']['exclusive_seconds'],
            })
            if shared and rank == 0:
                end = records[index + 1][0] if index + 1 < len(records) else float('inf')
                certs = [fields for line, fields in certificates if number < line < end]
                assert len(certs) == int(proof['entities']), (stderr, event['event'], len(certs), proof['entities'])
                event['certificates'] = certs
    return events


def proof_outcomes(events, reference=None):
    """Report root coverage and evaluate the specified shared-path and clock gates."""
    reports = []
    for event in events:
        ranks = event['ranks']
        cert = [Decimal(row['certification_exclusive_seconds']) for row in ranks]
        assert all(value.is_finite() and value >= 0 for value in cert)
        copies = [row['proof'] for row in ranks]
        reported_reference = [int(row['reference_facets']) for row in copies]
        reported_own = sum(int(row['own_roots']) for row in copies)
        shared = all(row['shared'] == '1' and int(row['rank']) == rank for rank, row in enumerate(copies))
        expected_communication = []
        if reference:
            for before, now in zip(reference[event['event'] - 1]['ranks'], ranks):
                predicted = before['communication_calls'] + 1 + 2 * int(now['proof']['entities'])
                expected_communication.append({'rank': now['rank'], 'reference': before['communication_calls'],
                                               'expected': predicted, 'candidate': now['communication_calls']})
        reports.append({
            'event': event['event'], 'all_ranks_shared': shared,
            'certification_calls_passed': all(row['certification_calls'] == 2 for row in ranks),
            'maximum_certification_exclusive_seconds': str(max(cert)),
            'certification_limit_seconds': str(CERT_LIMIT), 'purpose_passed': max(cert) <= CERT_LIMIT,
            'communication_counts': expected_communication,
            'own_roots_sum': reported_own, 'reference_facets_by_rank': reported_reference,
            'root_coverage_report_matches': set(reported_reference) == {reported_own},
        })
    standing = all(row['all_ranks_shared'] and row['certification_calls_passed'] for row in reports)
    return {'passed': standing and all(row['purpose_passed'] for row in reports),
            'standing_gates_passed': standing, 'events': reports}


def timer_identity(before, after):
    """Use the historical strict comparison except at shared remesh proof communications."""
    if before['key'] != after['key']:
        return {'key': 'snapshot_identity', 'reference': before['key'], 'candidate': after['key']}
    for side, snapshot in (('reference', before), ('candidate', after)):
        if snapshot['summary']['fixedstorage'] != '4120':
            return {'key': f'{side}.fixedstorage', 'expected': 4120, 'value': snapshot['summary']['fixedstorage']}
    change = base.exact_fields(before['summary'], after['summary'], base.SUMMARY_MEASUREMENTS)
    if change:
        return {'key': f'timer_summary.{change}'}
    if before['law_lines_before'] != after['law_lines_before']:
        return {'key': 'wall_law_lines_before_boundary',
                'reference': before['law_lines_before'], 'candidate': after['law_lines_before']}
    left, right = before['rows'], after['rows']
    if len(left) != len(right):
        return {'key': 'timer_row_count', 'reference': len(left), 'candidate': len(right)}
    for row_a, row_b in zip(left, right):
        a, b = row_a['fields'], row_b['fields']
        ignored = base.TIMER_MEASUREMENTS
        if (before['key'][0] == 'remesh' and before['key'][1] in {'exit', 'final'} and
                a['phase'] == 'final_proof' and a['operation'] == 'communication'):
            base.parse_calls(a, row_a['path'], row_a['line'])
            base.parse_calls(b, row_b['path'], row_b['line'])
            ignored = ignored | {'calls'}
        change = base.exact_fields(a, b, ignored)
        if change:
            return {'key': f'timer_row.{change}', 'phase': a['phase'], 'operation': a['operation']}
    return None


def proof_window(snapshots):
    """Locate the final proof and event-report blocks in the native stdout."""
    start = None
    for snapshot in snapshots:
        root, boundary, _ = snapshot['key']
        if root == 'remesh' and boundary == 'exit' and proof_operations(snapshot):
            assert start is None, snapshot['path']
            start = next(row['line'] for row in snapshot['rows'] if row['fields']['phase'] == 'final_proof')
        elif root == 'replace' and boundary == 'final' and start is not None:
            yield start, snapshot['line']
            start = None
    assert start is None


def exception_columns(row, windows):
    """Limit §5.6 to the native event report after a final proof."""
    if not any(start <= row.line <= end for start, end in windows):
        return ()
    if row.marker == 'NATIVE3D_STAGE' and row.fields.get('stage') == 'exit':
        return MEASURED + MODELED
    if row.marker == 'NATIVE3D_DETAIL' and row.fields.get('stage') in {'refine', 'coarsen'}:
        return MEASURED + MODELED
    if row.marker in {'NATIVE3D rank', 'NATIVE3D_STORAGE'}:
        return MEASURED + MODELED
    return ()


def compare_allowed(a, b, columns, report):
    """Record allowed fields and enforce nonincrease of every modeled-byte field."""
    omit = set()
    for key in columns:
        if key not in a.fields or key not in b.fields:
            continue
        old, new = a.fields[key], b.fields[key]
        if key in MODELED and int(new) > int(old):
            report.setdefault('modeled_byte_increases', []).append({
                'row': a.marker, 'rank': a.rank(), 'stage': a.fields.get('stage'),
                'field': key, 'reference': old, 'candidate': new, 'lines': [a.line, b.line]})
        omit.add(key)
    if omit:
        report['proof_counters'].append({
            'row': a.marker, 'rank': a.rank(), 'stage': a.fields.get('stage'),
            'lines': [a.line, b.line], 'fields': {key: [a.fields[key], b.fields[key]] for key in sorted(omit)}})
    return (replace(a, fields={key: value for key, value in a.fields.items() if key not in omit}),
            replace(b, fields={key: value for key, value in b.fields.items() if key not in omit}))


def stream_identity(left, right, rank):
    """Remove only the permitted proof fields, then use the N4 row comparator."""
    before, after = base.parse_timer_stream(left), base.parse_timer_stream(right)
    left_window, right_window = list(proof_window(before)), list(proof_window(after))
    assert len(left_window) == len(right_window) == 2, (left, right)
    assert len(before) == len(after), ('timer snapshot count', len(before), len(after), left, right)
    report = {'rank': rank, 'timer_snapshots': len(after), 'proof_counters': []}
    base.verify_law_ranks(left, rank)
    base.verify_law_ranks(right, rank)
    timer = [result for a, b in zip(before, after) if (result := timer_identity(a, b))]
    if timer:
        report['timer_first_difference'] = timer[0]
    left_rows, right_rows = [[row for row in N4.marker_rows(path) if row.marker != 'NATIVE3D_TIMER']
                             for path in (left, right)]
    for a, b in zip(left_rows, right_rows):
        columns = tuple(key for key in exception_columns(a, left_window)
                        if key in exception_columns(b, right_window))
        stripped_a, stripped_b = compare_allowed(a, b, columns, report)
        error = N4.compare_row(stripped_a, stripped_b)
        if error and 'integer_first_difference' not in report:
            report['integer_first_difference'] = {'key': error, 'lines': [a.line, b.line]}
    if len(left_rows) != len(right_rows):
        report.setdefault('integer_first_difference', {
            'key': 'row_count', 'reference': len(left_rows), 'candidate': len(right_rows)})
    # Native storage rows are outside the old generic comparator. This maximum reflects the atlas peak.
    def at_exit(path, windows):
        return [N4.Row(mark, fields, str(path), number) for number, mark, fields in lines(path)
                if mark == 'NATIVE3D_STORAGE' and fields.get('stage') == 'exit' and
                any(start <= number <= end for start, end in windows)]
    left_storage, right_storage = at_exit(left, left_window), at_exit(right, right_window)
    assert len(left_storage) == len(right_storage) == 2, (left, right)
    for a, b in zip(left_storage, right_storage):
        assert 'native_peak_bytes' in a.fields and 'native_peak_bytes' in b.fields
        compare_allowed(a, b, ('native_peak_bytes',), report)
    report['passed'] = not any(key in report for key in (
        'integer_first_difference', 'timer_first_difference', 'modeled_byte_increases'))
    return report


def identity(reference, candidate):
    ranks = []
    for rank in range(RANK_COUNT):
        name = f'rank_output/1/rank.{rank}/stdout'
        ranks.append(stream_identity(reference / name, candidate / name, rank))
    return {'passed': all(row['passed'] for row in ranks), 'reference': str(reference),
            'candidate': str(candidate), 'ranks': ranks,
            'timer_snapshots': sum(row['timer_snapshots'] for row in ranks)}
