exec(open('/tmp/native_cold_offedge_probe.py').read().split('reports=[]')[0])
from scipy.optimize import differential_evolution
source=[list(c) for c in old]
p=dict(base);p[200000]=np.array([0,0,.18]);p[200001]=np.array([.12,.12,.1]);cells=[]
for c in source:
 if 0 in c and 4 in c:cells += [[200000 if n==0 else n for n in c],[200000 if n==4 else n for n in c]]
 else:cells.append(c)
for c in cells:
 if orient(c,p)<0:c[0],c[1]=c[1],c[0]
star=[c for c in cells if 0 in c and 15 in c];outside=[c for c in cells if c not in star];counts=defaultdict(list)
for c in star:
 for f in faces(c):counts[tuple(sorted(f))].append(f)
skin=[f[0] for f in counts.values() if len(f)==1];current=outside+[[200001,*f] for f in skin];ids=sorted(set(n for c in current for n in c));lookup={n:i for i,n in enumerate(ids)};arr=np.array([[lookup[n] for n in c] for c in current]);points=np.array([p[n] for n in ids]);edges=sorted({tuple(sorted((c[i],c[j]))) for c in current for i,j in pairs})
def assess(v):
 x,z,s,ex,ez=v;points[lookup[A]]=[x,0,z];points[lookup[B]]=[0,x,z];points[lookup[200000]]=[0,0,s];points[lookup[200001]]=[ex,ex,ez];t=points[arr];e=t[:,1:,:]-t[:,:1,:];det=np.einsum('ij,ij->i',e[:,0,:],np.cross(e[:,1,:],e[:,2,:]));center=t[:,:,2].mean(axis=1);u=np.clip((center-.27)/.03,0,1);w=1-u*u*(3-2*u);tan=1.5**w;n=np.maximum(.1,2*(.1+.2*center)/2.2)**(-2*w)
 d=np.array([t[:,j,:]-t[:,i,:] for i,j in pairs]);sq=(1+tan)/2*(d[:,:,0]**2+d[:,:,1]**2)+(1-tan)*d[:,:,0]*d[:,:,1]+n*d[:,:,2]**2;vol=det*np.sqrt(tan*n);q=12*(np.maximum(vol,0)/2)**(2/3)/sq.sum(axis=0);jac=np.min([np.sqrt(2)*vol/np.sqrt(np.prod(sq[[pi[tuple(sorted((i,k)))] for k in range(4) if k!=i]],axis=0)) for i in range(4)],axis=0)
 # Penalty uses centroid shapes only; independent full-edge check follows search.
 return float(q.min()),float(jac.min()),float(det.min())
edge_indices=np.array([[lookup[a],lookup[b]] for a,b in edges]);lx,lw=np.polynomial.legendre.leggauss(12)
def fast_max():
 a=points[edge_indices[:,0]];d=points[edge_indices[:,1]]-a;dz=d[:,2];safe=np.where(dz!=0,dz,1);cuts=np.sort(np.column_stack([np.zeros(len(a)),np.ones(len(a))]+[np.clip((z-a[:,2])/safe,0,1) for z in [.05,.27,.3]]),axis=1);lo=cuts[:,:-1];hi=cuts[:,1:];t=(lo+hi)[:,:,None]/2+(hi-lo)[:,:,None]/2*lx;z=a[:,2,None,None]+dz[:,None,None]*t;u=np.clip((z-.27)/.03,0,1);w=1-u*u*(3-2*u);tan=1.5**w;normal=np.maximum(.1,2*(.1+.2*z)/2.2)**(-2*w);speed=np.sqrt((1+tan)/2*(d[:,0,None,None]**2+d[:,1,None,None]**2)+(1-tan)*d[:,0,None,None]*d[:,1,None,None]+normal*dz[:,None,None]**2);return float(np.max(np.sum((hi-lo)/2*np.sum(speed*lw,axis=2),axis=1)))
def score(v):
 q,j,det=assess(v);return -min(q/.205,j/.051,1.795/fast_max()) if det>0 else 100-det
r=differential_evolution(score,[(.19,.35),(.12,.2),(.14,.21),(.08,.2),(.04,.18)],seed=12,popsize=8,maxiter=70,tol=1e-7,workers=1,polish=True)
q,j,det=assess(r.x);lengths=sorted([(length(points[lookup[a]],points[lookup[b]]),[a,b]) for a,b in edges],reverse=True)
report={'proposal':r.x.tolist(),'q':q,'J':j,'min_det':det,'objective':r.fun,'evaluations':r.nfev,'max_length':lengths[0], 'long_edges':[v for v in lengths if v[0]>1.8], 'note':'Floating diagnostic proposal only; native geometry, overlap, full field and first-height validation still required.'}
Path('/tmp/native_cold_offedge_constrained.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
