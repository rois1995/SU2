#!/usr/bin/env python3
"""Independent complete saved-field audit of native-produced height-star geometry."""
import importlib.util,hashlib,json,sys,time
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
BASE=HERE.with_name('planar_symmetry_reference_v1')
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    folder=HERE/('serial_'+(sys.argv[1] if len(sys.argv)>1 else 'v1'))
    source=BASE/'annulus_v7_n1/symmetry_annulus_bl_budget_20_m1_rejected_rank0.json'
    profile=source.with_name('symmetry_annulus_bl_budget_20_m1_frozen_profile.json')
    raw=folder/'native_height_whole.json';data=json.loads(source.read_text());native=json.loads(raw.read_text())
    oldnodes={n[0]:n[1:] for n in data['nodes']};newnodes={n[0]:n[1:] for n in native['nodes']}
    assert set(oldnodes)-set(newnodes)=={50328}
    assert all(oldnodes[n]==newnodes[n] for n in oldnodes.keys() & newnodes.keys())
    assert {tuple(sorted(f['nodes'])) for f in data['surface']}=={tuple(sorted(f[:3])) for f in native['surface']}
    data['nodes']=native['nodes'];data['cells']=[dict(id=i,nodes=c) for i,c in enumerate(native['cells'])]
    data['surface']=[dict(nodes=f[:3],facet=f[3]) for f in native['surface']]
    spec=importlib.util.spec_from_file_location('annular_audit',BASE/'audit_annulus.py')
    audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)
    start=time.monotonic();result=audit.geometry(data,json.loads(profile.read_text())['core'],0.)
    result.update(status='NATIVE_HEIGHT_STAR_COMPLETE_FROZEN_MESH_AUDIT_PASS',seconds=time.monotonic()-start,
        scope='Native private operator and complete frozen mesh; not adaptation lifecycle/transfer/restart or MPI qualification',
        source_sha256={str(p.relative_to(ROOT)):sha(p) for p in (source,profile,raw,Path(__file__),BASE/'audit_annulus.py')})
    (folder/'independent_full_mesh.json').write_text(json.dumps(result,indent=2)+'\n')
    (folder/'canonical_full_mesh.json').write_text(json.dumps(data,separators=(',',':'))+'\n')
    print(json.dumps(result,indent=2),flush=True)
if __name__=='__main__':main()
