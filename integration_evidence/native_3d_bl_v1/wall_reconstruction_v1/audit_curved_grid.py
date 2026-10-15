#!/usr/bin/env python3
"""Independent geometry/height and exported-tensor quality audit; residuals remain rejection gates."""
import collections,csv,hashlib,itertools,json,math,sys
from fractions import Fraction as F
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
sys.path.insert(0,str(HERE.parent/'alignment_audit_v1'))
from audit_cavities import determinant,sub,dot,cross,overlap
from audit_coupled import surface
from baseline import footprints

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 stage=sys.argv[1] if len(sys.argv)>1 else 'curved_grid_audit'
 assert stage in ('curved_grid_audit','curved_interior_residual','curved_wall_refit_residual','curved_joint_connectivity_residual','curved_feature_residual')
 folder=HERE/stage;rp=HERE/(stage+'.json');receipt=json.loads(rp.read_text());assert receipt['status']=='PASS'
 for name in ['audited_grid.json','grid_samples.csv','grid_edges.csv','grid_metric_summary.json']:assert sha(folder/name)==receipt['artifacts'][name]
 data=json.loads((folder/'audited_grid.json').read_text());nodes={r[0]:tuple(map(F,r[1:])) for r in data['nodes']};uses=collections.defaultdict(list);volume=F(0);tets=[];boxes=[]
 samples={int(r['cell']):r for r in csv.DictReader((folder/'grid_samples.csv').open())};qmin=jmin=1.;difference=0.;bad=[]
 for c in data['cells']:
  t=[nodes[n] for n in c['nodes']];dv=determinant(t);assert dv>0;volume+=dv/6;tets.append(t)
  boxes.append(tuple((min(float(p[k]) for p in t),max(float(p[k]) for p in t)) for k in range(3)))
  a,b,d,e=c['nodes']
  for face in [(b,d,e),(a,e,d),(a,b,e),(a,d,b)]:uses[tuple(sorted(face))].append((-1)**sum(face[i]>face[j] for i in range(3) for j in range(i+1,3)))
  r=samples[c['id']];xx,xy,xz,yy,yz,zz=[F(float(r[k])) for k in ['xx','xy','xz','yy','yz','zz']]
  det=xx*(yy*zz-yz*yz)-xy*(xy*zz-yz*xz)+xz*(xy*yz-yy*xz);assert xx>0 and xx*yy-xy*xy>0 and det>0
  lengths={}
  for i,j in itertools.combinations(range(4),2):
   x,y,z=sub(t[j],t[i]);lengths[i,j]=math.sqrt(float(xx*x*x+yy*y*y+zz*z*z+2*(xy*x*y+xz*x*z+yz*y*z)))
  weighted=float(dv)*math.sqrt(float(det));q=12*(weighted/2)**(2/3)/sum(v*v for v in lengths.values())
  j=min(1.,min(math.sqrt(2)*weighted/math.prod(lengths[tuple(sorted((i,k)))] for k in range(4) if i!=k) for i in range(4)))
  delta=max(abs(q-float(r['q'])),abs(j-float(r['j'])));assert delta<=2e-10;difference=max(difference,delta);qmin=min(qmin,q);jmin=min(jmin,j)
  if q<.2 or j<.05:bad.append(c['id'])
 assert all(len(v)==1 or len(v)==2 and sum(v)==0 for v in uses.values())
 boundary={f for f,v in uses.items() if len(v)==1};assert boundary=={tuple(sorted(f['nodes'])) for f in data['surface']};surface(list(boundary),True)
 candidates=0
 for i,j in itertools.combinations(range(len(tets)),2):
  if any(boxes[i][k][1]<=boxes[j][k][0] or boxes[j][k][1]<=boxes[i][k][0] for k in range(3)):continue
  candidates+=1;assert not overlap(tets[i],tets[j]),'positive-volume overlap'
 rows=footprints(data,'cylinder');assert rows;exact_feet=all(not r['outside'] for r in rows);maximum=0.;cellmap={c['id']:c for c in data['cells']}
 for r in rows:
  a,b,c=[nodes[n] for n in r['face']];normal=cross(sub(b,a),sub(c,a));height=abs(float(dot(sub(nodes[r['apex']],a),normal)))/math.sqrt(float(dot(normal,normal)));maximum=max(maximum,abs(height-.05)/.05)
 assert maximum<=1e-10
 assert not bad,'native-exported field has inadmissible shape/sliver'
 lengths=list(csv.DictReader((folder/'grid_edges.csv').open()));oversized=[r for r in lengths if float(r['length'])>1.8];assert oversized
 report=dict(status='REJECTED_LENGTH_TARGET_GEOMETRY_SHAPE_PASS' if exact_feet else 'REJECTED_LENGTH_TARGET_EXACT_FOOT_FAILURE',exact_normal_foot_pass=exact_feet,outside_normal_feet=[r for r in rows if r['outside']],scope='Exact positive volumes, no positive-volume overlap, oriented closed ball skin, exact normal feet audited separately and declared altitude; independent q/J arithmetic from native-exported actual-centroid tensors. Full composed lengths are native diagnostics, not independent quadrature proof. Oversized edges reject the event; no CFD/transfer qualification.',cells=len(tets),nodes=len(nodes),volume=str(volume),pair_candidates=candidates,minimum_q=qmin,minimum_j=jmin,shape_failures=bad,wall_faces=len(rows),maximum_height_relative_error=maximum,maximum_native_shape_difference=difference,oversized_edges=len(oversized),maximum_native_length=max(float(r['length']) for r in lengths),native_receipt_sha256=sha(rp),helper_sha256=sha(Path(__file__)),predicates_sha256=sha(ROOT/'integration_evidence/native_3d_core_v1/audit_cavities.py'),surface_helper_sha256=sha(ROOT/'integration_evidence/native_3d_core_v1/audit_coupled.py'),footprint_helper_sha256=sha(HERE.parent/'alignment_audit_v1/baseline.py'))
 (HERE/('curved_grid_independent.json' if stage=='curved_grid_audit' else 'feature_grid_independent.json' if stage=='curved_feature_residual' else 'joint_connectivity_grid_independent.json' if stage=='curved_joint_connectivity_residual' else 'refit_grid_independent.json' if stage=='curved_wall_refit_residual' else 'interior_grid_independent.json')).write_text(json.dumps(report,indent=2)+'\n');print(report['status']);sys.exit(0 if exact_feet else 1)
if __name__=='__main__':main()
