"""Exact geometry/original-P1, actual BDF transfer/output and exclusive-phase audit of one envelope case."""
from pathlib import Path
from collections import Counter
import json,math,sys
from audit_coupled import frame
from audit_geometry_bridge import states,signature,su2_signature,cgns_signature,reject

def main(folder):
    p=json.loads((folder/'parameters.json').read_text());ranks=p['ranks'];anisotropy=(1.,1.5,2.)[p['anisotropy']]
    variation=.125 if p['varying'] else 0.
    def sensor_at(x,scale):
        q=scale*(1.+variation*x[0]);diag=.5*(anisotropy*anisotropy+1.);off=.5*(anisotropy*anisotropy-1.)
        return q*diag,q*off,0.,q*diag,0.,q
    records=[];transport=[];previous=None
    for cycle in range(3):
        name=f'envelope_s{cycle}'
        initial,a=frame(folder/(name+'_initial.json'),False)
        accepted,b=frame(folder/(name+'.json'),True)
        assert initial['volume']==accepted['volume']=='1'
        assert a['marker_names']==b['marker_names']
        if previous is not None:
            assert signature(a)==signature(previous),'Next event lost the last accepted mesh'
            assert a['facets']==previous['facets'] and a['facet_groups']==previous['facet_groups']
        state=json.loads((folder/(name+'_states.json')).read_text())
        assert state['sensor_scale']==p['size']**2*(.25 if cycle==1 else 1.)
        transport.append(dict(file=name+'_states.json',**states(b,state,sensor_at)))
        actual=folder/f'envelope_mesh_{cycle+1:05d}.su2'
        assert su2_signature(actual)==signature(b)
        sidecar=actual.with_suffix('.su2.native_ref')
        assert sidecar.is_file()
        if cycle:assert sidecar.read_bytes()==(folder/'envelope_mesh_00001.su2.native_ref').read_bytes()
        records.extend([initial,accepted]);previous=b
    assert records[0]['cells']==6*p['size']**3
    assert records[3]['cells']<records[1]['cells'] and records[5]['cells']>records[3]['cells']
    for fmt in ('SU2','CGNS'):
        actual=folder/('envelope_s2_written_'+fmt+('.cgns' if fmt=='CGNS' else '.su2'))
        assert (cgns_signature(actual) if fmt=='CGNS' else su2_signature(actual))==signature(previous)
        record,loaded=frame(folder/f'envelope_s2_written_{fmt}_reload.json',True)
        assert signature(loaded)==signature(previous);records.append(record)
    rank_rows=[];stages=[];native=[]
    for line in (folder/'case.stdout').read_text().splitlines():
        if line.startswith(('ENVELOPE ','NATIVE3D ')):
            row={k:float(v) for k,v in (word.split('=') for word in line.split()[1:])}
            assert all(math.isfinite(v) and v>=0 for v in row.values())
            assert row['ranks']==ranks and 0<=row['rank']<ranks and row['workers']==p['workers']
            if line.startswith('ENVELOPE '):
                assert row['size']==p['size'] and row['anisotropy']==anisotropy and row['variation']==variation
                assert row['repeat']==p['repeat'] and row['detailed']==p['detail'] and 0<=row['cycle']<3
                assert row['state_error']<2e-10;rank_rows.append(row)
            else:
                phases=('support','reference','import','source','weights','partition','donor_migration','communicator','refine','coarsen','wait','return')
                assert abs(sum(row[k] for k in phases)+row['other']-row['total'])<1e-12*max(1.,row['total'])
                assert row['complete']==1 and row['detailed']==p['detail'];native.append(row)
        elif line.startswith('NATIVE3D_STAGE '):
            row={k:(v if k=='stage' else float(v)) for k,v in (word.split('=') for word in line.split()[1:])}
            assert all(math.isfinite(v) and v>=0 for k,v in row.items() if k!='stage')
            fixed={'stage','rank','ranks','time_iter','total','other','cpu','cpu_other','process_peak_rss'}
            phases=[k for k in row if k not in fixed and not k.endswith('_cpu')]
            assert abs(sum(row[k] for k in phases)+row['other']-row['total'])<1e-12*max(1.,row['total'])
            assert abs(sum(row[k+'_cpu'] for k in phases)+row['cpu_other']-row['cpu'])<1e-12*max(1.,row['cpu'])
            stages.append(row)
    assert len(native)==len(rank_rows)==3*ranks
    assert {(r['cycle'],r['rank']) for r in rank_rows}=={(c,r) for c in range(3) for r in range(ranks)}
    envelope=[r for r in stages if r['stage']=='envelope'];assert len(envelope)==(3*ranks if p['detail'] else 0)
    if p['detail']:
        for rank in range(ranks):
            for cycle,row in enumerate(r for r in envelope if r['rank']==rank):
                parent=next(r for r in rank_rows if r['rank']==rank and r['cycle']==cycle)
                remesh=[r for r in native if r['rank']==rank][cycle]
                assert row['total']<=parent['parent_wall']+1e-12 and row['remesh']>=remesh['total']-1e-12
    source=json.loads((folder/'envelope_s2.json').read_text());state=json.loads((folder/'envelope_s2_states.json').read_text())
    import copy
    bad=copy.deepcopy(state);bad['accepted'][0]['metric'][0]+=.1;reject(lambda:states(source,bad,sensor_at))
    bad=copy.deepcopy(state);bad['accepted'][0]['states'][10]+=.1;reject(lambda:states(source,bad,sensor_at))
    summary=dict(status='PASS',parameters=p,frames=records,states=transport,rank_rows=rank_rows,rank_stages=stages,native_rows=native,adversaries_rejected=2,
                 scope='Three actual native3D backend/driver replacement events on a manufactured rotated anisotropic target. Exact embedding/incidence/finite-facet coverage/markers/volume, original-P1 metric and current/BDF2 audits; actual SU2/CGNS output and readback. Timed manual metric/observer/remesh/replace phases: observer is diagnostic work, not CFD. One repetition is a correctness/diagnostic pilot, not scaling, instrument-overhead, physical-solver or full-Goal1 qualification.')
    (folder/'independent_audit.json').write_text(json.dumps(summary,indent=2)+'\n')
    print('PASS',len(records),'strict/input/readback frames;',len(transport),'state frames;',len(stages),'closed rank-stage rows')
if __name__=='__main__':main(Path(sys.argv[1]).resolve())
