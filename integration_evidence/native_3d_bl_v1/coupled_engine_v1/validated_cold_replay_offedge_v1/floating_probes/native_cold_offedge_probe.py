exec(open('/tmp/native_cold_joint_probe.py').read().split('for reconnect in')[0])
from collections import defaultdict

def orient(c,p):
 t=[p[n] for n in c];return float(np.dot(t[1]-t[0],np.cross(t[2]-t[0],t[3]-t[0])))
def faces(c):
 a,b,c,d=c;return [(b,c,d),(a,d,c),(a,b,d),(a,c,b)]
reports=[]
for reconnect in [False,True]:
 source=[list(c) for c in old]
 if reconnect:source=[c for c in source if not (0 in c and 4 in c)]+[[0,4,A,B],[0,A,B,15],[4,A,B,15]]
 for x,z,split in [(.263738891974,.174635351379,.195844099193),(.269,.174,.195),(.271,.1615,.1687335)]:
  p=dict(base);p[A]=np.array([x,0,z]);p[B]=np.array([0,x,z]);p[200000]=np.array([0,0,split])
  cells=[]
  for c in source:
   if 0 in c and 4 in c:cells += [[200000 if n==0 else n for n in c],[200000 if n==4 else n for n in c]]
   else:cells.append(c)
  for c in cells:
   if orient(c,p)<0:c[0],c[1]=c[1],c[0]
  star=[c for c in cells if 0 in c and 15 in c];outside=[c for c in cells if c not in star];counts=defaultdict(list)
  for c in star:
   for f in faces(c):counts[tuple(sorted(f))].append(f)
  skin=[f[0] for f in counts.values() if len(f)==1]
  for position in [extra,[extra[0],extra[1],.04],[.1,.1,.04],[.1,.1,.07],[.1,.1,.12],[.12,.12,.1]]:
   p[200001]=np.array(position);children=[[200001,*f] for f in skin]
   if any(orient(c,p)<=0 for c in children):reports.append({'reconnect':reconnect,'surface':[x,z],'point':list(position),'visible':False});continue
   current=outside+children;ids=sorted(set(n for c in current for n in c));lookup={n:i for i,n in enumerate(ids)};t=np.array([[p[n] for n in c] for c in current]);e=t[:,1:,:]-t[:,:1,:];det=np.einsum('ij,ij->i',e[:,0,:],np.cross(e[:,1,:],e[:,2,:]));center=t[:,:,2].mean(axis=1);u=np.clip((center-.27)/.03,0,1);w=1-u*u*(3-2*u);tan=1.5**w;n=np.maximum(.1,2*(.1+.2*center)/2.2)**(-2*w)
   d=np.array([t[:,j,:]-t[:,i,:] for i,j in pairs]);sq=(1+tan)/2*(d[:,:,0]**2+d[:,:,1]**2)+(1-tan)*d[:,:,0]*d[:,:,1]+n*d[:,:,2]**2;vol=det*np.sqrt(tan*n);q=12*(vol/2)**(2/3)/sq.sum(axis=0);jac=np.min([np.sqrt(2)*vol/np.sqrt(np.prod(sq[[pi[tuple(sorted((i,k)))] for k in range(4) if k!=i]],axis=0)) for i in range(4)],axis=0)
   record={'reconnect':reconnect,'surface':[x,z],'point':list(position),'visible':True,'q':float(q.min()),'J':float(jac.min()),'weakest':current[int(np.argmin(np.minimum(q/.2,jac/.05)))]};reports.append(record);print(record,flush=True)
Path('/tmp/native_cold_offedge_probe.json').write_text(json.dumps(reports,indent=2)+'\n')
