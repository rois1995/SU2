"""Independent rational sensor transport/P1/composition audit of bounded MPI fixture observations."""
from collections import defaultdict
from fractions import Fraction as F
import hashlib
import json
from pathlib import Path
import sys
from audit_coupled import weights, spd
from audit_cavities import determinant

def audit(log, mesh, ranks):
    data = json.loads(mesh.read_text())
    expected_nodes = {n[0]: tuple(F(float(x)) for x in n[1:]) for n in data['nodes']}
    expected_cells = {c['id']: tuple(c['nodes']) for c in data['cells']}
    def sensor(p):
        x, y, z = p
        return (2+x, F(.2), F(.1), 3+y, F(.15), 4+z)
    transported = defaultdict(dict)
    observed = defaultdict(list)
    for line in log.read_text().splitlines():
        row = line.split()
        if not row or row[0] not in ('DONOR_SOURCE', 'DONOR_SAMPLE'): continue
        rank = int(row[1]); assert 0 <= rank < ranks
        if row[0] == 'DONOR_SOURCE':
            assert len(row) == 43
            cell = int(row[2]); assert cell not in transported[rank]
            ids, points, metrics = [], [], []
            for i in range(4):
                k = 3 + 10*i
                node = int(row[k]); p = tuple(F(float(x)) for x in row[k+1:k+4])
                m = tuple(F(float(x)) for x in row[k+4:k+10])
                assert p == expected_nodes[node] and m == sensor(p)
                spd(m); ids.append(node); points.append(p); metrics.append(m)
            assert tuple(ids) == expected_cells[cell] and determinant(points) > 0
            transported[rank][cell] = (tuple(sorted(ids)), points, metrics)
        else:
            assert len(row) == 21
            p = tuple(F(float(x)) for x in row[2:5])
            m = tuple(F(float(x)) for x in row[5:11])
            target = tuple(F(float(x)) for x in row[11:17])
            observed[rank].append((p, m, target, tuple(int(x) for x in row[17:21])))
    assert set(transported) == set(observed) == set(range(ranks))
    maximum = F(0)
    for rank in range(ranks):
        assert set(transported[rank]) == set(expected_cells)
        assert len(observed[rank]) == 4
        assert len({s[0] for s in observed[rank]}) == 4
        originals = sorted(transported[rank].values())
        for p, m, target, key in observed[rank]:
            donor = next(c for c in originals if min(weights(c[1], p)) >= 0)
            assert key == donor[0]
            w = weights(donor[1], p)
            reference = tuple(sum(w[i]*donor[2][i][j] for i in range(4)) for j in range(6))
            assert reference == sensor(p)
            maximum = max(maximum, *(abs(a-b) for a,b in zip(m,reference)))
            composed = list(reference)
            if p[2] < F(1,16): composed[5] += 100*(1-16*p[2])
            maximum = max(maximum, *(abs(a-b) for a,b in zip(target,composed)))
            spd(m); spd(target)
    assert maximum <= F(2,10**14)
    return {'status':'PASS', 'ranks':ranks, 'donors':6*ranks, 'samples':4*ranks,
            'maximum_absolute_residual':float(maximum),
            'scope':'Actual transported affine sensor records, exact canonical containment/P1 and manufactured thin-region actual-query composition; not production BL integration or full adaptation.',
            'source_log_sha256':hashlib.sha256(log.read_bytes()).hexdigest(),
            'mesh_sha256':hashlib.sha256(mesh.read_bytes()).hexdigest()}

if __name__ == '__main__':
    log, mesh, ranks, output = sys.argv[1:]
    result = audit(Path(log), Path(mesh), int(ranks))
    Path(output).write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result))
