#!/usr/bin/env python3
"""Reuse the independently implemented cube/profile/state audit, plus exact declared-plane/source checks."""
import importlib.util,json,os,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
HELPER=ROOT/'integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/budget_bl_policy_v1/audit_enabled.py'
spec=importlib.util.spec_from_file_location('enabled_audit',HELPER);audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)

def main():
    folder=Path(sys.argv[1]).resolve();view=folder/'audit_view';view.mkdir(exist_ok=True)
    # Closed aliases preserve the unchanged, independently authored budget auditor and its field oracle.
    for source in folder.glob('symmetry_bl_budget_*'):
        if not source.is_file() or source.name.endswith('_frozen_profile.json'):continue
        name=source.name[len('symmetry_'):];dest=view/name;target='../'+source.name
        if dest.is_symlink():assert os.readlink(str(dest))==target
        else:dest.symlink_to(target)
        assert os.path.commonpath([str(ROOT),str(dest.resolve())])==str(ROOT)
    reports=audit.check(view);extra=[]
    for p in sorted(folder.glob('symmetry_bl_budget_*_profile.json')):
        if p.name.endswith('_frozen_profile.json'):continue
        stem=p.name[:-len('_profile.json')];data=json.loads((folder/(stem+'.json')).read_text())
        points={n[0]:n[1:] for n in data['nodes']};facets={f['id']:f for f in data['facets']}
        plane_facets={f['id'] for f in data['facets'] if all(v[2]==0 for v in f['vertices'])}
        assert plane_facets
        selected=[f for f in data['surface'] if f['facet'] in plane_facets]
        assert selected and all(points[n][1]==0 for f in selected for n in f['nodes'])
        original={}
        for cell in data['original']:
            for v in cell['vertices']:
                old=original.setdefault(v[0],v[1:4]);assert old==v[1:4]
        assert len(original)==8 and all(p[0] in (0,1) and p[1] in (0,1) and p[2] in (0,1) for p in original.values())
        assert all(original[v[0]]==v[1:4] for f in data['facets'] for v in f['vertices'])
        sa=json.loads((folder/(stem+'_turb_states.json')).read_text())
        old=audit.audit.integrals(data,sa['donor'],True);new=audit.audit.integrals(data,sa['accepted'])
        sa_defect=max(abs(old[k]-new[k])/max(1e-30,abs(old[k])) for k in (0,5,10));assert sa_defect<1e-10
        restart=json.loads((folder/(stem+'_turb_restart_states.json')).read_text());expected={tuple(r['point']):r['states'] for r in restart['donor']}
        assert len(expected)==len(restart['accepted'])
        scale=max(abs(x) for row in expected.values() for x in row)
        sa_restart=max(abs(x-y)/max(1e-30,scale) for r in restart['accepted'] for x,y in zip(expected[tuple(r['point'])],r['states']));assert sa_restart<1e-12
        extra.append({'stem':stem,'exact_planar_faces':len(selected),'original_geometry_and_reference_agree':True,
            'original_sensor_donors':len(data['original']),'original_node_count':len(original),
            'SA_relative_integral_defect':sa_defect,'SA_restart_residual':sa_restart})
    assert len(extra)==2
    out={'status':'INDEPENDENT_SYMMETRY_RANS_LIFECYCLE_PASS','workflows':reports,'plane_and_source':extra,
         'geometry_scope':'Exact positive volume, topology, links, overlap, finite reference and prescribed height on the complete controlled cube',
         'metric_scope':'Independent original-sensor plus geometric BL profile, all cell tensors/edges; heuristic complexity indicator',
         'state_scope':'Actual current/BDF/SA conservation, transported sensor, SU2/CGNS reload and real new-output restart',
         'limits':['No converged viscous accuracy, M6 acceptance, arbitrary oblique plane or retrospective off-plane field adoption'],
         'audit_source_sha256':audit.audit.digest(Path(__file__)),'reused_helper_sha256':audit.audit.digest(HELPER)}
    (folder/'independent_symmetry.json').write_text(json.dumps(out,indent=2)+'\n');print(out['status'])
if __name__=='__main__':main()
