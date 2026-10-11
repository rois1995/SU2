"""Independent embedding, frozen residual and export checks for retained private 3D replay progress."""
from pathlib import Path
from fractions import Fraction as F
import hashlib,itertools,json,sys
from audit_coupled import frame,length,sources
from audit_geometry_bridge import signature,su2_signature

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main(folder):
    params=json.loads((folder/'parameters.json').read_text());rounds=params['replay_rounds']
    assert (params['ranks'],params['workers'],params['size'],params['anisotropy'],params['varying'])==(1,1,3,1,1)
    recipe=json.loads((folder/'replay_recipe.json').read_text());origin=Path(recipe['source_folder'])
    assert recipe['source_validation_sha256']==sha(origin/'validation.json')
    for name,digest in recipe['input_sha256'].items():assert sha(folder/name)==sha(origin/name)==digest
    initial,a=frame(folder/'replay_initial.json',False)
    original=json.loads((origin/'envelope_s0_initial.json').read_text())
    assert signature(a)==signature(original) and a['original']==original['original']
    assert a['facets']==original['facets'] and a['facet_groups']==original['facet_groups']
    result=json.loads((folder/'replay_result.json').read_text())
    assert result['maximum_rounds']==rounds and result['accepted_state_preserved']
    assert result['accepted']+result['rejected']==result['attempted'] and result['rounds']<=rounds
    stem='replay_accepted' if result['complete'] else 'replay_rejected'
    final,b=frame(folder/(stem+'.json'),result['complete'],validate_shape=result['complete'])
    assert initial['volume']==final['volume']=='1' and a['facets']==b['facets'] and a['original']==b['original']
    if rounds==512:
        assert not result['complete'] and (result['rounds'],result['accepted'],result['rejected'],result['moved'],result['reconnected'])==(512,59,453,18,26),'Replay differs from original backend'
    nodes={row[0]:tuple(F(float(v)) for v in row[1:]) for row in b['nodes']};original,_=sources(b)
    edges={tuple(sorted(edge)) for c in b['cells'] for edge in itertools.combinations(c['nodes'],2)}
    long_edges=[]
    for edge in sorted(edges):
        value=length(original,*(nodes[k] for k in edge))
        if value>1.8+1e-12:
            uses=[c['id'] for c in b['cells'] if set(edge)<=set(c['nodes'])]
            long_edges.append(dict(vertices=edge,points=[[float(v) for v in nodes[k]] for k in edge],metric_length=value,cells=uses))
    long_edges.sort(key=lambda r:(-r['metric_length'],r['vertices']))
    if result['complete']:assert not long_edges
    # This is a diagnostic private mesh, never an accepted driver output.
    rows=sorted(b['nodes']);index={r[0]:i for i,r in enumerate(rows)};facets={f['id']:f['marker'] for f in b['facets']}
    output=folder/(stem+'.su2')
    with output.open('w') as out:
        out.write('% Private native3D replay diagnostic; accepted CFD state unchanged.\nNDIME= 3\nNELEM= '+str(len(b['cells']))+'\n')
        for c in b['cells']:out.write('10 '+' '.join(str(index[k]) for k in c['nodes'])+'\n')
        out.write('NPOIN= '+str(len(rows))+'\n')
        for i,row in enumerate(rows):out.write(' '.join(format(x,'.17g') for x in row[1:])+' '+str(i)+'\n')
        out.write('NMARK= '+str(len(b['marker_names']))+'\n')
        for marker,name in enumerate(b['marker_names']):
            faces=[f for f in b['surface'] if facets[f['facet']]==marker]
            out.write('MARKER_TAG= '+name+'\nMARKER_ELEMS= '+str(len(faces))+'\n')
            for f in faces:out.write('5 '+' '.join(str(index[k]) for k in f['nodes'])+'\n')
    assert su2_signature(output)==signature(b)
    audit=dict(status='PASS',parameters=params,replay_result=result,initial=initial,private_progress=final,
               oversized_edges=long_edges,oversized_cells=len({c for e in long_edges for c in e['cells']}),
               exported_private_mesh=output.name,export_sha256=sha(output),auditor_sha256=sha(Path(__file__)),
               scope='Exact original n3 driver import/source match, independent positivity/incidence/embedding/finite-facet coverage/volume and actual frozen residuals, accepted-state preservation checks and diagnostic SU2 signature. Rejected private progress may violate length/shape targets and is never published to CFD. A successful direct Refine replay does not qualify backend coarsen/return/replace or alter production default.')
    (folder/'independent_audit.json').write_text(json.dumps(audit,indent=2)+'\n')
    print('PASS private replay audit; complete=',result['complete'],'cells=',final['cells'],'oversized edges=',len(long_edges),'saved',output)
if __name__=='__main__':main(Path(sys.argv[1]).resolve())
