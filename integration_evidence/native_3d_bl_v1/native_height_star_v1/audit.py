#!/usr/bin/env python3
"""Independent complete saved-field audit of native-produced height-star geometry."""
import importlib.util,hashlib,json,sys,time,math
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
BASE=HERE.with_name('planar_symmetry_reference_v1')
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    version=sys.argv[1] if len(sys.argv)>1 else 'v1';mode=sys.argv[2] if len(sys.argv)>2 else 'serial'
    assert mode in ('serial','mpi')
    folder=HERE/(('serial_' if mode=='serial' else 'mpi2_patch_')+version)
    source=BASE/'annulus_v7_n1/symmetry_annulus_bl_budget_20_m1_rejected_rank0.json' if mode=='serial' else HERE/'mpi2_rejected_merged.json'
    profile=source.with_name('symmetry_annulus_bl_budget_20_m1_frozen_profile.json') if mode=='serial' else BASE/'annulus_v9_n2/symmetry_annulus_bl_budget_20_m2_frozen_profile.json'
    raw=folder/'native_height_whole.json';data=json.loads(source.read_text());native=json.loads(raw.read_text())
    oldnodes={n[0]:n[1:] for n in data['nodes']};newnodes={n[0]:n[1:] for n in native['nodes']}
    assert set(oldnodes)-set(newnodes)==({50328} if mode=='serial' else {73419})
    assert all(oldnodes[n]==newnodes[n] for n in oldnodes.keys() & newnodes.keys())
    assert {tuple(sorted(f['nodes'])) for f in data['surface']}=={tuple(sorted(f[:3])) for f in native['surface']}
    data['nodes']=native['nodes'];data['cells']=[dict(id=i,nodes=c) for i,c in enumerate(native['cells'])]
    data['surface']=[dict(nodes=f[:3],facet=f[3]) for f in native['surface']]
    spec=importlib.util.spec_from_file_location('annular_audit',BASE/'audit_annulus.py')
    audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)
    parameters=json.loads(profile.read_text());assert parameters['original_field'] and parameters['parameter']==0
    start=time.monotonic()
    dependencies=(source,profile,raw,BASE/'audit_annulus.py',Path(audit.policy.__file__),Path(audit.base.__file__),
        ROOT/'integration_evidence/native_3d_core_v1/audit_cavities.py')
    proof={str(p.resolve().relative_to(ROOT)):sha(p) for p in dependencies}
    cache=folder/'independent_geometry_stage.json'
    if cache.exists():
        saved=json.loads(cache.read_text());assert saved['proof']==proof,'Independent audit provenance changed'
        result=dict(saved['result']);print('REUSE_VERIFIED_COMPLETE_GEOMETRY_STAGE',flush=True)
    else:
        result=audit.geometry(data,parameters['core'],0.)
        cache.write_text(json.dumps(dict(proof=proof,result=result),indent=2)+'\n')
    # Height/metric acceptance alone does not establish wall-normal alignment or viscous accuracy.
    points={n[0]:audit.np.array(n[1:],float) for n in data['nodes']};tilts=[];ownership={}
    for face in data['surface']:
        triangle=audit.np.array([points[n] for n in face['nodes']])
        if not audit.np.all(triangle[:,2]==0):continue
        incident=[c for c in data['cells'] if set(face['nodes']).issubset(c['nodes'])];assert len(incident)==1
        apex=next(n for n in incident[0]['nodes'] if n not in face['nodes']);ownership[apex]=ownership.get(apex,0)+1
        normal=audit.np.cross(triangle[1]-triangle[0],triangle[2]-triangle[0]);normal/=audit.np.linalg.norm(normal)
        offset=points[apex]-triangle.mean(axis=0);tangent=offset-float(offset@normal)*normal
        tilts.append(float(audit.np.linalg.norm(tangent))/.1)
    assert tilts
    result.update(maximum_first_row_tilt=max(tilts),maximum_first_row_angle_degrees=math.degrees(math.atan(max(tilts))),
        wall_faces_per_apex={str(k):v for k,v in ownership.items()},
        alignment_scope='Centroid-to-apex tangent/height diagnostic; no orthogonality or viscous-accuracy acceptance threshold added')
    result.update(status='NATIVE_HEIGHT_STAR_COMPLETE_FROZEN_MESH_AUDIT_PASS',seconds=time.monotonic()-start,
        scope='Native private operator and complete frozen mesh; not adaptation lifecycle/transfer/restart or MPI qualification',
        source_sha256={str(p.resolve().relative_to(ROOT)):sha(p) for p in (source,profile,raw,Path(__file__),BASE/'audit_annulus.py')})
    (folder/'independent_full_mesh.json').write_text(json.dumps(result,indent=2)+'\n')
    (folder/'canonical_full_mesh.json').write_text(json.dumps(data,separators=(',',':'))+'\n')
    print(json.dumps(result,indent=2),flush=True)
if __name__=='__main__':main()
