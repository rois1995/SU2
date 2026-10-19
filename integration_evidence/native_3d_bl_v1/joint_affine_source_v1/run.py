#!/usr/bin/env python3
"""Affected source/import and construction checks; reuse the existing pinned replay launcher."""
import functools,gzip,importlib.util,json,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
helper=HERE.with_name('oneram6_uncertainty_v1')/'run.py'
spec=importlib.util.spec_from_file_location('replay',helper);replay=importlib.util.module_from_spec(spec);spec.loader.exec_module(replay)
replay.sha=functools.lru_cache(maxsize=None)(replay.sha)
FILTERS={
 'source':'[NativeAffineSourceGeometry],[NativeJointAffineSource3D],[NativeAffineGeometry],[NativeAffineImport3D]',
 'affected':'[NativeAffineInitialDriver3D],[NativeFirstLayer3D],[NativeFirstLayerRanking3D],[NativeFirstLayerEngine3D],[NativeFirstLayerBatch3D],[NativeMetricMove3D],[NativeWallReconstruction3D],[NativeConnectivityHeight3D]'}
def main():
 receipt=json.loads((HERE/'build_process.json').read_text())
 assert receipt['status']=='PASS' and receipt['exit_code']==0 and receipt['source_unchanged'],'Linked build must finish before checks'
 assert receipt['binary_sha256']==replay.sha(replay.BINARY),'Executable changed'
 assert all(replay.sha(ROOT/p)==h for p,h in receipt['source_sha256'].items()),'Compiled sources changed'
 mode=sys.argv[1] if len(sys.argv)>1 else 'source';ranks=int(sys.argv[2]) if len(sys.argv)>2 else 1
 assert mode in FILTERS and ranks in (1,2,4)
 folder=HERE/(mode+'_n'+str(ranks));folder.mkdir(exist_ok=True)
 sources=[ROOT/p for p in receipt['source_sha256']]+[Path(__file__).resolve(),helper,
  ROOT/'UnitTests/SU2_CFD/adaptation/TransferTestCase.hpp',ROOT/'Common/include/adaptation/CNativeField3D.hpp',
  ROOT/'Common/src/adaptation/CNativeField3D.cpp',ROOT/'Common/include/adaptation/CNativeDonor3D.hpp',ROOT/'Common/src/adaptation/CNativeDonor3D.cpp',
  ROOT/'Common/include/adaptation/CNativePartition3D.hpp',ROOT/'Common/src/adaptation/CNativePartition3D.cpp']
 source_pins={str(p.relative_to(ROOT)):replay.sha(p) for p in sorted(set(sources))}
 previous=replay.pins;replay.pins=lambda:dict(previous(),**source_pins)
 payload=gzip.compress(json.dumps({str(p.relative_to(ROOT)):p.read_text() for p in sorted(set(sources))},sort_keys=True).encode(),mtime=0)
 capsule=folder/'executed_source.json.gz'
 if capsule.exists():assert capsule.read_bytes()==payload,'Source changed: preserve this folder and select a new version'
 else:capsule.write_bytes(payload)
 fixture=HERE.with_name('affine_initial_adoption_v1')/'initial.planes'
 result=replay.invoke(folder,{},fixture,ranks,test_filter=FILTERS[mode])
 artifacts={p.name:replay.sha(p) for p in folder.iterdir() if p.is_file() and p.name not in ('result.json','run.log','provenance.json','executed_source.json.gz')}
 if 'artifacts' in result:assert result['artifacts']==artifacts,'Saved case artifacts changed'
 else:
  result['artifacts']=artifacts;(folder/'result.json').write_text(json.dumps(result,indent=2)+'\n')
 print('PASS',mode,ranks,folder,flush=True)
if __name__=='__main__':main()
