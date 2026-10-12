"""Exact frozen-P1 directional bounds on saved complete-star collapse proposals."""
from collections import Counter
from fractions import Fraction as F
from itertools import combinations
from pathlib import Path
import hashlib
import json

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
TRACE = HERE.parent / 'full_star_facet_trace_n8_v1'
LIMIT = F(float(1.8)) ** 2


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def edges(cells):
    return {tuple(sorted(pair)) for cell in cells for pair in combinations(cell, 2)}


def reconstruct(row):
    keep, drop = row['edge']
    points = {}
    old = []
    for cell in row['old']:
        ids = []
        for node in cell['nodes']:
            point = tuple(F(float(v)) for v in node[1:])
            if node[0] in points:
                assert points[node[0]] == point
            points[node[0]] = point
            ids.append(node[0])
        assert len(set(ids)) == 4 and drop in ids
        old.append(tuple(ids))
    fresh = [tuple(keep if node == drop else node for node in cell)
             for cell in old if keep not in cell]
    assert 0 < len(fresh) < len(old)
    assert len({tuple(sorted(c)) for c in fresh}) == len(fresh)
    if row['approved']:
        assert Counter(tuple(sorted(c)) for c in fresh) == Counter(
            tuple(sorted(n[0] for n in c['nodes'])) for c in row['fresh'])
        for cell in row['fresh']:
            for n in cell['nodes']:
                assert points[n[0]] == tuple(F(float(v)) for v in n[1:])
    return points, edges(fresh) - edges(old), len(old) - len(fresh)


def quadratic(m, d):
    x, y, z = d
    xx, xy, xz, yy, yz, zz = m
    return xx*x*x + yy*y*y + zz*z*z + 2*(xy*x*y + xz*x*z + yz*y*z)


def bounds(tensors, d):
    values = [quadratic(m, d) for m in tensors]
    assert min(values) > 0
    return min(values), max(values)


def self_check():
    tensors = [(F(4), F(0), F(0), F(1), F(0), F(1)),
               (F(9), F(0), F(0), F(1), F(0), F(1))]
    assert bounds(tensors, (F(1), F(0), F(0))) == (4, 9)
    assert bounds(tensors, (F(-1), F(0), F(0))) == (4, 9)
    assert bounds(tensors, (F(0), F(1), F(0))) == (1, 1)
    mixed = tuple((a+b)/2 for a, b in zip(*tensors))
    assert 4 <= quadratic(mixed, (F(1), F(0), F(0))) <= 9
    assert F(81, 25) < LIMIT  # The production double threshold is slightly above decimal1.8.


def main():
    self_check()
    certificate = json.loads((TRACE / 'analysis.json').read_text())
    assert certificate['status'] == 'PASS' and certificate['adaptation_status'] == 'FAIL'
    for name, expected in certificate['artifact_sha256'].items():
        assert sha(TRACE / name) == expected, name
    build_path = TRACE.parent / 'full_star_facet_build_v1/validation.json'
    trace_receipt = json.loads((TRACE / 'validation.json').read_text())
    assert sha(build_path) == trace_receipt['qualified_build_receipt_sha256']
    build = json.loads(build_path.read_text())
    for name in ('Common/src/adaptation/CNativeBoundary3D.cpp',
                 'Common/src/adaptation/CNativeMetric3D.cpp'):
        assert sha(ROOT / name) == build['source_sha256'][name], name
    rows = [json.loads(line) for line in (TRACE / 'attempts_rank0.jsonl').read_text().splitlines()]
    records = []
    epochs = []
    for epoch, count, accepted in [(0, 428, 272), (1, 512, 437)]:
        source = json.loads((TRACE / ('envelope_s%d_initial.json' % epoch)).read_text())
        tensors = sorted({tuple(F(float(v)) for v in vertex[4])
                          for cell in source['original'] for vertex in cell['vertices']})
        cache = {}
        group = [row for row in rows if row['epoch'] == epoch]
        assert len(group) == count and sum(r['approved'] for r in group) == accepted
        counts = Counter()
        for row in group:
            assert row['rank'] == 0 and not row['repairing']
            points, changed, removed = reconstruct(row)
            witnesses = []
            upper_safe = True
            for a, b in sorted(changed):
                d = tuple(y-x for x, y in zip(points[a], points[b]))
                key = min(d, tuple(-x for x in d))
                if key not in cache:
                    cache[key] = bounds(tensors, key)
                lo, hi = cache[key]
                upper_safe = upper_safe and hi <= LIMIT
                if lo > LIMIT:
                    witnesses.append(dict(edge=[a, b], points=[[float(x) for x in points[n]] for n in [a, b]],
                                          squared_lower=str(lo), squared_upper=str(hi)))
            category = 'accepted' if row['approved'] else row['reason'].split(':', 1)[0]
            counts[category] += 1
            if witnesses:
                assert not row['approved'], (epoch, row['round'], witnesses)
                counts['certified_oversized'] += 1
                counts['certified_oversized:' + category] += 1
            if upper_safe:
                counts['all_changed_edges_upper_safe:' + category] += 1
            records.append(dict(epoch=epoch, round=row['round'], approved=row['approved'],
                                reason=row['reason'], edge=row['edge'], removed_cells=removed,
                                changed_edges=len(changed), all_changed_edges_upper_safe=upper_safe,
                                oversized_witnesses=witnesses))
        epochs.append(dict(epoch=epoch, attempts=count, accepted=accepted,
                           distinct_tensors=len(tensors), distinct_chords=len(cache), counts=dict(counts)))
    pins = {str(p.relative_to(ROOT)): sha(p) for p in [TRACE / 'analysis.json',
            TRACE / 'attempts_rank0.jsonl', TRACE / 'envelope_s0_initial.json',
            TRACE / 'envelope_s1_initial.json', build_path, Path(__file__).resolve(),
            ROOT / 'Common/src/adaptation/CNativeBoundary3D.cpp',
            ROOT / 'Common/src/adaptation/CNativeMetric3D.cpp']}
    result = dict(status='PASS', threshold_squared=str(LIMIT), epochs=epochs, records=records,
                  source_sha256=pins, accepted_false_rejections=0,
                  scope='Offline exact-rational analysis of the retained bb87 complete-star trace. '
                  'Reconstructed accepted topology matches all saved accepted proposals. '
                  'Directional extrema over all original nodal sensor tensors bound original-P1 '
                  'chord lengths; no composition, new MPI execution, final mesh certificate, '
                  'budget/default change, predicted new trajectory or performance claim.')
    (HERE / 'analysis.json').write_text(json.dumps(result, indent=2) + '\n')
    for data in epochs:
        print(json.dumps(data, sort_keys=True))
    print('PASS:940 reconstructed proposals; zero accepted false rejections')


if __name__ == '__main__':
    main()
