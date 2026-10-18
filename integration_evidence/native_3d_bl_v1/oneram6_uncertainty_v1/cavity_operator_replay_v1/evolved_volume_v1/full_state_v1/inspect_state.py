#!/usr/bin/env python3
"""Validate exact terminal records and export a compact rejected SU2 grid; no CFD run."""
import gzip,hashlib,json,shlex,sys
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent
NODE=np.dtype([('id','<u8'),('x','<f8',(3,))]);BIND=np.dtype([('present','u1'),('facet','<u8')])
CELL=np.dtype([('id','<u8'),('version','<u8'),('nodes',NODE,(4,)),('physical',BIND,(4,))]);assert CELL.itemsize==180

def digest(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(2**20),b''):h.update(b)
 return h.hexdigest()

def main():
 assert sys.byteorder=='little', 'This local diagnostic snapshot uses native little-endian binary64'
 proof=HERE/'state_summary.json'
 if proof.exists():
  summary=json.loads(proof.read_text());assert summary['helper_sha256']==digest(Path(__file__))
  assert summary['grid_sha256']==digest(HERE/'m6_rejected.su2.gz')
  for row in summary['owned_pieces']:assert row['compressed_sha256']==digest(HERE/f"rejected_working_rank{row['rank']}.bin.gz")
  print('REUSE',json.dumps(summary,indent=2));return
 pieces=[];sources=[]
 for rank in range(4):
  p=HERE/f'rejected_working_rank{rank}.bin';compressed=p.with_suffix('.bin.gz')
  opener=p.open if p.exists() else lambda _:gzip.open(compressed,'rb')
  with opener('rb') as f:
   assert f.read(8)==b'NATOWN1\0';count=int.from_bytes(f.read(8),'little');assert 0<=count<=2000000
   body=f.read();assert len(body)==180*count
   a=np.frombuffer(body,dtype=CELL).copy();pieces.append(a)
  sources.append({'rank':rank,'cells':count,'raw_sha256':digest(p) if p.exists() else None})
 data=np.concatenate(pieces);del pieces
 assert len(np.unique(data['id']))==len(data) and np.isfinite(data['nodes']['x']).all()
 assert np.all(data['physical']['present']<=1)
 nodes=data['nodes'].reshape(-1);order=nodes['id'].argsort(kind='stable');ids=nodes['id'][order];xyz=nodes['x'][order];same=ids[1:]==ids[:-1]
 assert not np.any(same & np.any(xyz[1:]!=xyz[:-1],axis=1)), 'Coordinate conflict across owners/versions'
 keep=np.r_[True,~same];global_ids=ids[keep];points=xyz[keep];del nodes,order,ids,xyz,same,keep
 conn=np.searchsorted(global_ids,data['nodes']['id']);assert np.array_equal(global_ids[conn],data['nodes']['id'])
 names=[shlex.split(line)[0] for line in (HERE/'marker_names.dat').read_text().splitlines()];assert names
 facets=np.loadtxt(HERE/'facet_markers.dat',dtype=np.uint64,ndmin=2);assert facets.shape[1]==2
 sort=facets[:,0].argsort();facets=facets[sort];assert len(np.unique(facets[:,0]))==len(facets)
 surface=[];face_slots=np.array([[1,2,3],[0,3,2],[0,1,3],[0,2,1]])
 for slot in range(4):
  selected=np.flatnonzero(data['physical']['present'][:,slot]);refs=data['physical']['facet'][selected,slot];where=np.searchsorted(facets[:,0],refs)
  assert np.all(where<len(facets)) and np.array_equal(facets[where,0],refs)
  marker=facets[where,1];assert np.all(marker<len(names));surface.append((conn[selected[:,None],face_slots[slot]],marker))
 with gzip.open(HERE/'m6_rejected.su2.gz','wt',compresslevel=1) as f:
  f.write(f'NDIME= 3\nNELEM= {len(conn)}\n');np.savetxt(f,np.c_[np.full(len(conn),10),conn],fmt='%d')
  f.write(f'NPOIN= {len(points)}\n');np.savetxt(f,np.c_[points,np.arange(len(points))],fmt=['%.17g','%.17g','%.17g','%d'])
  f.write(f'NMARK= {len(names)}\n')
  for index,name in enumerate(names):
   triangles=np.concatenate([tri[m==index] for tri,m in surface]);f.write(f'MARKER_TAG= {name}\nMARKER_ELEMS= {len(triangles)}\n');np.savetxt(f,np.c_[np.full(len(triangles),5),triangles],fmt='%d')
 with gzip.open(HERE/'m6_rejected_global_node_ids.npy.gz','wb',compresslevel=1) as f:np.save(f,global_ids,allow_pickle=False)
 states=[json.loads((HERE/f'result_rank{r}.json').read_text()) for r in range(4)]
 assert all(not s['accepted'] and s['atomic_input'] and s['CFD_steps']==0 for s in states)
 # Extract an EXACT complete current star union for the original blocked feature, if still present.
 edge=np.all(np.any(data['nodes']['id'][:,:,None]==np.array([47882,55976]),axis=1),axis=1)
 vertex=np.any(data['nodes']['id']==47883,axis=1);chosen=np.flatnonzero(edge|vertex)
 if len(chosen):
  p=HERE/'current_blocker_star.dat'
  with p.open('w') as f:
   physical=sum(int(data['physical']['present'][i].sum()) for i in chosen);f.write(f'NATIVE_PRIVATE_PATCH_V1 {len(chosen)} {physical}\n')
   for i in chosen:
    f.write(str(int(data['id'][i]))+'\n')
    for n in data['nodes'][i]:f.write(str(int(n['id']))+' '+' '.join(format(float(x),'.17g') for x in n['x'])+'\n')
   for i in chosen:
    for slot in range(4):
     if data['physical']['present'][i,slot]:f.write(' '.join(str(int(x)) for x in data['nodes']['id'][i,face_slots[slot]])+' '+str(int(data['physical']['facet'][i,slot]))+'\n')
 summary={'scope':'Complete REJECTED private working mesh; identity/coordinate screen is not full geometry/quality/field acceptance',
          'cells':len(data),'points':len(points),'owned_pieces':sources,'rank_results':states,'marker_names':names,
          'active_original_edge_cells':int(edge.sum()),'active_original_blocker_cells':int(vertex.sum()),'complete_current_star_union_cells':len(chosen),
          'grid_sha256':digest(HERE/'m6_rejected.su2.gz'),'helper_sha256':digest(Path(__file__))}
 # Preserve exact owner records losslessly and remove only this helper's uncompressed duplicate.
 for rank in range(4):
  p=HERE/f'rejected_working_rank{rank}.bin'
  if p.exists():
   q=p.with_suffix('.bin.gz');expected=digest(p)
   with p.open('rb') as a,gzip.open(q,'wb',compresslevel=1) as b:
    for block in iter(lambda:a.read(2**20),b''):b.write(block)
   h=hashlib.sha256()
   with gzip.open(q,'rb') as f:
    for block in iter(lambda:f.read(2**20),b''):h.update(block)
   assert h.hexdigest()==expected;p.unlink();summary['owned_pieces'][rank]['compressed_sha256']=digest(q)
 (HERE/'state_summary.json').write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps(summary,indent=2))
if __name__=='__main__':main()
