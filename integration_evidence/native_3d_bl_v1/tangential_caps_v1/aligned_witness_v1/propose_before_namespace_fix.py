"""Complete metric-aligned feasibility proposal, not native generation or budget acceptance."""
import collections,copy,functools,hashlib,itertools,json,math,sys,time
from pathlib import Path
from fractions import Fraction as F
import numpy as np
from scipy.spatial import Delaunay
from scipy.integrate import quad
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
from audit_coupled import frame
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_bl_v1/alignment_audit_v1'))
from baseline import footprints
ns={};source=HERE.parent/'reproduce.py';text=source.read_text();exec(compile(text[:text.index('results=[]')],str(source),'exec'),dict(__file__=str(source)),ns)
# Definitions above use their module global dictionary.
ns=dict(__file__=str(source));exec(compile(text[:text.index('results=[]')],str(source),'exec'),ns)
normal=ns['normal'];levels=ns['levels'];measures=ns['measures'];angle=math.pi/6;a=np.array([[math.cos(angle),-math.sin(angle)],[math.sin(angle),math.cos(angle)]]);metric=a@np.diag([256.,4.])@a.T;L=np.linalg.cholesky(metric).T
start=time.monotonic();spacing=1.;xy=[]
corners=np.array([[0.,0.],[1,0],[1,1],[0,1]])
for i in range(4):
 p,q=corners[i],corners[(i+1)%4];count=math.ceil(np.linalg.norm(L@(q-p))/spacing)
 for j in range(count):xy.append(p+(q-p)*j/count)
poly=corners@L.T;low=poly.min(axis=0);high=poly.max(axis=0)
for j in range(math.floor(low[1]/(.5*math.sqrt(3)))-1,math.ceil(high[1]/(.5*math.sqrt(3)))+1):
 for i in range(math.floor(low[0])-1,math.ceil(high[0])+1):
  y=np.array([i+.5*(j%2)+.25,j*math.sqrt(3)/2+.25]);p=np.linalg.solve(L,y)
  if not np.all(p>0) or not np.all(p<1):continue
  distances=[]
  for k in range(4):
   u,v=poly[k],poly[(k+1)%4];d=v-u;distances.append(abs(np.cross(d,y-u))/np.linalg.norm(d))
  if min(distances)>.45*spacing:xy.append(p)
xy=np.array(xy);base=Delaunay(xy@L.T).simplices
points={k*len(xy)+i:np.r_[p,z] for k,z in enumerate(levels) for i,p in enumerate(xy)};cells=[];qmin=Jmin=1.
for k in range(len(levels)-1):
 for tri in base:
  a,b,c=sorted(int(n)+k*len(xy) for n in tri);aa,bb,cc=[v+len(xy) for v in (a,b,c)]
  for ids in [[a,b,c,cc],[a,b,cc,bb],[a,bb,cc,aa]]:
   p=np.array([points[n] for n in ids]);det=float(np.linalg.det(p[1:]-p[0]));assert det!=0
   if det<0:ids[0],ids[1]=ids[1],ids[0];p=np.array([points[n] for n in ids])
   m=np.zeros((3,3));m[:2,:2]=metric;m[2,2]=normal(p[:,2].mean());q,J=measures(p,m);qmin=min(qmin,q);Jmin=min(Jmin,J)
   cells.append({'id':len(cells),'nodes':ids})
uses=collections.defaultdict(list)
for c in cells:
 a,b,c,d=c['nodes']
 for face in [(b,c,d),(a,d,c),(a,b,d),(a,c,b)]:uses[tuple(sorted(face))].append(face)
reference=ROOT/'integration_evidence/native_3d_bl_v1/interacting_walls_v1/native_opposing_v3/opposing_finer_grouped_coupled_BL_independent.json';data=copy.deepcopy(json.loads(reference.read_text()));surface=[]
for f in data['facets']:
 if f['marker']==2:f['marker']=0
for rows in uses.values():
 if len(rows)!=1:continue
 face=rows[0];axes=[(k,float(points[face[0]][k])) for k in range(3) if points[face[0]][k] in (0.,1.) and all(points[n][k]==points[face[0]][k] for n in face)];assert len(axes)==1
 axis,value=axes[0];matches=[f['id'] for f in data['facets'] if all(v[axis+1]==value for v in f['vertices'])];assert matches;surface.append({'nodes':face,'facet':min(matches)})
packed=[float(metric[0,0]),float(metric[0,1]),0.,float(metric[1,1]),0.,4.]
for c in data['original']:
 for v in c['vertices']:v[4]=packed
 data.pop('cells',None)
data.update(nodes=[[n]+list(map(float,p)) for n,p in sorted(points.items())],cells=cells,surface=surface)
path=HERE/'candidate.json';path.write_text(json.dumps(data,separators=(',',':'))+'\n')
status={'status':'PROPOSAL_SHAPE_DIAGNOSIS','cells':len(cells),'cap_triangles':len(base),'q':qmin,'J':Jmin,'metric':packed,'levels':levels,'scope':'Constant rotated tangential original-P1, h.1/g1.2/T.3 normal/fade, independent metric-space lattice and Delaunay cap; no production generation or budget proof.'}
(HERE/'shape.json').write_text(json.dumps(status,indent=2)+'\n');print(json.dumps(status),flush=True)
assert qmin>=.2 and Jmin>=.05
@functools.lru_cache(None)
def length(a,b):
 a=np.array(a,float);b=np.array(b,float);d=b-a;cuts=[0.,1.]
 if d[2]:cuts += [(z-a[2])/d[2] for z in (.05,.27,.3) if 0<(z-a[2])/d[2]<1]
 horizontal=float(d[:2]@metric@d[:2]);cuts=sorted(set(cuts));return sum(quad(lambda t:math.sqrt(horizontal+d[2]**2*normal(a[2]+t*d[2])),lo,hi,epsabs=1e-12,epsrel=1e-12)[0] for lo,hi in zip(cuts,cuts[1:]))
def tensor(original,p):return tuple(map(F,packed[:5]+[normal(float(p[2]))]))
report,checked=frame(path,True,True,tensor,lambda original,a,b:length(tuple(a),tuple(b)));assert F(report['volume'])==1
feet=footprints(data,'marker_10');assert feet and all(not r['outside'] for r in feet);height=max(abs(float(points[r['apex']][2])/.1-1) for r in feet);assert height<1e-10
report.update(status='PASS_COMPLETE_ROTATED_TANGENTIAL_FEASIBILITY_PROPOSAL_NOT_NATIVE',source_sha256=hashlib.sha256(reference.read_bytes()).hexdigest(),helper_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),maximum_height_relative_error=height,first_height_faces=len(feet),elapsed_seconds=time.monotonic()-start,scope=status['scope']+' Exact finite whole geometry/no overlap/manifold/links/height and all complete numerical metric edges independently checked. Numerical error indicators are not rigorous enclosures.')
(HERE/'independent.json').write_text(json.dumps(report,indent=2)+'\n');print(report['status'],flush=True)
