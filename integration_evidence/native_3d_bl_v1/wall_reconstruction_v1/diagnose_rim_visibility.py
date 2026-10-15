#!/usr/bin/env python3
"""Geometry-only wall-foot fit and directed visibility closure on the actual rim failure."""
import collections,hashlib,json,os,sys
os.environ['OPENBLAS_NUM_THREADS']='1'
from pathlib import Path
import numpy as np
from diagnose_wall_planes import fit,faces,volume
HERE=Path(__file__).resolve().parent

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 stage=sys.argv[1] if len(sys.argv)>1 else 'curved_rim_refit_probe';folder=HERE/stage;rp=HERE/(stage+'.json');receipt=json.loads(rp.read_text());assert receipt['status']=='FAIL'
 for name in ('audited_grid.json','wall_refit_original.json','native.log'):assert sha(folder/name)==receipt['artifacts'][name]
 original=json.loads((folder/'wall_refit_original.json').read_text());full=json.loads((folder/'audited_grid.json').read_text())
 import re
 edge=tuple(map(int,re.search(r'EDGE_REPLAY edge=(\d+),(\d+)',(folder/'native.log').read_text()).groups()))
 nodes={r[0]:np.array(r[1:]) for r in full['nodes']};point=(nodes[edge[0]]+nodes[edge[1]])/2;point[:2]/=np.linalg.norm(point[:2]);pid=max(nodes)+1;nodes[pid]=point
 cells={c['id']:c['nodes'] for c in full['cells']};selected={c['id'] for c in original['cells']};next_id=max(cells)+1
 for cid,c in list(cells.items()):
  if set(edge)<=set(c):
   assert cid in selected
   cells[cid]=[pid if n==edge[0] else n for n in c];cells[next_id]=[pid if n==edge[1] else n for n in c];selected.add(next_id);next_id+=1
 wall=[];apices=set()
 for f in original['surface']:
  if f['facet']!=8:continue
  triangles=[[pid if n==end else n for n in f['nodes']] for end in edge] if set(edge)<=set(f['nodes']) else [f['nodes']]
  for tri in triangles:
   owner=[c for cid,c in cells.items() if cid in selected and set(tri)<=set(c)];assert len(owner)==1
   apices.add(next(n for n in owner[0] if n not in tri));wall.append([nodes[n] for n in tri])
 assert len(apices)==1;apex=next(iter(apices));fitted,diagnosis=fit(nodes[apex],wall,.05);assert fitted is not None
 initial_minimum=min(volume(cells[i],dict(nodes,**{})) for i in selected)
 moved=dict(nodes);moved[apex]=fitted;after_fit_minimum=min(volume(cells[i],moved) for i in selected)
 adjacency=collections.defaultdict(set)
 for cid,c in cells.items():
  for face in faces(c):adjacency[tuple(sorted(face))].add(cid)
 history=[];boundary=[]
 for _ in range(64):
  incidence=collections.Counter(tuple(sorted(face)) for cid in selected for face in faces(cells[cid]))
  boundary=[face for cid in selected for face in faces(cells[cid]) if incidence[tuple(sorted(face))]==1]
  blocked=next((face for face in boundary if apex not in face and volume([apex,*face],moved)<=0),None)
  if blocked is None:break
  neighbors=adjacency[tuple(sorted(blocked))]-selected
  if not neighbors:raise AssertionError(('physical/outside obstruction',blocked))
  assert len(neighbors)==1;neighbor=next(iter(neighbors));selected.add(neighbor);history.append(dict(face=blocked,admitted_cell=neighbor,cells=len(selected)))
 else:raise AssertionError('visibility closure exceeded64 steps')
 assert len(selected)<=64
 boundary_nodes={n for f in boundary for n in f};retired={n for i in selected for n in cells[i]}-boundary_nodes-{apex}
 coned=[[apex,*f] for f in boundary if apex not in f];minimum=min(volume(c,moved) for c in coned);assert minimum>0
 for point_id in sorted(retired):
  selected_children=[]
  for i,c in enumerate(coned):
   total=volume(c,moved);weights=[]
   for k in range(4):
    child=c[:];child[k]=point_id;weights.append(volume(child,moved)/total)
   if min(weights)>=-1e-12:selected_children.append(i)
  assert selected_children
  counts=collections.Counter(tuple(sorted(f)) for i in selected_children for f in faces(coned[i]))
  skin=[f for i in selected_children for f in faces(coned[i]) if counts[tuple(sorted(f))]==1]
  inserted=[[point_id,*f] for f in skin];assert min(volume(c,moved) for c in inserted)>0
  coned=[c for i,c in enumerate(coned) if i not in selected_children]+inserted
 minimum=min(volume(c,moved) for c in coned);assert minimum>0
 checked_walls=[]
 for f in full['surface']:
  triangles=[[pid if n==end else n for n in f['nodes']] for end in edge] if set(edge)<=set(f['nodes']) else [f['nodes']]
  if f['facet']!=8:continue
  for face in triangles:
   owners=[c for c in coned if set(face)<=set(c)]
   if not owners:continue
   assert len(owners)==1;owner=next(n for n in owners[0] if n not in face);a,b,c=[moved[n] for n in face]
   normal=np.cross(b-a,c-a);normal/=np.linalg.norm(normal);altitude=-float(normal@(moved[owner]-a))
   uv=np.linalg.lstsq(np.array([b-a,c-a]).T,moved[owner]+.05*normal-a,rcond=None)[0]
   bary=[1-sum(uv),*uv];assert abs(altitude/.05-1)<1e-10 and min(bary)>-1e-10
   checked_walls.append(dict(face=face,apex=owner,height=altitude,minimum_barycentric=float(min(bary))))
 assert checked_walls
 report=dict(status='DOUBLE_GEOMETRY_VISIBILITY_WITNESS_ONLY',edge=edge,apex=apex,fit=diagnosis,fitted_point=fitted.tolist(),initial_split_cells=len(original['cells'])+2,
  initial_minimum_volume=initial_minimum,retained_fit_minimum_volume=after_fit_minimum,closed_cells=len(selected),new_cone_cells=len(coned),minimum_cone_volume=minimum,history=history,reinserted_points=list(retired),wall_checks=checked_walls,
  source_receipt_sha256=sha(rp),input_sha256={name:sha(folder/name) for name in ('audited_grid.json','wall_refit_original.json','native.log')},helper_sha256=sha(Path(__file__)),fit_helper_sha256=sha(HERE/'diagnose_wall_planes.py'),
  scope='Actual unit-cylinder rim fixture, projected geometric midpoint and double precision only. Height/foot feasibility plus specific face visibility closure without interior retirement. Not native/exact geometry/quality/full-edge acceptance, MPI, CFD or performance qualification.')
 (HERE/('rim_fit_visibility.json' if len(sys.argv)==1 else stage+'_visibility_replay.json')).write_text(json.dumps(report,indent=2)+'\n');print('foot_fit',fitted.tolist(),'retained_min_volume',after_fit_minimum,'closure',len(original['cells'])+2,'->',len(selected),'cone_cells',len(coned),'new_min_volume',minimum)
if __name__=='__main__':main()
