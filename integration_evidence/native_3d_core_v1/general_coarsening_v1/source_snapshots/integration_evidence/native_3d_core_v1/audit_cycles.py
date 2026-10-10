"""Independent actual-source/P1 and global embedding audit of repeated MPI planar adaptation."""
from collections import defaultdict
from fractions import Fraction as F
import hashlib
import json
from pathlib import Path
import sys
from audit_coupled import frame, sources, weights, spd

SPECS={'box':(1.,False),'thin8':(1e-8,False),'shear4':(1e-4,True)}
def faces(v):
    a,b,c,d=v
    return [(b,c,d),(a,d,c),(a,b,d),(a,c,b)]
def parse(text,ranks):
    meshes=defaultdict(dict);inputs=defaultdict(dict);transported=defaultdict(lambda:defaultdict(dict))
    samples=defaultdict(lambda:defaultdict(list));events=defaultdict(dict)
    for line in text.splitlines():
        row=line.split()
        if not row:continue
        if row[0]=='MESH' and row[1].startswith('cycle_'):
            assert len(row)==28
            label,id=row[1],int(row[2]);assert id not in meshes[label]
            meshes[label][id]=dict(id=id,version=int(row[3]),
                vertices=[[int(row[4+4*i])]+list(map(float,row[5+4*i:8+4*i])) for i in range(4)],
                physical=[list(map(int,row[20+2*i:22+2*i])) for i in range(4)])
        elif row[0] in ('CYCLE_INPUT','CYCLE_SOURCE'):
            assert len(row)==44
            label,rank,id=row[1],int(row[2]),int(row[3]);assert 0<=rank<ranks
            value=dict(vertices=[[int(row[4+10*i])]+list(map(float,row[5+10*i:8+10*i]))+
                      [list(map(float,row[8+10*i:14+10*i]))] for i in range(4)])
            target=inputs[label] if row[0]=='CYCLE_INPUT' else transported[label][rank]
            assert id not in target
            if row[0]=='CYCLE_INPUT':assert id%ranks==rank
            target[id]=value
        elif row[0]=='CYCLE_SAMPLE':
            assert len(row)==16
            label,rank=row[1],int(row[2]);assert 0<=rank<ranks
            samples[label][rank].append((tuple(F(float(x)) for x in row[3:6]),
                tuple(F(float(x)) for x in row[6:12]),tuple(map(int,row[12:16]))))
        elif row[0]=='CYCLE_EVENT':
            assert all('=' in v for v in row[1:])
            values=dict(v.split('=',1) for v in row[1:]);label=values['case'];rank=int(values['rank'])
            assert int(values['ranks'])==ranks and 0<=rank<ranks and rank not in events[label]
            events[label][rank]=values
    assert set(inputs)==set(transported)==set(samples)==set(events)
    residual=F(0);queries=0
    expected_meshes={'cycle_'+name+'_initial' for name in SPECS}
    for name,(height,shear) in SPECS.items():
        for m in {ranks,max(1,ranks-1),max(1,ranks//2)}:
            previous='cycle_'+name+'_initial'
            for step in range(4):
                label='cycle_'+name+'_m'+str(m)+'_s'+str(step)
                assert label in events and set(events[label])==set(range(ranks))
                value=events[label][0];valid=int(value['valid']);coarsening=step%2
                for row in events[label].values():
                    assert all(row[k]==value[k] for k in ('workers','step','coarsening','valid','initial','final'))
                    assert int(row['workers'])==m and int(row['step'])==step and int(row['coarsening'])==coarsening
                    phases=sum(float(row[k]) for k in ('preparation_s','migration_s','worker_comm_s','worker_s',
                        'return_s','test_audit_s','record_export_s','other_s'))
                    assert float(row['other_s'])>=-1e-12 and abs(phases-float(row['parent_s']))<=1e-10
                actual=label if valid else label+'_rejected';expected_meshes.add(actual)
                assert len(meshes[actual])==int(value['final'])
                original=inputs[label];old=meshes[previous]
                assert len(old)==int(value['initial']) and set(original)==set(old)
                factor=1. if coarsening else 4.
                tensor=(factor,-factor if shear else 0.,0.,2*factor if shear else factor,0.,factor/(height*height))
                for id,c in original.items():
                    assert [v[:4] for v in c['vertices']]==old[id]['vertices']
                    assert all(tuple(v[4])==tensor for v in c['vertices'])
                donors,_=sources({'original':[original[id] for id in sorted(original)]})
                assert set(transported[label])==set(samples[label])==set(range(m))
                for rank,imported in transported[label].items():
                    assert imported==original,'Transported original differs from actual accepted input'
                    assert len(samples[label][rank])==4 and len({v[0] for v in samples[label][rank]})==4
                    for p,observed,key in samples[label][rank]:
                        donor=next(c for c in donors if min(weights(c['points'],p))>=0)
                        assert key==donor['key'];w=weights(donor['points'],p)
                        exact=tuple(sum(w[i]*donor['metrics'][i][j] for i in range(4)) for j in range(6))
                        spd(observed);spd(exact)
                        residual=max(residual,*(abs(a-b)/max(F(1),abs(b)) for a,b in zip(observed,exact)));queries+=1
                if valid:
                    assert (len(meshes[actual])<len(old) if coarsening else len(meshes[actual])>len(old))
                else:break
                previous=actual
    assert set(meshes)==expected_meshes
    assert residual<=F(2,10**14)
    return meshes,inputs,events,dict(samples=queries,maximum_relative_component_residual=float(residual))
def audit(log,destination,ranks):
    meshes,inputs,events,queries=parse(log.read_text(),ranks)
    destination.mkdir(exist_ok=False);results=[]
    for name in SPECS:
        prefix='cycle_'+name;facets={}
        for cell in meshes[prefix+'_initial'].values():
            for face,(present,id) in zip(faces(cell['vertices']),cell['physical']):
                if present:
                    assert id not in facets;facets[id]=dict(id=id,marker=10+id,vertices=face)
        assert len(facets)==12
        volume=None
        for label,rows in sorted((k,v) for k,v in meshes.items() if k.startswith(prefix+'_')):
            nodes={};cells=[];physical=[];initial=label.endswith('_initial')
            event=label[:-9] if label.endswith('_rejected') else label
            for id,cell in sorted(rows.items()):
                assert cell['version']==(17+id if id<=5 else 0)
                cells.append(dict(id=id,nodes=[v[0] for v in cell['vertices']]))
                for v in cell['vertices']:
                    if v[0] in nodes:assert nodes[v[0]]==v
                    nodes[v[0]]=v
                for f,(present,id) in zip(faces(cell['vertices']),cell['physical']):
                    if present:physical.append(dict(facet=id,nodes=[v[0] for v in f]))
            original=inputs[prefix+'_m'+str(ranks)+'_s0'] if initial else inputs[event]
            path=destination/(label+'.json')
            path.write_text(json.dumps(dict(nodes=[nodes[i] for i in sorted(nodes)],cells=cells,surface=physical,
                facets=[facets[i] for i in sorted(facets)],original=[original[i] for i in sorted(original)]),indent=2)+'\n')
            measured,_=frame(path,not initial and not label.endswith('_rejected'))
            if volume is None:volume=measured['volume']
            assert measured['volume']==volume,'Exact domain changed during repeated adaptation'
            results.append(measured)
    result=dict(status='REJECTED' if any(k.endswith('_rejected') for k in meshes) else 'PASS',frames=results,
        query_audit=queries,events=events,source_log_sha256=hashlib.sha256(log.read_bytes()).hexdigest(),
        scope='Two repeated refine/general-coarsen cycles per faceted ordinary/thin/sheared case on each worker subset; actual accepted-input sensors, transported canonical/P1 observations, independent exact global embedding, marker/facet coverage, metric/shape gates and closed parent phase records. No CFD/history, CGNS, ParMETIS or scalability certificate.')
    result['artifact_sha256']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in destination.iterdir() if p.is_file()}
    (destination/'independent_audit.json').write_text(json.dumps(result,indent=2)+'\n');return result
if __name__=='__main__':
    log,destination,ranks=sys.argv[1:];log=Path(log);ranks=int(ranks)
    print('Case working and retained folder:',Path(destination).resolve(),flush=True)
    result=audit(log,Path(destination),ranks)
    print(result['status'],len(result['frames']),'cycle frames;',result['query_audit'],flush=True)
    lines=log.read_text().splitlines();source=next(i for i,v in enumerate(lines) if v.startswith('CYCLE_SOURCE '))
    sample=next(i for i,v in enumerate(lines) if v.startswith('CYCLE_SAMPLE '))
    variants=[lines[:source]+lines[source+1:]]
    changed=lines[:];row=changed[source].split();row[8]=str(float(row[8])+1);changed[source]=' '.join(row);variants.append(changed)
    changed=lines[:];row=changed[sample].split();row[6]=str(float(row[6])+1);changed[sample]=' '.join(row);variants.append(changed)
    for bad in variants:
        rejected=False
        try:parse('\n'.join(bad),ranks)
        except (AssertionError,KeyError,StopIteration):rejected=True
        assert rejected,'Missing/tampered actual source/query accepted'
    print('PASS 3 missing/tampered cycle observation controls',flush=True)
