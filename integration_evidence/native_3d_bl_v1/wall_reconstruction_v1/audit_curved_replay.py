#!/usr/bin/env python3
"""Exact local geometry and independent quality arithmetic using exported actual-query tensors."""
import hashlib,itertools,json,math,sys
from fractions import Fraction as F
from pathlib import Path
from audit_curved_cavity import frame
from audit_cavities import determinant,sub
HERE=Path(__file__).resolve().parent

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 stage=sys.argv[1] if len(sys.argv)>1 else 'curved_replay';assert stage in ('curved_replay','curved_replay_remaining','curved_replay_complete')
 folder=HERE/stage;receipt_path=HERE/(stage+'.json');receipt=json.loads(receipt_path.read_text());assert receipt['status']=='PASS'
 for f in folder.glob('replay*.json'):assert sha(f)==receipt['artifacts'][f.name]
 old,on,interface,ov=frame(folder/'replay_original.json');rows=[]
 for sample_path in sorted(folder.glob('replay_*_samples.json')):
  patch_path=sample_path.with_name(sample_path.name.replace('_samples',''));data,nodes,outer,volume=frame(patch_path)
  assert outer==interface and all(on[n]==nodes[n] for f in interface for n in f)
  samples={r['cell']:r for r in json.loads(sample_path.read_text())};assert set(samples)=={r['id'] for r in data['cells']}
  minimum_q=minimum_j=1.;loss=0.;bad=[];difference=0.
  for c in data['cells']:
   t=[nodes[n] for n in c['nodes']];r=samples[c['id']];xx,xy,xz,yy,yz,zz=map(F,r['tensor'])
   det=xx*(yy*zz-yz*yz)-xy*(xy*zz-yz*xz)+xz*(xy*yz-yy*xz);assert xx>0 and xx*yy-xy*xy>0 and det>0
   def quadratic(e):
    x,y,z=e;return xx*x*x+yy*y*y+zz*z*z+2*(xy*x*y+xz*x*z+yz*y*z)
   lengths={(i,j):math.sqrt(float(quadratic(sub(t[j],t[i])))) for i,j in itertools.combinations(range(4),2)}
   dv=float(determinant(t))*math.sqrt(float(det));q=12*(dv/2)**(2/3)/sum(v*v for v in lengths.values())
   j=min(1.,min(math.sqrt(2)*dv/math.prod(lengths[tuple(sorted((i,k)))] for k in range(4) if i!=k) for i in range(4)))
   error=max(abs(q-r['q']),abs(j-r['j']));assert error<=2e-10;difference=max(difference,error)
   minimum_q=min(minimum_q,q);minimum_j=min(minimum_j,j);loss+=max(0.,1-q/.2)**2+max(0.,1-j/.05)**2
   if q<.2 or j<.05:bad.append(c['id'])
  rows.append(dict(file=patch_path.name,cells=len(data['cells']),volume=str(volume),q=minimum_q,j=minimum_j,shape_deficit=loss,
      violating_cells=bad,maximum_native_arithmetic_difference=difference,input_sha256=sha(patch_path),sample_sha256=sha(sample_path)))
 assert len(rows)==2
 report=dict(status='PASS_DIAGNOSIS_ONLY',scope='Exact positive volumes, no positive-volume overlap and preserved artificial interface; independent q/J arithmetic from native-exported actual-centroid tensors. Does not independently validate the BL field, full composed edge lengths, whole domain or adaptation/CFD replacement.',
    rows=rows,native_receipt_sha256=sha(receipt_path),helper_sha256=sha(Path(__file__)),geometry_helper_sha256=sha(HERE/'audit_curved_cavity.py'),
    predicate_helper_sha256=sha(HERE.parents[2]/'integration_evidence/native_3d_core_v1/audit_cavities.py'))
 (HERE/(stage+'_audit.json')).write_text(json.dumps(report,indent=2)+'\n');print(report['status'])
if __name__=='__main__':main()
