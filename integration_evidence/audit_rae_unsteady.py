"""Inspect real static-geometry RAE native window meshes and transported states."""
import argparse
import hashlib
import json
import re
from pathlib import Path
import numpy as np
from audit_native_composite_rae import audit
from airfoil_reference_audit import reference_audit, loops
from audit_native_unsteady import capcheck, state, integral


def inspect(case):
    cfg = (case / 'run.cfg').read_text()
    def setting(name):return re.search(r'^'+name+r'\s*=\s*(.*)',cfg,re.M)[1].strip()
    freq,steps = int(setting('ADAP_FREQ')),int(setting('TIME_ITER'))
    requested = re.search(r'^ADAP_BL_FIRST_HEIGHT\s*=\s*\(\s*([\deE.+-]+)',cfg,re.M)
    height = float(requested[1]) if requested is not None else None
    rows = []
    for cycle,first in enumerate(range(freq,steps,freq),1):
        donor_path = case/('input.su2' if cycle==1 else f'mesh_{first-freq:05d}.su2')
        mesh_path = case/f'mesh_{first:05d}.su2'
        row = audit(case,cycle,donor_path,mesh_path,case/f'flow_{first-1:05d}.vtu')
        assert not row['bad_quality_cells'] and not row['bad_length_edges']
        mesh = capcheck.read_su2(mesh_path)
        validity = capcheck.check_validity(mesh)
        assert all(g[0]=='PASS' for g in validity.values()),validity
        refs = reference_audit(case/'input.su2',mesh_path)
        assert all(r['all_declared_features_exactly_retained'] and abs(r['covered_arc_over_period']-1)<1e-10
                   and r['max_corresponding_parameter_deviation']<=float(setting('ADAP_HAUSD')) for r in refs)
        adjacent = {tuple(sorted(e)):t for t in mesh.E for e in (t[[0,1]],t[[1,2]],t[[2,0]])}
        errors = []
        for edge in mesh.M['AIRFOIL'] if height is not None else []:
            t = adjacent[tuple(sorted(edge))];a,b = mesh.P[edge]
            v = mesh.P[next(i for i in t if i not in edge)]
            area2 = abs((b[0]-a[0])*(v[1]-a[1])-(b[1]-a[1])*(v[0]-a[0]))
            errors.append(abs(area2/np.linalg.norm(b-a)/height-1))
        if errors:assert max(errors)<=1e-8
        history = []
        donor = capcheck.read_su2(donor_path)
        def outer_area(m):
            ids = loops(m.P,m.E,m.M)['FARFIELD'][0]
            v = m.P[ids].astype(np.longdouble)
            return abs(np.sum(v[:,0]*np.roll(v[:,1],-1)-v[:,1]*np.roll(v[:,0],-1))/2)
        outer_delta = float(outer_area(mesh)-outer_area(donor))
        for index in (first-2,first-1):
            new,admissibility = state(mesh,case/f'solution_{index:05d}.dat')
            _,fields,_=capcheck.read_restart(case/f'solution_{index:05d}.dat')
            if 'Nu_Tilde' in fields:assert np.isfinite(fields['Nu_Tilde']).all() and fields['Nu_Tilde'].min()>=0
            defect = policy_residual = None
            if (case/f'flow_{index:05d}.vtu').exists():
                old,_=state(donor,case/f'flow_{index:05d}.vtu')
                norm=np.maximum(integral(donor,np.abs(old)),1e-14)
                difference=integral(mesh,new)-integral(donor,old)
                defect=float(np.max(np.abs(difference)/norm))
                # Default CLOSED policy keeps wall/symmetry content but lets
                # open farfield totals follow its changed polygonal domain.
                # This control has near-constant farfield conservative states.
                boundary=old[np.unique(donor.M['FARFIELD'])]
                freestream=boundary.mean(axis=0)
                assert np.max(np.ptp(boundary,axis=0)/np.maximum(abs(freestream),1))<1e-6
                policy_residual=float(np.max(np.abs(difference-outer_delta*freestream)/norm))
                assert policy_residual<1e-10
            history.append(dict(step=index,relative_integral_defect=defect,closed_policy_integral_residual=policy_residual,minimum_nu_tilde=float(fields['Nu_Tilde'].min()) if 'Nu_Tilde' in fields else None,**admissibility))
        row.update(status='PASS',first_step=first,validity=validity,original_reference=refs,outer_farfield_area_change=outer_delta,
                   max_relative_first_height_error=max(errors) if errors else None,history=history)
        rows.append(row)
    final_mesh=capcheck.read_su2(case/f'mesh_{(steps-1)//freq*freq:05d}.su2')
    _,final_admissibility=state(final_mesh,case/f'solution_{steps-1:05d}.dat')
    log=(case/'solver.log').read_text();assert 'Exit Success' in log
    assert log.count('Native adaptation:')==len(rows)
    timings=re.search(r'Total: solve ([\d.e+-]+) s, metric ([\d.e+-]+) s, remesh ([\d.e+-]+) s, replace ([\d.e+-]+) s \(transfer ([\d.e+-]+) s\), adapted mesh and restart output ([\d.e+-]+) s.',log)
    costs=dict(zip(('CFD','metric','remesh','replace','transfer','adapted_output'),map(float,timings.groups())))
    ratio=(costs['metric']+costs['remesh']+costs['replace']+costs['adapted_output'])/costs['CFD']
    return dict(status='PASS',windows=rows,final_admissibility=final_admissibility,phase_seconds=costs,
                adaptation_over_CFD=ratio,scope='Actual original P1 sensors plus original geometric BL; topology/reference/altitude, current and previous state positivity, available donor integral checks with independent near-constant farfield domain correction for default CLOSED sliver policy. Whole-domain totals change with open boundary resampling. Sparse previous-history donor snapshots are not independently conserved here. Inner/time/flow convergence and aerodynamic accuracy are not certified.',
                checker_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('case',type=Path);args=parser.parse_args()
    output=args.case/'independent_rae_unsteady_audit.json';assert not output.exists(),'Preserve existing evidence'
    result=inspect(args.case.resolve());output.write_text(json.dumps(result,indent=2)+'\n')
    print('PASS',[(r['points'],r['min_quality'],r['max_simpson_length'],r['max_relative_first_height_error']) for r in result['windows']],result['phase_seconds'],result['adaptation_over_CFD'],flush=True)
