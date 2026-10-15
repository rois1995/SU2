#!/usr/bin/env python3
"""Alignment diagnostics on the independently audited unit-cylinder first rows."""
import hashlib,json,math
from pathlib import Path
HERE=Path(__file__).resolve().parent

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def tilt(triangle,apex):
 a,b,c=triangle;u=[b[k]-a[k] for k in range(3)];v=[c[k]-a[k] for k in range(3)]
 normal=[u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]]
 norm=math.sqrt(sum(x*x for x in normal));assert norm>0;normal=[x/norm for x in normal]
 delta=[apex[k]-(a[k]+b[k]+c[k])/3 for k in range(3)];height=sum(x*y for x,y in zip(delta,normal));assert height!=0
 tangent=[delta[k]-height*normal[k] for k in range(3)]
 return math.sqrt(sum(x*x for x in tangent))/abs(height)

def main():
 triangle=[[0.,0.,0.],[3.,0.,0.],[0.,3.,0.]]
 assert abs(tilt(triangle,[1.,1.,.05]))<1e-12
 assert abs(tilt(triangle,[1.1,1.,.05])-2)<1e-12
 rows=[]
 for stage,name,geometry in [('curved_feature_residual','audited_grid.json','feature_grid_independent.json'),('curved_metric_patch_probe','metric_patch_repaired.json','metric_patch_geometry.json'),('curved_metric_residual','audited_grid.json','metric_grid_independent.json')]:
  rp=HERE/(stage+'.json');receipt=json.loads(rp.read_text());p=HERE/stage/name;assert sha(p)==receipt['artifacts'][name]
  proof=HERE/geometry;g=json.loads(proof.read_text());assert g['status'] in ('PASS_LOCAL_GEOMETRY_ONLY','REJECTED_LENGTH_TARGET_GEOMETRY_SHAPE_PASS')
  d=json.loads(p.read_text());nodes={n[0]:n[1:] for n in d['nodes']};markers={f['id']:d['marker_names'][f['marker']] for f in d['facets']};angles=[];clearances=[]
  for f in d['surface']:
   if markers[f['facet']]!='cylinder':continue
   owner=[c for c in d['cells'] if set(f['nodes'])<=set(c['nodes'])];assert len(owner)==1
   apex=nodes[next(n for n in owner[0]['nodes'] if n not in f['nodes'])];angles.append(tilt([nodes[n] for n in f['nodes']],apex));clearances.append((1-math.hypot(apex[0],apex[1]))/.05)
  assert len(angles)==g['wall_faces'] and min(clearances)>0
  rows.append(dict(stage=stage,wall_faces=len(angles),maximum_centroid_tilt=max(angles),minimum_smooth_clearance_over_prescribed_height=min(clearances),input_sha256=sha(p),native_receipt_sha256=sha(rp),geometry_proof_sha256=sha(proof)))
 report=dict(status='FIRST_ROW_ALIGNMENT_DIAGNOSTIC_ONLY',rows=rows,helper_sha256=sha(Path(__file__)),scope='Unit-cylinder authority (radius1,axisz) and configured facet altitude .05. Independent full geometry/normal-foot/height proofs referenced separately. Centroid tangent offset/altitude and smooth radial clearance are diagnostics, not an orthogonality or complete multilayer/viscous accuracy certificate.')
 (HERE/'metric_alignment.json').write_text(json.dumps(report,indent=2)+'\n');print(report['status'],rows)
if __name__=='__main__':main()
