"""Pinned joint-normalization integration build; no mesh acceptance claim."""
from pathlib import Path
import hashlib,json,os,subprocess,sys,time
R=Path(__file__).resolve().parent;REPO=R.parents[3]
sys.path.insert(0,str(R));from run_cases import sample_tree
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,v):p.write_text(json.dumps(v,indent=2)+'\n')
def main():
    status=R/'build393_status.json';assert not status.exists()
    terminal=json.loads((R/'m6_runs/rans_curved364_v1/terminal_handles_actual.json').read_text());assert terminal['all_owned_handles_terminal']
    for row in terminal['identities']:
        p=Path('/proc',str(row['pid']),'stat')
        if p.exists():assert int(p.read_text().rsplit(')',1)[1].split()[19])!=row['start_ticks'],row
    assert json.loads((R/'build365_terminal_handles.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'build366_terminal_handles.json').read_text())['all_owned_handles_terminal']
    for p in (R/'cases').glob('curved366_tangential_*/terminal_handles_actual.json'):
        receipt=json.loads(p.read_text());assert receipt['all_owned_handles_terminal']
        for row in receipt['sampled_identities']+[receipt['wrapper_excluded']]:
            q=Path('/proc',str(row['pid']),'stat')
            if q.exists():assert int(q.read_text().rsplit(')',1)[1].split()[19])!=row['start_ticks']
    assert json.loads((R/'build367_terminal_handles.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'build368_terminal_handles.json').read_text())['all_owned_handles_terminal']
    for p in (R/'cases').glob('curved368_tangential_*/terminal_handles_actual.json'):
        a=json.loads(p.read_text());assert a['all_owned_handles_terminal']
        for row in a['sampled_identities']+[a['wrapper_excluded']]:
            h=Path('/proc',str(row['pid']),'stat')
            if h.exists():assert int(h.read_text().rsplit(')',1)[1].split()[19])!=row['start_ticks']
    stage=R/'cases/curved385_root_point_frames_20261111T160544_2575874/terminal_handles_actual.json'
    receipt=json.loads(stage.read_text());assert receipt['all_owned_handles_terminal']
    for row in receipt['sampled_identities']+[receipt['wrapper_excluded']]:
        h=Path('/proc',str(row['pid']),'stat')
        if h.exists():assert int(h.read_text().rsplit(')',1)[1].split()[19])!=row['start_ticks']
    latest=json.loads((R/'build369_terminal_handles.json').read_text())
    diagnostic=json.loads((R/'geometry369_assessment.json').read_text())['terminal']
    assert latest['all_owned_handles_terminal'] and diagnostic['all_owned_handles_terminal']
    for row in latest['identities']+[latest['wrapper_excluded']]+diagnostic['sampled_identities']+[diagnostic['wrapper_excluded']]:
        h=Path('/proc',str(row['pid']),'stat')
        if h.exists():assert int(h.read_text().rsplit(')',1)[1].split()[19])!=row['start_ticks']
    latest=json.loads((R/'build372_terminal_handles.json').read_text());assert latest['all_owned_handles_terminal']
    for row in latest['identities']+[latest['wrapper_excluded']]:
        h=Path('/proc',str(row['pid']),'stat')
        if h.exists():assert int(h.read_text().rsplit(')',1)[1].split()[19])!=row['start_ticks']
    assert json.loads((R/'build385_terminal_handles.json').read_text())['all_owned_handles_terminal']
    for receiptPath in [R/'build392_terminal_handles.json',*(R/'cases').glob('curved392*/terminal_handles*.json')]:
        receipt=json.loads(receiptPath.read_text());assert receipt['all_owned_handles_terminal']
        for row in receipt.get('identities',receipt.get('sampled_identities',[]))+[receipt['wrapper_excluded']]:
            p=Path('/proc',str(row['pid']),'stat')
            if p.exists():assert int(p.read_text().rsplit(')',1)[1].split()[19])!=row['start_ticks'],row
    available=int(next(x.split()[1] for x in Path('/proc/meminfo').read_text().splitlines() if x.startswith('MemAvailable:')))*1024
    assert available>8<<30
    old=json.loads((R/'build392_source_pins.json').read_text());pins={p:sha(p) for p in old}
    changed=[p for p in old if pins[p]!=old[p]]
    expected={'Common/src/adaptation/CNativeAffineReference.cpp','UnitTests/Common/adaptation/CNativeGeometryBackend3D_tests.cpp'}
    assert set(changed)=={str(REPO/p) for p in expected},changed
    extras=['UnitTests/SU2_CFD/adaptation/fixtures/native_actual_bl_target_stability_probe.inc','UnitTests/SU2_CFD/adaptation/fixtures/native_actual_bl_accuracy_probe.inc','SU2_CFD/include/adaptation/CNativeNormalizationStage3D.hpp',
            'UnitTests/SU2_CFD/adaptation/fixtures/native_saved_cfd_metric_stage.inc',
            'UnitTests/SU2_CFD/adaptation/BoundaryLayerMetric_tests.cpp']
    for p in [*(REPO/p for p in extras),Path(__file__)]:pins[str(p)]=sha(p)
    save(R/'build393_source_pins.json',pins)
    command=json.loads((R/'build296_command.json').read_text());save(R/'build393_command.json',command)
    env=os.environ.copy();env.update(command['environment']);started=time.time();seen={}
    own=dict(pid=os.getpid(),start_ticks=int(Path('/proc/self/stat').read_text().rsplit(')',1)[1].split()[19]))
    state=dict(state='running',wrapper=own,started_at=started,changed_compiled_sources=changed,MemAvailable_bytes=available,loadavg=Path('/proc/loadavg').read_text())
    with (R/'build393_driver.log').open('x') as log,(R/'build393_resources.jsonl').open('x') as resources:
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
    save(status,result);save(R/'build393_observed_handles.json',dict(actual_host_proc=True,identities=list(seen.values()),wrapper_excluded=own));print(json.dumps(result),flush=True);assert not differences
    raise SystemExit(code)
if __name__=='__main__':main()
