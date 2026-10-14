#!/usr/bin/env python3
"""Construct and independently certify a control for the declared frozen cube field; not native output."""
import collections,hashlib,importlib.util,itertools,json,math
from fractions import Fraction as F
from pathlib import Path
HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('oracle',HERE/'audit_sensor.py');oracle=importlib.util.module_from_spec(spec);spec.loader.exec_module(oracle)
spec=importlib.util.spec_from_file_location('geometry',HERE/'audit_geometry.py');geo=importlib.util.module_from_spec(spec);spec.loader.exec_module(geo)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 sources=sorted(HERE.glob('sensor*/independent_finer_grouped_coupled_BL_rejected_n1.json'))
 assert sources;reference=sources[0];source=json.loads(reference.read_text())
 assert len(source['original'])==6
 planes={}
 for f in source['facets']:
  points=[p[1:] for p in f['vertices']];plane=[(k,points[0][k]) for k in range(3) if all(p[k]==points[0][k] for p in points)]
  assert len(plane)==1;planes.setdefault(tuple(plane[0]),[]).append(f['id'])
 xs=[k/4 for k in range(5)];zs=[0,.1,.2,.3,.55,.8,1.];nodes=[];index={}
 for k,z in enumerate(zs):
  for j,y in enumerate(xs):
   for i,x in enumerate(xs):index[i,j,k]=len(nodes);nodes.append([len(nodes),x,y,z])
 cells=[];uses=collections.defaultdict(list);ring=[1,2,3,7,4,5]
 for k in range(len(zs)-1):
  for j in range(4):
   for i in range(4):
    local=[index[i+dx,j+dy,k+dz] for dx,dy,dz in [(0,0,0),(1,0,0),(1,1,0),(0,1,0),(0,0,1),(1,0,1),(1,1,1),(0,1,1)]]
    for t in range(6):
     v=[local[0],local[6],local[ring[t]],local[ring[(t+1)%6]]];points=[tuple(map(F,nodes[n][1:])) for n in v]
     if oracle.base.determinant(points)<0:v[0],v[1]=v[1],v[0]
     assert oracle.base.determinant([tuple(map(F,nodes[n][1:])) for n in v])>0
     cells.append({'id':len(cells),'nodes':v})
     for face in oracle.faces(v):uses[tuple(sorted(face))].append(face)
 surface=[]
 for key,faces in uses.items():
  assert len(faces) in (1,2)
  if len(faces)==2:continue
  face=faces[0];points=[nodes[n][1:] for n in face];plane=[(k,points[0][k]) for k in range(3) if points[0][k] in (0,1) and all(p[k]==points[0][k] for p in points)]
  assert len(plane)==1;surface.append({'nodes':face,'facet':min(planes[tuple(plane[0])])})
 data={'nodes':nodes,'cells':cells,'surface':surface,'facets':source['facets'],'facet_groups':source['facet_groups'],'original':source['original'],'marker_names':['marker_'+str(k) for k in range(12)]}
 path=HERE/'feasibility_control.json';path.write_text(json.dumps(data,indent=2)+'\n');geo.write_su2(data,HERE/'feasibility_control_not_native.su2')
 report,_=oracle.base.frame(path,True,metric_query=oracle.tensor,edge_query=oracle.edge)
 assert report['volume']=='1';heights=[];footprints=[];tilts=[]
 for f in surface:
  points=[tuple(map(F,nodes[n][1:])) for n in f['nodes']]
  if any(p[2]!=0 for p in points):continue
  parents=[c for c in cells if set(f['nodes']).issubset(c['nodes'])];assert len(parents)==1
  apex=tuple(map(F,nodes[next(n for n in parents[0]['nodes'] if n not in f['nodes'])][1:]));heights.append(abs(float(apex[2])/.1-1))
  a,b,c=points
  def area(u,v,w):return (v[0]-u[0])*(w[1]-u[1])-(v[1]-u[1])*(w[0]-u[0])
  total=area(a,b,c);minimum=min(area(apex,b,c)/total,area(a,apex,c)/total,area(a,b,apex)/total);assert minimum>=0;footprints.append(float(minimum))
  center=[float(sum(p[k] for p in points)/3) for k in range(2)];tilts.append(math.hypot(float(apex[0])-center[0],float(apex[1])-center[1])/.1)
 assert heights and max(heights)<1e-8
 report.update(maximum_height_error=max(heights),minimum_footprint_barycentric=min(footprints),maximum_centroid_tilt=max(tilts),wall_faces=len(heights))
 result={'status':'PASS_DECLARED_TARGET_FEASIBILITY','frame':report,'mesh_sha256':sha(HERE/'feasibility_control_not_native.su2'),'helper_sha256':sha(Path(__file__)),'metric_oracle_sha256':sha(HERE/'audit_sensor.py'),'geometry_oracle_sha256':sha(oracle.ROOT/'integration_evidence/native_3d_core_v1/audit_coupled.py'),'reference_input':str(reference.relative_to(HERE)),'reference_input_sha256':sha(reference),'scope':'Constructive non-native control: exact cube embedding/coverage, positive/manifold/disjoint geometry, original six-cell scalar P1 field plus unchanged geometric BL/core4/fade, full composed edges and shape, prescribed altitude and supported normal footprints. Shows this declared target is feasible; does not establish native remeshing, zero centroid tilt, structured multilayer contract, CFD/transfer/MPI or performance.'}
 (HERE/'feasibility_certificate.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True)
if __name__=='__main__':main()
