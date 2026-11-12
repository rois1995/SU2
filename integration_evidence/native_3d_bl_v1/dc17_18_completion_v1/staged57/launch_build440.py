"""Build connected first-height continuation persistence and actual saved-cavity comparison."""
from pathlib import Path
import hashlib,json,os,subprocess,sys,time
R=Path(__file__).resolve().parent;REPO=R.parents[3]
sys.path.insert(0,str(R));from run_cases import sample_tree
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,v):p.write_text(json.dumps(v,indent=2)+'\n')
def main():
    status=R/'build440_status.json';assert not status.exists()
    receipts=[R/'build438d_terminal_handles.json',R/'cases/curved438_height_continuation_validation_mpi6_20261112T120218_1192372/terminal_handles_actual.json',R/'build438c_terminal_handles.json',R/'build438b_terminal_handles.json',R/'build438_terminal_handles.json',R/'build437_terminal_handles.json',R/'cases/curved437_cavity_correction_20261112T111648_1071306/terminal_handles_actual.json',R/'build417_terminal_handles.json',R/'build416_terminal_handles.json',R/'build415_terminal_handles.json',R/'build414_terminal_handles.json',R/'build413_terminal_handles.json',R/'build412_terminal_handles.json',*(R/'cases').glob('curved412*/terminal_handles_actual.json')]
    for path in receipts:
        receipt=json.loads(path.read_text());assert receipt['all_owned_handles_terminal']
        for row in receipt.get('identities',receipt.get('sampled_identities',[]))+[receipt['wrapper_excluded']]:
            p=Path('/proc',str(row['pid']),'stat')
            if p.exists():assert int(p.read_text().rsplit(')',1)[1].split()[19])!=row['start_ticks'],row
    available=int(next(x.split()[1] for x in Path('/proc/meminfo').read_text().splitlines() if x.startswith('MemAvailable:')))*1024
    assert available>8<<30
    old=json.loads((R/'build438d_source_pins.json').read_text());pins={p:sha(p) for p in old}
    changed=[p for p in old if pins[p]!=old[p]]
    expected={'UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp'}
    assert set(changed)=={str(REPO/p) for p in expected},changed
    new_source=REPO/'UnitTests/SU2_CFD/adaptation/fixtures/native_actual_bl_surface_stage.inc'
    pins[str(new_source)]=sha(new_source)
    for name in ('native_actual_bl_cavity_planning.inc','native_actual_bl_cavity_planning_body.inc'):
        added=REPO/'UnitTests/SU2_CFD/adaptation/fixtures'/name;pins[str(added)]=sha(added)
    correction=REPO/'UnitTests/SU2_CFD/adaptation/fixtures/native_actual_cavity_correction.inc'
    pins[str(correction)]=sha(correction)
    height=REPO/'UnitTests/SU2_CFD/adaptation/fixtures/native_actual_cavity_height_continuation.inc'
    pins[str(height)]=sha(height)
    pins[str(Path(__file__))]=sha(__file__)
    save(R/'build440_source_pins.json',pins)
    command=json.loads((R/'build296_command.json').read_text());save(R/'build440_command.json',command)
    env=os.environ.copy();env.update(command['environment']);started=time.time();seen={}
    own=dict(pid=os.getpid(),start_ticks=int(Path('/proc/self/stat').read_text().rsplit(')',1)[1].split()[19]))
    state=dict(state='running',wrapper=own,started_at=started,changed_compiled_sources=changed,MemAvailable_bytes=available,loadavg=Path('/proc/loadavg').read_text())
    with (R/'build440_driver.log').open('x') as log,(R/'build440_resources.jsonl').open('x') as resources:
        proc=subprocess.Popen(command['argv'],cwd=REPO,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
        state.update(pid=proc.pid,start_ticks=int(Path(f'/proc/{proc.pid}/stat').read_text().rsplit(')',1)[1].split()[19]));save(status,state);print(json.dumps(state),flush=True)
        while proc.poll() is None:
            rows=sample_tree(proc.pid)
            for row in rows:
                try:
                    t=int(Path('/proc',str(row['pid']),'stat').read_text().rsplit(')',1)[1].split()[19]);seen[(row['pid'],t)]=dict(pid=row['pid'],start_ticks=t)
                except (FileNotFoundError,ProcessLookupError):pass
            resources.write(json.dumps(dict(elapsed_seconds=time.time()-started,processes=rows))+'\n');resources.flush();time.sleep(2)
        code=proc.wait()
    differences=[p for p,h in pins.items() if sha(p)!=h]
    result=dict(state='terminal',exit_code=code,seconds=time.time()-started,pins_verified=not differences,changed=differences,completed_build=code==0 and not differences)
    if code==0:result['binaries']={str(REPO/p):sha(REPO/p) for p in ('integration_evidence/build-integrated-v3-currenthost/UnitTests/test_driver','integration_evidence/build-integrated-v3-currenthost/SU2_CFD/src/SU2_CFD')}
    save(status,result);save(R/'build440_observed_handles.json',dict(actual_host_proc=True,identities=list(seen.values()),wrapper_excluded=own));print(json.dumps(result),flush=True);assert not differences
    raise SystemExit(code)
if __name__=='__main__':main()
