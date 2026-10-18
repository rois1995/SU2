#!/usr/bin/env python3
"""One bounded current-binary geometry/lifecycle check; generated inputs are preserved with source provenance."""
import gzip,json,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(HERE.with_name('oneram6_uncertainty_v1')))
import run as replay

def main():
    mode=sys.argv[1];ranks=int(sys.argv[2]) if len(sys.argv)>2 else 1
    assert mode in ('geometry','workflow','baseline','annulus') and ranks in (1,2,4)
    version=sys.argv[3] if len(sys.argv)>3 else 'v7';assert version.startswith('v') and version[1:].isdigit()
    folder=HERE/(mode+'_'+version+'_n'+str(ranks))
    sources=[]
    for directory in ('Common/include/adaptation','Common/src/adaptation','SU2_CFD/include/adaptation','SU2_CFD/src/adaptation'):
        sources+=list((ROOT/directory).glob('*3D.*'))
    sources += [ROOT/p for p in ('Common/include/CConfig.hpp','Common/src/CConfig.cpp','SU2_CFD/src/drivers/CDriver.cpp',
        'SU2_CFD/src/drivers/CSinglezoneDriver.cpp','SU2_CFD/src/solvers/CSolver.cpp',
        'UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp','UnitTests/Common/adaptation/CNativeBoundary3D_tests.cpp',
        'UnitTests/Common/adaptation/CNativeMesh3D_tests.cpp',
        'Common/include/adaptation/CNativeAffineReference.hpp','Common/src/adaptation/CNativeAffineReference.cpp',
        'UnitTests/Common/adaptation/CNativeAffineReference_tests.cpp')]
    sources += list((ROOT/'UnitTests/SU2_CFD/adaptation/fixtures').glob('*.inc'))+[Path(__file__).resolve()]
    original_pins=replay.pins
    replay.pins=lambda:dict(original_pins(),**{str(p.relative_to(ROOT)):replay.sha(p) for p in sources})
    folder.mkdir(exist_ok=True)
    capsule=folder/'executed_source.json.gz'
    data=gzip.compress(json.dumps({str(p.relative_to(ROOT)):p.read_text() for p in sources},sort_keys=True).encode(),mtime=0)
    if capsule.exists():assert capsule.read_bytes()==data,'Use a new folder after source changes'
    else:capsule.write_bytes(data)
    # The legacy harness fixture argument pins the declaration schema; this filter generates its meshes/configs itself.
    tag={'geometry':'[NativePlanarUnion3D]','workflow':'[NativeSymmetryRuntime3D]','baseline':'[NativePlanarUnion3D],[NativeReferenceSearch3D],[NativeStaticFix3D],[NativeReference3D],[NativeCurvedGeometry3D]','annulus':'[NativeSymmetryRuntime3D]'}[mode]
    parameters={'SU2_NATIVE_SYMMETRY_ANNULUS':'1'} if mode=='annulus' else {}
    result=replay.invoke(folder,parameters,HERE/'initial.planes',ranks,test_filter=tag)
    if 'artifacts' in result:
        assert all(replay.sha(folder/p)==h for p,h in result['artifacts'].items())
    else:
        result['artifacts']={str(p.relative_to(folder)):replay.sha(p) for p in folder.iterdir() if p.is_file() and p.name not in ('result.json','provenance.json','run.log','executed_source.json.gz')}
        (folder/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS',mode,ranks,flush=True)
if __name__=='__main__':main()
