"""Constrained wall-surface coarsening for the whole-region experiment, not a volume-repair fallback."""
import collections,math
import numpy as np
from construct import normal,project

def coarsen(nodes,faces,walls,geometry,marker,kernel,design):
 wall=[f for f in faces if f['facet'] in walls];tags=collections.defaultdict(set);facet_by_marker={}
 for f in faces:
  if f['facet'] not in walls:
   m=marker[f['facet']];facet_by_marker[m]=min(facet_by_marker.get(m,f['facet']),f['facet'])
   for n in f['nodes']:tags[n].add(m)
 initial=len(wall);collapses=0;attempts=0
 while attempts<4096:
  uses=collections.defaultdict(list);neighbors=collections.defaultdict(set)
  for i,f in enumerate(wall):
   a=f['nodes']
   for k in range(3):
    x,y=a[k],a[(k+1)%3];uses[tuple(sorted((x,y)))].append(i);neighbors[x].add(y);neighbors[y].add(x)
  order=sorted((design.length(*e,nodes),e) for e in uses if design.length(*e,nodes)<.9)
  changed=False
  for _,edge in order:
   for removed,retained in [edge,edge[::-1]]:
    if not tags[removed]<=tags[retained]:continue
    if tags[removed] and tags[retained] and len(uses[edge])!=1:continue
    opposite={n for i in uses[edge] for n in wall[i]['nodes'] if n not in edge}
    if neighbors[removed]&neighbors[retained]!=opposite:continue
    attempts+=1
    star=[i for i,f in enumerate(wall) if removed in f['nodes']];new=[];valid=True
    old_edges={tuple(sorted((f['nodes'][i],f['nodes'][(i+1)%3]))) for j in star for f in [wall[j]] for i in range(3)}
    for i in star:
     f=wall[i];ids=[retained if n==removed else n for n in f['nodes']]
     if len(set(ids))<3:continue
     p=np.array([nodes[n] for n in ids]);g=geometry[f['facet']];kind,origin,extent,_,axis=g
     center=p.mean(axis=0);foot=project(center,g);out=foot-origin
     if kind==3:axis=axis/np.linalg.norm(axis);out-=out@axis*axis
     if np.cross(p[1]-p[0],p[2]-p[0])@out<=0:valid=False;break
     if any(np.linalg.norm(project(.5*(p[k]+p[(k+1)%3]),g)-.5*(p[k]+p[(k+1)%3]))>min(.04,.025) for k in range(3)):valid=False;break
     for k in range(3):
      e=tuple(sorted((ids[k],ids[(k+1)%3])))
      if e not in old_edges and design.length(*e,nodes)>.95*1.8:valid=False;break
     if not valid:break
     apex=center-.05*normal(p);tet=np.array([[apex,*p]]);m=design.metric(tet.mean(axis=1));e=tet[:,None,:,:]-tet[:,:,None,:];sq=np.einsum('nabi,nij,nabj->nab',e,m,e);det=np.linalg.det(tet[:,1:]-tet[:,:1]);weighted=det*np.sqrt(np.linalg.det(m));ii,jj=np.triu_indices(4,1)
     q=12*(weighted[0]/2)**(2/3)/sq[:,ii,jj].sum();products=[np.prod(np.delete(sq[0,k],k)) for k in range(4)];jac=min(math.sqrt(2)*weighted[0]/np.sqrt(products))
     if q<.24 or jac<.06:valid=False;break
     new.append(dict(facet=f['facet'],nodes=ids))
    if not valid:continue
    removed_set=set(star);proposal=[f for i,f in enumerate(wall) if i not in removed_set]+new
    keys=[tuple(sorted(f['nodes'])) for f in proposal]
    if len(set(keys))!=len(keys):continue
    wall=proposal;collapses+=1;changed=True;break
   if changed or attempts>=4096:break
  if not changed:break
 # Rebuild nonwall caps from the new wall rims, preserving their declared geometry and marker.
 uses=collections.defaultdict(list)
 for f in wall:
  a=f['nodes']
  for k in range(3):uses[tuple(sorted((a[k],a[(k+1)%3])))].append((a[k],a[(k+1)%3]))
 rims=collections.defaultdict(list)
 for (a,b),directions in uses.items():
  assert len(directions) in (1,2)
  if len(directions)==1:
   shared=tags[a]&tags[b];assert len(shared)==1,'Wall rim lost its physical feature authority'
   x,y=directions[0];rims[next(iter(shared))].append((y,x))
 result=list(wall)
 for m,edges in rims.items():
  facet=facet_by_marker[m];center=max(nodes)+1;nodes[center]=project(kernel,geometry[facet])
  outgoing={a:b for a,b in edges};assert len(outgoing)==len(edges)
  at=min(outgoing);walk=[]
  for _ in edges:walk.append(at);at=outgoing[at]
  assert at==walk[0] and len(set(walk))==len(edges),'Surface cap rim is not one closed curve'
  result.extend(dict(facet=facet,nodes=[center,a,b]) for a,b in edges)
 return result,dict(initial_wall_faces=initial,final_wall_faces=len(wall),collapses=collapses,attempts=attempts)
