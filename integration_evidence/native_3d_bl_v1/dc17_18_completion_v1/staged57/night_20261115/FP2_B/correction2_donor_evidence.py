"""Match diagnostic donor identities to the published core allocation counters."""
from collections import defaultdict
import gzip
import re

from report_d1 import MARKER, PREFIX, fields, SHORT, SURFACE, TIMER, table

DONOR = re.compile(r'\bsite=(?:affine|orientation) donor_cell=\d+ donor_index=\d+ donors=\d+'
                   r'(?: v[0-3]=\d+@[-+0-9.eE,nNaAiIfF]+){4}')
NAMES = ('moves', 'splits'), ('splits_length', 'splits_distance', 'v_splits')


def collect(case):
    counts, bases, records = {}, defaultdict(set), []
    seen = set()
    for path in sorted(case.glob('rank_output/1/rank.*/stdout*')):
        if path.name not in ('stdout', 'stdout.gz'):
            continue
        rank = int(path.parent.name.rsplit('.', 1)[1])
        if rank in seen:
            continue
        seen.add(rank)
        event, stage = 1, None
        opener = gzip.open if path.name.endswith('.gz') else open
        with opener(path, 'rt', errors='replace') as stream:
            for line, raw in enumerate(stream, 1):
                if match := MARKER.search(PREFIX.sub('', raw).strip()):
                    marker, text = match.groups()
                    row = fields(text)
                    if row.get('rank') == str(rank):
                        if row.get('stage', '').isdigit():
                            stage = int(row['stage'])
                        key = event, stage
                        if marker == SHORT and row.get('input_point_base', '').isdigit():
                            bases[key].add(int(row['input_point_base']))
                        if marker in ('NATIVE3D_LOCAL_CORE', SURFACE) and row.get('pass', '').isdigit():
                            counts[*key, rank, int(row['pass']), marker] = row
                        if marker == TIMER and row.get('root') == 'remesh' and row.get('boundary') == 'final':
                            event += 1
                            stage = None
                for match in DONOR.finditer(raw):
                    records.append((event, stage, rank, line, fields(match[0])))
    return counts, bases, records, seen


def allocation_bounds(counts, bases, ranks, key):
    if len(bases[key]) != 1:
        return None
    base, = bases[key]
    last = {}
    for rank in ranks:
        allocated = 0
        for phase in (1, 2):
            for marker, names in zip(('NATIVE3D_LOCAL_CORE', SURFACE), NAMES):
                row = counts.get((*key, rank, phase, marker), {})
                if not all(row.get(name, '').isdigit() for name in names):
                    return None
                allocated += sum(int(row[name]) for name in names)
        last[rank] = base + rank + 1 + len(ranks) * (allocated - 1) if allocated else base
    return base, last


def donor_evidence(case):
    counts, bases, records, ranks = collect(case)
    lines = ['## Donor provenance from S1', '']
    if not records:
        return lines + ['No affine or orientation donor context was printed.', '']
    lines += ['Core point IDs start at input maximum + rank + 1 and advance by rank count. Each accepted '
              'core length split, surface split, volume-repair split or move allocates one ID. '
              'A vertex outside that rank sequence was allocated after the core. A vertex inside it '
              'does not certify that the donor tetrahedron belonged to the hand-over.', '']
    data = []
    for event, stage, rank, line, row in records:
        vertices = tuple(row[f'v{i}'] for i in range(4))
        ids = [int(vertex.split('@')[0]) for vertex in vertices]
        proof = 'Core allocation evidence unavailable'
        if ranks == set(range(6)) and (bounds := allocation_bounds(counts, bases, ranks, (event, stage))):
            base, last = bounds
            later = [point for point in ids if point > base and point > last[(point - base - 1) % len(ranks)]]
            proof = ('Engine-created vertex IDs ' + ', '.join(map(str, later)) if later
                     else 'All vertex IDs fit the hand-over allocation bounds. Tetrahedron membership is unresolved.')
        data.append([event, stage, rank, row['site'], row['donor_cell'], row['donor_index'], row['donors'],
                     *vertices, proof, f'r{rank}:{line}'])
    return lines + table(('Event', 'Stage', 'Rank', 'Site', 'Donor cell', 'Index', 'Batch size',
                          'v0', 'v1', 'v2', 'v3', 'Hand-over relation', 'Log row'), data)
