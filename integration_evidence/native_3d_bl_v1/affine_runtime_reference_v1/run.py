#!/usr/bin/env python3
"""Bounded actual-import/authority checks; this is not an adapted oblique lifecycle."""
import gzip,importlib.util,json,sys
from fractions import Fraction as F
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
spec=importlib.util.spec_from_file_location('saved_replay',HERE.with_name('oneram6_uncertainty_v1')/'run.py')
replay=importlib.util.module_from_spec(spec);spec.loader.exec_module(replay)

def main():
    ranks=int(sys.argv[1]);assert ranks in (1,2,4)
    version=sys.argv[2] if len(sys.argv)>2 else 'v4';assert version.startswith('v') and version[1:].isdigit()
    folder=HERE/('serial_'+version if ranks==1 else 'mpi%d_'%ranks+version);folder.mkdir(exist_ok=True)
    sources=[]
    for directory in ('Common/include/adaptation','Common/src/adaptation','SU2_CFD/include/adaptation','SU2_CFD/src/adaptation'):
        sources+=list((ROOT/directory).glob('*3D.*'))
    sources += [ROOT/p for p in ('Common/include/adaptation/CNativeAffineReference.hpp','Common/src/adaptation/CNativeAffineReference.cpp',
        'Common/include/adaptation/CNativeGeometryValidation2D.hpp','Common/src/adaptation/CNativePredicates2D.cpp',
        'Common/src/meson.build','UnitTests/meson.build','UnitTests/Common/adaptation/CNativeAffineReference_tests.cpp',
        'UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp','SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp',
        'UnitTests/Common/adaptation/CNativeField3D_tests.cpp','UnitTests/SU2_CFD/adaptation/TransferTestCase.hpp',
        'UnitTests/Common/adaptation/SimplexMeshTestCase.hpp','Common/include/adaptation/CSimplexMesh.hpp','Common/include/adaptation/CTransferMemory.hpp')]
    sources += list((ROOT/'UnitTests/SU2_CFD/adaptation/fixtures').glob('*.inc'))+[Path(__file__).resolve()]
    sources=sorted(set(sources));old=replay.pins
    replay.pins=lambda:dict(old(),**{str(p.relative_to(ROOT)):replay.sha(p) for p in sources})
    capsule=folder/'executed_source.json.gz';payload=gzip.compress(json.dumps({str(p.relative_to(ROOT)):p.read_text() for p in sources},sort_keys=True).encode(),mtime=0)
    if capsule.exists():assert capsule.read_bytes()==payload,'Source changed; use a new folder'
    else:capsule.write_bytes(payload)
    tag='[NativeAffineImport3D],[NativeReference3D],[NativeCurvedCoverage3D]'
    if ranks==1:tag+=',[NativeAffineGeometry],[NativePlanarUnion3D]'
    result=replay.invoke(folder,{},HERE.with_name('planar_symmetry_reference_v1')/'initial.planes',ranks,test_filter=tag)
    if 'artifacts' in result:assert all(replay.sha(folder/p)==h for p,h in result['artifacts'].items()),'Saved artifact changed'
    if ranks==1:
        rows=json.loads((folder/'affine_lift_samples.json').read_text());assert len(rows)==320
        nonzero=0
        for row in rows:
            n=list(map(F,row['normal']));o=list(map(F,row['origin']));p=list(map(F,row['point']));bound=F(row['bound'])
            residual=sum(n[k]*(p[k]-o[k]) for k in range(row['dimension']));square=sum(x*x for x in n)
            assert bound>0 and residual*residual<=bound*bound*square
            nonzero+=residual!=0
        assert nonzero
        (folder/'independent_lift.json').write_text(json.dumps(dict(status='EXACT_RATIONAL_LIFT_PASS',samples=len(rows),nonzero_equation_residuals=nonzero,
            samples_sha256=replay.sha(folder/'affine_lift_samples.json'),audit_source_sha256=replay.sha(Path(__file__))),indent=2)+'\n')
    result['scope']='Actual Euler/RANS geometry import, frozen original sensor support/priority and owner/private queries, native-ref codec. No complete oblique adaptation/transfer/restart/exterior qualification.'
    result['artifacts']={p.name:replay.sha(p) for p in folder.iterdir() if p.is_file() and p.name not in ('result.json','provenance.json','run.log')}
    (folder/'result.json').write_text(json.dumps(result,indent=2)+'\n');print('PASS',ranks)
if __name__=='__main__':main()
