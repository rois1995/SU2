"""Freeze cooperative geometry/exact deadline integration."""
from pathlib import Path
import hashlib,json,os,shutil,subprocess,sys,time
ROOT=Path(__file__).resolve().parent;REPO=ROOT.parents[3]
sys.path.insert(0,str(ROOT));from run_cases import sample_tree
sha=lambda p:hashlib.file_digest(Path(p).open("rb"),"sha256").hexdigest()
def save(p,d):p.write_text(json.dumps(d,indent=2)+"\n")
def main():
    status=ROOT/"build271_status.json";assert not status.exists()
    assert json.loads((ROOT/"m6_runs/euler_build267_coupled_geometry_repair_mach_only_real_cfd50k/terminal_handles.json").read_text())["all_owned_handles_terminal"]
    old_attempt=ROOT/"m6_runs/euler_build262_compact_capture_mach_only_real_cfd50k"
    for name in ("terminal_handles.json","inspection_export_terminal_handles.json","inspection_reader_terminal_handles.json"):
        assert json.loads((old_attempt/name).read_text())["all_owned_handles_terminal"]
    for name in ("owned_startup.json","inspection_export_owned_startup.json"):
        for row in json.loads((old_attempt/name).read_text())["owned_tree"]:
            proc=Path("/proc")/str(row["pid"])/"stat"
            if proc.exists():
                fields=proc.read_text().rsplit(")",1)[1].split()
                assert int(fields[19])!=row["start_ticks"] or fields[0]=="Z"
    assert json.loads((ROOT/"cases/current_production_20261108T100814_3110252/terminal_handles.json").read_text())["all_owned_handles_terminal"]
    mem=next(int(s.split()[1])*1024 for s in Path("/proc/meminfo").read_text().splitlines() if s.startswith("MemAvailable:"))
    assert mem>16*1024**3 and shutil.disk_usage(REPO).free>4*1024**3
    old=json.loads((ROOT/"build270_source_pins.json").read_text());pins={p:sha(p) for p in old}
    changed=[p for p in old if pins[p]!=old[p]]
    allowed={str(REPO/'UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp')}
    assert set(changed)==allowed,changed
    save(ROOT/"build271_source_pins.json",pins)
    helpers=[Path(__file__),ROOT/"run_cases.py",ROOT/"deadline271_validate.py",ROOT/"prepare_current263_witnesses.py",ROOT/"prepare_current237_witnesses.py",ROOT/"inspect_native_failure3d.py"]
    helper_pins={str(p):sha(p) for p in helpers};save(ROOT/"build271_helper_pins.json",helper_pins)
    command=json.loads((ROOT/"build270_command.json").read_text());save(ROOT/"build271_command.json",command)
    env=os.environ.copy();env.update(command["environment"])
    cfd=REPO/"integration_evidence/build-integrated-v3-currenthost/SU2_CFD/src/SU2_CFD";cfd_before=sha(cfd)
    started=time.time();state={"state":"running","started_at":started,"helper_pid":os.getpid(),"helper_start_ticks":int(Path("/proc/self/stat").read_text().rsplit(")",1)[1].split()[19]),"source_commit":subprocess.check_output(["git","rev-parse","HEAD"],cwd=REPO,text=True).strip(),"changed_compiled_sources":changed,"production_cfd_sha256_before":cfd_before,"loadavg":Path("/proc/loadavg").read_text(),"MemAvailable_bytes":mem,"disk_free_bytes":shutil.disk_usage(REPO).free}
    with (ROOT/"build271_driver.log").open("x") as log,(ROOT/"build271_resources.jsonl").open("x") as resources:
        proc=subprocess.Popen(command["argv"],cwd=REPO,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
        state["pid"]=proc.pid;save(status,state);print(json.dumps(state),flush=True)
        while proc.poll() is None:
            resources.write(json.dumps({"elapsed_seconds":time.time()-started,"processes":sample_tree(proc.pid)})+"\n");resources.flush();time.sleep(2)
        code=proc.wait()
    changed_after=[p for p,h in {**pins,**helper_pins}.items() if sha(p)!=h];cfd_after=sha(cfd)
    result={"state":"terminal","exit_code":code,"seconds":time.time()-started,"pins_verified":not changed_after,"changed":changed_after,"completed_build":code==0 and not changed_after,"production_cfd_byte_identical":cfd_after==cfd_before,"production_cfd_sha256_before":cfd_before,"production_cfd_sha256_after":cfd_after}
    save(status,result);print(json.dumps(result),flush=True);assert not changed_after
    raise SystemExit(code)
if __name__=="__main__":main()
