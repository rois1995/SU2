"""Independent constant-target audits of raw native engine probe meshes (standard library only)."""
import argparse,csv,hashlib,json,math
from collections import defaultdict
from fractions import Fraction
from pathlib import Path
parser=argparse.ArgumentParser()
parser.add_argument('directory',type=Path)
args=parser.parse_args()
root=args.directory.resolve(strict=True)
rows=[]
for metadata in sorted(root.glob('*/native_scaling.json')):
 wd=metadata.parent;m=json.loads(metadata.read_text())
 def read(name):
  with (wd/name).open() as stream:return list(csv.DictReader(stream))
 points={};errors=[]
 for row in read('points.csv'):
  key=int(row['id']);value=(float(row['x']),float(row['y']))
  if key in points:errors.append('duplicate point identity')
  if not all(map(math.isfinite,value)):errors.append('nonfinite point')
  points[key]=value
 if len(set(points.values()))!=len(points):errors.append('duplicate coordinates under different identities')
 used=set();edges=defaultdict(list);cellIds=set();areas=[];qmin=1.;lmax=0.;cells=read('cells.csv')
 hn=.04/m['anisotropy'];heightError=0.;boundaryApex={}
 for row in cells:
  cid=int(row['id']);ids=tuple(int(row[k]) for k in ['a','b','c'])
  if cid in cellIds or len(set(ids))!=3:errors.append('duplicate cell identity or repeated cell vertex')
  cellIds.add(cid);used.update(ids)
  xyz=[points[i] for i in ids]
  # Exact sign of the determinant of the binary64 coordinates, independent of SU2 predicates.
  exact=[tuple(Fraction.from_float(v) for v in p) for p in xyz]
  determinant=(exact[1][0]-exact[0][0])*(exact[2][1]-exact[0][1])-(exact[1][1]-exact[0][1])*(exact[2][0]-exact[0][0])
  if determinant<=0:errors.append('nonpositive exact orientation')
  area=float(determinant)/2;areas.append(area)
  squares=[]
  for k in range(3):
   a,b=ids[k],ids[(k+1)%3];pa,pb=points[a],points[b]
   squared=((pb[0]-pa[0])*25)**2+((pb[1]-pa[1])/hn)**2
   squares.append(squared);lmax=max(lmax,math.sqrt(squared))
   edges[tuple(sorted((a,b)))].append((a,b,ids[(k+2)%3]))
  qmin=min(qmin,4*math.sqrt(3)*area*25/hn/math.fsum(squares))
 if used!=set(points):errors.append('unused or missing mesh points')
 perimeter={}
 for key,incident in edges.items():
  if len(incident)==1:perimeter[key]=incident[0]
  elif len(incident)!=2 or incident[0][:2]!=incident[1][:2][::-1]:errors.append('nonmanifold or inconsistent edge orientation')
 if len(points)-len(edges)+len(cells)!=1:errors.append('Euler characteristic differs from a disk')
 physical={};successors={};maxDeviation=0.
 for row in read('faces.csv'):
  a,b=int(row['a']),int(row['b']);marker=int(row['marker']);key=tuple(sorted((a,b)))
  if key in physical:errors.append('duplicate physical face')
  physical[key]=marker
  if key not in perimeter or perimeter[key][:2]!=(a,b):errors.append('physical face differs from the mesh perimeter');continue
  if a in successors:errors.append('branched boundary')
  successors[a]=b
  pa,pb=points[a],points[b]
  if marker in (10,12):
   expectedY=0 if marker==10 else m['ymax']
   maxDeviation=max(maxDeviation,abs(pa[1]-expectedY),abs(pb[1]-expectedY))
   apex=points[perimeter[key][2]]
   length=math.hypot(pb[0]-pa[0],pb[1]-pa[1])
   altitude=abs((pb[0]-pa[0])*(apex[1]-pa[1])-(pb[1]-pa[1])*(apex[0]-pa[0]))/length
   heightError=max(heightError,abs(altitude/m['h0']-1))
  elif marker==11:
   maxDeviation=max(maxDeviation,min(max(abs(pa[0]),abs(pb[0])),max(abs(pa[0]-m['xmax']),abs(pb[0]-m['xmax']))))
  else:errors.append('unexpected physical marker')
 if set(physical)!=set(perimeter):errors.append('physical boundary does not cover the complete perimeter')
 if successors:
  first=next(iter(successors));seen=set();cur=first
  while cur not in seen and cur in successors:seen.add(cur);cur=successors[cur]
  if cur!=first or len(seen)!=len(successors):errors.append('boundary is not one closed connected loop')
 if abs(math.fsum(areas)-m['xmax']*m['ymax'])>1e-10*m['xmax']*m['ymax']:errors.append('total domain area changed')
 # Numerical comparison tolerance is solely for independent floating evaluation, never a modified target.
 tolerance=1e-12
 independentComplete=qmin>=.18-tolerance and lmax<=1.8+tolerance and heightError<=1e-8 and maxDeviation<=1e-10
 if independentComplete!=m['complete']:errors.append('independent contract outcome disagrees with engine report')
 if abs(qmin-m['qmin'])>1e-11 or abs(lmax-m['lmax'])>1e-10*max(1,lmax):errors.append('independent target values disagree')
 row={'directory':str(wd),'structural_pass':not errors,'errors':sorted(set(errors)),'complete':independentComplete,'qmin':qmin,'lmax':lmax,'height_error':heightError,'reference_deviation':maxDeviation,'proven_incompatible':m['metric_height']>1.8,'files_sha256':{name:hashlib.sha256((wd/name).read_bytes()).hexdigest() for name in ['points.csv','cells.csv','faces.csv','native_scaling.json']}}
 rows.append(row)
report={'scope':'constant manufactured frozen target; incomplete meshes are not installed in SU2','evaluation_tolerance':1e-12,'audit_source_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'cases':rows,'all_structural_pass':bool(rows) and all(row['structural_pass'] for row in rows)}
output=root/'independent_audit.json'
if output.exists():raise RuntimeError('Preserve the previous audit; use a fresh copy of the dataset.')
output.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({'cases':len(rows),'all_structural_pass':report['all_structural_pass'],'output':str(output)}))
raise SystemExit(0 if report['all_structural_pass'] else 1)
