import json,itertools,math
from pathlib import Path
import numpy as np
root=Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated')
v=json.loads((root/'integration_evidence/native_3d_bl_v1/coupled_engine_v1/independent_grouped_coupled_BL_rejected_n1.json').read_text());base={n[0]:np.array(n[1:]) for n in v['nodes']};old=[c['nodes'] for c in v['cells']]
corner=[c for c in old if 0 in c and 4 in c];movers=sorted({n for c in corner for n in c}-{0,4,15});assert len(corner)==2 and len(movers)==2
A,B=movers;print('movers',movers,flush=True)
pairs=list(itertools.combinations(range(4),2));pi={p:i for i,p in enumerate(pairs)};gl,gw=np.polynomial.legendre.leggauss(32)
def length(a,b):
 d=b-a;dz=d[2];cuts=[0.,1.]
 if dz:cuts += [(z-a[2])/dz for z in [.05,.27,.3] if 0<(z-a[2])/dz<1]
 total=0
 for lo,hi in zip(sorted(cuts),sorted(cuts)[1:]):
  t=(lo+hi)/2+(hi-lo)/2*gl;z=a[2]+dz*t;u=np.clip((z-.27)/.03,0,1);w=1-u*u*(3-2*u);tan=1.5**w;normal=np.maximum(.1,2*(.1+.2*z)/2.2)**(-2*w)
  speed=np.sqrt((1+tan)/2*(d[0]**2+d[1]**2)+(1-tan)*d[0]*d[1]+normal*dz**2);total+=(hi-lo)/2*np.dot(gw,speed)
 return float(total)
def balanced(a,b,f=.5):
 requested=length(a,b)*f;lo=0;hi=1
 for _ in range(35):
  t=(lo+hi)/2
  if length(a,(1-t)*a+t*b)<requested:lo=t
  else:hi=t
 return (1-t)*a+t*b
extra=balanced(base[0],base[15]);print('extra',extra.tolist(),flush=True)
for reconnect in [False,True]:
 cells=[list(c) for c in old]
 if reconnect:cells=[c for c in cells if not (0 in c and 4 in c)]+[[0,4,A,B],[0,A,B,15],[4,A,B,15]]
 for edge,newid in [((0,4),200000),((0,15),200001)]:
  rebuilt=[]
  for c in cells:
   if all(n in c for n in edge):rebuilt += [[newid if n==edge[0] else n for n in c],[newid if n==edge[1] else n for n in c]]
   else:rebuilt.append(c)
  cells=rebuilt
 ids=sorted(set(n for c in cells for n in c));indices={n:i for i,n in enumerate(ids)};arr=np.array([[indices[n] for n in c] for c in cells]);ep=np.array(sorted({tuple(sorted((indices[c[i]],indices[c[j]]))) for c in cells for i,j in pairs}))
 def assess(x,z,split):
  p=dict(base);p[A]=np.array([x,0,z]);p[B]=np.array([0,x,z]);p[200000]=np.array([0,0,split]);p[200001]=extra;points=np.array([p[n] for n in ids]);t=points[arr]
  e=t[:,1:,:]-t[:,:1,:];det=np.abs(np.einsum('ij,ij->i',e[:,0,:],np.cross(e[:,1,:],e[:,2,:])))
  center=t[:,:,2].mean(axis=1);u=np.clip((center-.27)/.03,0,1);w=1-u*u*(3-2*u);tan=1.5**w;n=np.maximum(.1,2*(.1+.2*center)/2.2)**(-2*w)
  d=np.array([t[:,j,:]-t[:,i,:] for i,j in pairs]);sq=(1+tan)/2*(d[:,:,0]**2+d[:,:,1]**2)+(1-tan)*d[:,:,0]*d[:,:,1]+n*d[:,:,2]**2;vol=det*np.sqrt(tan*n)
  q=12*(vol/2)**(2/3)/sq.sum(axis=0);jac=np.min([np.sqrt(2)*vol/np.sqrt(np.prod(sq[[pi[tuple(sorted((i,k)))] for k in range(4) if k!=i]],axis=0)) for i in range(4)],axis=0)
  weak=int(np.argmin(np.minimum(q/.2,jac/.05)));return float(q.min()),float(jac.min()),cells[weak]
 for x,z,split in [(.263738891974,.174635351379,.195844099193),(.269,.174,.195),(.271,.1615,.1687335),(.1866561024,.1866561024,.1687335)]:
  print(reconnect,(x,z,split),assess(x,z,split),flush=True)
