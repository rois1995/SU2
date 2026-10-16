#!/usr/bin/env python3
"""Extract the planar cap of the supplied one-layer CGNS grid; validate and triangulate only test input."""
import hashlib,json,os,subprocess,sys
from collections import Counter
from pathlib import Path
import numpy as np
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
SOURCE=Path('/media/rausa/4TB/Validation/ThreeElementAirfoil/30P30N/Meshes/2D_L1_coarse_r1.cgns')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 env=dict(os.environ,TMPDIR=str(ROOT/'integration_evidence/build-integrated-v2/compile-tmp'))
 command=['mpicc','-I'+str(ROOT/'externals/cgns'),'-I'+str(ROOT/'integration_evidence/build-integrated-v2/externals/cgns'),str(HERE/'inventory.c'),str(ROOT/'integration_evidence/build-integrated-v2/externals/cgns/libcgns.a'),str(ROOT/'integration_evidence/build-integrated-v2/externals/cgns/hdf5/libsu2hdf5.a'),'-ldl','-lm','-lz','-pthread','-o',str(HERE/'inventory')]
 with (HERE/'build.log').open('w') as log:subprocess.run(command,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
 raw=HERE/'source_cap_data.bin'
 result=subprocess.run([str(HERE/'inventory'),str(SOURCE),str(raw)],capture_output=True,text=True,check=True)
 with raw.open('rb') as f:
  assert f.read(8)==b'NATCGNS1'
  n,ns=np.fromfile(f,dtype=np.uint64,count=2)
  points=np.fromfile(f,dtype=np.float64,count=3*int(n)).reshape(3,int(n)).T
  sections=[]
  for _ in range(ns):
   typ,first,count=np.fromfile(f,dtype=np.uint64,count=3)
   conn=np.fromfile(f,dtype=np.uint64,count=4*int(count)).reshape(int(count),4).astype(np.int64)-1
   assert np.all(conn>=0) and np.all(conn<n)
   sections.append((int(first),conn))
  assert not f.read(1)
 assert np.all(np.isfinite(points))
 # Cap sections contain a whole planar mesh, whereas extrusion-side sections are ribbons.
 candidates=[]
 for i,(_,q) in enumerate(sections):
  if not len(q):continue
  ids=np.unique(q);p=points[ids];width=np.ptp(p,axis=0)
  for axis in np.flatnonzero(width==0):candidates.append((len(q),i,int(axis),float(p[0,axis])))
 assert len(candidates)==2 and candidates[0][0]==candidates[1][0]
 _,cap,axis,plane=min(candidates,key=lambda x:x[3])
 _,other,other_axis,other_plane=max(candidates,key=lambda x:x[3])
 assert axis==other_axis and plane<other_plane
 ids=np.unique(sections[cap][1]);other_ids=np.unique(sections[other][1]);assert len(ids)+len(other_ids)==n
 dims=[d for d in range(3) if d!=axis]
 P=points[ids][:,dims];op=points[other_ids][:,dims]
 # The selected source cap is authoritative; do not snap it to the opposite extrusion plane.
 cap_pair_difference=np.max(np.abs(np.array(sorted(map(tuple,P)))-np.array(sorted(map(tuple,op)))),axis=0)
 lookup=np.full(int(n),-1,dtype=np.int64);lookup[ids]=np.arange(len(ids))
 Q=lookup[sections[cap][1]];assert np.all(Q>=0)
 L=P.astype(np.longdouble)
 def area(a,b,c):return np.cross(L[b]-L[a],L[c]-L[a])
 def quality(tri):
  a,b,c=tri;den=sum(np.sum((L[i]-L[j])**2) for i,j in [(a,b),(b,c),(c,a)])
  return abs(area(a,b,c))/den
 triangles=[];quad_area=0
 for a,b,c,d in Q:
  signed=area(a,b,c)+area(a,c,d);assert signed!=0
  choices=[[(a,b,c),(a,c,d)],[(a,b,d),(b,c,d)]]
  valid=[ts for ts in choices if all(area(*t)*signed>0 for t in ts)];assert valid
  ts=max(valid,key=lambda ts:min(map(quality,ts)))
  for x,y,z in ts:triangles.append((x,y,z) if area(x,y,z)>0 else (x,z,y))
  quad_area+=abs(signed)/2
 triangles=np.array(triangles,dtype=np.int64)
 edges=Counter(tuple(sorted((int(t[i]),int(t[(i+1)%3])))) for t in triangles for i in range(3))
 assert all(v in (1,2) for v in edges.values())
 boundaries={e for e,count in edges.items() if count==1}
 bocos={}
 for line in result.stdout.splitlines():
  fields=line.split('\t')
  if fields[0]=='boundary':bocos.setdefault(int(fields[3]),[]).append(fields[1])
 markers={};boundary_source=set();marker_extents={}
 for i,(_,q) in enumerate(sections):
  if i in (cap,other) or not len(q):continue
  names=bocos.get(len(q),[]);assert len(names)==1
  name=names[0];segments=[]
  for row in lookup[q]:
   edge=row[row>=0];assert len(edge)==2
   key=tuple(sorted(map(int,edge)));assert key in boundaries and key not in boundary_source
   boundary_source.add(key);segments.append(tuple(map(int,edge)))
  markers[name]=segments
  p=P[np.unique(segments)];marker_extents[name]={'segments':len(segments),'min':p.min(axis=0).tolist(),'max':p.max(axis=0).tolist()}
 assert boundaries==boundary_source and len(markers)==4
 degrees=Counter(x for e in boundaries for x in e);assert all(n==2 for n in degrees.values())
 assert set(triangles.flat)==set(range(len(P)))
 triangle_area=sum(area(*t)/2 for t in triangles)
 assert abs(triangle_area-quad_area)<=1e-12*quad_area
 assert len(P)-len(edges)+len(triangles)==-2 # exterior plus three airfoil holes
 mesh=HERE/'30P30N_coarse_tri.su2'
 with mesh.open('w') as f:
  f.write('NDIME= 2\nNELEM= %d\n'%len(triangles))
  for i,t in enumerate(triangles):f.write('5 %d %d %d %d\n'%(*t,i))
  f.write('NPOIN= %d\n'%len(P))
  for i,p in enumerate(P):f.write('%.17g %.17g %d\n'%(*p,i))
  f.write('NMARK= %d\n'%len(markers))
  for name,es in markers.items():
   f.write('MARKER_TAG= %s\nMARKER_ELEMS= %d\n'%(name,len(es)))
   for e in es:f.write('3 %d %d\n'%e)
 report={'source':str(SOURCE),'source_sha256':sha(SOURCE),'mesh_sha256':sha(mesh),'helper_sha256':sha(Path(__file__)),
 'inspector_sha256':sha(HERE/'inventory.c'),'compile_command':command,'original_metadata':result.stdout,
 'axis_removed':axis,'plane':plane,'other_plane':other_plane,'opposite_cap_projection_difference':cap_pair_difference.tolist(),'vertices':len(P),'quadrilaterals':len(Q),'triangles':len(triangles),
 'boundary_edges':len(boundaries),'markers':marker_extents,'area_relative_error':float(abs(triangle_area-quad_area)/quad_area),
 'minimum_double_area':float(min(area(*t) for t in triangles)),'CFD_steps':0,'adaptation_performed':False}
 (HERE/'preparation.json').write_text(json.dumps(report,indent=2)+'\n');raw.unlink()
 print(json.dumps(report,indent=2))
if __name__=='__main__':main()
