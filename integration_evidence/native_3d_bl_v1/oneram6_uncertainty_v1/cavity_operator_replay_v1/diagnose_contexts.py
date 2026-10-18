#!/usr/bin/env python3
"""Read saved exact contexts; compare with original connectivity and export diagnostic grids."""
import hashlib,json,sys
from collections import Counter
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2

def patch(path):
 rows=path.read_text().splitlines();head=rows[0].split();assert head[0]=='NATIVE_PRIVATE_PATCH_V1'
 nc,nf=map(int,head[1:]);assert 0<=nc<=128 and 0<=nf<=512
 cells=[];nodes={};pos=1
 for _ in range(nc):
  cid=int(rows[pos]);pos+=1;v=[]
  for _ in range(4):
   r=rows[pos].split();pos+=1;n=int(r[0]);p=tuple(map(float,r[1:]));assert len(p)==3 and np.isfinite(p).all()
   assert n not in nodes or nodes[n]==p
   nodes[n]=p;v.append(n)
  cells.append((cid,tuple(v)))
 faces=[tuple(map(int,r.split())) for r in rows[pos:]];assert len(faces)==nf and all(len(r)==4 for r in faces)
 return cells,nodes,faces

def grid(path,cells,nodes):
 if not cells:return
 ids={n:i for i,n in enumerate(sorted(nodes))};skin=Counter()
 for _,v in cells:
  for face in [(v[1],v[2],v[3]),(v[0],v[3],v[2]),(v[0],v[1],v[3]),(v[0],v[2],v[1])]:skin[tuple(sorted(face))]+=1
 with path.open('w') as f:
  f.write(f'NDIME= 3\nNELEM= {len(cells)}\n')
  for _,v in cells:f.write('10 '+' '.join(str(ids[n]) for n in v)+'\n')
  f.write(f'NPOIN= {len(nodes)}\n')
  for n in sorted(nodes):f.write(' '.join(format(x,'.17g') for x in nodes[n])+f' {ids[n]}\n')
  boundary=[q for q,n in skin.items() if n==1]
  f.write(f'NMARK= 1\nMARKER_TAG= DIAGNOSTIC_CAVITY_SKIN\nMARKER_ELEMS= {len(boundary)}\n')
  for q in boundary:f.write('5 '+' '.join(str(ids[n]) for n in q)+'\n')
 # Viewing indices must never be used to address the original sensor field.
 path.with_suffix('.ids.json').write_text(json.dumps(sorted(nodes))+'\n')

def main():
 folder=HERE/'actual_context_np4';mesh=read_su2(folder/'Coarse.su2')
 original={tuple(sorted(map(int,row))) for row in mesh.E}
 results=[]
 for index in sorted(folder.glob('contexts_rank*.jsonl')):
  rows=[json.loads(s) for s in index.read_text().splitlines()]
  last={tuple([r['feature_count'],*r['feature']]):r for r in rows}
  for r in last.values():
   path=folder/(r['stem']+'_old.dat');cells,nodes,faces=patch(path)
   incident=[v for _,v in cells if r['feature_count']==2 and set(r['feature'][:2])<=set(v)]
   unchanged=sum(tuple(sorted(v)) in original and all(n<len(mesh.P) and tuple(mesh.P[n])==nodes[n] for n in v) for _,v in cells)
   coords=np.array(list(nodes.values()));lo=coords.min(0) if len(coords) else [];hi=coords.max(0) if len(coords) else []
   donors=0
   if len(coords):
    points=mesh.P[mesh.E];donors=int(np.count_nonzero(np.all((points.min(1)<=hi)&(points.max(1)>=lo),axis=1)))
   result=dict(r,old_cells=len(cells),old_nodes=len(nodes),physical_faces=len(faces),incident_cells=len(incident),unchanged_original_cells=unchanged,new_node_ids=sum(n>=len(mesh.P) for n in nodes),box_donor_candidates=donors,old_patch_sha256=hashlib.sha256(path.read_bytes()).hexdigest())
   results.append(result);grid(path.with_suffix('.su2'),cells,nodes)
   private=folder/(r['stem']+'_private.dat');pc,pn,pf=patch(private)
   grid(private.with_name(r['stem']+('_rejected.su2' if r['candidate_geometry'] else '_private_unverified.su2')),pc,pn)
 summary={'scope':'Actual private proposal observations, not complete rejected meshes; observer precedes final dependency/collision/publication gates',
          'contexts':results,'rank_results':[json.loads(p.read_text()) for p in sorted(folder.glob('result_rank*.json'))],
          'input_mesh_sha256':hashlib.sha256((folder/'Coarse.su2').read_bytes()).hexdigest(),
          'helper_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
 assert len(summary['rank_results'])==4 and all(r['atomic_input'] and r['retained']<=16 for r in summary['rank_results'])
 (HERE/'context_summary.json').write_text(json.dumps(summary,indent=2)+'\n')
 print(json.dumps(summary,indent=2))
if __name__=='__main__':main()
