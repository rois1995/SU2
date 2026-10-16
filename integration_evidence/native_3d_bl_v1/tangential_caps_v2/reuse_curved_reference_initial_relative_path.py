"""Reuse an unchanged cylinder model/mesh reference after checking every new native sample and edge."""
import csv,hashlib,json,sys
from pathlib import Path
import numpy as np
ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_bl_v1/region_construction_v1'))
from construct import write_su2
from audit_full_metric import unpack
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 old,new,reference=map(Path,sys.argv[1:]);receipts=[]
 for case in (old,new):
  r=json.loads(case.with_suffix('.json').read_text());assert r['status']=='PASS'
  assert all(sha(case/p)==h for p,h in r['artifacts'].items());receipts.append(r)
 geometry=old/'core_independent.json';g=json.loads(geometry.read_text())
 assert g['status']=='PASS_COMPLETE_STRONGER_FIELD_CURVED_REGION_SCOPE_LIMITED'
 assert g['native_receipt_sha256']==sha(old.with_suffix('.json'))
 assert g['helper_sha256']==sha(ROOT/'integration_evidence/native_3d_bl_v1/region_construction_v1/audit_region_core.py')
 proof=json.loads(reference.read_text());assert proof['status']=='PASS_INDEPENDENT_COMPLETE_CYLINDER_METRIC_SCOPE_LIMITED'
 assert proof['contract']['native_receipt_sha256']==sha(old.with_suffix('.json'))
 for name,digest in proof['contract']['inputs'].items():assert sha(old/name)==digest
 for name,digest in proof['contract']['helpers'].items():assert sha(ROOT/'integration_evidence/native_3d_bl_v1/region_construction_v1'/name)==digest
 identical={}
 for name in ['core_accepted.json','core_initial.json','core.cfg','core_initial.su2.native_ref','core_sensors.csv']:
  assert sha(old/name)==sha(new/name),name;identical[name]=sha(new/name)
 data=json.loads((new/'core_accepted.json').read_text());nodes={n[0]:np.array(n[1:]) for n in data['nodes']};cells={c['id']:c['nodes'] for c in data['cells']}
 def samples(case):return {int(row['cell']):row for row in csv.DictReader((case/'core_samples.csv').open())}
 before,after=samples(old),samples(new);assert before.keys()==after.keys()==cells.keys();delta=shape_error=0.
 for key in cells:
  a=unpack(before[key][k] for k in ('xx','xy','xz','yy','yz','zz'));b=unpack(after[key][k] for k in ('xx','xy','xz','yy','yz','zz'))
  assert np.linalg.eigvalsh(b)[0]>0;delta=max(delta,np.linalg.norm(a-b)/max(1.,np.linalg.norm(a)))
  p=np.array([nodes[n] for n in cells[key]]);v=abs(np.linalg.det(p[1:]-p[0]))*np.sqrt(np.linalg.det(b));lengths={(i,j):(p[j]-p[i])@b@(p[j]-p[i]) for i in range(4) for j in range(i+1,4)}
  q=12*(v/2)**(2/3)/sum(lengths.values());j=min(1.,min(np.sqrt(2)*v/np.sqrt(np.prod([lengths[tuple(sorted((i,k)))] for k in range(4) if k!=i])) for i in range(4)))
  assert q>=.2 and j>=.05;shape_error=max(shape_error,abs(q-float(after[key]['q'])),abs(j-float(after[key]['j'])))
 assert shape_error<1e-10
 old_error=proof['maximum_point_relative_error'];point_bound=old_error+delta*(1+old_error)+1e-15;assert point_bound<1e-10
 ref_csv=reference.with_name(reference.stem+'_edges.csv');assert sha(ref_csv)==proof['edge_csv_sha256']
 edges={tuple(sorted((int(row['a']),int(row['b'])))):row for row in csv.DictReader(ref_csv.open())}
 actual={tuple(sorted((int(row['a']),int(row['b'])))):float(row['length']) for row in csv.DictReader((new/'core_edges.csv').open())}
 complete={tuple(sorted((c[i],c[j]))) for c in cells.values() for i in range(4) for j in range(i+1,4)}
 assert edges.keys()==actual.keys()==complete and len(complete)==proof['full_edges']
 maximum=error=0.
 for key,row in edges.items():
  value=float(row['independent_length']);indicator=float(row['estimated_error']);assert value+indicator<=1.8 and indicator<=1e-8*(1+value)
  residual=abs(actual[key]-value)/(1+value);assert residual<=1e-7;error=max(error,residual);maximum=max(maximum,value)
 write_su2(new/'core_accepted.su2',{n[0]:n[1:] for n in data['nodes']},[c['nodes'] for c in data['cells']],data['surface'],{f['id']:f['marker'] for f in data['facets']},data['marker_names'])
 Path(str(new/'core_accepted.su2')+'.native_ref').write_bytes((new/'core_initial.su2.native_ref').read_bytes())
 result=dict(status='PASS_COMPLETE_UNCHANGED_CYLINDER_REFERENCE_WITH_NEW_NATIVE_TENSOR_AND_EDGE_CHECKS',cells=len(cells),full_edges=len(complete),maximum_new_tensor_relative_change=float(delta),new_point_reference_relative_bound=float(point_bound),maximum_shape_recomputation_error=float(shape_error),maximum_full_length=maximum,maximum_native_relative_edge_difference=error,identical_model_mesh_inputs=identical,old_native_receipt_sha256=sha(old.with_suffix('.json')),new_native_receipt_sha256=sha(new.with_suffix('.json')),geometry_reference_sha256=sha(geometry),full_metric_reference=str(reference.relative_to(ROOT)),full_metric_reference_sha256=sha(reference),edge_reference_sha256=sha(ref_csv),helper_sha256=sha(Path(__file__)),export_sha256=sha(new/'core_accepted.su2'),numpy_version=np.__version__,scope='Bit-identical complete finite mesh/geometry/original sensors/config and unchanged independent physical-model helper. Exact geometry/height/links/no overlap proof reused. Every new actual tensor is compared to the old independently bounded sample; triangle inequality plus a numerical guard transfers the reference tolerance. Every new shape is recomputed; full edge coverage and native/reference residuals checked anew. The production closest-facet source changed: no unchanged-composition-source or identical-native-output claim. Reuses independent numerical quadrature, not rigorous enclosures or a wider geometry/MPI/CFD/performance certificate.')
 (new/'core_reference_reuse.json').write_text(json.dumps(result,indent=2)+'\n');print(result['status'],flush=True)
if __name__=='__main__':main()
