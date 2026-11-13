"""Read-only directions at recorded split bindings, with explicit incident wing sides."""
import ast,json,math,struct,time
from pathlib import Path
import numpy as np
from assess_surface_transaction470 import sha
R=Path(__file__).resolve().parent;started=time.monotonic()
jet_source=R/'analyze_nose492_directions.py';tree=ast.parse(jet_source.read_text())
jet_nodes=[n for n in tree.body if (isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='POWERS' for t in n.targets)) or (isinstance(n,ast.FunctionDef) and n.name in ('differential','self_check'))]
ns={'np':np,'math':math};exec(compile(ast.Module(body=jet_nodes,type_ignores=[]),str(jet_source),'exec'),ns);ns['self_check']()
checkpoint=json.loads((R/'saved_surface492_checkpoint.json').read_text());case=Path(checkpoint['source_case'])
prior=json.loads((R/'nose492_direction_supplement.json').read_text());assert prior['case']==str(case)
reference=Path(checkpoint['prepared_reference']['path']);assert sha(reference)==checkpoint['prepared_reference']['sha256']
raw=case/'nose_target_observation.jsonl';splits=[json.loads(s) for s in raw.read_text().splitlines() if json.loads(s)['stage']=='accepted_split']
by_point={s['point'][0]:s for s in splits};wanted_points={r['point'][0] for r in prior['records']};assert len(wanted_points)==prior['nose_split_records']
parameters=Path(checkpoint['parameters']['path']);assert sha(parameters)==checkpoint['parameters']['sha256'];data=parameters.read_bytes()
magic,schema,version,epoch,npoints,nfaces,policy,atlas=struct.unpack_from('<8Q',data)
assert magic==0x4e41544956454138 and schema==9 and len(data)==72+npoints*72+nfaces*32+atlas
bindings={}
for i in range(npoints):
    row=struct.unpack_from('<QdddQQQdd',data,72+i*72)
    if row[0] in wanted_points:
        node,x,y,z,feature,v,dim,u,w=row;assert v==version and node not in bindings
        assert (x,y,z)==tuple(by_point[node]['point'][1:4]);bindings[node]=(feature,dim,u,w)
assert set(bindings)==wanted_points
with reference.open() as stream:
    assert stream.readline().strip()=='SU2_NATIVE_REFERENCE_3D 9';header=stream.readline().split();assert len(header)==12
    h=list(map(int,header[:11]));assert h[1]<=1_000_000 and h[7]==version
    names=[json.loads(stream.readline()) for _ in range(h[0])];wing=names.index('WING3D')
    facets={};edges=set();wing_edges={};wing_vertices={}
    for _ in range(h[1]):
        row=stream.readline().split();assert len(row)==14;f,m=map(int,row[:2]);ids=tuple(int(row[2+4*k]) for k in range(3));assert f not in facets;facets[f]=(m,ids)
        for i in range(3):
            e=tuple(sorted((ids[i],ids[(i+1)%3])));edges.add(e)
            if m==wing:wing_edges.setdefault(e,[]).append(f)
        if m==wing:
            for node in ids:wing_vertices.setdefault(node,[]).append(f)
    edges=sorted(edges);samples={};wanted_facets=set()
    for node,(feature,dim,u,v) in bindings.items():
        if dim==2:
            assert facets[feature][0]==wing;sides=[(feature,(1-u-v,u,v))]
        else:
            assert dim in (0,1)
            weights={feature:1.} if dim==0 else {edges[feature][0]:1-u,edges[feature][1]:u}
            candidates=wing_vertices[feature] if dim==0 else wing_edges[edges[feature]]
            sides=[(f,tuple(weights.get(i,0.) for i in facets[f][1])) for f in candidates]
        assert sides and all(all(x>=0 for x in bary) and abs(sum(bary)-1)<1e-14 for _,bary in sides)
        samples[node]=sides;wanted_facets.update(f for f,_ in sides)
    for _ in range(h[2]+h[3]+h[4]):assert stream.readline()
    nets={}
    for _ in range(h[9]):
        row=stream.readline().split();assert len(row)==31;f=int(row[0])
        if f in wanted_facets:nets[f]=np.array(list(map(float,row[1:]))).reshape(10,3)
assert set(nets)==wanted_facets
counts=dict(around_nose=0,spanwise=0,ambiguous=0,unknown=0);rows=[];maximum_realization_error=0
for old in prior['records']:
    point=old['point'];node=point[0];split=by_point[node];v=np.array(split['b'])-np.array(split['a']);length=np.linalg.norm(v);assert length>0
    classes=set();detail=[]
    for facet,bary in samples[node]:
        net=nets[facet];realized=np.zeros(3)
        for powers,control in zip(ns['POWERS'],net):realized+=math.factorial(3)/math.prod(math.factorial(k) for k in powers)*math.prod(x**k for x,k in zip(bary,powers))*control
        error=float(np.max(np.abs(realized-np.array(point[1:4]))));maximum_realization_error=max(maximum_realization_error,error)
        numerical_match=bool(error<=64*np.finfo(float).eps*max(1.,float(np.max(np.abs(net)))))
        k,d=ns['differential'](net,bary);principal=int(np.argmax(np.abs(k)));other=1-principal;projection=np.abs(d@v)/length
        if not numerical_match or not old['saved_coordinates_exact']:label='unknown'
        elif abs(abs(k[principal])-abs(k[other]))<=1e-12*max(abs(k[principal]),1e-300):label='ambiguous'
        else:
            delta=projection[principal]-projection[other];label='around_nose' if delta>1e-12 else 'spanwise' if delta < -1e-12 else 'ambiguous'
        classes.add(label);detail.append(dict(facet=facet,barycentric=list(bary),curvatures=k.tolist(),directions=d.tolist(),projections=projection.tolist(),realization_roundoff_error=error,numerical_forward_match=numerical_match,label=label))
    label=next(iter(classes)) if len(classes)==1 else 'unknown' if 'unknown' in classes else 'ambiguous'
    counts[label]+=1;rows.append(dict(edge=split['edge'],point=point,binding=list(bindings[node]),label=label,incident_samples=detail,centroid_label=old['label']))
result=dict(case=str(case),counts=counts,nose_split_records=len(rows),records=rows,
    method='Retained cubic jets at the exact saved split binding. Evaluate every explicitly incident WING3D side; disagreement is ambiguous. Require saved coordinates and numerical forward realization agreement.',
    scope='Local differential direction diagnostic; not certified global curvature or face deviation. Geometry/halving criterion remains unresolved.',
    maximum_forward_roundoff_error=maximum_realization_error,helper=dict(path=str(Path(__file__).resolve()),sha256=sha(__file__),jet_self_checks=True),
    provenance={str(p):sha(p) for p in (reference,parameters,jet_source,R/'nose492_direction_supplement.json',raw)},seconds=time.monotonic()-started)
(R/'nose492_point_direction_supplement.json').write_text(json.dumps(result,indent=2)+'\n');print(dict(counts=counts,nose_split_records=len(rows),maximum_forward_roundoff_error=maximum_realization_error,seconds=result['seconds']))
