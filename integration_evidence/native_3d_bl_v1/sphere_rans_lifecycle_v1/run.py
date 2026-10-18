#!/usr/bin/env python3
"""One bounded actual RANS event; resume only complete stages with identical provenance."""
import gzip,json,os,subprocess,sys,time
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];BUILD=ROOT/'integration_evidence/build-integrated-v2'
SHARED=HERE.with_name('oneram6_uncertainty_v1');sys.path.insert(0,str(SHARED));import run as shared
sha=shared.sha

def main():
 stage,workers,ranks=sys.argv[1],int(sys.argv[2]),int(sys.argv[3])
 assert stage in ('construct','update','release') and ranks in (1,2,4) and 1<=workers<=ranks
 case=sys.argv[4] if len(sys.argv)>4 else 'n%d_m%d'%(ranks,workers)
 assert Path(case).name==case and case not in ('.','..')
 folder=HERE/case/stage;folder.mkdir(parents=True,exist_ok=True)
 old=HERE.with_name('closed_sphere_construction_v1')/'geodesic_v1'
 text=(old/'run.cfg').read_text()
 text=text.replace('ADAP_NATIVE_RANKS= 1','ADAP_NATIVE_RANKS= '+str(workers))
 text+='''ADAP_NATIVE_REPARTITION= YES
WRT_PERFORMANCE= YES
TIME_DOMAIN= YES
TIME_STEP= 1e-3
TIME_ITER= 10
TIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER
INNER_ITER= 2
MESH_OUT_FORMAT= SU2
RESTART_FILENAME= checkpoint
SOLUTION_FILENAME= checkpoint
CONV_FILENAME= history
WRT_RESTART_COMPACT= YES
'''
 inputs=[]
 if stage=='construct':
  for name in ['sphere.su2','sphere.su2.native_ref']:
   p=folder/name;target=(old/name).resolve()
   if not p.exists():p.symlink_to(os.path.relpath(target,p.parent))
   assert sha(p)==sha(target);inputs.append(p)
 else:
  previous=folder.with_name('construct');proof=previous/'provenance.json';result=json.loads((previous/'result.json').read_text())
  assert result['exit_code']==0 and result['provenance_sha256']==sha(proof) and result['log_sha256']==sha(previous/'run.log')
  pins=json.loads(proof.read_text())['pins']
  previous_source=json.loads(gzip.decompress((previous/'executed_source.json.gz').read_bytes()))
  # A completed checkpoint is immutable input, not a claim that its producer is the current binary.
  import hashlib
  for name,wanted in pins.items():
   if name in previous_source:assert hashlib.sha256(previous_source[name].encode()).hexdigest()==wanted,name
   elif name!=str(shared.BINARY.relative_to(ROOT)):assert sha(ROOT/name)==wanted,name
  for name,wanted in result['artifacts'].items():assert sha(previous/name)==wanted,name
  independent=json.loads((previous/'independent.json').read_text())
  assert independent['status']=='PASS_ACTUAL_RANS_STATE_TRANSFER_RESTART_AND_OUTPUT'
  if stage=='release':text='\n'.join(line for line in text.splitlines() if not line.startswith('ADAP_BL_'))+'\n'
  text=text.replace('MESH_FILENAME= sphere.su2','MESH_FILENAME= ../construct/event_written_CGNS.cgns').replace('MESH_FORMAT= SU2','MESH_FORMAT= CGNS')
  text=text.replace('SOLUTION_FILENAME= checkpoint','SOLUTION_FILENAME= ../construct/checkpoint')
  text+='RESTART_SOL= YES\nRESTART_ITER= 3\n'
  inputs=[previous/name for name in ['event_written_CGNS.cgns','event_written_CGNS.cgns.native_ref','event_states.json','accepted_turb.csv']]
  inputs+=sorted(previous.glob('checkpoint*'))
  assert len(inputs)>4
 config=folder/'run.cfg'
 if config.exists():assert config.read_text()==text,'Preserve changed/failed attempt'
 else:config.write_text(text)
 sources=[p for d in ['Common/include/adaptation','Common/src/adaptation'] for p in (ROOT/d).glob('*3D.*')]
 sources += [ROOT/p for p in ['UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp','UnitTests/SU2_CFD/adaptation/fixtures/native_sphere_bl_runtime.inc','SU2_CFD/src/drivers/CSinglezoneDriver.cpp','SU2_CFD/src/adaptation/CConservativeTransfer.cpp','SU2_CFD/src/adaptation/CSolutionTransfer.cpp','SU2_CFD/include/adaptation/CConservativeTransfer.hpp'] if (ROOT/p).exists()]
 sources += [Path(__file__).resolve()]
 identity={'pins':dict(shared.pins(),**{str(p.relative_to(ROOT)):sha(p) for p in sources}),
   'inputs':{str(p.relative_to(ROOT)):sha(p) for p in inputs+[config]},'stage':stage,'ranks':ranks,'workers':workers}
 if stage!='construct':identity['checkpoint_producer']={'provenance_sha256':sha(previous/'provenance.json'),'result_sha256':sha(previous/'result.json'),'independent_sha256':sha(previous/'independent.json'),'binary_sha256':pins[str(shared.BINARY.relative_to(ROOT))],'current_binary_matches':sha(shared.BINARY)==pins[str(shared.BINARY.relative_to(ROOT))]}
 proof=folder/'provenance.json'
 if proof.exists():assert json.loads(proof.read_text())==identity,'Provenance changed; preserve old attempt'
 else:proof.write_text(json.dumps(identity,indent=2)+'\n')
 result=folder/'result.json'
 if result.exists():
  record=json.loads(result.read_text());assert record['exit_code']==0 and record['provenance_sha256']==sha(proof) and record['log_sha256']==sha(folder/'run.log')
  for name,wanted in record['artifacts'].items():assert sha(folder/name)==wanted,name
  print('REUSE',folder,flush=True);return 0
 capsule=json.dumps({str(p.relative_to(ROOT)):p.read_text() for p in sources},sort_keys=True).encode()
 (folder/'executed_source.json.gz').write_bytes(gzip.compress(capsule,mtime=0))
 env=dict(os.environ,OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',TMPDIR=str(BUILD/'compile-tmp'),SU2_NATIVE_SPHERE_BL_CFG='run.cfg')
 command=['/usr/bin/time','-v','nice','-n','19','taskset','-c','0' if ranks==1 else '0-'+str(ranks-1),'timeout','--kill-after=5s','420s']
 if ranks>1:command+=['mpirun','--bind-to','none','-np',str(ranks)]
 command+=[str(shared.BINARY),'[NativeSphereBLRuntime3D]','--abort','--reporter','compact']
 start=time.monotonic()
 with (folder/'run.log').open('w') as log:r=subprocess.run(command,cwd=folder,env=env,stdout=log,stderr=subprocess.STDOUT)
 record={'exit_code':r.returncode,'seconds':time.monotonic()-start,'command':command,'provenance_sha256':sha(proof),'log_sha256':sha(folder/'run.log'),
   'artifacts':{p.name:sha(p) for p in folder.iterdir() if p.is_file() and p.name not in ['result.json','run.log','provenance.json']}}
 result.write_text(json.dumps(record,indent=2)+'\n');print(folder,json.dumps({k:v for k,v in record.items() if k!='artifacts'}),flush=True)
 return r.returncode
if __name__=='__main__':raise SystemExit(main())
