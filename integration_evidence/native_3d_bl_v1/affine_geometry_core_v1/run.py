#!/usr/bin/env python3
"""One focused serial core check with exact lifting audit; no CFD/adaptation claim."""
import gzip,hashlib,importlib.util,json,os,sys
from fractions import Fraction as F
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
spec=importlib.util.spec_from_file_location('saved_replay',HERE.with_name('oneram6_uncertainty_v1')/'run.py')
replay=importlib.util.module_from_spec(spec);spec.loader.exec_module(replay)

def main():
    folder=HERE/sys.argv[1];folder.mkdir(exist_ok=True)
    sources=[ROOT/p for p in ('Common/include/adaptation/CNativeAffineReference.hpp','Common/src/adaptation/CNativeAffineReference.cpp',
        'Common/include/adaptation/CNativeBoundary3D.hpp','Common/src/adaptation/CNativeBoundary3D.cpp',
        'Common/include/adaptation/CNativeGeometryValidation2D.hpp','Common/include/adaptation/CNativeMesh2D.hpp',
        'Common/src/adaptation/CNativePredicates2D.cpp','Common/src/adaptation/CNativePredicates3D.cpp',
        'Common/src/meson.build','UnitTests/meson.build','UnitTests/Common/adaptation/CNativeAffineReference_tests.cpp',
        'UnitTests/Common/adaptation/CNativeBoundary3D_tests.cpp','UnitTests/Common/adaptation/CNativeGeometryValidation2D_tests.cpp')]+[Path(__file__).resolve()]
    old=replay.pins;replay.pins=lambda:dict(old(),**{str(p.relative_to(ROOT)):replay.sha(p) for p in sources})
    capsule=folder/'executed_source.json.gz';payload=gzip.compress(json.dumps({str(p.relative_to(ROOT)):p.read_text() for p in sources},sort_keys=True).encode(),mtime=0)
    if capsule.exists():assert capsule.read_bytes()==payload,'Source changed; use a new folder'
    else:capsule.write_bytes(payload)
    result=replay.invoke(folder,{},HERE.with_name('planar_symmetry_reference_v1')/'initial.planes',1,
        test_filter='[NativeAffineGeometry],[NativePlanarUnion3D],[NativeReferenceSearch3D],[NativeGeometry]')
    records=json.loads((folder/'affine_lift_samples.json').read_text());assert len(records)==320
    worst=0.;nonzero=0
    for row in records:
        n=list(map(F,row['normal']));o=list(map(F,row['origin']));p=list(map(F,row['point']));bound=F(row['bound'])
        residual=sum(n[k]*(p[k]-o[k]) for k in range(row['dimension']));square=sum(x*x for x in n)
        assert bound>0 and residual*residual<=bound*bound*square,'Outward physical error bound failed'
        nonzero+=residual!=0;worst=max(worst,float(residual*residual/(bound*bound*square)))
    assert nonzero>0,'Must exercise rounded points that are not on the exact mathematical plane/line'
    audit={'status':'EXACT_RATIONAL_LIFTING_AUDIT_PASS','samples':len(records),'nonzero_exact_equation_residuals':nonzero,
        'maximum_squared_distance_to_bound_ratio':worst,'scope':'Exact rational squared Euclidean point-to-authority distance versus reported physical bound, for saved samples',
        'limits':['Geometry kernel only; no new CFD/donor/transfer/restart/MPI/exterior workflow qualification'],
        'sample_sha256':replay.sha(folder/'affine_lift_samples.json'),'audit_sha256':replay.sha(Path(__file__))}
    (folder/'independent_lift.json').write_text(json.dumps(audit,indent=2)+'\n')
    result['artifacts']={p.name:replay.sha(p) for p in folder.iterdir() if p.is_file() and p.name not in ('result.json','run.log','provenance.json')}
    (folder/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(audit,indent=2))
if __name__=='__main__':main()
