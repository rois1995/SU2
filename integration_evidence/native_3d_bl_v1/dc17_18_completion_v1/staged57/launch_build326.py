"""Freeze and build the connected cubic-wall BL integration; qualification is separate."""
from pathlib import Path
import hashlib,json,os,shutil,subprocess,sys,time
R=Path(__file__).resolve().parent;REPO=R.parents[3]
sys.path.insert(0,str(R));from run_cases import sample_tree
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,value):p.write_text(json.dumps(value,indent=2)+'\n')
def main():
    status=R/'build326_status.json';assert not status.exists()
    previous=json.loads((R/'m6_runs/euler_build296_boundary_reuse_mach_only_real_cfd50k/terminal_handles.json').read_text())
    assert previous['all_owned_handles_terminal']
    available=next(int(x.split()[1])*1024 for x in Path('/proc/meminfo').read_text().splitlines() if x.startswith('MemAvailable:'))
    assert available>16*1024**3 and shutil.disk_usage(REPO).free>4*1024**3
    old=json.loads((R/'build325_source_pins.json').read_text());pins={p:sha(p) for p in old}
    changed=[p for p in old if old[p]!=pins[p]]
    assert json.loads((R/'build325_terminal_handles.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'m6_runs/rans_curved303_v1/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'cases/cubic_cell_replay317_v1/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    expected={'Common/src/adaptation/CNativeAffineReference.cpp','UnitTests/Common/adaptation/CNativeGeometryBackend3D_tests.cpp'}
    assert json.loads((R/'m6_runs/rans_curved325_v1/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'cases/cubic_near_point_replay325_v1/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'m6_runs/rans_curved323_v1/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'cases/cubic_point_replay323_v1/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'cases/cubic_cell_replay318_v1/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'cases/cubic_point_replay315_v1/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'m6_runs/rans_curved314_v1/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert set(changed)=={str(REPO/p) for p in expected},changed
    pins[str(Path(__file__))]=sha(__file__);pins[str(R/'run_cases.py')]=sha(R/'run_cases.py')
    save(R/'build326_source_pins.json',pins)
    command=json.loads((R/'build296_command.json').read_text());save(R/'build326_command.json',command)
    env=os.environ.copy();env.update(command['environment']);started=time.time()
    state=dict(state='running',helper_pid=os.getpid(),helper_start_ticks=int(Path('/proc/self/stat').read_text().rsplit(')',1)[1].split()[19]),
        started_at=started,changed_compiled_sources=changed,MemAvailable_bytes=available,loadavg=Path('/proc/loadavg').read_text())
    with (R/'build326_driver.log').open('x') as log,(R/'build326_resources.jsonl').open('x') as resources:
        proc=subprocess.Popen(command['argv'],cwd=REPO,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
        state['pid']=proc.pid;state['start_ticks']=int(Path(f'/proc/{proc.pid}/stat').read_text().rsplit(')',1)[1].split()[19]);save(status,state);print(json.dumps(state),flush=True)
        while proc.poll() is None:
            resources.write(json.dumps(dict(elapsed_seconds=time.time()-started,processes=sample_tree(proc.pid)))+'\n');resources.flush();time.sleep(2)
        code=proc.wait()
    differences=[p for p,h in pins.items() if sha(p)!=h]
    result=dict(state='terminal',exit_code=code,seconds=time.time()-started,pins_verified=not differences,changed=differences,completed_build=code==0 and not differences)
    if code==0:result['binaries']={str(REPO/p):sha(REPO/p) for p in ('integration_evidence/build-integrated-v3-currenthost/UnitTests/test_driver','integration_evidence/build-integrated-v3-currenthost/SU2_CFD/src/SU2_CFD')}
    save(status,result);print(json.dumps(result),flush=True);assert not differences
    raise SystemExit(code)
if __name__=='__main__':main()
