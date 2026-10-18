#!/usr/bin/env python3
"""Actual original-family budget selection from the closed M6 restart; never Solve/advance CFD."""
import gzip,json,os,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE.parent));import run
ROOT=run.ROOT
budget=int(sys.argv[1]);growth=float(sys.argv[2]);ranks=int(sys.argv[3]);expected=sys.argv[4]
assert budget>0 and growth>=1.2 and ranks in (2,4) and expected in ('conflict','selected')
version='' # Fresh producer namespace; historical receipts remain untouched.
folder=HERE/('m6_growth%g_budget%d_np%d%s'%(growth,budget,ranks,version));folder.mkdir(exist_ok=True)
source=HERE.parent/'construction_first_trial_v1';old=json.loads((source/'provenance.json').read_text())['identity']
inputs=[]
for name in ('Coarse.su2','Coarse.su2.native_ref','donor_flow.dat'):
 origin=source/name;assert run.sha(origin)==old[str(origin.relative_to(ROOT))]
 dest=folder/name
 if not dest.exists():dest.symlink_to(os.path.relpath(origin.resolve(),folder))
 assert ROOT in dest.resolve().parents and run.sha(dest)==run.sha(origin);inputs.append(dest)
remove={'ADAP_LOOP','ADAP_SIZES','ADAP_SUBITER','ADAP_FLOW_ITER','ADAP_FLOW_CFL','ADAP_COMPLEXITY','ADAP_NATIVE_RANKS'}
lines=[line for line in (source/'run.cfg').read_text().splitlines() if line.split('=',1)[0].strip() not in remove]
text='\n'.join(lines)+'\nADAP_LOOP= NO\nADAP_COMPLEXITY= %d\nADAP_NATIVE_RANKS= %d\nADAP_BL_BUDGET= YES\nADAP_BL_PROTECT_DISTANCE= 2E-6\nADAP_BL_OUTER_GROWTH_MAX= %.17g\n'%(budget,ranks,growth)
config=folder/'run.cfg'
if config.exists():assert config.read_text()==text
else:config.write_text(text)
sources=[ROOT/p for p in ('Common/include/CConfig.hpp','Common/src/CConfig.cpp','Common/include/adaptation/CComplexityNormalization.hpp',
 'SU2_CFD/include/adaptation/CNativeMetricFamily3D.hpp','SU2_CFD/include/adaptation/CNativeComplexity3D.hpp',
 'SU2_CFD/src/adaptation/CNativeComplexity3D.cpp','SU2_CFD/src/drivers/CSinglezoneDriver.cpp',
 'UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp','UnitTests/SU2_CFD/adaptation/fixtures/native_saved_metric_probe.inc')]+[Path(__file__).resolve()]
base=run.pins
run.pins=lambda:dict(base(),**{str(p.relative_to(ROOT)):run.sha(p) for p in sources+inputs+[config]})
archive=folder/'executed_source.json.gz';capsule=json.dumps({str(p.relative_to(ROOT)):p.read_text() for p in sources},sort_keys=True).encode()
if archive.exists():assert gzip.decompress(archive.read_bytes())==capsule
else:archive.write_bytes(gzip.compress(capsule,mtime=0))
result=run.invoke(folder,{'SU2_NATIVE_FROZEN_CONFIG':str(config)},run.OLD/'cell329/replay.dat',ranks,
 test_filter='[NativeSavedMetric3D]',expected_failure=expected=='conflict')
log=(folder/'run.log').read_text()
if expected=='conflict':assert 'Estimated BL budget conflict' in log
else:
 saved=json.loads((folder/'saved_metric_budget.json').read_text());assert saved['CFD_steps']==0 and saved['frozen']
 assert saved['flow_and_turbulence_unchanged'] and saved['reference_unchanged'] and not saved['mesh_construction_attempted']
print('OBSERVED',expected,budget,growth,ranks,result['exit_code'],flush=True)
