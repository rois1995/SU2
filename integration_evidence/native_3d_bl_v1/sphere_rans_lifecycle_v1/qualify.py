#!/usr/bin/env python3
"""Focused exact/protocol witness controls and one meaningful existing RANS compatibility batch."""
import gzip,json,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
SHARED=HERE.with_name('oneram6_uncertainty_v1');sys.path.insert(0,str(SHARED));import run
mode=sys.argv[1];assert mode in ['serial','mpi2','mpi4','rans_mpi4']
ranks=1 if mode=='serial' else 2 if mode=='mpi2' else 4
witness=HERE/'donor_witness_v1/witness.dat'
sources=[p for d in ['Common/include/adaptation','Common/src/adaptation'] for p in (ROOT/d).glob('*3D.*')]
sources += [ROOT/p for p in ['UnitTests/Common/adaptation/CNativeMesh3D_tests.cpp','UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp','UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp','UnitTests/SU2_CFD/adaptation/fixtures/native_sphere_bl_runtime.inc','UnitTests/SU2_CFD/adaptation/fixtures/native_normalization_stages.inc','SU2_CFD/src/drivers/CSinglezoneDriver.cpp']]
sources += [Path(__file__).resolve()]
base=run.pins;run.pins=lambda:dict(base(),**{str(p.relative_to(ROOT)):run.sha(p) for p in sources})
folder=HERE/'query_controls_v2'/mode;folder.mkdir(parents=True,exist_ok=True)
capsule=json.dumps({str(p.relative_to(ROOT)):p.read_text() for p in sources},sort_keys=True).encode();archive=folder/'executed_source.json.gz'
if archive.exists():assert gzip.decompress(archive.read_bytes())==capsule
else:archive.write_bytes(gzip.compress(capsule,mtime=0))
filters='[NativeMetricFootprint3D],[NativePointSamples3D],[NativeRegionField3D],[NativeRegionFieldReplay3D],[NativeContact3D],Native 3D donor footprint uses exact closed segments including point-only contact,Native closed-query filters settle containment without weakening crossing or tiny signs'
env={'SU2_NATIVE_REGION_FIELD_REPLAY':str(witness)}
if mode=='rans_mpi4':
 filters='[NativeBLPolicyRuntime3D],[NativeBLBudgetRuntime3D]';env={'SU2_NATIVE_POLICY_WORKERS':'2'}
result=run.invoke(folder,env,witness,ranks,test_filter=filters)
print('QUALIFIED',mode,result['exit_code'],flush=True)
