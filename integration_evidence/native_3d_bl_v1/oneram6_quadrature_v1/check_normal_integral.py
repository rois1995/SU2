#!/usr/bin/env python3
"""Independent triangle-distance/Duffy quadrature check of the actual M6 cell13 BL normal density."""
from pathlib import Path
import hashlib,json,sys
import numpy as np
from numpy.polynomial.legendre import leggauss
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2
m=read_su2(ROOT/'integration_evidence/native_3d_bl_v1/oneram6_coarse_v1/Coarse.su2')
cell=m.P[np.sort(m.E[13])];alltri=m.P[m.M['WING3D']]
diameter=np.linalg.norm(cell[:,None,:]-cell[None,:,:],axis=2).max()
keep=np.all(alltri.max(1)>=cell.min(0)-diameter,axis=1)&np.all(alltri.min(1)<=cell.max(0)+diameter,axis=1)
tri=alltri[keep];a=tri[:,0];ab=tri[:,1]-a;ac=tri[:,2]-a
n=np.cross(ab,ac);n/=np.linalg.norm(n,axis=1)[:,None]
d00=np.einsum('ij,ij->i',ab,ab);d01=np.einsum('ij,ij->i',ab,ac);d11=np.einsum('ij,ij->i',ac,ac);det=d00*d11-d01*d01

def distance(points):
 result=[]
 for start in range(0,len(points),512):
  p=points[start:start+512,None,:];r=p-a
  q0=np.einsum('pfj,fj->pf',r,ab);q1=np.einsum('pfj,fj->pf',r,ac)
  u=(q0*d11-q1*d01)/det;v=(q1*d00-q0*d01)/det
  plane=np.einsum('pfj,fj->pf',r,n);inside=(u>=0)&(v>=0)&(u+v<=1)
  d2=np.where(inside,plane*plane,np.inf)
  for k in range(3):
   edge=tri[:,(k+1)%3]-tri[:,k];r=p-tri[:,k]
   t=np.clip(np.einsum('pfj,fj->pf',r,edge)/np.einsum('fj,fj->f',edge,edge),0,1)
   dr=r-t[:,:,None]*edge;d2=np.minimum(d2,np.einsum('pfj,pfj->pf',dr,dr))
  result.append(np.sqrt(np.maximum(0,d2.min(1))))
 return np.concatenate(result)

h0=2e-6;growth=1.2;volume=abs(np.linalg.det(cell[1:]-cell[0]))/6
assert np.max(distance(cell[:3]))<1e-14

def density(d):
 assert d.max()<.009 # This whole cell stays in the unchanged full-strength band.
 return 1/np.maximum(h0,2*(h0+(growth-1)*d)/(growth+1))

native={}
for suffix in ('150','125'):
 p=HERE/('replay_rule_'+suffix+'.csv')
 if not p.is_file():continue
 table=np.genfromtxt(p,delimiter=',',names=True);bary=np.column_stack([table['b'+str(k)] for k in range(4)])
 d=distance(bary@cell);exact=density(d)
 assert np.max(abs(d-table['distance']))<4e-14
 assert np.max(abs(exact/table['normal_density']-1))<2e-8
 native[suffix]={'points':len(table),'normal_integral':float(table['weight']@exact),
                 'full_unit_integral':float(table['weight']@table['full_unit_density']),
                 'volume_relative_error':float(abs(table['weight'].sum()/volume-1)),
                 'max_distance_absolute_error':float(np.max(abs(d-table['distance']))),
                 'rule_sha256':hashlib.sha256(p.read_bytes()).hexdigest()}
 assert native[suffix]['volume_relative_error']<2e-10

# Different coordinates from native clipping: each triangle-base point follows a ray to the apex.
height=distance(cell[3:])[0];cuts=[0.,.5*h0/height];level=h0/height
while level<1:cuts.append(level);level*=1.5
cuts.append(1.)
radial_nodes,radial_weights=leggauss(10)
t=[];wt=[]
for left,right in zip(cuts[:-1],cuts[1:]):
 half=.5*(right-left);t.extend(.5*(left+right)+half*radial_nodes);wt.extend(half*radial_weights)
t=np.array(t);wt=np.array(wt)
refs=[]
for order in (32,48,72):
 path=HERE/('normal_reference_'+str(order)+'.json')
 identity={'order':order,'script_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
           'fixture_sha256':hashlib.sha256((HERE/'wing_cell13.dat').read_bytes()).hexdigest()}
 if path.is_file():
  saved=json.loads(path.read_text())
  if all(saved.get(k)==v for k,v in identity.items()):refs.append(saved);continue
 z,w=leggauss(order);z=.5*(1+z);w=.5*w
 u,v=np.meshgrid(z,z,indexing='ij');angular=(w[:,None]*w[None,:]*u).ravel()
 base=(1-u.ravel())[:,None]*cell[0]+(u*(1-v)).ravel()[:,None]*cell[1]+(u*v).ravel()[:,None]*cell[2]
 integral=0.;mass=0.
 for k in range(len(t)):
  p=(1-t[k])*base+t[k]*cell[3];d=distance(p)
  factor=6*volume*(1-t[k])**2*wt[k]
  integral+=factor*(angular@density(d));mass+=factor*angular.sum()
 assert abs(mass/volume-1)<1e-12
 saved=dict(identity,normal_integral=float(integral),volume_relative_error=float(abs(mass/volume-1)))
 path.write_text(json.dumps(saved,indent=2)+'\n');refs.append(saved)
 print('Saved independent angular order',order,'integral',integral,flush=True)
reference=refs[-1]['normal_integral'];change=abs(reference/refs[-2]['normal_integral']-1)
report={'scope':'Independent exact all-nearby-triangle distances and product Gauss/Duffy reference for normal density only; no independent full tensor-density or remeshing proof.',
        'native':native,'references':refs,'last_reference_relative_change':change,
        'normal_relative_errors':{k:abs(v['normal_integral']/reference-1) for k,v in native.items()},
        'reference_converged':change<1e-4,'normal_density_tolerance':5e-4}
report['passed']=bool(report['reference_converged'] and len(native)==2 and all(v<5e-4 for v in report['normal_relative_errors'].values()))
(HERE/'normal_integral_check.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
assert report['passed'], 'Normal-density independent convergence/accuracy gate is not satisfied.'
