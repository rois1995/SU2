"""Exact binary64 determinant/quadratic reference for transition diagnostics; no acceptance claim."""
import hashlib,json,math,struct,sys
from fractions import Fraction as F
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2
m=read_su2(HERE.parent/'first_row_shape_ranking_v1/Coarse.su2');measure=np.load(HERE/'wall_geometry.npz')['measurements']
folder=HERE/'transition_np2';a=np.concatenate([np.genfromtxt(p,delimiter=',',names=True) for p in sorted(folder.glob('layer_probe_rank*.csv'))]);a.sort(order='row')
assert len(a)==3*len(measure);assert np.array_equal(a['row'],np.arange(len(a)))
g=np.concatenate([np.genfromtxt(p,delimiter=',',names=True) for p in sorted(folder.glob('layer_guide_rank*.csv'))]);g.sort(order='node')
assert np.array_equal(g['node'],np.arange(len(m.P)))
dtype=np.dtype([('parent','<u8'),('tet','<f8',(4,3))]);cap=np.memmap(HERE/'transition_children.bin',mode='r',dtype=dtype,offset=16,shape=(len(a),))
assert np.array_equal(cap['parent'],a['parent'])
def det(t):
 d=[[F(float(x))-F(float(y)) for x,y in zip(p,t[0])] for p in t[1:]]
 return d[0][0]*(d[1][1]*d[2][2]-d[1][2]*d[2][1])-d[0][1]*(d[1][0]*d[2][2]-d[1][2]*d[2][0])+d[0][2]*(d[1][0]*d[2][1]-d[1][1]*d[2][0])
def quad(m,d):
 xx,xy,xz,yy,yz,zz=m;x,y,z=d;return xx*x*x+yy*y*y+zz*z*z+2*(xy*x*y+xz*x*z+yz*y*z)
selection=np.unique(np.r_[np.arange(32),np.argsort(a['q'])[:32],np.argsort(a['J'])[:32]])
quality_error=0.;jacobian_error=0.;height_error=0.;bary_error=0.;centroid_error=0.
eps_ld=F(1,1<<np.finfo(np.longdouble).nmant)
for index in selection:
 row=a[index];tet=cap['tet'][index];original=m.P[m.E[int(row['parent'])]].copy()
 if det(original)<0:original[[0,1]]=original[[1,0]]
 center=[float(row[k]) for k in ('x','y','z')]
 for k,value in enumerate(center):
  exact=sum(F(float(p[k])) for p in tet)/4;absolute=sum(abs(F(float(p[k]))) for p in tet)/4
  # Match the actual query; four long-double additions then a binary64 cast can double-round a midpoint.
  bound=4*eps_ld*absolute+abs(F(float(np.spacing(value))))/2
  residual=abs(F(value)-exact);assert residual<=bound,(index,k,float(residual),float(bound))
  centroid_error=max(centroid_error,float(residual))
 vol=det(original);exact=[]
 for j in range(4):
  changed=original.copy();changed[j]=center;exact.append(det(changed)/vol)
 assert min(exact)>=0
 bary_error=max(bary_error,max(abs(float(x)-row['b'+str(k)]) for k,x in enumerate(exact)))
 metric=[F(float(row[k])) for k in ('mxx','mxy','mxz','myy','myz','mzz')];xx,xy,xz,yy,yz,zz=metric
 determinant=xx*(yy*zz-yz*yz)-xy*(xy*zz-yz*xz)+xz*(xy*yz-yy*xz);assert determinant>0
 lengths={}
 for i in range(4):
  for j in range(i+1,4):lengths[i,j]=quad(metric,[F(float(x))-F(float(y)) for x,y in zip(tet[j],tet[i])])
 volume=abs(det(tet));squared=sum(lengths.values());q=float(432*volume*volume*determinant/squared**3)**(1/3)
 jac=min(1.,min(math.sqrt(float(2*volume*volume*determinant/math.prod(lengths[tuple(sorted((i,j)))] for j in range(4) if j!=i))) for i in range(4)))
 quality_error=max(quality_error,abs(q-row['q']));jacobian_error=max(jacobian_error,abs(jac-row['J']))
assert quality_error<1e-9 and jacobian_error<1e-9 and bary_error<2e-12
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
report=dict(status='PASS_INDEPENDENT_TRANSITION_DIAGNOSTIC_NOT_ACCEPTED_MESH',transition_cells=len(a),
 q_min=float(a['q'].min()),J_min=float(a['J'].min()),shape_failures=int(sum((a['q']<.2)|(a['J']<.05))),
 exact_sample_count=len(selection),exact_q_absolute_error=quality_error,exact_J_absolute_error=jacobian_error,
 exact_barycentric_error=bary_error,centroid_absolute_roundoff=centroid_error,
 limits=['All direct transition cells require repair; no complete mesh, full interior edge acceptance or transfer established.',
         'Exact determinant audit uses actual native-exported composed tensor; not an independent whole-field composition oracle.'],
 hashes={str(p.relative_to(ROOT)):sha(p) for p in [Path(__file__).resolve(),HERE/'transition_children.bin',folder/'provenance.json',folder/'result.json']})
(HERE/'transition_audit.json').write_text(json.dumps(report,indent=2,allow_nan=False)+'\n');print(json.dumps(report),flush=True)
