"""Bounded persistent length-witness eligibility replay; historical trajectory remains unchanged."""
from collections import Counter, OrderedDict
from fractions import Fraction as F
from pathlib import Path
import hashlib
import json

HERE = Path(__file__).resolve().parent
BASE = HERE.parent
ROOT = BASE.parents[1]
TRACE = BASE/'full_star_facet_trace_n8_v1'
BOUNDS = BASE/'collapse_bound_replay_v1'
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
LIMIT = F(float(1.8))**2


def node_map(row):
    nodes = {}
    cells = []
    for cell in row['old']:
        ids = []
        for node in cell['nodes']:
            point = tuple(F(float(v)) for v in node[1:])
            if node[0] in nodes:
                assert nodes[node[0]] == point
            nodes[node[0]] = point
            ids.append(node[0])
        assert len(set(ids)) == 4 and row['edge'][1] in ids
        cells.append(tuple(ids))
    return nodes, cells


def evidence(row, witnesses):
    keep, drop = row['edge']
    nodes, cells = node_map(row)
    result = []
    for witness in witnesses:
        a, b = witness['edge']
        assert keep in (a, b)
        neighbor = b if a == keep else a
        assert neighbor != drop and F(witness['squared_lower']) > LIMIT
        support = [cell for cell in cells if drop in cell and neighbor in cell and keep not in cell]
        assert support, 'Certified changed chord must occur in a surviving collapsed tetrahedron'
        result.append(dict(keep=keep, drop=drop, neighbor=neighbor,
                           keep_point=nodes[keep], neighbor_point=nodes[neighbor],
                           supporting_cells=(min(tuple(sorted(c)) for c in support),),
                           lower=F(witness['squared_lower']), learned_round=row['round']))
    return sorted(result, key=lambda w: (-w['lower'], w['neighbor']))


def persists(row, witness, mode):
    keep, drop = row['edge']
    assert (keep, drop) == (witness['keep'], witness['drop'])
    nodes, cells = node_map(row)
    neighbor = witness['neighbor']
    if nodes.get(keep) != witness['keep_point'] or nodes.get(neighbor) != witness['neighbor_point']:
        return False
    if mode == 'cell':
        current = {tuple(sorted(c)) for c in cells}
        return bool(current.intersection(witness['supporting_cells']))
    assert mode == 'adjacency'
    return any(drop in c and neighbor in c and keep not in c for c in cells)


def replay(rows, bounds, capacity, witness_limit, mode):
    cache = OrderedDict()
    counts = Counter()
    records = []
    maximum_entries = maximum_witnesses = 0
    for row in rows:
        edge = tuple(row['edge'])
        matched = next((w for w in cache.get(edge, []) if persists(row, w, mode)), None)
        if matched is not None:
            assert not row['approved'], 'Would suppress a historically accepted collapse'
            counts['certified_retry_suppressed'] += 1
            cache.move_to_end(edge)
            records.append(dict(round=row['round'], edge=list(edge), reason=row['reason'],
                                learned_round=matched['learned_round'], neighbor=matched['neighbor'],
                                lower_squared=str(matched['lower'])))
        elif row['approved']:
            counts['accepted_retained'] += 1
            # Accepted directed collapse removes this vertex; discard dead incident edge keys.
            removed = edge[1]
            for key in list(cache):
                if removed in key:
                    del cache[key]
        else:
            counts['rejected_retained'] += 1
            witnesses = evidence(row, bounds[row['round']]['oversized_witnesses'])[:witness_limit]
            if witnesses:
                cache[edge] = witnesses
                cache.move_to_end(edge)
                counts['witness_entries_learned'] += 1
                while len(cache) > capacity:
                    cache.popitem(last=False)
                    counts['capacity_evictions'] += 1
            else:
                cache.pop(edge, None)
        maximum_entries = max(maximum_entries, len(cache))
        maximum_witnesses = max(maximum_witnesses, sum(map(len, cache.values())))
        assert len(cache) <= capacity and all(len(w) <= witness_limit for w in cache.values())
    assert counts['accepted_retained'] == sum(row['approved'] for row in rows)
    return dict(capacity=capacity, witness_limit=witness_limit, mode=mode,
                attempts=len(rows), counts=dict(counts), maximum_entries=maximum_entries,
                maximum_witnesses=maximum_witnesses,
                retained_recorded_attempts=len(rows)-counts['certified_retry_suppressed'],
                suppressed_records=records,
                scope='Eligibility on the unchanged saved proposal/mesh trajectory, not a new selection trajectory or measured round reduction.')


def self_check():
    def row(cells, keep=1, drop=2):
        points={1:(0.,0.,0.),2:(.25,0.,0.),3:(1.,0.,0.),4:(0.,1.,0.),5:(0.,0.,1.),6:(0.,1.,1.)}
        return dict(edge=[keep,drop],old=[dict(nodes=[[i,*points[i]] for i in c]) for c in cells],round=0,approved=False,reason='fixture')
    old=row([(2,3,4,5),(1,2,4,5)])
    witness=evidence(old,[dict(edge=[1,3],squared_lower='4')])[0]
    assert persists(old,witness,'cell') and persists(old,witness,'adjacency')
    replaced=row([(2,3,4,6),(1,2,4,5)])
    assert not persists(replaced,witness,'cell') and persists(replaced,witness,'adjacency')
    lost=row([(1,2,3,4),(1,2,4,5)])
    assert not persists(lost,witness,'adjacency')
    moved=row([(2,3,4,5),(1,2,4,5)])
    for cell in moved['old']:
        for node in cell['nodes']:
            if node[0]==3:node[1]+=.125
    assert not persists(moved,witness,'adjacency')
    below=dict(edge=[1,3],squared_lower='3')
    try:evidence(old,[below])
    except AssertionError:pass
    else:raise AssertionError('Uncertified length witness was admitted')
    approved=dict(old,approved=True,round=1)
    try:replay([old,approved],{0:dict(oversized_witnesses=[dict(edge=[1,3],squared_lower='4')])},1,1,'cell')
    except AssertionError:pass
    else:raise AssertionError('Accepted-false-suppression guard failed')


def main():
    self_check()
    trace = json.loads((TRACE/'analysis.json').read_text())
    assert trace['status'] == 'PASS' and trace['adaptation_status'] == 'FAIL'
    for name, expected in trace['artifact_sha256'].items():
        assert sha(TRACE/name) == expected, name
    analysis = json.loads((BOUNDS/'analysis.json').read_text())
    assert analysis['status'] == 'PASS' and analysis['accepted_false_rejections'] == 0
    for name, expected in analysis['source_sha256'].items():
        assert sha(ROOT/name) == expected, name
    rows = [json.loads(line) for line in (TRACE/'attempts_rank0.jsonl').read_text().splitlines()]
    assert len(rows) == len(analysis['records']) == 940
    reports = []
    for epoch, expected, accepted in [(0,428,272),(1,512,437)]:
        events = [r for r in rows if r['epoch']==epoch]
        bounds = {r['round']:r for r in analysis['records'] if r['epoch']==epoch}
        assert len(events)==len(bounds)==expected and sum(r['approved'] for r in events)==accepted
        cases=[]
        for capacity in (16,128):
            for witness_limit in (1,4):
                for mode in ('cell','adjacency'):
                    cases.append(replay(events,bounds,capacity,witness_limit,mode))
        reports.append(dict(epoch=epoch,cases=cases))
    pins={str(p.relative_to(ROOT)):sha(p) for p in [Path(__file__).resolve(),TRACE/'analysis.json',TRACE/'attempts_rank0.jsonl',BOUNDS/'analysis.json',ROOT/'Common/src/adaptation/CNativeEngine3D.cpp']}
    result=dict(status='PASS_OFFLINE_ELIGIBILITY_NOT_NEW_TRAJECTORY',source_sha256=pins,
                epochs=reports,accepted_false_suppressions=0,
                scope='Bounded16/128-entry,1/4-chord counterexample caches replayed on940historical complete-star proposals. Exact original-P1 directional lower-bound witnesses remain invalid when fixed endpoints and a surviving drop-neighbor cell relation persist, even after other topology changes. No target interpolation/geometry change, actual metric-query replacement, new CPP/MPI run, forecast of production512completion or performance claim. Distributed witness life/ownership and outward floating bounds are not implemented.')
    (HERE/'analysis.json').write_text(json.dumps(result,indent=2)+'\n')
    for r in reports:
        print('epoch',r['epoch'],[(c['capacity'],c['witness_limit'],c['mode'],c['counts'].get('certified_retry_suppressed',0),c['maximum_entries']) for c in r['cases']])
    print('PASS:940saved proposals; bounded caches; all709accepted proposals retained')


if __name__=='__main__':main()
