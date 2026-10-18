#!/usr/bin/env python3
"""Resumable affected controls and difficult oblique pilot; no CFD iteration/full regression."""
import gzip,importlib.util,json,sys,functools
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
path=HERE.with_name('oneram6_uncertainty_v1')/'run.py';spec=importlib.util.spec_from_file_location('replay',path)
replay=importlib.util.module_from_spec(spec);spec.loader.exec_module(replay)
# One process handles immutable pinned sources; avoid hashing the large executable repeatedly.
replay.sha=functools.lru_cache(maxsize=None)(replay.sha)
def main():
    receipt=json.loads((HERE/'build_provenance.json').read_text())
    assert receipt['status']=='PASS' and receipt.get('binary_sha256')==replay.sha(replay.BINARY),'Build must finish and be sealed before running'
    assert all(replay.sha(ROOT/p)==v for p,v in receipt['source'].items()),'Compiled source changed'
    mode=sys.argv[1] if len(sys.argv)>1 else 'controls';ranks=int(sys.argv[2]) if len(sys.argv)>2 else 1
    version=sys.argv[3] if len(sys.argv)>3 else 'v1'
    assert mode in ('controls','annulus','compatibility') and ranks in (1,2,4) and version.startswith('v') and version[1:].isdigit()
    folder=HERE/(mode+'_'+version+'_n'+str(ranks));folder.mkdir(exist_ok=True)
    sources=[]
    for directory in ('Common/include/adaptation','Common/src/adaptation','SU2_CFD/include/adaptation','SU2_CFD/src/adaptation'):
        sources+=list((ROOT/directory).glob('*3D.*'))
    sources+=[ROOT/p for p in ('Common/include/adaptation/CNativeAffineReference.hpp','Common/src/adaptation/CNativeAffineReference.cpp',
        'Common/include/CConfig.hpp','Common/src/CConfig.cpp','SU2_CFD/src/drivers/CDriver.cpp',
        'UnitTests/Common/adaptation/CNativeAffineReference_tests.cpp','UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp',
        'SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp','SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp',
        'Common/include/adaptation/CDistributedSearch.hpp','Common/src/adaptation/CDistributedSearch.cpp')]
    sources+=list((ROOT/'UnitTests/SU2_CFD/adaptation/fixtures').glob('*.inc'))+[Path(__file__).resolve()]
    sources=sorted(set(sources));original=replay.pins
    replay.pins=lambda:dict(original(),**{str(p.relative_to(ROOT)):replay.sha(p) for p in sources})
    payload=gzip.compress(json.dumps({str(p.relative_to(ROOT)):p.read_text() for p in sources},sort_keys=True).encode(),mtime=0)
    capsule=folder/'executed_source.json.gz'
    if capsule.exists():assert capsule.read_bytes()==payload,'Source changed: use a new folder'
    else:capsule.write_bytes(payload)
    parameters={};tag='[NativeAffineGeometry],[NativeAffineInitialDriver3D],[NativeAffineImport3D]'
    if mode in ('annulus','compatibility'):
        parameters['SU2_NATIVE_SYMMETRY_ANNULUS']='1';tag='[NativeSymmetryRuntime3D]'
        if mode=='annulus':parameters['SU2_NATIVE_SYMMETRY_OBLIQUE']='1'
    result=replay.invoke(folder,parameters,HERE/'initial.planes',ranks,test_filter=tag)
    if 'artifacts' in result:
        assert all(replay.sha(folder/p)==v for p,v in result['artifacts'].items())
    else:
        result['artifacts']={p.name:replay.sha(p) for p in folder.iterdir() if p.is_file() and p.name not in ('result.json','run.log','provenance.json','executed_source.json.gz')}
        (folder/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS',mode,ranks,folder,flush=True)
if __name__=='__main__':main()
