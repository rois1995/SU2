"""Exact binary64 determinant/quadratic reference for wall-cap diagnostics; no acceptance claim."""
import hashlib,json,math,struct,sys
from fractions import Fraction as F
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2
m=read_su2(HERE.parent/'first_row_shape_ranking_v1/Coarse.su2');measure=np.load(HERE/'wall_geometry.npz')['measurements']
folder=HERE/'normal_caps_np2';a=np.concatenate([np.genfromtxt(p,delimiter=',',names=True) for p in sorted(folder.glob('layer_probe_rank*.csv'))]);a.sort(order='row')
assert np.array_equal(a['row'],np.arange(len(measure)))
g=np.concatenate([np.genfromtxt(p,delimiter=',',names=True) for p in sorted(folder.glob('layer_guide_rank*.csv'))]);g.sort(order='node')
assert np.array_equal(g['node'],np.arange(len(m.P)))
dtype=np.dtype([('parent','<u8'),('tet','<f8',(4,3))]);cap=np.memmap(HERE/'normal_caps.bin',mode='r',dtype=dtype,offset=16,shape=(len(a),))
assert np.array_equal(cap['parent'],a['parent'])
def det(t):
 d=[[F(float(x))-F(float(y)) for x,y in zip(p,t[0])] for p in t[1:]]
 return d[0][0]*(d[1][1]*d[2][2]-d[1][2]*d[2][1])-d[0][1]*(d[1][0]*d[2][2]-d[1][2]*d[2][0])+d[0][2]*(d[1][0]*d[2][1]-d[1][1]*d[2][0])
def quad(m,d):
 xx,xy,xz,yy,yz,zz=m;x,y,z=d;return xx*x*x+yy*y*y+zz*z*z+2*(xy*x*y+xz*x*z+yz*y*z)
selection=np.unique(np.r_[np.arange(32),np.argsort(a['q'])[:32],np.argsort(a['J'])[:32]])
quality_error=0.;jacobian_error=0.;height_error=0.;bary_error=0.
for index in selection:
 row=a[index];tet=cap['tet'][index];original=m.P[m.E[int(row['parent'])]].copy()
 if det(original)<0:original[[0,1]]=original[[1,0]]
 center=[float(sum(F(float(p[k])) for p in tet)/4) for k in range(3)]
 assert center==[row[k] for k in ('x','y','z')]
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
 ab=[F(float(tet[1,k]))-F(float(tet[0,k])) for k in range(3)];ac=[F(float(tet[2,k]))-F(float(tet[0,k])) for k in range(3)]
 cross=[ab[1]*ac[2]-ab[2]*ac[1],ab[2]*ac[0]-ab[0]*ac[2],ab[0]*ac[1]-ab[1]*ac[0]]
 height_error=max(height_error,abs(math.sqrt(float(volume*volume/sum(x*x for x in cross)))/2e-6-1))
assert quality_error<1e-9 and jacobian_error<1e-9 and bary_error<2e-12 and height_error<1e-8
ratio=measure[:,3]/(2e-6*g['normalized_distance'][measure[:,2].astype(int)])
quantiles=lambda x:{str(p):float(np.quantile(x,p)) for p in (0,.001,.01,.1,.5,.9,.99,1)}
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
report=dict(status='PASS_DIAGNOSTIC_NOT_A_COMPLETE_MESH',wall_caps=len(a),q=quantiles(a['q']),J=quantiles(a['J']),
 shape_failures=int(sum((a['q']<.2)|(a['J']<.05))),exact_sample_count=len(selection),exact_q_absolute_error=quality_error,
 exact_J_absolute_error=jacobian_error,exact_barycentric_error=bary_error,exact_height_relative_error=height_error,
 plane_height_over_actual_apex_distance=quantiles(ratio),normal_apex_maximum_affine_guide=float(max(1/ratio)),
 original_positive_guide_minimum=float(g['normalized_distance'][g['normalized_distance']>0].min()),
 source_rule='Native actual-centroid original owner-UPPER6 P1 plus complete original geometric BL/finer/fade; original geometry retained.',
 limits=['No full composed edges, transition topology, complete generation/adaptation/transfer or CFD accuracy checked.','Native whole-field composition sampled, independently exact determinant/shape/barycentric/height checked on a declared subset; not an independent whole-field geometry oracle.','Throughput is diagnostic; no lifecycle speedup or new M6 acceptance.'],
 hashes={str(p.relative_to(ROOT)):sha(p) for p in [Path(__file__).resolve(),HERE/'normal_caps.bin',HERE/'wall_geometry.npz',folder/'provenance.json',folder/'result.json']})
(HERE/'caps_audit.json').write_text(json.dumps(report,indent=2,allow_nan=False)+'\n');print(json.dumps(report,indent=2),flush=True)
