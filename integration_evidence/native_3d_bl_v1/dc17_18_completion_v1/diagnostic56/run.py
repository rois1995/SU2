"""One bounded six-rank capture, preserving source/input/runtime pins and owned-job receipts."""
import hashlib
import json
import os
import subprocess
import time
from pathlib import Path
ROOT=Path(__file__).resolve().parent
REPO=ROOT.parents[3]
PREVIOUS=ROOT.parent/'diagnostic55'

def sha(p):
    h=hashlib.sha256()
    with open(p,'rb') as f:
        for block in iter(lambda:f.read(1048576),b''):h.update(block)
    return h.hexdigest()

def save(path,value):path.write_text(json.dumps(value,indent=2)+'\n')

def main():
    folder=ROOT/'capture'
    assert not (folder/'status.json').exists(), 'Inspect saved PID/session before restarting'
    folder.mkdir(exist_ok=True)
    manifest=json.loads((PREVIOUS/'input_manifest.json').read_text())
    for name,key in [('m6.su2','mesh_sha256'),('m6.su2.native_ref','reference_sha256')]:
        target=PREVIOUS/'observed'/name
        assert sha(target)==manifest[key],name
        p=folder/name
        if not p.exists():p.symlink_to(target)
    assert sha(PREVIOUS/'sensor_original.bin')==manifest['sensor_sha256']
    config=(PREVIOUS/'observed/run.cfg').read_bytes()
    (folder/'run.cfg').write_bytes(config)
    pins=json.loads((ROOT/'build_source_pins.json').read_text())
    pins.update(json.loads((ROOT.parent/'validation54/m6_euler/loaded_runtime_pins.json').read_text()))
    for name,expected in pins.items():assert sha(name)==expected,name
    binary=REPO/'integration_evidence/build-integrated-v2/UnitTests/test_driver'
    for p in [binary,folder/'run.cfg',folder/'m6.su2',folder/'m6.su2.native_ref',PREVIOUS/'sensor_original.bin',Path(__file__)]:pins[str(p)]=sha(p)
    save(folder/'pins.json',pins)
    env=os.environ.copy();env.update(OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1',
        SU2_NATIVE_FROZEN_CONFIG=str(folder/'run.cfg'),SU2_NATIVE_FROZEN_SENSOR3D=str(PREVIOUS/'sensor_original.bin'),
        SU2_NATIVE_TRANSACTION_CAPTURE='1')
    command=['nice','-n','19','/usr/bin/time','-v','timeout','240','mpirun','--bind-to','none','-n','6',str(binary),'[NativeFrozenVolume3D]']
    save(folder/'command.json',dict(argv=command,environment={k:v for k,v in env.items() if k.startswith(('SU2_NATIVE_','OMP_','OPENBLAS_','MKL_'))}))
    start=time.time()
    with (folder/'run.log').open('w') as out:
        p=subprocess.Popen(command,cwd=folder,env=env,stdout=out,stderr=subprocess.STDOUT)
        save(folder/'status.json',dict(state='running',pid=p.pid,start=start))
        with (folder/'resource_samples.jsonl').open('w') as receipt:
            while p.poll() is None:
                pending=[p.pid];seen=set();rows=[]
                while pending:
                    pid=pending.pop()
                    if pid in seen:continue
                    seen.add(pid)
                    try:
                        proc=Path('/proc')/str(pid)
                        pending.extend(int(x) for x in (proc/'task'/str(pid)/'children').read_text().split())
                        rss=int((proc/'statm').read_text().split()[1])*os.sysconf('SC_PAGE_SIZE')
                        rows.append(dict(pid=pid,rss_bytes=rss,name=(proc/'comm').read_text().strip()))
                    except (FileNotFoundError,ProcessLookupError):pass
                receipt.write(json.dumps(dict(seconds=time.time()-start,processes=rows,
                    summed_rss_bytes=sum(row['rss_bytes'] for row in rows)))+'\n');receipt.flush()
                time.sleep(2)
        code=p.wait()
    changed=[name for name,expected in pins.items() if sha(name)!=expected]
    result=dict(state='terminal',exit_code=code,elapsed_seconds=time.time()-start,pins_verified=not changed,changed=changed)
    if (folder/'construction_result.json').exists():result['native']=json.loads((folder/'construction_result.json').read_text())
    save(folder/'status.json',result);print(json.dumps(result))
    assert not changed,changed
    raise SystemExit(code)

if __name__=='__main__':main()
