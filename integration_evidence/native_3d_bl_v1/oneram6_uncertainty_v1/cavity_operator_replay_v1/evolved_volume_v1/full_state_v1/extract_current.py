#!/usr/bin/env python3
"""Extract complete current edge/vertex dependencies from all exact terminal owner pieces."""
import gzip,json,sys
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[5];sys.path.insert(0,str(HERE));from inspect_state import CELL,digest
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'));from capcheck import read_su2

def main():
 pieces=[]
 for rank in range(4):
  with gzip.open(HERE/f'rejected_working_rank{rank}.bin.gz','rb') as f:
   assert f.read(8)==b'NATOWN1\0';count=int.from_bytes(f.read(8),'little');body=f.read();assert len(body)==180*count
   pieces.append(np.frombuffer(body,dtype=CELL).copy())
 data=np.concatenate(pieces);del pieces
 original=read_su2(HERE/'Coarse.su2');boxes=original.P[original.E];lo=boxes.min(1);hi=boxes.max(1);del boxes
 folder=HERE/'current_stars';folder.mkdir(exist_ok=True);results=[]
 slots=np.array([[1,2,3],[0,3,2],[0,1,3],[0,2,1]])
 def support(selected):
  xyz=data['nodes']['x'][selected].reshape(-1,3)
  return int(np.count_nonzero(np.all((lo<=xyz.max(0))&(hi>=xyz.min(0)),axis=1)))
 def save(selected,path):
  with path.open('w') as f:
   physical=int(data['physical']['present'][selected].sum());f.write(f'NATIVE_PRIVATE_PATCH_V1 {len(selected)} {physical}\n')
   for i in selected:
    f.write(str(int(data['id'][i]))+'\n')
    for n in data['nodes'][i]:f.write(str(int(n['id']))+' '+' '.join(format(float(x),'.17g') for x in n['x'])+'\n')
   for i in selected:
    for slot in range(4):
     if data['physical']['present'][i,slot]:f.write(' '.join(str(int(x)) for x in data['nodes']['id'][i,slots[slot]])+' '+str(int(data['physical']['facet'][i,slot]))+'\n')
 for rank in range(4):
  state=json.loads((HERE/f'result_rank{rank}.json').read_text());edge=state['last_feature'][:2];assert state['last_feature_count']==2
  mask=np.all(np.any(data['nodes']['id'][:,:,None]==np.array(edge),axis=1),axis=1);selected=np.flatnonzero(mask)
  row={'rank':rank,'edge':edge,'current_edge_cells':len(selected),'still_present':bool(len(selected)),'complete_global_snapshot':True,'vertices':[]}
  if len(selected):
   dest=folder/f'rank{rank}_edge.dat';save(selected,dest);row['edge_patch_sha256']=digest(dest);row['edge_bbox_donors']=support(selected);row['physical_faces']=int(data['physical']['present'][selected].sum())
   for n in np.unique(data['nodes']['id'][selected]):
    if n in edge:continue
    v=np.flatnonzero(np.any(data['nodes']['id']==n,axis=1));union=np.union1d(v,selected)
    incident=data['nodes']['id'][v]==n;physical=False
    for slot in range(4):physical=physical or bool(np.any(data['physical']['present'][v,slot] & ~incident[:,slot]))
    vertex={'id':int(n),'star_cells':len(v),'edge_union_cells':len(union),'physical':physical,'union_bbox_donors':support(union)}
    if not physical and len(union)<=64:
     dest=folder/f'rank{rank}_with_vertex{int(n)}.dat';save(union,dest);vertex['patch_sha256']=digest(dest)
    row['vertices'].append(vertex)
  results.append(row)
 report={'scope':'Exact complete current stars, not accepted meshes; present features still need actual frozen metric measurement',
         'snapshot_sha256':digest(HERE/'state_summary.json'),'helper_sha256':digest(Path(__file__)),
         'features':results}
 (folder/'summary.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
