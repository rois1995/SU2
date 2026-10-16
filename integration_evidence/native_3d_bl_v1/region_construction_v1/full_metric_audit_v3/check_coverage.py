"""Verify complete edge coverage and the saved independent convergence control."""
import csv,hashlib,json
from pathlib import Path
HERE=Path(__file__).resolve().parent;CASE=HERE.parent/'native_core_rotated_v10'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def coverage(rows,expected):
    actual=[tuple(sorted((int(r['a']),int(r['b'])))) for r in rows]
    assert len(actual)==len(expected) and len(set(actual))==len(actual) and set(actual)==expected
report=json.loads((HERE/'native_core_rotated_v10.json').read_text());assert report['status']=='PASS_INDEPENDENT_COMPLETE_CYLINDER_METRIC_SCOPE_LIMITED'
assert all(sha(CASE/p)==h for p,h in report['contract']['inputs'].items())
assert sha(HERE.parent/'audit_full_metric.py')==report['contract']['helpers']['audit_full_metric.py']
assert sha(HERE/'native_core_rotated_v10_edges.csv')==report['edge_csv_sha256']
data=json.loads((CASE/'core_accepted.json').read_text());expected={tuple(sorted((c['nodes'][i],c['nodes'][j]))) for c in data['cells'] for i in range(4) for j in range(i+1,4)}
rows=list(csv.DictReader((HERE/'native_core_rotated_v10_edges.csv').open()));coverage(rows,expected)
for bad in (rows[:-1],rows[:-1]+[rows[0]]):
    try:coverage(bad,expected)
    except AssertionError:pass
    else:raise AssertionError('Incomplete/duplicate edge control was accepted')
diagnosis=HERE.parent/'full_metric_audit_v1/edge_diagnosis.json';d=json.loads(diagnosis.read_text())
r=next(r for r in rows if (int(r['a']),int(r['b']))==tuple(d['edge']))
reference=float(r['independent_length']);high=[v for v in d['results'] if v.get('tolerance',1)<1e-10 or v.get('panels_per_shell',0)>=64]
assert high and all(abs(v['value']-reference)<2e-10 for v in high)
result={'status':'PASS_COMPLETE_EDGE_SET_AND_CONVERGENCE_CONTROL','full_edges':len(expected),'duplicate_and_missing_controls_rejected':2,
        'convergence_control_edge':d['edge'],'convergence_control_maximum_difference':max(abs(v['value']-reference) for v in high),
        'audit_sha256':sha(HERE/'native_core_rotated_v10.json'),'edge_csv_sha256':report['edge_csv_sha256'],
        'convergence_diagnosis_sha256':sha(diagnosis),'helper_sha256':sha(Path(__file__))}
(HERE/'coverage.json').write_text(json.dumps(result,indent=2)+'\n');print(result['status'])
