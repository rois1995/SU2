exec(open('/tmp/native_cold_offedge_search.py').read().split('r=differential')[0])
def tensor(p):
 z=p[2];u=np.clip((z-.27)/.03,0,1);w=1-u*u*(3-2*u);t=1.5**w;n=max(.1,2*(.1+.2*z)/2.2)**(-2*w);return np.array([[(1+t)/2,(1-t)/2,0],[(1-t)/2,(1+t)/2,0],[0,0,n]])
for name,nid,neighborids in [('surface',A,{n for f in v['surface'] if A in f['nodes'] for n in f['nodes']}-{A}),('extra',200001,{n for c in star for n in c})]:
 origin=base.get(nid,extra);mats=[tensor((origin+base.get(n,points[lookup[n]]))/2) for n in neighborids];rhs=sum(m@base.get(n,points[lookup[n]]) for n,m in zip(neighborids,mats));mean=np.linalg.solve(sum(mats),rhs);print(name,'neighbors',len(neighborids),'origin',origin,'mean',mean,flush=True)
 if name=='surface':
  for damping in [1,3]:
   M=tensor(origin);q=np.linalg.solve(np.eye(3)+damping*M/np.max(abs(M)),mean+damping*M@origin/np.max(abs(M)));print('damp',damping,'half',(origin+q)/2)
