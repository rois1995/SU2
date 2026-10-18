#!/usr/bin/env python3
"""Resumable saved-patch/native gate checks, without CFD or a full regression."""
import gzip,importlib.util,json,resource,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
spec=importlib.util.spec_from_file_location('bounded_replay',HERE.with_name('oneram6_uncertainty_v1')/'run.py')
replay=importlib.util.module_from_spec(spec);spec.loader.exec_module(replay)
def main():
    version=sys.argv[1] if len(sys.argv)>1 else 'v1';assert version.startswith('v') and version[1:].isdigit()
    mode=sys.argv[2] if len(sys.argv)>2 else 'joint';assert mode in ('joint','height')
    folder=(HERE/('native_serial_'+version)) if mode=='joint' else HERE.with_name('native_height_star_v1')/('serial_'+version)
    folder.mkdir(exist_ok=True)
    sources=[]
    for directory in ('Common/include/adaptation','Common/src/adaptation','SU2_CFD/include/adaptation','SU2_CFD/src/adaptation'):
        sources+=list((ROOT/directory).glob('*3D.*'))
    sources += [ROOT/p for p in ('SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp','SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp',
        'UnitTests/Common/adaptation/CNativeBoundary3D_tests.cpp','Common/include/adaptation/CNativeGeometryValidation2D.hpp',
        'Common/src/adaptation/CNativePredicates2D.cpp','Common/include/adaptation/CTransferMemory.hpp')]
    sources += [Path(__file__).resolve(),Path(replay.__file__).resolve()]
    sources=sorted(set(sources));oldpins=replay.pins
    replay.pins=lambda:dict(oldpins(),**{str(p.relative_to(ROOT)):replay.sha(p) for p in sources})
    payload=gzip.compress(json.dumps({str(p.relative_to(ROOT)):p.read_text() for p in sources},sort_keys=True).encode(),mtime=0)
    capsule=folder/'executed_source.json.gz'
    if capsule.exists():assert capsule.read_bytes()==payload,'Changed source: use a new result version'
    else:capsule.write_bytes(payload)
    filters='[NativeJointSurfaceReplay3D],[NativeInteriorPlacement3D],[NativeSurfaceStrip3D],[NativeMetricMove3D],[NativeConnectivityInterface3D],[NativeConnectivityHeight3D]'
    parameters={'SU2_NATIVE_JOINT_REPLAY':str(HERE/'patch.dat'),'SU2_NATIVE_CURVED_EVIDENCE':'1'}
    fixture=HERE/'patch.dat'
    if mode=='height':
        filters='[NativeHeightStarReplay3D],'+filters;fixture=HERE.with_name('native_height_star_v1')/'patch.dat'
        parameters['SU2_NATIVE_HEIGHT_REPLAY']=str(fixture)
    result=replay.invoke(folder,parameters,fixture,test_filter=filters)
    if 'artifacts' in result:
        assert all(replay.sha(folder/p)==h for p,h in result['artifacts'].items()),'Saved artifact changed'
        print('REUSE_VERIFIED',folder,flush=True)
        return
    result['largest_child_rss_KiB']=resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss
    result['memory_scope']='Largest child RSS observed by this Python process; not aggregate whole-job peak'
    result['scope']='Exact saved patch construction and full native metric change, complete-star rejection, existing affected geometry/height controls; not an accepted whole mesh'
    result['artifacts']={p.name:replay.sha(p) for p in folder.iterdir() if p.is_file() and p.name not in ('result.json','provenance.json','run.log')}
    (folder/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS',folder,flush=True)
if __name__=='__main__':main()
