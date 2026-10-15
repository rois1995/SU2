#!/usr/bin/env python3
"""Audit saved curved RANS meshes without treating failed transfer assertions as a pass."""
import collections,itertools,json,math,re,sys,time
from fractions import Fraction as F
from pathlib import Path
import numpy as np
from construct import HERE,ROOT,sha,authorities
sys.path.insert(0,str(ROOT/'integration_evidence/native_3d_core_v1'))
sys.path.insert(0,str(HERE.parent/'alignment_audit_v1'))
from audit_cavities import determinant,sub,dot,cross,overlap
from audit_coupled import connected,surface
from baseline import footprints

def mesh(data,geometry,layers):
    nodes={n[0]:tuple(map(F,n[1:])) for n in data['nodes']}
    assert len(nodes)==len(data['nodes']) and len(set(nodes.values()))==len(nodes)
    assert len({c['id'] for c in data['cells']})==len(data['cells'])
    assert len({tuple(sorted(c['nodes'])) for c in data['cells']})==len(data['cells'])
    uses=collections.defaultdict(list);links=collections.defaultdict(list);tets=[];boxes=[];cv=collections.defaultdict(F)
    for index,c in enumerate(data['cells']):
        points=[nodes[n] for n in c['nodes']];volume=determinant(points)/6;assert volume>0;tets.append(points)
        boxes.append(tuple((min(p[k] for p in points),max(p[k] for p in points)) for k in range(3)))
        a,b,d,e=c['nodes']
        for face in [(b,d,e),(a,e,d),(a,b,e),(a,d,b)]:
            uses[tuple(sorted(face))].append((index,(-1)**sum(face[i]>face[j] for i in range(3) for j in range(i+1,3))))
        for n in c['nodes']:links[n].append(tuple(v for v in c['nodes'] if v!=n));cv[n]+=volume/4
    assert all(len(v)==1 or len(v)==2 and sum(x[1] for x in v)==0 for v in uses.values())
    boundary={f for f,v in uses.items() if len(v)==1};assert boundary=={tuple(sorted(f['nodes'])) for f in data['surface']}
    assert len(boundary)==len(data['surface']);surface(list(boundary),True)
    boundaryNodes={n for f in boundary for n in f};adjacency={i:set() for i in range(len(tets))}
    for v in uses.values():
        if len(v)==2:a,b=v[0][0],v[1][0];adjacency[a].add(b);adjacency[b].add(a)
    assert connected(adjacency)
    for n,triangles in links.items():surface(triangles,n not in boundaryNodes)
    pairs=0
    for i,j in itertools.combinations(range(len(tets)),2):
        if any(boxes[i][k][1]<=boxes[j][k][0] or boxes[j][k][1]<=boxes[i][k][0] for k in range(3)):continue
        pairs+=1;assert not overlap(tets[i],tets[j]),('positive overlap',i,j)
    print('Exact embedding checked:',len(tets),'cells',pairs,'candidate pairs',flush=True)
    height=0.;feet=footprints(data,'cylinder') if layers else []
    if layers:assert feet and all(not r['outside'] for r in feet)
    for r in feet:
        a,b,c=[nodes[n] for n in r['face']];n=cross(sub(b,a),sub(c,a))
        h=abs(float(dot(sub(nodes[r['apex']],a),n)))/math.sqrt(float(dot(n,n)));height=max(height,abs(h-.05)/.05)
    assert height<=1e-10
    error=sag=0.
    for f in data['surface']:
        kind,origin,extent,tolerance,axis=geometry[f['facet']];axis=axis/np.linalg.norm(axis)
        p=np.array([[float(x) for x in nodes[n]] for n in f['nodes']])-origin;z=p@axis;r=p-z[:,None]*axis
        if kind==4:error=max(error,float(np.max(np.abs(z))));assert np.max(np.linalg.norm(r,axis=1))<=extent[0]+tolerance
        else:
            assert kind==3;error=max(error,float(np.max(np.abs(np.linalg.norm(r,axis=1)-extent[0]))))
            assert z.min()>=-tolerance and z.max()<=extent[1]+tolerance
            for k in range(3):
                a,b=r[k],r[(k+1)%3];d=b-a;t=np.clip(-a@d/(d@d),0,1) if d@d else 0
                sag=max(sag,float(extent[0]-np.linalg.norm(a+t*d)))
    assert error<=1e-12 and sag<=(.025 if layers else .04)
    return dict(cells=len(tets),nodes=len(nodes),volume=float(sum(cv.values())),exact_overlap_pairs=pairs,
                vertex_links=len(links),wall_height_faces=len(feet),maximum_height_relative_error=height,
                maximum_authority_node_error=error,maximum_wall_chord_sag=sag),cv

def totals(data,cv,records,sa):
    values={r['id']:r['states'] for r in records};points={n[0]:tuple(n[1:]) for n in data['nodes']}
    turbulence={tuple(r[:3]):r[3:] for r in sa};assert set(values)==set(cv) and set(turbulence)==set(points.values())
    flow=[float(sum(cv[n]*F(values[n][k]) for n in cv)) for k in range(15)]
    turb=[float(sum(cv[n]*F(turbulence[points[n]][k]) for n in cv)) for k in range(3)]
    for row in values.values():
        for l in range(3):u=row[5*l:5*l+5];assert all(math.isfinite(x) for x in u) and u[0]>0 and u[4]-sum(x*x for x in u[1:4])/(2*u[0])>0
    assert all(math.isfinite(v) and v>=1e-16 for row in turbulence.values() for v in row)
    return [v for l in (0,2) for v in flow[5*l:5*l+5]+[turb[l]]]

def main():
    started=time.monotonic();name=sys.argv[1];case=HERE/name;receiptPath=HERE/(name+'.json');receipt=json.loads(receiptPath.read_text())
    assert receipt['status'] in ('PASS','FAIL') and receipt['returncode']!=124
    assert all(sha(case/p)==h for p,h in receipt['artifacts'].items()),'Saved evidence changed'
    workers=receipt['contract']['environment']['SU2_NATIVE_POLICY_WORKERS'];prefix='curved_rans_m'+workers
    geometry,_=authorities(case/(prefix+'.su2.native_ref'));reports=[]
    log=(case/'native.log').read_text();native=[dict(t.split('=',1) for t in line.split()[1:]) for line in log.splitlines() if line.startswith('NATIVE3D rank=')]
    assert len(native)==2*receipt['contract']['MPI_ranks'] and all(v['complete']=='1' for v in native)
    for cycle in (0,1):
        stem=prefix+'_s'+str(cycle);data=json.loads((case/(stem+'_accepted.json')).read_text())
        report,cv=mesh(data,geometry,cycle==0)
        initial=json.loads((case/(stem+'_initial.json')).read_text());oldNodes={r[0]:tuple(map(F,r[1:])) for r in initial['nodes']};oldCV=collections.defaultdict(F)
        for c in initial['cells']:
            volume=determinant([oldNodes[n] for n in c['nodes']])/6;assert volume>0
            for n in c['nodes']:oldCV[n]+=volume/4
        state=json.loads((case/(stem+'_states.json')).read_text());sa=json.loads((case/(stem+'_SA.json')).read_text());p=json.loads((case/(stem+'_projection.json')).read_text())
        old=totals(initial,oldCV,state['donor'],sa['before']);new=totals(data,cv,state['accepted'],sa['after'])
        scaled=lambda a,b:max(abs(x-y)/max(abs(sc),1e-300) for x,y,sc in zip(a,b,p['scale']))
        report['donor_integral_residual']=scaled(old,p['donor_total']);report['accepted_integral_residual']=scaled(new,p['new_total'])
        report['open_boundary_ledger_residual']=scaled(p['target_total'],[d+f-l for d,f,l in zip(p['donor_total'],p['fill_open'],p['lost_open'])])
        assert max(report[k] for k in ['donor_integral_residual','accepted_integral_residual','open_boundary_ledger_residual'])<1e-10
        residual=[(n-t)/max(sc,1e-300) for n,t,sc in zip(new,p['target_total'],p['scale'])]
        report['flow_conservation_residual']=max(abs(v) for i,v in enumerate(residual) if i%6<5);assert report['flow_conservation_residual']<1e-10
        report['SA_integral_relative_changes']=[residual[5],residual[11]]
        report['all_projected_integrals_conserved']=max(map(abs,residual))<1e-10
        if 'turbulence_bound_change' in p:
            assert p['flow_fields']==5 and len(p['turbulence_bound_change'])==2
            correction=max(abs(residual[f]-c) for f,c in zip((5,11),p['turbulence_bound_change']))
            assert correction<1e-10
            if any(p['turbulence_bound_change']):assert p['turbulence_limited']+p['turbulence_limited_stage2']>0
            report['SA_reported_correction_residual']=correction
            report['solver_bounds_accounted']=True
        else:assert receipt['status']=='FAIL','Passing RANS workflow must export SA bounds accounting'
        for ext in ('SU2','CGNS'):
            reload=json.loads((case/(stem+'_written_'+ext+'_reload_target.json')).read_text())
            def signature(d):
                n={r[0]:tuple(r[1:]) for r in d['nodes']}
                return sorted(tuple(sorted(n[i] for i in c['nodes'])) for c in d['cells']),sorted((s['facet'],tuple(sorted(n[i] for i in s['nodes']))) for s in d['surface'])
            assert signature(reload)==signature(data)
            assert all(reload[k]==data[k] for k in ['original','facets','facet_groups','marker_names'])
        report.update(stage=cycle,mesh_status='PASS_EXACT_GEOMETRY_HEIGHT_EMBEDDING',SU2_CGNS_geometry_and_source_equal=True);reports.append(report)
    restart=json.loads((case/(prefix+'_restart_states.json')).read_text());wanted={tuple(r['point']):r['states'] for r in restart['donor']}
    assert len(wanted)==len(restart['accepted'])
    restartError=max(abs(a-b)/max(abs(a),1e-300) for r in restart['accepted'] for a,b in zip(wanted[tuple(r['point'])],r['states']))
    assert restartError<1e-10
    sa=json.loads((case/(prefix+'_restart_SA.json')).read_text());wanted={tuple(r[:3]):r[3:] for r in sa['before']}
    saError=max(abs(a-b)/sa['donor_scale'] for r in sa['after'] for a,b in zip(wanted[tuple(r[:3])],r[3:]));assert saError<2e-10
    helpers=[Path(__file__).resolve(),HERE/'construct.py',ROOT/'integration_evidence/native_3d_core_v1/audit_cavities.py',ROOT/'integration_evidence/native_3d_core_v1/audit_coupled.py',HERE.parent/'alignment_audit_v1/baseline.py']
    out=dict(mesh_status='PASS_COMPLETE_CURVED_MESH',workflow_test_status=receipt['status'],frames=reports,flow_restart_residual=restartError,SA_restart_residual=saError,
             native_receipt_sha256=sha(receiptPath),helpers={str(p.relative_to(ROOT)):sha(p) for p in helpers},elapsed_seconds=time.monotonic()-started,
             scope='Exact binary64 rational volumes, oriented manifold connected domain and vertex links, no tetrahedron interior overlap, wall normal feet/height, analytic curved node/chord checks. Independent physical P1 flow/SA integrals and flow conservation/open ledger, SU2/CGNS original-source equivalence and current/BDF flow/SA restart. Native metric shape/edge gates are separate, not independently integrated here. SA integral differences remain explicitly unconserved; this report does not turn the rejected workflow into a pass. SA solver-bound correction accounting is checked when exported; independent raw-projection/clipping replay is separate. No general geometry, accuracy or performance qualification.')
    (case/'independent_mesh.json').write_text(json.dumps(out,indent=2)+'\n');print(out['mesh_status'],'workflow',out['workflow_test_status'],flush=True)
if __name__=='__main__':main()
