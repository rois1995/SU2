"""Exact whole-geometry/height/source audit of the complete coordinated proposal."""
import csv,json,sys,time
from pathlib import Path
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE.parent))
from construct import sha,authorities
from audit_runtime_mesh import mesh
start=time.monotonic();source=HERE.parent/'native_front_constant_v1';candidate=HERE/'candidate.json';data=json.loads(candidate.read_text());r=json.loads((HERE/'result.json').read_text())
assert r['status']=='PASS_COMPLETE_POINT_SHAPE_AND_FULL_EDGES_GEOMETRY_PENDING' and sha(candidate)==r['candidate_sha256']
original=json.loads((source/'core_construction_rejected.json').read_text());assert all(data[k]==original[k] for k in ('original','facets','facet_groups','marker_names'))
marker={f['id']:original['marker_names'][f['marker']] for f in original['facets']};wall={n for f in original['surface'] if marker[f['facet']]=='cylinder' for n in f['nodes']}
apices={n for c in original['cells'] if sum(i in wall for i in c['nodes'])==3 for n in c['nodes'] if n not in wall}
first={tuple(sorted(c['nodes'])) for c in original['cells'] if any(n in apices for n in c['nodes'])};assert len(first)==840
assert first<={tuple(sorted(c['nodes'])) for c in data['cells']};old_nodes={n[0]:tuple(n[1:]) for n in original['nodes']};new_nodes={n[0]:tuple(n[1:]) for n in data['nodes']}
assert all(new_nodes[n]==old_nodes[n] for c in first for n in c)
expected={tuple(sorted((c['nodes'][i],c['nodes'][j]))) for c in data['cells'] for i in range(4) for j in range(i+1,4)};rows=list(csv.DictReader((HERE/'full_edges.csv').open()));actual=[(int(row['a']),int(row['b'])) for row in rows]
assert len(actual)==len(set(actual)) and set(actual)==expected and len(actual)==r['full_edges']
assert all(float(row['length'])+float(row['estimated_error'])<=1.8 for row in rows)
geometry,_=authorities(source/'core_initial.su2.native_ref');report={'status':'RUNNING','candidate_sha256':sha(candidate)};(HERE/'independent_geometry.json').write_text(json.dumps(report,indent=2)+'\n')
try:
 report,_=mesh(data,geometry,True,.01)
 report.update(status='PASS_COMPLETE_INDEPENDENT_HIGH_RATIO_PROPOSAL_NOT_NATIVE_ACCEPTED',unchanged_first_band_tets=len(first),unchanged_original_sensors_and_reference=True,
               candidate_sha256=sha(candidate),metric_report_sha256=sha(HERE/'result.json'),full_edges_sha256=sha(HERE/'full_edges.csv'),helper_sha256=sha(Path(__file__)),
               geometry_helper_sha256=sha(HERE.parent/'audit_runtime_mesh.py'),scope='Complete independent numerical metric/point shape and exact geometry/embedding/manifold/height/authority/first-row/source prototype. No native construction/publication, charged-work/time, MPI/CFD/transfer/restart or general-geometry/performance acceptance.')
except Exception as error:
 report.update(status='FAIL',error=repr(error));raise
finally:
 report['elapsed_seconds']=time.monotonic()-start;(HERE/'independent_geometry.json').write_text(json.dumps(report,indent=2)+'\n')
print(report['status'],flush=True)
