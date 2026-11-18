"""Compare each rank's event-one output before the first private repair round."""
from pathlib import Path
import re
import runpy
import sys

from run_scoped import N16, cut

INTEGER = re.compile(r'[+-]?\d+(?:,[+-]?\d+)*\Z')
MARKER = re.compile(r'\bNATIVE3D_[A-Z0-9_]+\b')
FIELD = re.compile(r'([A-Za-z_]\w*)=([^\s]+)')
CANONICAL = runpy.run_path(str(N16.parent / 'N4/compare_integer_rows.py'))


def marked_rows(log, rank):
    """Strip MPI stamps before fields are split, including stamps inside long records."""
    for line_no, line in enumerate(log.read_text(errors='replace').splitlines(), 1):
        text = CANONICAL['strip_mpi_prefixes'](line)
        marks = list(MARKER.finditer(text))
        for index, mark in enumerate(marks):
            end = marks[index + 1].start() if index + 1 < len(marks) else len(text)
            fields = dict(FIELD.findall(text[mark.end():end]))
            if fields.get('rank') != str(rank):
                continue
            yield mark[0], fields, line_no


def prefix(log, rank):
    rows = []
    for tag, fields, line_no in marked_rows(log, rank):
        if tag == 'NATIVE3D_PRIVATE_WORK':
            return rows
        row = CANONICAL['Row'](tag, fields, str(log), line_no)
        measured = CANONICAL['is_measurement']
        integers = {}
        for key, value in fields.items():
            if not INTEGER.fullmatch(value) or measured(row, key) or key in {'cpu_other', 'idle_cpu'}:
                continue
            if tag == 'NATIVE3D_DETAIL' and key in CANONICAL['DETAIL_TIMES']:
                continue
            integers[key] = value
        rows.append((tag, integers, line_no))
    raise ValueError(f'Missing initial private repair marker: {log}')


def compare(reference, candidate):
    reports = []
    for rank in range(6):
        name = f'rank_output/1/rank.{rank}/stdout'
        left, right = reference / name, candidate / name
        before, after = prefix(left, rank), prefix(right, rank)
        receipt = {'rank': rank, 'reference_sha256': cut.sha(left), 'candidate_sha256': cut.sha(right),
                   'reference_rows': len(before), 'candidate_rows': len(after)}
        for index, (a, b) in enumerate(zip(before, after)):
            if a[:2] == b[:2]:
                continue
            differences = {key: [a[1].get(key), b[1].get(key)] for key in sorted(a[1].keys() | b[1].keys())
                           if a[1].get(key) != b[1].get(key)}
            receipt['first_difference'] = {'index': index, 'tags': [a[0], b[0]], 'lines': [a[2], b[2]], 'fields': differences}
            break
        if 'first_difference' not in receipt and len(before) != len(after):
            receipt['first_difference'] = {'key': 'row_count', 'values': [len(before), len(after)]}
        receipt['passed'] = 'first_difference' not in receipt
        reports.append(receipt)
    return {'passed': all(row['passed'] for row in reports), 'ranks': reports}


if __name__ == '__main__':
    reference, candidate, output = map(Path, sys.argv[1:])
    result = compare(reference, candidate)
    cut.save(output, result)
    raise SystemExit(0 if result['passed'] else 2)
