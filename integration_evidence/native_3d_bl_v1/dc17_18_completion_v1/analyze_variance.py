"""Attribute diagnostic estimator contributions; check reconstruction against each completed assessment."""
import bisect
import hashlib
import json
import math
import sys
from collections import defaultdict
from pathlib import Path
folder = Path(sys.argv[1])
bounds = [.0032, .9, 1., 2., 5., 10., math.inf]
rows = defaultdict(list)
for line in (folder / 'run.log').open():
    if not line.startswith('VARIANCE '):
        continue
    v = list(map(float, line.split()[1:]))
    assert len(v) == 15
    rows[(v[0], int(v[1]))].append(v)
result = []
for line in (folder / 'profiles.jsonl').open():
    assessment = json.loads(line)
    n = assessment['samples']
    data = rows[(assessment['profile'], n)]
    assert len(data) == assessment['inside']
    correction = math.fsum(v[8] for v in data) / n
    assert math.isclose(correction, assessment['correction'], rel_tol=1e-11)
    bins = [[0., 0., 0., 0] for _ in bounds]
    for v in data:
        b = bins[min(bisect.bisect_left(bounds, v[6]), len(bounds)-1)]
        b[0] += v[8] / n
        b[1] += (math.exp(v[10])-math.exp(v[9])) * v[7] / n
        b[2] += (math.exp(v[11])-math.exp(v[9])) * v[7] / n
        b[3] += 1
    result.append(dict(assessment, bins=[dict(through_d_over_T=str(edge), option_a=b[0], base=b[1], full_d1=b[2], queries=b[3]) for edge,b in zip(bounds,bins)],
        largest_queries=sorted(data,key=lambda v:v[8],reverse=True)[:10]))
for name in ('source_input_pins.json','runtime_pins.json'):
    for path, expected in json.loads((folder/name).read_text()).items():
        assert hashlib.sha256(Path(path).read_bytes()).hexdigest() == expected, path
(folder/'attribution.json').write_text(json.dumps(result,indent=2,allow_nan=False)+'\n')
for row in result:
    if row['samples'] != 32768:
        continue
    print('profile',row['profile'],'total',round(row['value']))
    for b in row['bins']:
        print(b['through_d_over_T'],*[round(b[k]) for k in ('option_a','base','full_d1')],b['queries'])
