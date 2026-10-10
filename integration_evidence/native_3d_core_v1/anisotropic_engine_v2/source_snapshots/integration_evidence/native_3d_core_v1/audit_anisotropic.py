"""Independent actual transported-source/P1 and global mesh audit of the frozen anisotropic MPI matrix."""
from collections import defaultdict
from fractions import Fraction as F
import hashlib
import json
import math
from pathlib import Path
import sys
from audit_coupled import frame, sources, weights, spd

SPECS={'thin4':(1e-4,False,False),'thin8':(1e-8,False,False),
       'shear4':(1e-4,True,False),'spatial':(1.,False,1),'turning':(1.,False,2)}
def sensor(p,spec):
    h,shear,spatial=spec
    if shear:return (4.,-4.,0.,8.,0.,4/(h*h))
    if not spatial:return (4.,0.,0.,4.,0.,4/(h*h))
    angle=.2+.6*p[0] if spatial==2 else .37
    c,s=math.cos(angle),math.sin(angle);x,y,z=4+.4*p[0],2+.4*p[1],4+.4*p[2]
    return (x*c*c+y*s*s,(x-y)*c*s,0.,x*s*s+y*c*c,0.,z)
def parse(text,ranks):
    meshes=defaultdict(list);inputs=defaultdict(dict);transported=defaultdict(lambda:defaultdict(dict))
    samples=defaultdict(lambda:defaultdict(list))
    for line in text.splitlines():
        row=line.split()
        if not row:continue
        if row[0]=='MESH' and row[1].startswith('aniso_'):
            assert len(row)==28
            meshes[row[1]].append(dict(id=int(row[2]),version=int(row[3]),
                vertices=[[int(row[4+4*i])]+list(map(float,row[5+4*i:8+4*i])) for i in range(4)],
                physical=[list(map(int,row[20+2*i:22+2*i])) for i in range(4)]))
        elif row[0] in ('ANISO_INPUT','ANISO_SOURCE'):
            assert len(row)==44
            label,rank,id=row[1],int(row[2]),int(row[3]);assert 0<=rank<ranks
            vertices=[[int(row[4+10*i])]+list(map(float,row[5+10*i:8+10*i]))+
                      [list(map(float,row[8+10*i:14+10*i]))] for i in range(4)]
            value=dict(vertices=vertices)
            if row[0]=='ANISO_INPUT':
                assert id%ranks==rank and id not in inputs[label];inputs[label][id]=value
            else:
                assert id not in transported[label][rank];transported[label][rank][id]=value
        elif row[0]=='ANISO_SAMPLE':
            assert len(row)==16
            label,rank=row[1],int(row[2]);assert 0<=rank<ranks
            samples[label][rank].append((tuple(F(float(x)) for x in row[3:6]),
                tuple(F(float(x)) for x in row[6:12]),tuple(map(int,row[12:16]))))
    assert set(inputs)=={'aniso_'+name for name in SPECS}
    expected={name+'_m'+str(m) for name in inputs for m in {ranks,max(1,ranks-1),max(1,ranks//2)}}
    assert set(transported)==set(samples)==expected
    residual=F(0);query_count=0
    for name,original in inputs.items():
        assert set(original)==set(range(6))
        initial=meshes[name+'_initial'];assert len(initial)==6
        actual={c['id']:c['vertices'] for c in initial};assert set(actual)==set(original)
        for id,cell in original.items():
            assert [v[:4] for v in cell['vertices']]==actual[id]
            for v in cell['vertices']:
                expected_sensor=sensor(v[1:4],SPECS[name[6:]])
                assert all(abs(a-b)<=2e-14*max(1.,abs(b)) for a,b in zip(v[4],expected_sensor))
        originals,_=sources({'original':list(original.values())})
        for label in sorted(l for l in transported if l.startswith(name+'_m')):
            m=int(label.rsplit('_m',1)[1]);assert set(transported[label])==set(samples[label])==set(range(m))
            for rank,donors in transported[label].items():
                assert donors==original,'Transported original sensors/coordinates differ'
                assert len(samples[label][rank])==4 and len({r[0] for r in samples[label][rank]})==4
                for p,observed,key in samples[label][rank]:
                    donor=next(c for c in originals if min(weights(c['points'],p))>=0)
                    assert key==donor['key'];w=weights(donor['points'],p)
                    reference=tuple(sum(w[i]*donor['metrics'][i][j] for i in range(4)) for j in range(6))
                    spd(observed);spd(reference)
                    residual=max(residual,*(abs(a-b)/max(F(1),abs(b)) for a,b in zip(observed,reference)))
                    query_count+=1
    assert residual<=F(2,10**14),'Original-P1 query residual'
    assert set(meshes)=={name+'_initial' for name in inputs}|{label if label in meshes else label+'_rejected' for label in expected}
    return meshes,inputs,dict(samples=query_count,maximum_relative_component_residual=float(residual))
def audit(log,destination,ranks):
    meshes,inputs,query=parse(log.read_text(),ranks)
    destination.mkdir(exist_ok=False)
    def faces(v):a,b,c,d=v;return [(b,c,d),(a,d,c),(a,b,d),(a,c,b)]
    results=[];complete=True
    for name,original in sorted(inputs.items()):
        facets={}
        for c in meshes[name+'_initial']:
            for f,(present,id) in zip(faces(c['vertices']),c['physical']):
                if present:
                    assert id not in facets;facets[id]=dict(id=id,marker=10+id,vertices=f)
        assert len(facets)==12
        volume=None
        for label,rows in sorted((k,v) for k,v in meshes.items() if k.startswith(name+'_')):
            nodes={};cells=[];physical=[]
            for c in rows:
                assert c['version']==(17+c['id'] if c['id']<=5 else 0)
                cells.append(dict(id=c['id'],nodes=[v[0] for v in c['vertices']]))
                for v in c['vertices']:
                    if v[0] in nodes:assert nodes[v[0]]==v
                    nodes[v[0]]=v
                for f,(present,id) in zip(faces(c['vertices']),c['physical']):
                    if present:physical.append(dict(facet=id,nodes=[v[0] for v in f]))
            initial=label.endswith('_initial');rejected=label.endswith('_rejected')
            assert (len(cells)==6 if initial else 6<len(cells)<=128)
            path=destination/(label+'.json')
            path.write_text(json.dumps(dict(nodes=[nodes[i] for i in sorted(nodes)],cells=cells,surface=physical,
                facets=[facets[i] for i in sorted(facets)],original=list(original.values())),indent=2)+'\n')
            measured,_=frame(path,not initial and not rejected)
            if volume is None:volume=measured['volume']
            assert measured['volume']==volume,'Adaptation changed exact domain volume'
            results.append(measured);complete &= not rejected
    result=dict(status='PASS' if complete else 'REJECTED',frames=results,query_audit=query,
        source_log_sha256=hashlib.sha256(log.read_bytes()).hexdigest(),
        scope='Actual migrated/imported original nodal tensors, exact canonical/P1 residuals and independently audited global embedding/facets/markers/metric gates for frozen thin/sheared/spatial cases. Worker-matrix meshes audited separately; no CFD, CGNS, coarsening or scaling certificate.')
    result['artifact_sha256']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in destination.iterdir() if p.is_file()}
    (destination/'independent_audit.json').write_text(json.dumps(result,indent=2)+'\n')
    return result
if __name__=='__main__':
    log,destination,ranks=sys.argv[1:]
    print('Case working and retained folder:',Path(destination).resolve(),flush=True)
    result=audit(Path(log),Path(destination),int(ranks))
    print(result['status'],len(result['frames']),'anisotropic mesh frames;',result['query_audit'],flush=True)
    # Cheap parser/residual adversaries, before any geometry export.
    text=Path(log).read_text();good=parse(text,int(ranks))
    lines=text.splitlines();source=next(i for i,x in enumerate(lines) if x.startswith('ANISO_SOURCE '))
    sample=next(i for i,x in enumerate(lines) if x.startswith('ANISO_SAMPLE '))
    variants=[lines[:source]+lines[source+1:]]
    changed=lines[:];row=changed[source].split();row[8]=str(float(row[8])+1);changed[source]=' '.join(row);variants.append(changed)
    changed=lines[:];row=changed[sample].split();row[6]=str(float(row[6])+1);changed[sample]=' '.join(row);variants.append(changed)
    for bad in variants:
        rejected=False
        try:parse('\n'.join(bad),int(ranks))
        except (AssertionError,KeyError,StopIteration):rejected=True
        assert rejected,'Auditor accepted missing/tampered source or query'
    print('PASS 3 missing/tampered observation controls',flush=True)
