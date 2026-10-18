#!/usr/bin/env python3
"""Bounded current-binary protected-demand preflight; no remeshing or CFD advance."""
import gzip,json,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.path.insert(0,str(HERE.with_name('oneram6_uncertainty_v1')));import run
folder=HERE/'cylinder_floor_n1'
sources=[p for directory in ['Common/include/adaptation','Common/src/adaptation','SU2_CFD/include/adaptation','SU2_CFD/src/adaptation'] for p in (ROOT/directory).glob('*3D.*')]
sources += [ROOT/p for p in ['UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp','SU2_CFD/src/drivers/CSinglezoneDriver.cpp','SU2_CFD/src/solvers/CSolver.cpp']]
sources += list((ROOT/'UnitTests/SU2_CFD/adaptation/fixtures').glob('*.inc'))+[Path(__file__).resolve()]
base=run.pins;run.pins=lambda:dict(base(),**{str(p.relative_to(ROOT)):run.sha(p) for p in sources},**{str(p.relative_to(ROOT)):run.sha(p) for p in [folder/'run.cfg',folder/'input.su2.native_ref']})
(folder/'executed_source.json.gz').write_bytes(gzip.compress(json.dumps({str(p.relative_to(ROOT)):p.read_text() for p in sources},sort_keys=True).encode(),mtime=0))
result=run.invoke(folder,{'SU2_NATIVE_FROZEN_CONFIG':'run.cfg'},folder/'input.su2',1,test_filter='[NativeSavedMetric3D]',expected_failure=True)
log=(folder/'run.log').read_text();assert 'Estimated BL budget conflict' in log and 'protected_minimum_exceeds_budget=1' in log,'Unexpected refusal'
print('ESTIMATED_PROTECTED_BUDGET_CONFLICT_NO_CONSTRUCTION',result['exit_code'],flush=True)
