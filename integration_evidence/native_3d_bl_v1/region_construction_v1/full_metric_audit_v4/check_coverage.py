"""Check exact native and independent metric-edge coverage, including negative controls."""
import csv,hashlib,json
from pathlib import Path
HERE=Path(__file__).resolve().parent;CASE=HERE.parent/'native_front_constant_v2'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def coverage(rows,expected):
    actual=[tuple(sorted((int(r['a']),int(r['b'])))) for r in rows]
    assert len(actual)==len(expected) and len(set(actual))==len(actual) and set(actual)==expected
report=json.loads((HERE/'native_front_constant_v2.json').read_text());assert report['status']=='PASS_INDEPENDENT_COMPLETE_CYLINDER_METRIC_SCOPE_LIMITED'
assert all(sha(CASE/p)==h for p,h in report['contract']['inputs'].items())
assert sha(HERE/'audit_full_metric_executed.py')==report['contract']['helpers']['audit_full_metric.py']
assert sha(HERE/'native_front_constant_v2_edges.csv')==report['edge_csv_sha256']
data=json.loads((CASE/'core_accepted.json').read_text());expected={tuple(sorted((c['nodes'][i],c['nodes'][j]))) for c in data['cells'] for i in range(4) for j in range(i+1,4)}
for path in (HERE/'native_front_constant_v2_edges.csv',CASE/'core_edges.csv'):
    rows=list(csv.DictReader(path.open()));coverage(rows,expected)
    for bad in (rows[:-1],rows[:-1]+[rows[0]]):
        try:coverage(bad,expected)
        except AssertionError:pass
        else:raise AssertionError('Incomplete/duplicate edge control was accepted')
result={'status':'PASS_COMPLETE_NATIVE_AND_REFERENCE_EDGE_COVERAGE','full_edges':len(expected),'negative_controls_rejected':4,
        'audit_sha256':sha(HERE/'native_front_constant_v2.json'),'edge_csv_sha256':report['edge_csv_sha256'],
        'native_edge_csv_sha256':sha(CASE/'core_edges.csv'),'helper_sha256':sha(Path(__file__))}
(HERE/'coverage.json').write_text(json.dumps(result,indent=2)+'\n');print(result['status'])
