"""Read-only split directions from immutable cubic jets; no geometry acceptance or mesh edits."""
import ast,json,math,time
from pathlib import Path
import numpy as np
from assess_surface_transaction470 import sha
R=Path(__file__).resolve().parent
POWERS=((3,0,0),(0,3,0),(0,0,3),(2,1,0),(1,2,0),(0,2,1),(0,1,2),(1,0,2),(2,0,1),(1,1,1))

def differential(net,bary=(1/3,1/3,1/3)):
    """Same first/second fundamental forms as MappedFacetReference::EvaluateSurface."""
    def derivative(indices):
        out=np.zeros(3)
        for powers,point in zip(POWERS,net):
            p=list(powers);factor=math.factorial(3)/math.prod(math.factorial(k) for k in p)
            for axis in indices:
                factor*=p[axis];p[axis]-=1
                if factor==0:break
            if factor:out+=factor*math.prod(x**k for x,k in zip(bary,p))*point
        return out
    u=derivative((1,))-derivative((0,));v=derivative((2,))-derivative((0,))
    normal=np.cross(u,v);normal/=np.linalg.norm(normal)
    uu=derivative((1,1))-2*derivative((1,0))+derivative((0,0))
    uv=derivative((1,2))-derivative((1,0))-derivative((0,2))+derivative((0,0))
    vv=derivative((2,2))-2*derivative((2,0))+derivative((0,0))
    a=np.linalg.norm(u);t1=u/a;t2=np.cross(normal,t1);b=v@t1;c=v@t2
    assert a>0 and c>0 and np.isfinite([a,b,c]).all()
    k11=normal@uu/a**2;k12=((normal@uv)/a-b*k11)/c
    k22=((normal@vv)-b*b*k11-2*b*c*k12)/(c*c)
    k,d=np.linalg.eigh([[k11,k12],[k12,k22]])
    directions=np.array([d[0,i]*t1+d[1,i]*t2 for i in range(2)])
    assert np.isfinite(k).all() and np.isfinite(directions).all()
    return k,directions

def self_check():
    plane=np.array([[j/3,k/3,0.] for i,j,k in POWERS]);k,_=differential(plane)
    assert np.max(np.abs(k))<1e-12
    parabola=np.array([[j/3,k/3,2*j*(j-1)/6+.5*k*(k-1)/6] for i,j,k in POWERS])
    k,d=differential(parabola,(1.,0.,0.));assert np.allclose(k,[1.,4.],rtol=1e-12,atol=1e-12)
    assert abs(d[1]@np.array([1.,0.,0.]))>1-1e-12
    skew=np.array([[(j+.3*k)/3,k/3,2*j*(j-1)/6+1.2*j*k/6+.68*k*(k-1)/6] for i,j,k in POWERS])
    k,_=differential(skew,(1.,0.,0.));assert np.allclose(k,[1.,4.],rtol=1e-12,atol=1e-12)

self_check();started=time.monotonic()
checkpoint=json.loads((R/'saved_surface492_checkpoint.json').read_text());case=Path(checkpoint['source_case'])
assert json.loads((case/'status.json').read_text())['pins_verified']
assert json.loads((case/'terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
reference=Path(checkpoint['prepared_reference']['path']);assert sha(reference)==checkpoint['prepared_reference']['sha256']
fixed=json.loads((R/'nose492_fixed_faces.json').read_text());raw=case/'nose_target_observation.jsonl'
records=[json.loads(line) for line in raw.read_text().splitlines()];splits=[r for r in records if r['stage']=='accepted_split']
assessment=json.loads((R/'nose_target_observation492_assessment.json').read_text())
assert assessment['lineage_reconstruction']['no_compound_edits_proven']
positions={};roots=[]
for face in fixed['faces']:
    roots.append((int(face['facet']),tuple(sorted(n['id'] for n in face['nodes']))))
    for node in face['nodes']:
        xyz=tuple(node['xyz']);assert positions.setdefault(node['id'],xyz)==xyz
source=R/'assess_saved_production_surface492_v2.py';tree=ast.parse(source.read_text())
ns={};function=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='replay_face_splits')
exec(compile(ast.Module(body=[function],type_ignores=[]),str(source),'exec'),ns)
leaves,affected,exact,missing=ns['replay_face_splits'](roots,splits,positions);assert missing==0
wanted={root[0] for rows in affected for root in rows};nets={}
with reference.open() as stream:
    assert stream.readline().strip()=='SU2_NATIVE_REFERENCE_3D 9'
    header=stream.readline().split();assert len(header)==12
    counts=list(map(int,header[:11]));assert counts[1]<=1_000_000 and counts[9]<=counts[1]
    for _ in range(counts[0]+counts[1]+counts[2]+counts[3]+counts[4]):assert stream.readline()
    for _ in range(counts[9]):
        row=stream.readline().split();assert len(row)==31;facet=int(row[0])
        if facet in wanted:
            assert facet not in nets;nets[facet]=np.array(list(map(float,row[1:]))).reshape(10,3)
assert set(nets)==wanted
jets={f:differential(net) for f,net in nets.items()};counts=dict(around_nose=0,spanwise=0,ambiguous=0,unknown=0);rows=[]
for i,split in enumerate(splits):
    contexts=affected[i]
    if not contexts:continue
    classes=set();detail=[]
    if exact[i]:
        edge=np.array(split['b'])-np.array(split['a']);length=np.linalg.norm(edge);assert length>0
        for facet,face in contexts:
            k,d=jets[facet];principal=int(np.argmax(np.abs(k)));other=1-principal
            projections=np.abs(d@edge)/length
            if abs(abs(k[principal])-abs(k[other]))<=1e-12*max(abs(k[principal]),1e-300):label='ambiguous'
            else:
                delta=projections[principal]-projections[other]
                label='around_nose' if delta>1e-12 else 'spanwise' if delta < -1e-12 else 'ambiguous'
            classes.add(label);detail.append(dict(facet=facet,original_face=list(face),curvatures=k.tolist(),directions=d.tolist(),projections=projections.tolist(),label=label))
    label=next(iter(classes)) if len(classes)==1 else 'ambiguous' if classes else 'unknown'
    counts[label]+=1;rows.append(dict(edge=split['edge'],point=split['point'],a=split.get('a'),b=split.get('b'),saved_coordinates_exact=exact[i],label=label,ancestor_samples=detail))
result=dict(case=str(case),counts=counts,nose_split_records=len(rows),all_305_split_records_retained_in_raw=len(splits),
    method='At each frozen original ancestor-facet barycenter, evaluate the retained cubic first/second fundamental forms and project the actual split chord onto both principal directions; require consistent labels across incident ancestors.',
    scope='Local differential direction diagnostic, not certified global curvature/direction bounds or geometric deviation acceptance. No change to the formal unresolved geometry outcome.',
    records=rows,helper=dict(path=str(Path(__file__).resolve()),sha256=sha(__file__),self_check=True),
    provenance={str(p):sha(p) for p in (reference,R/'nose492_fixed_faces.json',raw,source,R/'nose_target_observation492_assessment.json')},seconds=time.monotonic()-started)
(R/'nose492_direction_supplement.json').write_text(json.dumps(result,indent=2)+'\n');print(dict(counts=counts,nose_split_records=len(rows),seconds=result['seconds']))
