"""Independent exact geometry and spatial-P1 checks for the local dependency reproduction."""
from collections import defaultdict
from fractions import Fraction as F
import hashlib,itertools,json,math,sys
from pathlib import Path
sys.dont_write_bytecode=True
folder=Path(__file__).resolve().parent
sys.path.insert(0,str(folder.parent))
from audit_cavities import determinant,overlap,export,sub
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()

def hat(p):return max(F(0),4*min(*p,1-sum(p)))
def measure(t):
 c=tuple(sum(p[j] for p in t)/4 for j in range(3));xx=F(1,4)+F(27,4)*hat(c)
 q=lambda d:xx*d[0]**2+F(1,4)*(d[1]**2+d[2]**2)
 edges={(i,j):float(q(sub(t[j],t[i]))) for i,j in itertools.combinations(range(4),2)}
 dv=float(determinant(t))*math.sqrt(float(xx)/16)
 values=[12*(dv/2)**(2/3)/sum(edges.values()),min(1.,min(math.sqrt(2)*dv/math.sqrt(math.prod(edges[tuple(sorted((i,j)))] for j in range(4) if j!=i)) for i in range(4))),math.sqrt(sum(edges.values())/6),math.sqrt(max(edges.values()))]
 return values

def edge_length(a,b):
 d=sub(b,a);affine=[(a[i],d[i]) for i in range(3)]+[(1-sum(a),-sum(d))]
 cuts={F(0),F(1)}
 for x,y in affine+[(x-u,y-v) for (x,y),(u,v) in itertools.combinations(affine,2)]:
  if y and 0<-x/y<1:cuts.add(-x/y)
 cuts=sorted(cuts);base=sum(x*x for x in d)/4;total=0.
 def q(t):return float(base+F(27,4)*hat(tuple(x+t*y for x,y in zip(a,d)))*d[0]**2)
 for begin,end in zip(cuts,cuts[1:]):
  A=q(begin);B=q(end);sa=math.sqrt(A);sb=math.sqrt(B)
  total+=float(end-begin)*2*(A+sa*sb+B)/(3*(sa+sb))
 return total

def read(name):
 p=folder/name;data=json.loads(p.read_text());nodes={n[0]:tuple(F(float(x)) for x in n[1:]) for n in data['nodes']}
 assert len(nodes)==len(data['nodes']) and len(set(nodes.values()))==len(nodes)
 assert len({c['id'] for c in data['cells']})==len(data['cells'])
 assert len({tuple(sorted(c['nodes'])) for c in data['cells']})==len(data['cells'])
 assert data['sensor_peak_vertex']==4 and data['sensor_peak_xx']==7 and data['metric']==[.25,0,0,.25,0,.25]
 geometry=[[nodes[i] for i in c['nodes']] for c in data['cells']]
 assert all(determinant(t)>0 for t in geometry)
 assert all(not overlap(a,b) for a,b in itertools.combinations(geometry,2))
 uses=defaultdict(list)
 for c in data['cells']:
  a,b,c,d=c['nodes']
  for f in [(b,c,d),(a,d,c),(a,b,d),(a,c,b)]:uses[tuple(sorted(f))].append((-1)**sum(f[i]>f[j] for i in range(3) for j in range(i+1,3)))
 assert all(len(v)==1 or len(v)==2 and sum(v)==0 for v in uses.values())
 skin={k:v[0] for k,v in uses.items() if len(v)==1}
 measured=[measure(t) for t in geometry]
 assert all(all(abs(x-y)<=1e-8*max(1.,abs(x),abs(y)) for x,y in zip(m,c['measures'])) for m,c in zip(measured,data['cells']))
 maximum=max(edge_length(t[i],t[j]) for t in geometry for i,j in itertools.combinations(range(4),2))
 qmin=min(m[0] for m in measured);jmin=min(m[1] for m in measured)
 if data['metric_gates_required']:assert qmin>=.20 and jmin>=.05 and maximum<=1.8
 else:assert jmin<.05
 export(p.with_suffix('.su2'),data,skin)
 return data,nodes,skin,sum(determinant(t)/6 for t in geometry),dict(cells=len(geometry),qmin=qmin,jmin=jmin,maximum_original_P1_edge=maximum)

receipt=json.loads((folder/'validation.json').read_text());assert receipt['status']=='PASS' and not receipt['source_drift']
assert all(sha(folder/n)==h for n,h in receipt['artifact_sha256'].items())
names=['dependency_initial.json','dependency_candidate.json','dependency_collapsed.json','collapse_rejected_patch.json']
rows=[read(n) for n in names]
a,b,c,rejected=rows
assert [r[4]['cells'] for r in rows]==[5,6,3,1]
assert a[2]==b[2]==c[2] and a[3]==b[3]==c[3]==F(1,2)
assert a[0]['surface']==b[0]['surface']==c[0]['surface']
assert rejected[3]==F(1,6)
assert all(a[1][i]==b[1][i]==c[1][i] for f in a[2] for i in f)
def star(data,ids):return {c['id'] for c in data['cells'] if set(ids).issubset(c['nodes'])}
assert star(a[0],[0,4])==star(b[0],[0,4])
assert star(a[0],[4])!=star(b[0],[4])
assert all(determinant([p[1],p[0],p[2],p[3]])<0 for p in [[a[1][i] for i in x['nodes']] for x in a[0]['cells']])
t=[a[1][i] for i in a[0]['cells'][0]['nodes']];assert overlap(t,t)
result=dict(status='PASS',cases={n:r[4] for n,r in zip(names,rows)},exact_domain_volume='1/2',edge_star_unchanged=True,removed_vertex_star_changed=True,rejected_sliver=jmin if False else rejected[4]['jmin'],negative_controls=['inverted determinants','positive-volume self-overlap','reject sliver under unchanged original-P1 field'],scope='Independent exact rational small-mesh positive volume, incidence, pairwise SAT overlap, unchanged oriented skin/coordinates/physical bindings/volume, analytic spatial original-P1 edge integration and centroid quality/sliver audit. Local selector/kernel only; no distributed retry fix, whole engine completion, n8 causation, actual CGNS output or timing claim.',receipt_sha256=sha(folder/'validation.json'),source_sha256={'audit.py':sha(Path(__file__)),'../audit_cavities.py':sha(folder.parent/'audit_cavities.py')},artifact_sha256={n:sha(folder/n) for n in names+[Path(n).with_suffix('.su2').name for n in names]+['probe.stdout']})
(folder/'independent_audit.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
