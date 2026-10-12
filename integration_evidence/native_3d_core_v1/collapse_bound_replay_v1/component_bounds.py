"""Assess a constant-size conservative tensor-component envelope on retained collapses."""
from collections import Counter
from pathlib import Path
import json
from analyze import HERE, TRACE, LIMIT, F, sha, reconstruct, quadratic, bounds


def lower(envelope, d):
    x, y, z = d
    coeff = (x*x, 2*x*y, 2*x*z, y*y, 2*y*z, z*z)
    return sum(c*(lo if c >= 0 else hi) for c, (lo, hi) in zip(coeff, envelope))


def main():
    # Correlation can make this lower bound negative even when every tensor is SPD.
    test = [(F(2), F(1), F(0), F(2), F(0), F(1)),
            (F(20), F(-10), F(0), F(20), F(0), F(1))]
    envelope = [tuple(f(values) for f in (min, max)) for values in zip(*test)]
    assert lower(envelope, (F(1), F(1), F(0))) < 0
    for d in [(F(1), F(1), F(0)), (F(-1), F(1), F(0)), (F(0), F(0), F(1))]:
        assert lower(envelope, d) <= bounds(test, d)[0]
    certificate = json.loads((HERE/'analysis.json').read_text())
    assert certificate['status'] == 'PASS' and certificate['accepted_false_rejections'] == 0
    for name, expected in certificate['source_sha256'].items():
        assert sha(HERE.parents[2]/name) == expected, name
    rows = [json.loads(line) for line in (TRACE/'attempts_rank0.jsonl').read_text().splitlines()]
    epochs = []
    records = []
    for epoch in (0, 1):
        source = json.loads((TRACE/('envelope_s%d_initial.json' % epoch)).read_text())
        tensors = sorted({tuple(F(float(x)) for x in vertex[4])
                          for cell in source['original'] for vertex in cell['vertices']})
        envelope = [tuple(f(values) for f in (min, max)) for values in zip(*tensors)]
        cache = {}
        counts = Counter()
        for row in rows:
            if row['epoch'] != epoch:
                continue
            points, changed, removed = reconstruct(row)
            witnesses = []
            for a, b in sorted(changed):
                d = tuple(y-x for x, y in zip(points[a], points[b]))
                key = min(d, tuple(-x for x in d))
                if key not in cache:
                    bound = lower(envelope, key)
                    exact_min = min(quadratic(m, key) for m in tensors)
                    assert bound <= exact_min
                    cache[key] = bound
                bound = cache[key]
                if bound > LIMIT:
                    witnesses.append(dict(edge=[a, b], squared_lower=str(bound)))
            if witnesses:
                assert not row['approved'], (epoch, row['round'])
                counts[row['reason'].split(':', 1)[0]] += 1
            records.append(dict(epoch=epoch, round=row['round'], approved=row['approved'],
                                oversized_witnesses=witnesses))
        epochs.append(dict(epoch=epoch, certified_oversized=sum(counts.values()),
                           categories=dict(counts), component_envelope=[[str(x) for x in pair] for pair in envelope]))
    result = dict(status='PASS', epochs=epochs, records=records, accepted_false_rejections=0,
                  input_sha256=sha(HERE/'analysis.json'), script_sha256=sha(Path(__file__).resolve()),
                  scope='Exact-rational twelve-scalar global sensor envelope on saved serial collapse records; '
                  'not a rounded C++ predicate, implemented MPI reduction, changed trajectory or timing result.')
    (HERE/'component_analysis.json').write_text(json.dumps(result, indent=2)+'\n')
    for row in epochs:
        print(json.dumps({k:v for k,v in row.items() if k != 'component_envelope'}, sort_keys=True))
    print('PASS:component lower bounds conservative; zero accepted false rejections')


if __name__ == '__main__':
    main()
