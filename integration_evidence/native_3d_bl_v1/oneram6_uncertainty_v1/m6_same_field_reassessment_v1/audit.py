#!/usr/bin/env python3
"""Independent recorded-field/partition screens; not full mesh acceptance or an exact integral."""
import gzip,hashlib,json,sys
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
sys.path.insert(0,str(ROOT/'TestCases/adaptation/capability'))
from capcheck import read_su2,volumes
NODE=np.dtype([('id','<u8'),('x','<f8',(3,))]);BIND=np.dtype([('present','u1'),('facet','<u8')])
CELL=np.dtype([('id','<u8'),('version','<u8'),('nodes',NODE,(4,)),('physical',BIND,(4,))]);assert CELL.itemsize==180
SLOTS=np.array([[1,2,3],[0,3,2],[0,1,3],[0,2,1]])
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for b in iter(lambda:f.read(2**20),b''):h.update(b)
 return h.hexdigest()
def receipts(folder):
 r=json.loads((folder/'receipt.json').read_text());assert r['compile']['exit']==r['link']['exit']==r['run']['exit']==0
 assert sha(folder/'run.log')==r['run']['log_sha256']
 for name,value in r['hashes'].items():assert sha(ROOT/name)==value,name
 for name,value in r['linked_archives'].items():assert sha(Path(name))==value,name
 for name,value in r['stage_hashes'].items():assert sha(folder/name)==value,name
 return r

def main():
 receipts(HERE);receipts(HERE/'wall_reference_v1')
 stages={}
 for name in ['coarse','fine']:
  rows=[json.loads((HERE/f'{name}_rank{rank}.json').read_text()) for rank in range(4)];first=rows[0]
  assert all(row['ok'] and row['estimated'] and row['immutable_source'] and row['samples']==8192 for row in rows)
  for key in ['value','uncertainty','centroid','point_SE','block_SE','profile_version']:
   assert all(row[key]==first[key] for row in rows),key
  assert first['uncertainty']==3*max(first['point_SE'],first['block_SE'])
  stages[name]={'value':first['value'],'heuristic_indicator':first['uncertainty'],'relative_indicator':first['uncertainty']/first['value'],
   'centroid':first['centroid'],'profile_version':first['profile_version'],'maximum_seconds':max(row['seconds'] for row in rows),'global_source_queries':sum(row['source_queries'] for row in rows),
   'maximum_modeled_bytes':max(row['maximum_native_bytes'] for row in rows),'maximum_tracked_bytes':max(row['tracked_bytes'] for row in rows)}
 ref=[json.loads((HERE/'wall_reference_v1'/f'reference_rank{rank}.json').read_text()) for rank in range(4)]
 for key in ['value','uncertainty','sensor_lower','sensor_upper','correction','profile_version']:
  assert all(row[key]==ref[0][key] for row in ref),key
 assert all(row['immutable_source'] and row['profile_version']==stages['coarse']['profile_version'] for row in ref)
 for name,stage in stages.items():
  stage['difference_from_independent_proposal']=stage['value']-ref[0]['value']
  stage['inside_sum_of_heuristic_indicators']=abs(stage['difference_from_independent_proposal'])<=stage['heuristic_indicator']+ref[0]['uncertainty']
 # Screen integration domains independently using long-double volume and face incidences.
 mesh=read_su2(HERE/'Coarse.su2');points=mesh.P.astype(np.longdouble);coarse=0.;coarse_min=float('inf')
 for begin in range(0,len(mesh.E),8192):
  v=volumes(points,mesh.E[begin:begin+8192],3);coarse+=np.sum(abs(v),dtype=np.longdouble);coarse_min=min(coarse_min,float(min(abs(v))))
 pieces=[];manifest=json.loads((HERE/'inputs.json').read_text())
 for row in manifest['pieces']:
  original=ROOT/row['compressed_source'];assert sha(original)==row['compressed_sha256']
  with gzip.open(original,'rb') as f:
   header=f.read(16);assert header[:8]==b'NATOWN1\0' and int.from_bytes(header[8:],'little')==row['cells'];body=f.read()
  h=hashlib.sha256(header+body).hexdigest();assert h==row['sha256'] and len(body)==180*row['cells']
  pieces.append(np.frombuffer(body,dtype=CELL).copy())
 data=np.concatenate(pieces);del pieces;fine=0.;minimum=float('inf');nonpositive=0
 for begin in range(0,len(data),8192):
  x=data['nodes']['x'][begin:begin+8192].astype(np.longdouble);u=x[:,1]-x[:,0];v=x[:,2]-x[:,0];w=x[:,3]-x[:,0];vol=np.einsum('ij,ij->i',u,np.cross(v,w))/6
  fine+=np.sum(vol,dtype=np.longdouble);minimum=min(minimum,float(min(vol)));nonpositive+=int(np.count_nonzero(vol<=0))
 ids=data['nodes']['id'];faces=np.vstack([ids[:,slot] for slot in SLOTS]);faces.sort(axis=1);unique,counts=np.unique(faces,axis=0,return_counts=True);del faces
 boundary=unique[counts==1];physical=np.vstack([ids[data['physical']['present'][:,k]!=0][:,slot] for k,slot in enumerate(SLOTS)]);physical.sort(axis=1)
 unique_physical=np.unique(physical,axis=0)
 assert np.max(counts)==2 and len(physical)==len(unique_physical) and np.array_equal(boundary,unique_physical),'Open/nonmanifold/unbound partition faces'
 relative=float(abs(fine-coarse)/coarse)
 assert nonpositive==0 and relative<2e-12 and len(np.unique(data['id']))==len(data),'Invalid integration partition screen'
 out={'scope':'Same original field; rejected finer partition integration only. Empirical uncertainty and finite domain screens, not exact complexity/full acceptance.',
 'stages':stages,'independent_wall_proposal':ref[0],'indicator_ratio_fine_over_coarse':stages['fine']['heuristic_indicator']/stages['coarse']['heuristic_indicator'],
 'reassessment_time_ratio':stages['fine']['maximum_seconds']/stages['coarse']['maximum_seconds'],
 'domain_screen':{'coarse_cells':len(mesh.E),'finer_cells':len(data),'coarse_absolute_volume':float(coarse),'finer_signed_volume':float(fine),'relative_volume_difference':relative,'finer_minimum_long_double_volume':minimum,'finer_nonpositive':nonpositive,'unique_cell_ids':True,'physical_boundary_faces':len(physical),'maximum_face_incidence':int(max(counts)),'all_open_faces_physically_bound':True,'scope':'No independent global no-overlap/metric/quality/transfer certificate.'}}
 (HERE/'assessment_comparison.json').write_text(json.dumps(out,indent=2,allow_nan=False)+'\n');print(json.dumps(out,indent=2,allow_nan=False))
if __name__=='__main__':main()
