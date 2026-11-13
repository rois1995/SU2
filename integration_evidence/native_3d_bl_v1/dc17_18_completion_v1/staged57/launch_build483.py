"""Build complete requested-wall census before optional surface collapse search."""
from pathlib import Path
import hashlib,json,os,subprocess,sys,time
R=Path(__file__).resolve().parent;REPO=R.parents[3]
sys.path.insert(0,str(R));from run_cases import sample_tree
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,v):p.write_text(json.dumps(v,indent=2)+'\n')
def main():
    status=R/'build483_status.json';assert not status.exists()
    receipts=[R/'build482_terminal_handles.json',R/'cases/curved482_saved_production_surface_serial_20261113T100940_2712169/terminal_handles_actual.json',R/'cases/curved482_saved_production_surface_mpi6_20261113T102543_2774247/terminal_handles_actual.json']
    for path in receipts:
        receipt=json.loads(path.read_text());assert receipt['all_owned_handles_terminal']
        rows=receipt.get('identities',receipt.get('sampled_identities',[]))
        if 'wrapper_excluded' in receipt:rows=rows+[receipt['wrapper_excluded']]
        for row in rows:
            p=Path('/proc',str(row['pid']),'stat')
            if p.exists():assert int(p.read_text().rsplit(')',1)[1].split()[19])!=row['start_ticks'],row
    available=int(next(x.split()[1] for x in Path('/proc/meminfo').read_text().splitlines() if x.startswith('MemAvailable:')))*1024
    assert available>8<<30
    old=json.loads((R/'build482_source_pins.json').read_text());pins={p:sha(p) for p in old}
    changed=[p for p in old if pins[p]!=old[p]]
    expected={'Common/src/adaptation/CNativeEngine3D.cpp','UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp'}
    assert set(changed)=={str(REPO/p) for p in expected},changed
    new_source=REPO/'UnitTests/SU2_CFD/adaptation/fixtures/native_actual_bl_surface_stage.inc'
    pins[str(new_source)]=sha(new_source)
    for name in ('native_actual_bl_cavity_planning.inc','native_actual_bl_cavity_planning_body.inc'):
        added=REPO/'UnitTests/SU2_CFD/adaptation/fixtures'/name;pins[str(added)]=sha(added)
    correction=REPO/'UnitTests/SU2_CFD/adaptation/fixtures/native_actual_cavity_correction.inc'
    pins[str(correction)]=sha(correction)
    height=REPO/'UnitTests/SU2_CFD/adaptation/fixtures/native_actual_cavity_height_continuation.inc'
    pins[str(height)]=sha(height)
    mpi=REPO/'UnitTests/SU2_CFD/adaptation/fixtures/native_actual_cavity_correction_mpi.inc'
    assert mpi.exists();pins[str(mpi)]=sha(mpi)
    added=REPO/'UnitTests/SU2_CFD/adaptation/fixtures/native_actual_boundary_planning_cost.inc'
    pins[str(added)]=sha(added)
    added=REPO/'UnitTests/SU2_CFD/adaptation/fixtures/native_boundary_guidance_reference.inc'
    pins[str(added)]=sha(added)
    added=REPO/'UnitTests/SU2_CFD/adaptation/fixtures/native_actual_surface_clearance.inc'
    pins[str(added)]=sha(added)
    added=REPO/'UnitTests/SU2_CFD/adaptation/fixtures/native_actual_surface_apex_probe.inc'
    pins[str(added)]=sha(added)
    added=REPO/'UnitTests/SU2_CFD/adaptation/fixtures/native_actual_surface_geometry_probe.inc'
    pins[str(added)]=sha(added)
    added=REPO/'UnitTests/SU2_CFD/adaptation/fixtures/native_actual_surface_compound_split.inc'
    pins[str(added)]=sha(added)
    added=REPO/'UnitTests/SU2_CFD/adaptation/fixtures/native_actual_surface_split_continuation.inc'
    pins[str(added)]=sha(added)
    added=REPO/'UnitTests/SU2_CFD/adaptation/fixtures/native_actual_surface_residual_probe.inc'
    pins[str(added)]=sha(added)
    added=REPO/'UnitTests/SU2_CFD/adaptation/fixtures/native_actual_surface_red_reconnect.inc'
    pins[str(added)]=sha(added)
    added=REPO/'UnitTests/SU2_CFD/adaptation/fixtures/native_actual_surface_strip_cavity.inc'
    pins[str(added)]=sha(added)
    added=REPO/'UnitTests/SU2_CFD/adaptation/fixtures/native_actual_surface_patch_trajectory.inc'
    pins[str(added)]=sha(added)
    for name in ('native_actual_surface_trial_context.inc','native_actual_current_trajectory.inc'):
        added=REPO/'UnitTests/SU2_CFD/adaptation/fixtures'/name;pins[str(added)]=sha(added)
    added=REPO/'UnitTests/SU2_CFD/adaptation/fixtures/native_actual_saved_surface_corrections.inc';pins[str(added)]=sha(added)
    added=REPO/'UnitTests/SU2_CFD/adaptation/fixtures/native_actual_saved_surface_publication.inc';pins[str(added)]=sha(added)
    added=REPO/'UnitTests/SU2_CFD/adaptation/fixtures/native_actual_saved_surface_stage.inc';pins[str(added)]=sha(added)
    pins[str(Path(__file__))]=sha(__file__)
    save(R/'build483_source_pins.json',pins)
    command=json.loads((R/'build296_command.json').read_text());save(R/'build483_command.json',command)
    env=os.environ.copy();env.update(command['environment']);started=time.time();seen={}
    previous_test_sha=sha(REPO/'integration_evidence/build-integrated-v3-currenthost/UnitTests/test_driver')
    own=dict(pid=os.getpid(),start_ticks=int(Path('/proc/self/stat').read_text().rsplit(')',1)[1].split()[19]))
    state=dict(state='running',wrapper=own,started_at=started,changed_compiled_sources=changed,MemAvailable_bytes=available,loadavg=Path('/proc/loadavg').read_text())
    with (R/'build483_driver.log').open('x') as log,(R/'build483_resources.jsonl').open('x') as resources:
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
    dry=subprocess.run(command['argv']+['-n'],cwd=REPO,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    save(R/'build483_dryrun_completion.json',dict(exit_code=dry.returncode,output=dry.stdout))
    stopped=(R/'build483_authorized_stop.json').exists()
    changed_binary=sha(REPO/'integration_evidence/build-integrated-v3-currenthost/UnitTests/test_driver')!=previous_test_sha
    result['completed_build']=bool(code==0 and not differences and not stopped and changed_binary and dry.returncode==0 and 'no work to do' in dry.stdout)
    result['test_binary_changed']=changed_binary;result['authorized_stop']=stopped
    save(status,result);save(R/'build483_observed_handles.json',dict(actual_host_proc=True,identities=list(seen.values()),wrapper_excluded=own));print(json.dumps(result),flush=True);assert not differences
    raise SystemExit(0 if result['completed_build'] else code or 1)
if __name__=='__main__':main()
