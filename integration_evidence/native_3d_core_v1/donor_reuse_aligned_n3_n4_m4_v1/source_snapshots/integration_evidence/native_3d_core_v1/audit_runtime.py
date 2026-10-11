"""Independent small actual-driver audit, including fixed-work detailed-timer controls."""
from collections import Counter
from pathlib import Path
import copy,json,math,sys
from audit_coupled import frame
from audit_geometry_bridge import signature,states,su2_signature,cgns_signature,reject

PHASES=('support','reference','import','source','weights','partition','donor_migration','communicator','refine','coarsen','wait','return')

def normalized_state(data):
    return {key:sorted((tuple(r['point']),tuple(r['states']),tuple(r.get('metric',()))) for r in data[key]) for key in ('donor','accepted')}

def main(folder,ranks,log):
    records=[];state_records=[];pairs=[];outputs=0;trajectories={}
    for method in ('barycentric','conservative'):
        for m in range(1,ranks+1):
            for detail in (0,1):
                previous=None;counts=[]
                for cycle in range(3):
                    stem='runtime_%s_m%d_d%d_s%d'%(method,m,detail,cycle)
                    record,data=frame(folder/(stem+'.json'),True);records.append(record)
                    state=json.loads((folder/(stem+'_states.json')).read_text())
                    state_records.append(dict(file=stem+'_states.json',**states(data,state)))
                    assert state['sensor_scale']==(1. if cycle==1 else 4.)
                    if previous is not None:
                        original=Counter(tuple(sorted(tuple(v[1:4]) for v in c['vertices'])) for c in data['original'])
                        assert original==signature(previous)[0], 'Next driver event lost accepted topology'
                    counts.append(len(data['cells']));previous=data
                    if detail:
                        off=stem.replace('_d1_','_d0_');baseline=json.loads((folder/(off+'.json')).read_text())
                        assert signature(data)==signature(baseline),'Detailed timers changed fixed-work geometry/connectivity'
                        assert data['original']==baseline['original'],'Detailed timers changed the frozen original sensor'
                        old=json.loads((folder/(off+'_states.json')).read_text())
                        assert normalized_state(state)==normalized_state(old),'Detailed timers changed fixed-work solution/metric transport'
                        pairs.append(stem)
                    elif method=='barycentric':
                        wanted=signature(data)
                        assert su2_signature(folder/(stem+'_written_SU2.su2'))==wanted
                        assert cgns_signature(folder/(stem+'_written_CGNS.cgns'))==wanted
                        for fmt in ('SU2','CGNS'):
                            _,loaded=frame(folder/(stem+'_written_'+fmt+'_reload.json'),True)
                            assert signature(loaded)==wanted
                            outputs+=1
                        filename='runtime_%s_m%d_d0_mesh_%05d.su2'%(method,m,cycle+1)
                        assert su2_signature(folder/filename)==wanted,'Actual driver output differs from accepted mesh'
                        assert (folder/(filename+'.native_ref')).is_file()
                assert counts[1]<counts[0] and counts[2]>counts[1], 'Actual driver did not refine/coarsen/refine'
                trajectories['%s_m%d_d%d'%(method,m,detail)]=counts
    samples=[]
    for line in log.read_text().splitlines():
        if not line.startswith('NATIVE3D '):continue
        row={k:float(v) for k,v in (p.split('=') for p in line.split()[1:])}
        assert all(math.isfinite(v) for v in row.values())
        assert row['ranks']==ranks and 0<=row['rank']<ranks and 1<=row['workers']<=ranks
        assert all(row[p]>=0 for p in PHASES) and row['other']>=-1e-12
        assert abs(sum(row[p] for p in PHASES)+row['other']-row['total'])<1e-12*max(1,row['total'])
        assert row['total']>0 and row['cpu']>=0 and row['idle_cpu']>=0
        if row['complete']:assert row['final']>0
        else:assert row['final']==0 and row['refine_accepted']==row['coarsen_accepted']==0
        samples.append(row)
    assert len(samples)==16*ranks*ranks, 'Missing runtime event/rank records'
    assert sum(s['complete'] for s in samples)==12*ranks*ranks
    d=json.loads((folder/'runtime_barycentric_m1_d0_s0.json').read_text())
    s=json.loads((folder/'runtime_barycentric_m1_d0_s0_states.json').read_text())
    bad=copy.deepcopy(s);bad['accepted'][0]['metric'][0]+=.1;reject(lambda:states(d,bad))
    bad=copy.deepcopy(s);bad['accepted'][0]['states'][10]+=.1;reject(lambda:states(d,bad))
    result=dict(status='PASS',frames=records,states=state_records,detail_control_pairs=pairs,
                actual_output_roundtrips=outputs,trajectories=trajectories,rank_samples=samples,adversaries_rejected=2,
                scope='Configured actual native3D driver factory and atomic reference notification through ReplaceMesh, immutable original-P1 transported metric/current/BDF2 state audits, exclusive remesher-only phase closure, actual SU2 driver output/sidecars and SU2/CGNS house output plus independent parse/reader audits. Small manufactured cube only; no CFD iteration, complete metric/rebuild lifecycle accounting, 3D BL or scaling qualification.')
    (folder/'independent_audit.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS',len(records),'driver frames,',len(pairs),'OFF/ON pairs,',outputs,'output roundtrips')

if __name__=='__main__':main(Path(sys.argv[1]).resolve(),int(sys.argv[2]),Path(sys.argv[3]).resolve())
