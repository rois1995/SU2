import importlib.util,json,hashlib,sys,time
from pathlib import Path
import numpy as np
root=Path(__file__).resolve().parents[3]
helper=root/'integration_evidence/native_3d_bl_v1/affine_initial_adoption_v1/source_policy_replay_v1/run.py'
spec=importlib.util.spec_from_file_location('source_policy',helper);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
sidecar,receipts,destination=map(Path,sys.argv[1:]);destination.mkdir(exist_ok=True)
lines=sidecar.read_text().splitlines();assert lines[0]=='SU2_NATIVE_REFERENCE_3D 7'
header=list(map(int,lines[1].split()));markers,count=header[:2]
tri=[];ids=[]
for line in lines[2+markers:2+markers+count]:
 row=line.split();ids.append(int(row[0]));tri.append([[float(x) for x in row[3+4*k:6+4*k]] for k in range(3)])
assert len(set(ids))==count
tri=np.array(tri,dtype=np.longdouble);u=tri[:,1]-tri[:,0];v=tri[:,2]-tri[:,0]
g=dict(tri=tri,u=u,v=v,uu=np.sum(u*u,axis=1),uv=np.sum(u*v,axis=1),vv=np.sum(v*v,axis=1));g['gram']=g['uu']*g['vv']-g['uv']**2
assert np.all(g['gram']>0)
normal=np.cross(u,v);normal/=np.sqrt(np.sum(normal*normal,axis=1))[:,None];lookup={id:i for i,id in enumerate(ids)}
module.checks();distances=[];signed=[];worst=[];start=time.monotonic()
with (destination/'parent_gaps.jsonl').open('w') as out:
 for line in receipts.open():
  row=json.loads(line);i=lookup[row['facet']]
  for sample in row['outside']:
   if time.monotonic()-start>120:raise RuntimeError('Gap diagnostic work limit exceeded')
   p=np.array(sample['point'],dtype=np.longdouble);square,w,foot=module.nearest_triangle(g,i,p)
   distance=float(np.sqrt(square));side=float(np.dot(p-tri[i,0],normal[i]));distances.append(distance);signed.append(side)
   record=dict(facet=row['facet'],marker=row['marker'],sample=sample['sample'],parent_distance=distance,parent_signed_plane=side,point=sample['point'],foot=list(map(float,foot)))
   out.write(json.dumps(record)+'\n');worst.append(record)
worst=sorted(worst,key=lambda r:r['parent_distance'],reverse=True)[:10]
summary=dict(samples=len(distances),parent_distance_quantiles=dict(zip(['0','50','90','99','100'],map(float,np.percentile(distances,[0,50,90,99,100])))),signed_plane_negative=sum(x<0 for x in signed),signed_plane_positive=sum(x>0 for x in signed),worst=worst,scope='Parent triangle distance is an upper bound on nearest original boundary distance; numerical diagnostic, not a coverage certificate.',seconds=time.monotonic()-start,pins={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in (sidecar,receipts,helper,Path(__file__))})
(destination/'parent_gap_summary.json').write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps({k:v for k,v in summary.items() if k not in ('pins','worst')}))
