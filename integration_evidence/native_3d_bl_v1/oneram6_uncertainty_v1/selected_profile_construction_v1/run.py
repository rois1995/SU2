#!/usr/bin/env python3
"""One complete saved-state normalized construction attempt, never Solve/advance CFD."""
import gzip,json,os,subprocess,sys,time
from pathlib import Path
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE.parent));import run as common
ROOT=common.ROOT;folder=HERE/'m6_budget300000_np4';folder.mkdir(exist_ok=True)
source=HERE.parent/'adaptive_budget_evidence_v1/m6_growth2_budget300000_np4'
prior=json.loads((source/'provenance.json').read_text())['pins']
for name in ('Coarse.su2','Coarse.su2.native_ref','donor_flow.dat'):
 origin=source/name;assert common.sha(origin)==prior[str(origin.relative_to(ROOT))]
 dest=folder/name
 if not dest.exists():dest.symlink_to(os.path.relpath(origin.resolve(),folder))
 assert ROOT in dest.resolve().parents and common.sha(dest)==common.sha(origin)
config=folder/'run.cfg'
if config.exists():assert config.read_bytes()==(source/'run.cfg').read_bytes()
else:config.write_bytes((source/'run.cfg').read_bytes())
paths=[ROOT/p for p in ('UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp','UnitTests/SU2_CFD/adaptation/fixtures/native_saved_metric_probe.inc',
 'Common/src/adaptation/CNativeRemesher3D.cpp','SU2_CFD/src/drivers/CSinglezoneDriver.cpp','SU2_CFD/include/adaptation/CNativeNormalizationStage3D.hpp',
 'SU2_CFD/src/adaptation/CNativeComplexity3D.cpp','SU2_CFD/include/adaptation/CNativeComplexity3D.hpp')]+[Path(__file__).resolve()]
paths += [p for directory in ('Common/include/adaptation','Common/src/adaptation') for p in (ROOT/directory).glob('CNative*3D.*')]
pins=common.pins();pins.update({str(p.relative_to(ROOT)):common.sha(p) for p in paths+[config]+[folder/n for n in ('Coarse.su2','Coarse.su2.native_ref','donor_flow.dat')]})
identity={'pins':pins,'prior_selection_receipt_sha256':common.sha(source/'result.json'),'CFD_steps':0,'ranks':4}
proof=folder/'provenance.json'
if proof.exists():assert json.loads(proof.read_text())==identity
else:proof.write_text(json.dumps(identity,indent=2)+'\n')
capsule=json.dumps({str(p.relative_to(ROOT)):p.read_text() for p in paths},sort_keys=True).encode();archive=folder/'executed_source.json.gz'
if archive.exists():assert gzip.decompress(archive.read_bytes())==capsule
else:archive.write_bytes(gzip.compress(capsule,mtime=0))
output=folder/'result.json'
if output.exists():
 result=json.loads(output.read_text());assert result['provenance_sha256']==common.sha(proof) and result['log_sha256']==common.sha(folder/'run.log')
 print('REUSE',json.dumps(result),flush=True);sys.exit(0)
assert not (folder/'live.json').exists(),'Inspect the recorded process before restarting unfinished work.'
command=['taskset','-c','0-3','nice','-n','19','/usr/bin/time','-v','-o','resources.txt','timeout','--kill-after=10s','600s',
 'mpirun','-np','4','--bind-to','none',str(common.BINARY),'[NativeSavedMetric3D]','--abort','--reporter','compact']
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',TMPDIR=str(common.BUILD/'compile-tmp'),
 SU2_NATIVE_FROZEN_CONFIG=str(config),SU2_NATIVE_SAVED_CONSTRUCTION='1')
start=time.monotonic()
with (folder/'run.log').open('w') as log:
 child=subprocess.Popen(command,cwd=folder,env=env,stdout=log,stderr=subprocess.STDOUT)
 (folder/'live.json').write_text(json.dumps({'pid':child.pid,'command':command,'started':time.time()},indent=2)+'\n');code=child.wait()
result={'exit_code':code,'elapsed_seconds':time.monotonic()-start,'command':command,'provenance_sha256':common.sha(proof),
 'log_sha256':common.sha(folder/'run.log'),'resource_sha256':common.sha(folder/'resources.txt'),'CFD_steps':0}
for name in ('saved_metric_budget.json','selected_construction.json'):
 if (folder/name).exists():result[name]=json.loads((folder/name).read_text())
result['owned_sensor_files']={p.name:common.sha(p) for p in folder.glob('selected_sensor_rank*.bin')}
output.write_text(json.dumps(result,indent=2)+'\n');(folder/'live.json').rename(folder/'terminal_process.json')
print(json.dumps(result,indent=2),flush=True)
assert code==0,'Inspect preserved failure; no automatic restart.'
assert 'selected_construction.json' in result
