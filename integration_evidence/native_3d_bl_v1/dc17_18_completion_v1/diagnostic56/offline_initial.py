"""Run the saved-state serial experiment with immutable inputs; no new MPI prefix."""
import json,os,subprocess,time
from pathlib import Path
from run import ROOT,REPO,PREVIOUS,sha,save

def main():
    folder=ROOT/'offline';assert not (folder/'status.json').exists(),'Inspect terminal status before retrying'
    folder.mkdir(exist_ok=True)
    inputs=ROOT/'replay_input';pins={str(inputs/name):h for name,h in json.loads((inputs/'pins.json').read_text()).items()}
    pins.update(json.loads((ROOT/'build_source_pins.json').read_text()))
    pins.update(json.loads((ROOT.parent/'validation54/m6_euler/loaded_runtime_pins.json').read_text()))
    for name,h in pins.items():assert sha(name)==h,name
    binary=REPO/'integration_evidence/build-integrated-v2/UnitTests/test_driver'
    authority=PREVIOUS/'observed/m6.su2.native_ref'
    assert sha(authority)==json.loads((PREVIOUS/'input_manifest.json').read_text())['reference_sha256']
    for p in [binary,authority,Path(__file__)]:pins[str(p)]=sha(p)
    save(folder/'pins.json',pins)
    env=os.environ.copy();env.update(OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',
        SU2_NATIVE_TRANSACTION_REPLAY=str(inputs),SU2_NATIVE_TRANSACTION_REFERENCE=str(authority),SU2_NATIVE_TRANSACTION_HAUSD='0.0001')
    command=['nice','-n','19','/usr/bin/time','-v','timeout','90',str(binary),'[NativeTransactionReplay3D]']
    save(folder/'command.json',dict(argv=command,environment={k:v for k,v in env.items() if k.startswith(('OMP_','OPENBLAS_','MKL_','SU2_NATIVE_'))}))
    start=time.time()
    with (folder/'run.log').open('w') as log:
        p=subprocess.Popen(command,cwd=folder,env=env,stdout=log,stderr=subprocess.STDOUT)
        save(folder/'status.json',dict(state='running',pid=p.pid,start=start));code=p.wait()
    changed=[name for name,h in pins.items() if sha(name)!=h]
    result=dict(state='terminal',exit_code=code,seconds=time.time()-start,pins_verified=not changed,changed=changed)
    save(folder/'status.json',result);print(json.dumps(result));assert not changed,changed
    raise SystemExit(code)

if __name__=='__main__':main()
