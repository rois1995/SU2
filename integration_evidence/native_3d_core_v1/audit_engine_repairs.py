"""Independent exact mesh/P1 audit of automatic repair events; bad initial shapes are deliberate."""
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import sys
import tempfile
from audit_coupled import frame
from audit_repairs import faces, SPECS, KINDS

PREFIXES=('auto','restoreR','restoreC')
def audit(log,destination,ranks):
    meshes=defaultdict(dict);original=defaultdict(dict);facets=defaultdict(dict);events=defaultdict(dict);final={};work=defaultdict(dict)
    for line in log.read_text().splitlines():
        row=line.split()
        if not row:continue
        if row[0]=='MESH' and row[1].startswith(tuple(p+'_' for p in PREFIXES)):
            assert len(row)==28
            label,id=row[1],int(row[2]);assert id not in meshes[label]
            meshes[label][id]=dict(id=id,version=int(row[3]),vertices=[[int(row[4+4*i])]+list(map(float,row[5+4*i:8+4*i])) for i in range(4)],physical=[list(map(int,row[20+2*i:22+2*i])) for i in range(4)])
        elif row[0]=='AUTO_INPUT':
            assert len(row)==44
            label,rank,id=row[1],int(row[2]),int(row[3]);assert 0<=rank<ranks and id%ranks==rank and id not in original[label]
            original[label][id]=dict(vertices=[[int(row[4+10*i])]+list(map(float,row[5+10*i:8+10*i]))+[list(map(float,row[8+10*i:14+10*i]))] for i in range(4)])
        elif row[0]=='REPAIR_FACET' and row[1].startswith(tuple(p+'_' for p in PREFIXES)):
            assert len(row)==16
            label,id,marker=row[1],int(row[2]),int(row[3]);assert id not in facets[label] and marker==id+10
            facets[label][id]=dict(id=id,marker=marker,vertices=[[int(row[4+4*i])]+list(map(float,row[5+4*i:8+4*i])) for i in range(3)])
        elif row[0] in ('AUTO_EVENT','ENGINE'):
            assert all('=' in v for v in row[1:])
            values=dict(v.split('=',1) for v in row[1:]);label=values['case']
            if not label.startswith(tuple(p+'_' for p in PREFIXES)):continue
            rank=int(values['rank']);dest=events if row[0]=='AUTO_EVENT' else work
            assert rank not in dest[label];dest[label][rank]=values
        elif row[0]=='AUTO_FINAL':
            assert len(row)==5 and row[1] not in final
            final[row[1]]=list(map(float,row[2:]))
    expected_initial={prefix+'_'+k+'_'+s for prefix in PREFIXES for k in KINDS for s in SPECS}
    expected_events={name+'_m'+str(m) for name in expected_initial for m in {ranks,max(1,ranks-1),max(1,ranks//2)}}
    assert set(original)==set(facets)==expected_initial and set(events)==set(work)==set(final)==expected_events
    assert set(meshes)=={n+'_initial' for n in expected_initial}|expected_events
    destination.mkdir(exist_ok=False);results=[]
    for name in sorted(expected_initial):
        prefix,rest=name.split('_',1);kind,spec=rest.rsplit('_',1);height,shear=SPECS[spec]
        assert len(facets[name])=={'face':6,'edge4':8,'wall':4,'interior':4}[kind]
        initial=meshes[name+'_initial'];source=original[name];assert set(initial)==set(source)
        tensor=(1.,-1. if shear else 0.,0.,2. if shear else 1.,0.,(16. if kind=='face' else 1.)/(height*height))
        if kind=='edge4':tensor=(.04,-.04 if shear else 0.,0.,.08 if shear else .04,0.,1./(height*height))
        for id,c in source.items():
            assert [v[:4] for v in c['vertices']]==initial[id]['vertices']
            for v in c['vertices']:assert all(abs(a-(1+.1*v[1])*b)<=2e-14*max(1.,abs((1+.1*v[1])*b)) for a,b in zip(v[4],tensor))
        volume=None
        for label in [name+'_initial']+sorted(k for k in expected_events if k.startswith(name+'_m')):
            nodes={};cells=[];physical=[];is_initial=label.endswith('_initial')
            for id,c in sorted(meshes[label].items()):
                assert c['version']==(17+id if is_initial else 0)
                cells.append(dict(id=id,nodes=[v[0] for v in c['vertices']]))
                for v in c['vertices']:
                    if v[0] in nodes:assert nodes[v[0]]==v
                    nodes[v[0]]=v
                for f,(present,facet) in zip(faces(c['vertices']),c['physical']):
                    if present:physical.append(dict(facet=facet,nodes=[v[0] for v in f]))
            path=destination/(label+'.json');path.write_text(json.dumps(dict(nodes=[nodes[i] for i in sorted(nodes)],cells=cells,surface=physical,facets=[facets[name][i] for i in sorted(facets[name])],original=[source[i] for i in sorted(source)]),indent=2)+'\n')
            measured,_=frame(path,not is_initial,validate_shape=not is_initial)
            if is_initial:
                volume=measured['volume'];assert measured['minimum_mean_ratio']<.20 or measured['minimum_scaled_jacobian']<.05 or measured['maximum_length']>1.8
            else:
                assert measured['volume']==volume
                if prefix=='auto':assert len(cells)==(2 if kind=='face' else len(initial))
                else:assert 0<len(cells)<=12
                if prefix=='restoreC':assert len(cells)<=len(initial)
                assert all(abs(a-b)<=1e-8*max(1.,abs(b)) for a,b in zip(final[label],[measured['minimum_mean_ratio'],measured['minimum_scaled_jacobian'],measured['maximum_length']]))
                m=int(label.rsplit('_m',1)[1]);assert set(events[label])==set(range(ranks)) and set(work[label])==set(range(m))
                for rank,row in events[label].items():
                    assert int(row['ranks'])==ranks and int(row['workers'])==m and int(row['valid'])==1 and int(row['final'])==len(cells)
                    phases=sum(float(row[k]) for k in ('migration_s','worker_comm_s','worker_s','return_s','test_audit_s','record_export_s','other_s'))
                    assert float(row['other_s'])>=-1e-12 and abs(phases-float(row['parent_s']))<=1e-10
                for rank,row in work[label].items():
                    assert int(row['ranks'])==m and int(row['valid'])==1
                    phases=sum(float(row[k]) for k in ('init_s','seed_s','selection_s','dependency_s','reservation_s','donor_s','reconstruction_s','metric_s','routing_s','publication_s','update_s','completion_s','other_s'))
                    assert float(row['other_s'])>=-1e-12 and abs(phases-float(row['total_s']))<=1e-10
                assert sum(int(v['accepted']) for v in work[label].values())>0
                if prefix=='auto':assert sum(int(v['moved' if kind in ('wall','interior') else 'reconnected']) for v in work[label].values())>0
                if prefix=='restoreC' and kind=='edge4':assert sum(int(v['fallback_accepted']) for v in work[label].values())>0
                if prefix=='restoreR' and kind!='face':assert all(int(v['recovery_starts'])==1 for v in work[label].values())
                assert all(int(v['recovery_starts'])<=1 and int(v['rounds'])<=512 for v in work[label].values())
            results.append(measured)
    result=dict(status='PASS',frames=results,events=events,work=work,source_log_sha256=hashlib.sha256(log.read_bytes()).hexdigest(),scope='Automatic bounded failing-feature selection plus in-event refinement/coarsening recovery, complete distributed dependencies, frozen original-P1 progress and atomic publication; independent exact geometry/marker/global embedding and full final metric checks. Deliberately bad initial shape is audited geometrically and measured, never accepted as final. Private donor/query transport is covered by the shared core checkpoint, not a new per-candidate observer here. No runtime/CFD/CGNS/scaling qualification.')
    result['artifact_sha256']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in destination.iterdir() if p.is_file()}
    (destination/'independent_audit.json').write_text(json.dumps(result,indent=2)+'\n');return result
if __name__=='__main__':
    log,destination,ranks=sys.argv[1:];print('Case working and retained folder:',Path(destination).resolve(),flush=True)
    result=audit(Path(log),Path(destination),int(ranks));print(result['status'],len(result['frames']),'automatic repair frames',flush=True)
    lines=Path(log).read_text().splitlines();source=next(i for i,v in enumerate(lines) if v.startswith('AUTO_INPUT '))
    final=next(i for i,v in enumerate(lines) if v.startswith('AUTO_FINAL '));variants=[lines[:source]+lines[source+1:]]
    changed=lines[:];row=changed[source].split();row[8]=str(float(row[8])+1);changed[source]=' '.join(row);variants.append(changed)
    changed=lines[:];row=changed[final].split();row[2]=str(float(row[2])+1);changed[final]=' '.join(row);variants.append(changed)
    for bad in variants:
        with tempfile.TemporaryDirectory(prefix='su2_auto_repair_audit_') as temporary:
            path=Path(temporary)/'bad.log';path.write_text('\n'.join(bad));rejected=False
            try:audit(path,Path(temporary)/'audit',int(ranks))
            except (AssertionError,KeyError,ValueError):rejected=True
            assert rejected,'Missing/tampered original field or final metric accepted'
    print('PASS 3 missing/tampered automatic repair controls',flush=True)
