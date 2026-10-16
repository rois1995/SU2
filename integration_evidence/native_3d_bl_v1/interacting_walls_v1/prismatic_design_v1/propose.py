"""Independent complete opposing-wall feasibility witness, not native construction."""
import collections,functools,hashlib,itertools,json,math,re,sys,time
from fractions import Fraction as F
from pathlib import Path
import numpy as np
from scipy.integrate import quad as integrate
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];SOURCE=HERE.parent/'native_opposing_v1'
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_bl_v1/alignment_audit_v1'))
from audit_coupled import frame
from baseline import footprints

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
h=.02;growth=1.2;thickness=.6
# Each planar square has tangent covariance eigenvalues 2 and 2/3.
# Its inverse eigenvalues <=1.5, below the sensor >=4, also through fade to core4.
# Nearest-wall distance <=.5<.9*T=.54: the stronger normal constraint is unfaded.
def tensor(p):
    x,y,z=map(float,p);s=4+12*x;d=min(z,1-z);hn=max(h,2*(h+(growth-1)*d)/(growth+1))
    return (s,0.,0.,s,0.,max(s,1/(hn*hn)))
@functools.lru_cache(None)
def length(a,b):
    a=np.array(a,float);b=np.array(b,float);d=b-a;cuts=[0.,1.]
    if d[2]:cuts+= [(z-a[2])/d[2] for z in (h/2,.5,1-h/2) if 0<(z-a[2])/d[2]<1]
    def value(t):
        xx,_,_,yy,_,zz=tensor(a+t*d);return math.sqrt(xx*d[0]**2+yy*d[1]**2+zz*d[2]**2)
    cuts=sorted(cuts);return sum(integrate(value,l,r,epsabs=1e-12,epsrel=1e-12)[0] for l,r in zip(cuts,cuts[1:]))
def measures(points):
    p=np.array(points,float);xx,_,_,yy,_,zz=tensor(p.mean(axis=0));m=np.diag([xx,yy,zz]);v=np.linalg.det(p[1:]-p[0])*math.sqrt(xx*yy*zz)
    squared={(i,j):(p[j]-p[i])@m@(p[j]-p[i]) for i,j in itertools.combinations(range(4),2)}
    q=12*(v/2)**(2/3)/sum(squared.values());j=min(1.,min(math.sqrt(2)*v/math.sqrt(math.prod(squared[tuple(sorted((i,k)))] for k in range(4) if k!=i)) for i in range(4)))
    return q,j
start=time.monotonic();receipt=json.loads(SOURCE.with_suffix('.json').read_text());assert receipt['status']=='FAIL'
assert all(sha(SOURCE/p)==v for p,v in receipt['artifacts'].items())
# Confirm the simplified independent field against every exported native violation.
rows={};violations={}
for line in (SOURCE/'native.log').read_text().splitlines():
    if line.startswith('MESH opposing_finer_grouped_coupled_BL_rejected '):
        values=line.split();rows[int(values[2])]=[tuple(map(float,values[5+4*k:8+4*k])) for k in range(4)]
    elif line.startswith('BL_VIOLATION '):
        values=dict(token.split('=',1) for token in line.split()[1:]);violations[int(values['cell'])]=values
assert rows and violations
qError=jError=lError=0.
for c,r in violations.items():
    p=rows[c];q,j=measures(p);maximum=max(length(tuple(p[i]),tuple(p[k])) for i,k in itertools.combinations(range(4),2))
    qError=max(qError,abs(q-float(r['q'])));jError=max(jError,abs(j-float(r['j'])));lError=max(lError,abs(maximum-float(r['length'])))
assert max(qError,jError)<2e-10 and lError<2e-8,(qError,jError,lError)
# Preserve the actual initial SU2 geometry, markers and original P1 donors.
lines=(SOURCE/'opposing_finer_grouped_initial_euler_n1.su2').read_text().splitlines()
e=next(i for i,l in enumerate(lines) if l.startswith('NELEM='));n=int(lines[e].split('=')[1]);oldCells=[list(map(int,lines[e+1+i].split()[1:5])) for i in range(n)]
p=next(i for i,l in enumerate(lines) if l.startswith('NPOIN='));n=int(lines[p].split('=')[1]);oldNodes={i:list(map(float,lines[p+1+i].split()[:3])) for i in range(n)}
facets=[];names=[];i=next(i for i,l in enumerate(lines) if l.startswith('NMARK='))+1
while i<len(lines):
    name=lines[i].split('=')[1].strip();names.append(name);n=int(lines[i+1].split('=')[1]);i+=2
    for line in lines[i:i+n]:
        f=list(map(int,line.split()[1:4]));facets.append({'id':len(facets),'marker':len(names)-1,'vertices':[[v]+oldNodes[v] for v in f]})
    i+=n
groups=collections.defaultdict(list)
for f in facets:
    points=[v[1:] for v in f['vertices']];axis=next(k for k in range(3) if len({p[k] for p in points})==1);groups[axis,points[0][axis]].append(f['id'])
original=[{'id':i,'vertices':[[v]+oldNodes[v]+[[4+12*oldNodes[v][0],0,0,4+12*oldNodes[v][0],0,4+12*oldNodes[v][0]]] for v in c]} for i,c in enumerate(oldCells)]
def primitive(z):
    return z/h if z<=h/2 else .5+(growth+1)/(2*(growth-1))*math.log((h+(growth-1)*z)/(h+(growth-1)*h/2))
def inverse(s):
    return h*s if s<=.5 else ((h+(growth-1)*h/2)*math.exp((s-.5)*2*(growth-1)/(growth+1))-h)/(growth-1)
span=2*(primitive(.5)-primitive(h));segments=math.ceil(span/(.55*1.8));z=[0.,h]
for i in range(1,segments):
    s=primitive(h)+span*i/segments;z.append(inverse(s) if s<=primitive(.5) else 1-inverse(2*primitive(.5)-s))
z += [1-h,1.];assert all(a<b for a,b in zip(z,z[1:]))
xy=[(i/4,j/4) for j in range(5) for i in range(5)];base=[]
for j in range(4):
    for i in range(4):
        a=j*5+i;b=a+1;c=a+6;d=a+5;base += [(a,b,c),(a,c,d)]
nodes=[[k*len(xy)+i,x,y,height] for k,height in enumerate(z) for i,(x,y) in enumerate(xy)];points={n[0]:n[1:] for n in nodes};cells=[]
for k in range(len(z)-1):
    for triangle in base:
        a,b,c=sorted(v+k*len(xy) for v in triangle);aa,bb,cc=[v+len(xy) for v in (a,b,c)]
        for row in ([a,b,c,cc],[a,b,cc,bb],[a,bb,cc,aa]):
            p=np.array([points[v] for v in row]);assert np.linalg.det(p[1:]-p[0])!=0
            if np.linalg.det(p[1:]-p[0])<0:row[0],row[1]=row[1],row[0]
            cells.append({'id':len(cells),'nodes':row})
uses=collections.defaultdict(list)
for c in cells:
    a,b,c,d=c['nodes']
    for f in ((b,c,d),(a,d,c),(a,b,d),(a,c,b)):uses[tuple(sorted(f))].append(f)
surface=[]
for rows in uses.values():
    if len(rows)!=1:continue
    f=rows[0];axis=next(k for k in range(3) if len({points[v][k] for v in f})==1 and points[f[0]][k] in (0.,1.));surface.append({'nodes':f,'facet':min(groups[axis,points[f[0]][axis]])})
data={'nodes':nodes,'cells':cells,'surface':surface,'facets':facets,'facet_groups':list(groups.values()),'marker_names':names,'original':original}
proposal=HERE/'candidate.json';proposal.write_text(json.dumps(data,separators=(',',':'))+'\n')
report,checked=frame(proposal,True,True,lambda original,p:tuple(map(F,tensor(p))),lambda original,a,b:length(tuple(a),tuple(b)))
assert F(report['volume'])==1
heightError=0.;count=0
for name in ('marker_10','marker_12'):
    feet=footprints(data,name);assert feet and all(not r['outside'] for r in feet)
    for r in feet:
        apex=points[r['apex']];wall=points[r['face'][0]][2];heightError=max(heightError,abs(abs(apex[2]-wall)/h-1));count+=1
assert heightError<1e-10
report.update(status='PASS_COMPLETE_INDEPENDENT_OPPOSING_WALL_PROPOSAL_NOT_NATIVE_ACCEPTED',first_height=h,thickness=thickness,normal_intervals=len(z)-1,first_height_faces=count,maximum_height_relative_error=heightError,normal_span=2*primitive(.5),nodes_sha256=sha(proposal),source_native_receipt_sha256=sha(SOURCE.with_suffix('.json')),native_failure_model_check={'cells':len(violations),'q_error':qError,'J_error':jError,'full_length_error':lError},helper_sha256=sha(Path(__file__)),geometry_helper_sha256=sha(ROOT/'integration_evidence/native_3d_core_v1/audit_coupled.py'),elapsed_seconds=time.monotonic()-start,scope='Independent complete unit-gap feasibility witness under the same geometry, immutable affine original-P1, two actual planar BL/growth/fade and height/q/J/full-length gates. Exact rational embedding/manifold/links/finite convex facets and no positive overlap, analytic field and independent full-edge quadrature. Not production reconstruction, charged native work/time, MPI, CFD, general geometry or affordability acceptance.')
(HERE/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
