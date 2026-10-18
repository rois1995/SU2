#!/usr/bin/env python3
"""Replay the exact MPI terminal star through the existing serial native operator."""
import gzip,importlib.util,json,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def main():
    version=sys.argv[1] if len(sys.argv)>1 else 'v1';assert version.startswith('v') and version[1:].isdigit()
    folder=HERE/('mpi2_patch_'+version);folder.mkdir(exist_ok=True)
    mesh_path=HERE/'mpi2_rejected_merged.json';mesh=json.loads(mesh_path.read_text())
    base=HERE.with_name('planar_symmetry_reference_v1')/'annulus_v9_n2';stem='symmetry_annulus_bl_budget_20_m2'
    initial=base/(stem+'_initial.json');profile=base/(stem+'_frozen_profile.json')
    residual=HERE/'mpi2_residuals.json';owners=json.loads(residual.read_text())['wall_ownership'];assert len(owners)==1
    core=json.loads(profile.read_text())['core'];lines=['NATIVE_HEIGHT_STAR_V1',f"{core:.17g} {owners[0]['apex']} {len(mesh['facets'])}"]
    for facet in mesh['facets']:
        lines.append(' '.join(map(str,[facet['id'],facet['marker']]+[x for row in facet['vertices'] for x in row])))
    for data in (mesh,json.loads(initial.read_text())):
        nodes={n[0]:n for n in data['nodes']};lines.append(f"{len(data['cells'])} {len(data['surface'])}")
        for cell in data['cells']:lines.append(' '.join(map(str,[cell['id']]+[x for n in cell['nodes'] for x in nodes[n]])))
        for face in data['surface']:lines.append(' '.join(map(str,[face['facet']]+face['nodes'])))
    fixture=folder/'patch.dat';content='\n'.join(lines)+'\n'
    if fixture.exists():assert fixture.read_text()==content
    else:fixture.write_text(content)
    path=HERE.with_name('oneram6_uncertainty_v1')/'run.py';spec=importlib.util.spec_from_file_location('bounded',path)
    replay=importlib.util.module_from_spec(spec);spec.loader.exec_module(replay)
    sources=[mesh_path,initial,profile,residual,Path(__file__).resolve(),ROOT/'UnitTests/Common/adaptation/CNativeBoundary3D_tests.cpp']
    for directory in ('Common/include/adaptation','Common/src/adaptation'):
        sources+=list((ROOT/directory).glob('*3D.*'))
    sources=sorted(set(sources));old=replay.pins
    replay.pins=lambda:dict(old(),**{str(p.relative_to(ROOT)):replay.sha(p) for p in sources})
    capsule=folder/'executed_source.json.gz';payload=gzip.compress(json.dumps({str(p.relative_to(ROOT)):p.read_text() for p in sources},sort_keys=True).encode(),mtime=0)
    if capsule.exists():assert capsule.read_bytes()==payload
    else:capsule.write_bytes(payload)
    replay.invoke(folder,{'SU2_NATIVE_HEIGHT_REPLAY':str(fixture),'SU2_NATIVE_CURVED_EVIDENCE':'1'},fixture,test_filter='[NativeHeightStarReplay3D]')
if __name__=='__main__':main()
