"""Whole-cap point distribution and triangulation diagnostic; not native acceptance."""
import itertools,json,math,sys,time
from pathlib import Path
import numpy as np
from scipy.spatial import Delaunay
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
source=ROOT/'integration_evidence/native_3d_bl_v1/tangential_caps_v1/reproduce.py'
ns=dict(__file__=str(source));exec(compile(source.read_text().split('results=[]')[0],str(source),'exec'),ns)
normal,levels,measures=ns['normal'],ns['levels'],ns['measures']
def rotated(h,s):return np.array([[.75*h+1,s*.25*math.sqrt(3)*(h-4)],[s*.25*math.sqrt(3)*(h-4),.25*h+3]])
def sensor(p,kind):
 if kind=='constant':return rotated(256,1)
 if kind=='rotating':return (1-p[0])*rotated(64,-1)+p[0]*rotated(256,1)
 return np.eye(2)*(4+12*p[0])
def q2(tri,kind):
 p=np.array([xy[i] for i in tri]);m=sensor(p.mean(axis=0),kind);s=sum((p[j]-p[i])@m@(p[j]-p[i]) for i,j in itertools.combinations(range(3),2));return 2*math.sqrt(3)*abs(np.linalg.det(p[1:]-p[0]))*math.sqrt(np.linalg.det(m))/s
reports=[]
for kind in ['constant','rotating','scalar']:
 start=time.monotonic();spacing=.85;m=sensor([.5,.5],kind);L=np.linalg.cholesky(m).T;corners=np.array([[0.,0.],[1.,0.],[1.,1.],[0.,1.]])
 xy=[]
 for p,q in zip(corners,np.roll(corners,-1,axis=0)):
  def span(t):
   d=q-p;mm=sensor(p+t*d,kind);return math.sqrt(d@mm@d)
  from scipy.integrate import quad
  total=quad(span,0,1,epsabs=1e-12)[0];count=math.ceil(total/spacing)
  xy.append(p)
  for j in range(1,count):
   lo=0.;hi=1.
   for k in range(40):
    mid=(lo+hi)/2
    if quad(span,0,mid,epsabs=1e-12)[0]<total*j/count:lo=mid
    else:hi=mid
   xy.append(p+.5*(lo+hi)*(q-p))
 poly=corners@L.T;low=poly.min(axis=0);high=poly.max(axis=0)
 for j in range(math.floor(low[1]/(spacing*.5*math.sqrt(3)))-1,math.ceil(high[1]/(spacing*.5*math.sqrt(3)))+1):
  for i in range(math.floor(low[0]/spacing)-1,math.ceil(high[0]/spacing)+1):
   y=spacing*np.array([i+.5*(j%2)+.25,j*math.sqrt(3)/2+.25]);p=np.linalg.solve(L,y)
   if not np.all(p>0) or not np.all(p<1):continue
   mm=sensor(p,kind);near=False
   for a,b in zip(corners,np.roll(corners,-1,axis=0)):
    d=b-a;t=np.clip((p-a)@mm@d/(d@mm@d),0,1);r=p-(a+t*d)
    if math.sqrt(r@mm@r)<.45*spacing:near=True;break
   if not near:xy.append(p)
 xy=np.array(xy);base=[list(map(int,t)) for t in Delaunay(xy@L.T).simplices];flips=0
 # Reconnect against local metric, independent of the chosen global proposal chart.
 while True:
  changed=False;edges={}
  for k,t in enumerate(base):
   for a,b in itertools.combinations(t,2):edges.setdefault(tuple(sorted((a,b))),[]).append(k)
  for (a,b),uses in edges.items():
   if len(uses)!=2:continue
   i,j=uses;c=next(x for x in base[i] if x not in (a,b));d=next(x for x in base[j] if x not in (a,b));p=xy[[a,b,c,d]]
   cross=lambda x,y:float(x[0]*y[1]-x[1]*y[0])
   if cross(p[1]-p[0],p[2]-p[0])*cross(p[1]-p[0],p[3]-p[0])>=0 or cross(p[3]-p[2],p[0]-p[2])*cross(p[3]-p[2],p[1]-p[2])>=0:continue
   old=min(q2(base[i],kind),q2(base[j],kind));new=min(q2([c,d,a],kind),q2([d,c,b],kind))
   if new>old+1e-7:base[i]=[c,d,a];base[j]=[d,c,b];flips+=1;changed=True;break
  if not changed:break
  assert flips<4096
 q=J=1.;bad=0;maxedge=0.
 for tri in base:
  for k,(lo,hi) in enumerate(zip(levels,levels[1:])):
   a,b,c=sorted(tri);points=[np.r_[xy[n],z] for z in (lo,hi) for n in (a,b,c)]
   for ids in [(0,1,2,5),(0,1,5,4),(0,4,5,3)]:
    p=np.array([points[i] for i in ids]);m=np.zeros((3,3));m[:2,:2]=sensor(p.mean(axis=0),kind);m[2,2]=normal(p[:,2].mean());qq,jj=measures(p,m);q=min(q,qq);J=min(J,jj);bad+=qq<.2 or jj<.05
    for i,j in itertools.combinations(range(4),2):
     d=p[j]-p[i];x=.5*(p[j]+p[i]);mm=sensor(x,kind);maxedge=max(maxedge,math.sqrt(d[:2]@mm@d[:2]+normal(x[2])*d[2]**2))
 report=dict(kind=kind,cap_triangles=len(base),cells=len(base)*3*(len(levels)-1),q=q,J=J,bad=bad,midpoint_edge=maxedge,flips=flips,elapsed_seconds=time.monotonic()-start,scope='Offline coordinated cap/three-tet fronts, local centroid shape and midpoint-edge diagnostic only; no full metric/geometry/native/work proof.')
 reports.append(report);(HERE/(kind+'.json')).write_text(json.dumps(dict(report=report,xy=xy.tolist(),base=base,levels=levels),separators=(',',':'))+'\n');print(json.dumps(report),flush=True)
(HERE/'summary.json').write_text(json.dumps(reports,indent=2)+'\n')
