#!/usr/bin/env python3
"""Constrained rounding evidence for the captured midpoint; no new native operator or CFD run."""
import itertools,json,math,sys
from fractions import Fraction as F
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE))
from audit_segment import determinant,weights,interval,gaps,read_su2
mesh=read_su2(HERE/'Coarse.su2');packet=json.loads((HERE/'exact_segment_audit.json').read_text())
a,rounded=[tuple(x) for x in packet['edge']];ids=np.flatnonzero(np.all(mesh.P==a,axis=1));assert len(ids)==1;first=int(ids[0])
star=np.flatnonzero(np.any(mesh.E==first,axis=1));parents=[]
for n in np.unique(mesh.E[star]):
 exact=[(F(x)+F(float(y)))/2 for x,y in zip(a,mesh.P[n])]
 if tuple(float(v) for v in exact)==rounded:parents.append((int(n),exact))
assert len(parents)==1;second,exact=parents[0]
bounds=[]
for x in exact:
 near=float(x);low=near if F(near)<=x else np.nextafter(near,-math.inf);high=near if F(near)>=x else np.nextafter(near,math.inf)
 assert F(low)<=x<=F(high);assert low==high or np.nextafter(low,math.inf)==high
 bounds.append(sorted(set((low,high))))
# The original complete edge star is the local constructor's source cavity.
rows=np.flatnonzero(np.any(mesh.E==first,axis=1)&np.any(mesh.E==second,axis=1));assert len(rows)>0
original=[]
for i in rows:
 nodes=list(map(int,mesh.E[i]));tet=[tuple(F(float(x)) for x in mesh.P[n]) for n in nodes]
 if determinant(tet)<0:nodes[0],nodes[1]=nodes[1],nodes[0];tet[0],tet[1]=tet[1],tet[0]
 original.append((int(i),nodes,tet))
results=[]
for candidate in itertools.product(*bounds):
 point=tuple(F(x) for x in candidate);inside=[i for i,n,t in original if min(weights(t,point))>=0]
 children=[];positive=True
 for i,nodes,tet in original:
  for parent in (first,second):
   child=list(tet);child[nodes.index(parent)]=point;positive=positive and determinant(child)>0;children.append(child)
 edge_gaps=[]
 for child in children:
  for u,v in itertools.combinations(child,2):
   if u==v:continue
   covered=[]
   for _,_,tet in original:
    cut=interval(weights(tet,u),weights(tet,v))
    if cut is not None:covered.append(cut)
   missing=gaps(covered)
   if missing:edge_gaps.append([[str(x),str(y)] for x,y in missing])
 old_volume=sum(determinant(t) for _,_,t in original)/6
 new_volume=sum(determinant(t) for t in children)/6
 results.append({'point':candidate,'nearest_binary64':candidate==rounded,'source_cells':inside,'children_positive':positive,
   'all_child_edges_covered':not edge_gaps,'child_edge_gaps':edge_gaps,'relative_volume_change':str((new_volume-old_volume)/old_volume),
   'coordinate_errors':[str(F(c)-e) for c,e in zip(candidate,exact)]})
result={'parent_nodes':[first,second],'original_edge_star_cells':[i for i,_,_ in original],
 'exact_midpoint':[str(v) for v in exact],'rounding_bounds':bounds,'corners':results,
 'scope':'Every corner of the exact componentwise nearest binary64 rounding box. Exact positive child volumes and literal source-edge coverage on the original local edge star only. No boundary-authority, full metric, first-height, whole M6 construction or transfer acceptance claim.'}
(HERE/'rounding_experiment.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
