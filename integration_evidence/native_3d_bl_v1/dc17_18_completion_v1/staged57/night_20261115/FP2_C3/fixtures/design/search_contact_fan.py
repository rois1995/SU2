import numpy as np
from itertools import combinations
from time import monotonic
rng=np.random.default_rng(2748039)
vs = np.array([(x,y,z) for z in range(2) for y in range(4) for x in range(4)], dtype=float)
vs[5,2] = .2
t=[]
for y in range(3):
 for x in range(3):
  v=np.array([0,1,5,4,16,17,21,20])+x+4*y
  r=[1,2,3,7,4,5]
  t.extend([v[[0,6,a,b]] for a,b in zip(r,r[1:]+r[:1])])
ts=np.array(t)
ns=ts[np.any(ts==5,axis=1)&np.all(ts!=6,axis=1)].copy()
ns[ns==5]=6
fs=np.array([[0,1,6],[0,4,6],[4,6,9],[6,9,10]])
at=np.vstack([ts,ns])
ijk=np.array(list(combinations(range(4),3)))
ij=np.array(list(combinations(range(4),2)))
def dets(v):
 return np.linalg.det(v[:,at,1:] - v[:,at,:1]) if v.ndim==4 else np.linalg.det(v[:,at][:,:,1:]-v[:,at][:,:,:1])
sign=np.sign(dets(vs[None]))[0]
pairs=[(a,b) for a,b in combinations(range(len(ns)),2) if len(set(ns[a]) & set(ns[b])) < 3]
def overlapping(v,a,b):
 av,bv=v[a],v[b]
 if np.any(np.max(av,axis=0)<np.min(bv,axis=0)) or np.any(np.max(bv,axis=0)<np.min(av,axis=0)):
  return False
 avf,bvf=av[ijk],bv[ijk]
 nd=np.concatenate([np.cross(avf[:,1]-avf[:,0],avf[:,2]-avf[:,0]),
                    np.cross(bvf[:,1]-bvf[:,0],bvf[:,2]-bvf[:,0])])
 ae,be=av[ij[:,1]]-av[ij[:,0]],bv[ij[:,1]]-bv[ij[:,0]]
 nd=np.vstack([nd,np.cross(ae[:,None],be[None,:]).reshape(-1,3)])
 ap,bp=av@nd.T,bv@nd.T
 return (not np.any(((ap.max(axis=0)-bp.min(axis=0)<1e-11) | (bp.max(axis=0)-ap.min(axis=0)<1e-11)) & (np.linalg.norm(nd,axis=1)>1e-10)))
start=monotonic()
samples=kept=0
while monotonic()-start < 8:
 v=np.broadcast_to(vs,(512,32,3)).copy()
 scale=rng.uniform(.08,.6,(v.shape[0],1,1))
 v+=rng.normal(size=v.shape)*scale
 v=v[np.min(dets(v)*sign,axis=1)>1e-8]
 g=v[:,fs]
 area=np.linalg.norm(np.cross(g[:,:,1]-g[:,:,0],g[:,:,2]-g[:,:,0]),axis=-1)
 lb=np.max(np.linalg.norm(g-np.roll(g,1,axis=2),axis=-1),axis=2)
 v=v[np.min(area/lb,axis=1)>=.35]
 samples+=512
 kept+=len(v)
 for candidate in v:
  for a,b in pairs:
   if overlapping(candidate[ns],a,b):
    if not any(overlapping(candidate[ts],c,d) for c,d in combinations(range(len(ts)),2)
               if len(set(ts[c])&set(ts[d]))<3):
     print('FOUND',samples,kept,a,b,np.round(candidate,6).tolist())
     quit()
print('SEARCH',samples,kept,'no positive old embedded and positive new overlapping cells; before reverse check')
