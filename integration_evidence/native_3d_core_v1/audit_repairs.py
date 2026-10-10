"""Independent exact geometry, actual transported original-P1, and metric repair audit."""
from collections import defaultdict
from fractions import Fraction as F
import hashlib
import json
import math
from pathlib import Path
import sys
from audit_coupled import frame, sources, weights, spd

SPECS={'unit':(1.,False),'thin8':(1e-8,False),'shear4':(1e-4,True)}
KINDS=('face','edge4','wall','interior')
def faces(v):
    a,b,c,d=v
    return [(b,c,d),(a,d,c),(a,b,d),(a,c,b)]
def parse(text,ranks):
    meshes=defaultdict(dict);inputs=defaultdict(dict);transported=defaultdict(lambda:defaultdict(dict))
    samples=defaultdict(lambda:defaultdict(list));events=defaultdict(dict);facets=defaultdict(dict)
    for line in text.splitlines():
        row=line.split()
        if not row:continue
        if row[0]=='MESH' and row[1].startswith('repair_'):
            assert len(row)==28
            label,id=row[1],int(row[2]);assert id not in meshes[label]
            meshes[label][id]=dict(id=id,version=int(row[3]),
                vertices=[[int(row[4+4*i])]+list(map(float,row[5+4*i:8+4*i])) for i in range(4)],
                physical=[list(map(int,row[20+2*i:22+2*i])) for i in range(4)])
        elif row[0]=='REPAIR_FACET':
            assert len(row)==16
            label,id,marker=row[1],int(row[2]),int(row[3]);assert id not in facets[label] and marker==id+10
            facets[label][id]=dict(id=id,marker=marker,
                vertices=[[int(row[4+4*i])]+list(map(float,row[5+4*i:8+4*i])) for i in range(3)])
        elif row[0] in ('REPAIR_INPUT','REPAIR_SOURCE'):
            assert len(row)==44
            label,rank,id=row[1],int(row[2]),int(row[3]);assert 0<=rank<ranks
            value=dict(vertices=[[int(row[4+10*i])]+list(map(float,row[5+10*i:8+10*i]))+
                [list(map(float,row[8+10*i:14+10*i]))] for i in range(4)])
            dest=inputs[label] if row[0]=='REPAIR_INPUT' else transported[label][rank]
            assert id not in dest
            if row[0]=='REPAIR_INPUT':assert id%ranks==rank
            dest[id]=value
        elif row[0]=='REPAIR_SAMPLE':
            assert len(row)==16
            label,rank=row[1],int(row[2]);assert 0<=rank<ranks
            samples[label][rank].append((tuple(F(float(x)) for x in row[3:6]),
                tuple(F(float(x)) for x in row[6:12]),tuple(map(int,row[12:16]))))
        elif row[0]=='REPAIR_EVENT':
            assert all('=' in value for value in row[1:])
            values=dict(value.split('=',1) for value in row[1:]);label=values['case'];rank=int(values['rank'])
            assert int(values['ranks'])==ranks and 0<=rank<ranks and rank not in events[label]
            events[label][rank]=values
    assert set(inputs)==set(transported)==set(samples)==set(events)
    expected_meshes=set();expected_events=set();residual=F(0);queries=0
    for spec,(height,shear) in SPECS.items():
        for kind in KINDS:
            name='repair_'+kind+'_'+spec;expected_meshes.add(name+'_initial')
            assert len(facets[name])=={'face':6,'edge4':8,'wall':4,'interior':4}[kind]
            coordinates=([(0,0,0),(1,0,0),(.5,math.sqrt(3)/2,0),(.5,math.sqrt(3)/6,.25),(.5,math.sqrt(3)/6,-.25)]
                if kind=='face' else ([ (.5,0,0),(0,1,0),(-.5,0,0),(0,-1,0),(0,0,-.75),(0,0,.75)]
                if kind=='edge4' else [(0,0,0),(1,0,0),(0,1,0),(0,0,1)]))
            corners={id:(x+(y if shear else 0),y,z*height) for id,(x,y,z) in enumerate(coordinates)}
            for f in facets[name].values():
                assert all(tuple(v[1:])==corners[v[0]] for v in f['vertices'])
            for m in {ranks,max(1,ranks-1),max(1,ranks//2)}:
                previous=name+'_initial'
                for step in range(3 if kind=='face' else 1):
                    label=name+'_m'+str(m)+'_s'+str(step);expected_events.add(label)
                    assert set(events[label])==set(range(ranks));metadata=events[label][0]
                    for rank,row in events[label].items():
                        assert all(row[k]==metadata[k] for k in ('workers','step','valid','final'))
                        assert int(row['workers'])==m and int(row['step'])==step
                        phases=sum(float(row[k]) for k in ('preparation_s','migration_s','worker_comm_s','worker_s',
                            'return_s','test_audit_s','record_export_s','other_s'))
                        assert float(row['other_s'])>=-1e-12 and abs(phases-float(row['parent_s']))<=1e-10
                    valid=int(metadata['valid']);actual=label if valid else label+'_rejected';expected_meshes.add(actual)
                    old=meshes[previous];original=inputs[label];assert set(original)==set(old)
                    tensor=(1.,-1. if shear else 0.,0.,2. if shear else 1.,0.,(9. if step==1 else 1.)/(height*height))
                    for id,cell in original.items():
                        assert [v[:4] for v in cell['vertices']]==old[id]['vertices']
                        for v in cell['vertices']:
                            expected=tuple((1+.1*v[1])*x for x in tensor)
                            assert all(abs(a-b)<=2e-14*max(1.,abs(b)) for a,b in zip(v[4],expected))
                    donors,_=sources({'original':[original[i] for i in sorted(original)]})
                    assert set(transported[label])==set(samples[label])==set(range(m))
                    assert all(samples[label][rank]==samples[label][0] for rank in range(m)),'Worker query/metric inconsistency'
                    for rank,observed in transported[label].items():
                        assert observed==original,'Original sensor/geometry changed on transport'
                        assert len(samples[label][rank])==4 and len({p for p,m,k in samples[label][rank]})==4
                        for p,observed_metric,key in samples[label][rank]:
                            donor=next(d for d in donors if min(weights(d['points'],p))>=0)
                            assert key==donor['key'];w=weights(donor['points'],p)
                            exact=tuple(sum(w[i]*donor['metrics'][i][j] for i in range(4)) for j in range(6))
                            spd(observed_metric);spd(exact);residual=max(residual,*(abs(a-b)/max(F(1),abs(b)) for a,b in zip(observed_metric,exact)));queries+=1
                    assert len(meshes[actual])==int(metadata['final'])
                    if valid:
                        assert len(meshes[actual])==({'face':2 if step==1 else 3,'edge4':4,'wall':3,'interior':4}[kind])
                        if kind in ('wall','interior'):
                            before={v[0] for c in old.values() for v in c['vertices']};after={v[0] for c in meshes[actual].values() for v in c['vertices']}
                            assert before-after=={4} and after-before=={5} and len(before)==len(after)
                    else:break
                    previous=actual
    assert set(events)==expected_events and set(meshes)==expected_meshes
    assert set(facets)=={'repair_'+kind+'_'+name for name in SPECS for kind in KINDS}
    assert residual<=F(2,10**14)
    return meshes,inputs,facets,events,dict(samples=queries,maximum_relative_component_residual=float(residual))
def audit(log,destination,ranks):
    meshes,inputs,facets,events,queries=parse(log.read_text(),ranks);destination.mkdir(exist_ok=False)
    results=[]
    for name,authority in sorted(facets.items()):
        volume=None
        for label,rows in sorted((k,v) for k,v in meshes.items() if k.startswith(name+'_')):
            nodes={};cells=[];physical=[];initial=label.endswith('_initial')
            event=label[:-9] if label.endswith('_rejected') else label
            for id,cell in sorted(rows.items()):
                assert cell['version']==17+id
                cells.append(dict(id=id,nodes=[v[0] for v in cell['vertices']]))
                for v in cell['vertices']:
                    if v[0] in nodes:assert nodes[v[0]]==v
                    nodes[v[0]]=v
                for f,(present,id) in zip(faces(cell['vertices']),cell['physical']):
                    if present:physical.append(dict(facet=id,nodes=[v[0] for v in f]))
            original=inputs[name+'_m'+str(ranks)+'_s0'] if initial else inputs[event]
            path=destination/(label+'.json')
            data=dict(nodes=[nodes[i] for i in sorted(nodes)],cells=cells,surface=physical,
                facets=[authority[i] for i in sorted(authority)],original=[original[i] for i in sorted(original)])
            path.write_text(json.dumps(data,indent=2)+'\n');measured,_=frame(path,not initial and not label.endswith('_rejected'))
            if volume is None:volume=measured['volume']
            assert measured['volume']==volume,'Private repair changed exact domain volume'
            results.append(measured)
    result=dict(status='REJECTED' if any(k.endswith('_rejected') for k in meshes) else 'PASS',frames=results,
        query_audit=queries,events=events,source_log_sha256=hashlib.sha256(log.read_bytes()).hexdigest(),
        scope='Actual complete face/edge/vertex imports, original sensor migration/P1, rank-veto/storage atomic publication and return, independently exact-audited planar/interior movement and 2->3->2->3/4->4 reconnection on three anisotropic domains and M<=N workers. Prescribed proposals, not automatic repair selection/runtime/CGNS/scaling qualification.')
    result['artifact_sha256']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in destination.iterdir() if p.is_file()}
    (destination/'independent_audit.json').write_text(json.dumps(result,indent=2)+'\n');return result
if __name__=='__main__':
    log,destination,ranks=sys.argv[1:];log=Path(log);ranks=int(ranks)
    print('Case working and retained folder:',Path(destination).resolve(),flush=True)
    result=audit(log,Path(destination),ranks);print(result['status'],len(result['frames']),'repair frames;',result['query_audit'],flush=True)
    lines=log.read_text().splitlines();source=next(i for i,v in enumerate(lines) if v.startswith('REPAIR_SOURCE '))
    sample=next(i for i,v in enumerate(lines) if v.startswith('REPAIR_SAMPLE '));variants=[lines[:source]+lines[source+1:]]
    changed=lines[:];row=changed[source].split();row[8]=str(float(row[8])+1);changed[source]=' '.join(row);variants.append(changed)
    changed=lines[:];row=changed[sample].split();row[6]=str(float(row[6])+1);changed[sample]=' '.join(row);variants.append(changed)
    for bad in variants:
        rejected=False
        try:parse('\n'.join(bad),ranks)
        except (AssertionError,KeyError,StopIteration):rejected=True
        assert rejected,'Missing/tampered original sensor/query accepted'
    print('PASS 3 missing/tampered repair controls',flush=True)
