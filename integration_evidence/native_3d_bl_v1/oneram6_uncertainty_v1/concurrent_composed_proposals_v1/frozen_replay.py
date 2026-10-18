#!/usr/bin/env python3
"""One exact selected-profile M6 construction replay; no metric/Hessian/normalization rebuild."""
import gzip,json,os,subprocess,sys,time
from pathlib import Path
HERE=Path(__file__).resolve().parent;sys.path.insert(0,str(HERE.parent));import run
ROOT=run.ROOT;folder=HERE/'frozen_m6_np4';folder.mkdir(exist_ok=True)
source=HERE.parent/'selected_profile_construction_v1/m6_budget300000_np4'
manifest=json.loads((source/'selected_field_manifest.json').read_text())
assert manifest['sensor_only_original_P1'] and not manifest['clipped_sensor_rescaling_permitted']
assert run.sha(source/'selected_sensor.bin')==manifest['selected_sensor_sha256']
assert run.sha(source/'Coarse.su2')==manifest['source_mesh_sha256'] and run.sha(source/'run.cfg')==manifest['config_sha256']
inputs=[]
for name in ('Coarse.su2','Coarse.su2.native_ref','donor_flow.dat','selected_sensor.bin','selected_field_manifest.json'):
 origin=source/name;dest=folder/name
 if not dest.exists():dest.symlink_to(os.path.relpath(origin.resolve(),folder))
 assert ROOT in dest.resolve().parents and run.sha(dest)==run.sha(origin);inputs.append(dest)
config=folder/'run.cfg'
if config.exists():assert config.read_bytes()==(source/'run.cfg').read_bytes()
else:config.write_bytes((source/'run.cfg').read_bytes())
paths=[p for directory in ('Common/include/adaptation','Common/src/adaptation') for p in (ROOT/directory).glob('CNative*3D.*')]
paths += [ROOT/p for p in ('UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp','UnitTests/SU2_CFD/adaptation/CNativeRemesher3D_tests.cpp',
 'UnitTests/SU2_CFD/adaptation/fixtures/native_saved_metric_probe.inc','SU2_CFD/src/drivers/CSinglezoneDriver.cpp')]+[Path(__file__).resolve()]
pins=run.pins();pins.update({str(p.relative_to(ROOT)):run.sha(p) for p in paths+inputs+[config]})
identity={'pins':pins,'source_construction_receipt_sha256':run.sha(source/'result.json'),'selected_profile':manifest,
 'CFD_steps':0,'metric_initialization_repeated':False,'original_family_correction_available':False,'ranks':4}
proof=folder/'provenance.json'
if proof.exists():assert json.loads(proof.read_text())==identity
else:proof.write_text(json.dumps(identity,indent=2)+'\n')
capsule=json.dumps({str(p.relative_to(ROOT)):p.read_text() for p in paths},sort_keys=True).encode();archive=folder/'executed_source.json.gz'
if archive.exists():assert gzip.decompress(archive.read_bytes())==capsule
else:archive.write_bytes(gzip.compress(capsule,mtime=0))
result=folder/'result.json'
if result.exists():
 r=json.loads(result.read_text());assert r['provenance_sha256']==run.sha(proof) and r['log_sha256']==run.sha(folder/'run.log')
 print('REUSE',json.dumps(r),flush=True);sys.exit(0)
assert not (folder/'live.json').exists(),'Inspect the actual recorded process before resuming.'
command=['taskset','-c','0-3','nice','-n','19','/usr/bin/time','-v','-o','resources.txt','timeout','--kill-after=10s','600s',
 'mpirun','-np','4','--bind-to','none',str(run.BINARY),'[NativeFrozenVolume3D]','--abort','--reporter','compact']
env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',TMPDIR=str(run.BUILD/'compile-tmp'),
 SU2_NATIVE_FROZEN_CONFIG=str(config),SU2_NATIVE_FROZEN_SENSOR3D=str(folder/'selected_sensor.bin'),
 SU2_NATIVE_FROZEN_BL_PARAMETER=str(manifest['profile_parameter']),SU2_NATIVE_FROZEN_BL_VERSION=str(manifest['profile_version']))
start=time.monotonic()
with (folder/'run.log').open('w') as log:
 child=subprocess.Popen(command,cwd=folder,env=env,stdout=log,stderr=subprocess.STDOUT)
 (folder/'live.json').write_text(json.dumps({'pid':child.pid,'command':command,'started':time.time()},indent=2)+'\n');code=child.wait()
r={'exit_code':code,'elapsed_seconds':time.monotonic()-start,'command':command,'provenance_sha256':run.sha(proof),
 'log_sha256':run.sha(folder/'run.log'),'resource_sha256':run.sha(folder/'resources.txt'),'CFD_steps':0,'metric_initialization_repeated':False}
if (folder/'construction_result.json').exists():r['construction']=json.loads((folder/'construction_result.json').read_text())
result.write_text(json.dumps(r,indent=2)+'\n');(folder/'live.json').rename(folder/'terminal_process.json')
print(json.dumps(r,indent=2),flush=True)
assert code==0,'Inspect preserved failure before another action.'
assert 'construction' in r
