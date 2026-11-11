"""Freeze and build the connected cubic-wall BL integration; qualification is separate."""
from pathlib import Path
import hashlib,json,os,shutil,subprocess,sys,time
R=Path(__file__).resolve().parent;REPO=R.parents[3]
sys.path.insert(0,str(R));from run_cases import sample_tree
sha=lambda p:hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,value):p.write_text(json.dumps(value,indent=2)+'\n')
def main():
    status=R/'build362_status.json';assert not status.exists()
    previous=json.loads((R/'m6_runs/euler_build296_boundary_reuse_mach_only_real_cfd50k/terminal_handles.json').read_text())
    assert previous['all_owned_handles_terminal']
    assert json.loads((R/'build346_terminal_handles.json').read_text())['all_owned_handles_terminal']
    for case in ('curved346_curved_height_20261111T021215_480645','curved346_front_smooth_upper_20261111T021330_483617'):
        receipt=json.loads((R/'cases'/case/'terminal_handles_actual.json').read_text());assert receipt['all_owned_handles_terminal']
        for row in receipt['identities']:assert not Path('/proc',str(row['pid'])).exists(),row
    assert json.loads((R/'build345_terminal_handles.json').read_text())['all_owned_handles_terminal']
    receipt=json.loads((R/'cases/curved345_front_patches_20261111T014839_426314/terminal_handles_actual.json').read_text());assert receipt['all_owned_handles_terminal']
    for row in receipt['identities']:assert not Path('/proc',str(row['pid'])).exists(),row
    assert json.loads((R/'build344_terminal_handles.json').read_text())['all_owned_handles_terminal']
    for case in ('curved344_construction_20261111T013305_389577','curved344_entry_20261111T013353_391416'):
        receipt=json.loads((R/'cases'/case/'terminal_handles_actual.json').read_text());assert receipt['all_owned_handles_terminal']
        for row in receipt['identities']:assert not Path('/proc',str(row['pid'])).exists(),row
    assert json.loads((R/'build343_terminal_handles.json').read_text())['all_owned_handles_terminal']
    for case in ('curved343_construction_20261111T012815_378105','curved343_entry_20261111T012957_382153'):
        receipt=json.loads((R/'cases'/case/'terminal_handles_actual.json').read_text());assert receipt['all_owned_handles_terminal']
        for row in receipt['identities']:assert not Path('/proc',str(row['pid'])).exists(),row
    assert json.loads((R/'build342_terminal_handles.json').read_text())['all_owned_handles_terminal']
    for case in ('curved342_construction_20261111T012141_362705','curved342_entry_20261111T012219_364139'):
        receipt=json.loads((R/'cases'/case/'terminal_handles_actual.json').read_text());assert receipt['all_owned_handles_terminal']
        for row in receipt['identities']:assert not Path('/proc',str(row['pid'])).exists(),row
    assert json.loads((R/'build341_terminal_handles.json').read_text())['all_owned_handles_terminal']
    latest_build=json.loads((R/'build340_terminal_handles.json').read_text());assert latest_build['all_owned_handles_terminal']
    for row in latest_build['identities']:assert not Path('/proc',str(row['pid'])).exists(),row
    latest_check=json.loads((R/'cases/curved340_core_policy_20261111T000546_194997/terminal_handles_actual.json').read_text());assert latest_check['all_owned_handles_terminal']
    last=json.loads((R/'cases/curved355_front_smooth_upper_20261111T034922_724526/terminal_handles_actual.json').read_text())
    assert last['all_owned_handles_terminal']
    for row in last['identities']:
        handle=Path('/proc',str(row['pid']),'stat')
        if handle.exists():assert int(handle.read_text().rsplit(')',1)[1].split()[19])!=row['start_ticks'],row
    available=next(int(x.split()[1])*1024 for x in Path('/proc/meminfo').read_text().splitlines() if x.startswith('MemAvailable:'))
    assert available>16*1024**3 and shutil.disk_usage(REPO).free>4*1024**3
    for name in ('curved334_association_20261110T221800_4123044','curved334_surface_components_20261110T222410_4141734'):
        final=json.loads((R/'cases'/name/'terminal_handles_actual.json').read_text())
        assert final['all_owned_handles_terminal']
        for row in final['identities']:assert not Path('/proc',str(row['pid'])).exists(),row
    assert json.loads((R/'build338_terminal_handles.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'cases/curved338_precision_20261110T232529_102468/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'build339_terminal_handles.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'cases/curved339_surface_interfaces_20261110T235858_178894/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    old=json.loads((R/'build361_source_pins.json').read_text());pins={p:sha(p) for p in old}
    changed=[p for p in old if old[p]!=pins[p]]
    assert json.loads((R/'build328_terminal_handles.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'m6_runs/rans_curved303_v1/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'cases/cubic_cell_replay317_v1/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'m6_runs/rans_curved326_v1/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'cases/curved327_cache_20261110T093443_2122647/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'m6_runs/rans_curved328_v1/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'m6_runs/rans_curved329_v1/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'m6_runs/rans_curved330_v1/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'cases/curved331_actual_chord_20261110T173020_3365437/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'cases/curved331_actual_points_20261110T173915_3389504/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'build331_terminal_handles.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'cases/curved331_bounds_20261110T180316_3455968/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'build332_terminal_handles.json').read_text())['all_owned_handles_terminal']
    expected={'UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp'}
    failed=json.loads((R/'cases/curved335_surface_cavities_20261110T224442_4603/terminal_handles_actual.json').read_text())
    assert failed['all_owned_handles_terminal']
    for row in failed['identities']:assert not Path('/proc',str(row['pid'])).exists(),row
    assert json.loads((R/'build336_terminal_handles.json').read_text())['all_owned_handles_terminal']
    latest=json.loads((R/'cases/curved337_surface_cavities_20261110T225500_31104/terminal_handles_actual.json').read_text())
    for row in latest['identities']:assert not Path('/proc',str(row['pid'])).exists(),row
    previous_case=json.loads((R/'cases/curved333_translated_20261110T182532_3517263/terminal_handles_actual.json').read_text())
    for row in previous_case['identities']:
        handle=Path('/proc',str(row['pid']),'stat')
        if handle.exists():assert int(handle.read_text().rsplit(')',1)[1].split()[19])!=row['start_ticks'],row
    assert json.loads((R/'m6_runs/rans_curved325_v1/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'cases/cubic_near_point_replay325_v1/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'m6_runs/rans_curved323_v1/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'cases/cubic_point_replay323_v1/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'cases/cubic_cell_replay318_v1/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'cases/cubic_point_replay315_v1/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert json.loads((R/'m6_runs/rans_curved314_v1/terminal_handles_actual.json').read_text())['all_owned_handles_terminal']
    assert set(changed)=={str(REPO/p) for p in expected},changed
    new_fixture=REPO/'UnitTests/SU2_CFD/adaptation/fixtures/native_actual_bl_front_probe.inc'
    pins[str(new_fixture)]=sha(new_fixture)
    surface_fixture=REPO/'UnitTests/SU2_CFD/adaptation/fixtures/native_actual_bl_surface_height_probe.inc'
    pins[str(surface_fixture)]=sha(surface_fixture)
    row_fixture=REPO/'UnitTests/SU2_CFD/adaptation/fixtures/native_actual_bl_prepared_row_probe.inc'
    pins[str(row_fixture)]=sha(row_fixture)
    repair_fixture=REPO/'UnitTests/SU2_CFD/adaptation/fixtures/native_actual_bl_row_repair_probe.inc'
    pins[str(repair_fixture)]=sha(repair_fixture)
    pins[str(Path(__file__))]=sha(__file__);pins[str(R/'run_cases.py')]=sha(R/'run_cases.py')
    save(R/'build362_source_pins.json',pins)
    command=json.loads((R/'build296_command.json').read_text());save(R/'build362_command.json',command)
    env=os.environ.copy();env.update(command['environment']);started=time.time()
    state=dict(state='running',helper_pid=os.getpid(),helper_start_ticks=int(Path('/proc/self/stat').read_text().rsplit(')',1)[1].split()[19]),
        started_at=started,changed_compiled_sources=changed,MemAvailable_bytes=available,loadavg=Path('/proc/loadavg').read_text())
    with (R/'build362_driver.log').open('x') as log,(R/'build362_resources.jsonl').open('x') as resources:
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
