"""Local computed-metric lifecycle correctness; no timing or profiling analysis."""
from pathlib import Path
import copy,json,sys
from audit_coupled import frame
from audit_geometry_bridge import signature,su2_signature,reject
from audit_restart import state_rows

def main(folder,ranks):
    workers=[m for m in (1,2,4) if m<=ranks]
    records=[]
    for m in workers:
        stem='lifecycle_m%d_d0_r0'%m
        record,data=frame(folder/(stem+'.json'),True)
        assert record['cells']!=len(data['original']),'Lifecycle did not change the initial mesh'
        assert su2_signature(folder/(stem+'_mesh_00001.su2'))==signature(data)
        assert (folder/(stem+'_mesh_00001.su2.native_ref')).is_file()
        expected=[4.,0.,0.,4.,0.,4.]
        sensor_error=max(abs(a-b) for cell in data['original'] for v in cell['vertices'] for a,b in zip(v[4],expected))
        assert sensor_error<1e-12, 'Actual computed sensor changed from declared target'
        state=json.loads((folder/(stem+'_states.json')).read_text())
        assert state['sensor_scale']==4.
        initial={tuple(v[1:4]) for cell in data['original'] for v in cell['vertices']}
        accepted={tuple(n[1:]) for n in data['nodes']}
        _,before=state_rows(state['donor'],initial)
        _,after=state_rows(state['accepted'],accepted)
        records.append(dict(workers=m,mesh=record,maximum_computed_sensor_residual=sensor_error,maximum_state_relative_error=max(before,after)))
        bad=copy.deepcopy(state['accepted']);bad[0]['states'][10]+=.1
        reject(lambda:state_rows(bad,accepted))
        reject(lambda:state_rows(state['accepted'][:-1],accepted))
        bad=copy.deepcopy(data);bad['nodes'][0][1]+=10.
        def invalid():
            assert signature(bad)==su2_signature(folder/(stem+'_mesh_00001.su2'))
        reject(invalid)
    expected_files={'lifecycle_m%d_d0_r0.json'%m for m in workers}
    actual={p.name for p in folder.glob('lifecycle_m*_d*_r*.json') if not p.name.endswith('_states.json')}
    assert actual==expected_files,'Unexpected repeated or instrumentation-control cases'
    result=dict(status='PASS',ranks=ranks,frames=records,adversaries_rejected=3*len(workers),
                scope='One actual metric/Hessian/remesh/repartition/rebuild/transfer/output lifecycle per supported M on local N ranks. Exact finite-facet geometry, positive volume, conformity/no-gap/no-overlap, frozen original-P1 q/J/edge contracts, actual computed sensor, current/BDF2 fields and actual SU2 output. No timing, overhead, performance, scaling, PDE iteration, production BL or curved-geometry claim.')
    (folder/'metric_lifecycle_audit.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS',len(records),'computed-metric lifecycle frames and',result['adversaries_rejected'],'adversaries')

if __name__=='__main__':main(Path(sys.argv[1]).resolve(),int(sys.argv[2]))
