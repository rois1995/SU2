"""Freeze and build the bounded capture-only change with the existing build command."""
from pathlib import Path
import hashlib,json,os,shutil,subprocess,sys,time
ROOT=Path(__file__).resolve().parent;REPO=ROOT.parents[3]
sys.path.insert(0,str(ROOT));from run_cases import sample_tree
sha=lambda p:hashlib.file_digest(Path(p).open("rb"),"sha256").hexdigest()
def save(p,d):p.write_text(json.dumps(d,indent=2)+"\n")
def main():
    status=ROOT/"build262_status.json";assert not status.exists()
    assert json.loads((ROOT/"build261_terminal_handles.json").read_text())["all_owned_handles_terminal"]
    assert json.loads((ROOT/"cases/failure_capture_20261108T063637_2542612/terminal_handles.json").read_text())["all_owned_handles_terminal"]
    old_attempt=ROOT/"m6_runs/euler_build260_paired_geometry_priority_mach_only_real_cfd50k"
    assert json.loads((old_attempt/"terminal_handles.json").read_text())["all_owned_handles_terminal"]
    for p in json.loads((old_attempt/"owned_startup.json").read_text())["owned_tree"]:
        proc=Path("/proc")/str(p["pid"])/"stat"
        if proc.exists():
            fields=proc.read_text().rsplit(")",1)[1].split()
            assert int(fields[19])!=p["start_ticks"] or fields[0]=="Z"
    mem=next(int(s.split()[1])*1024 for s in Path("/proc/meminfo").read_text().splitlines() if s.startswith("MemAvailable:"))
    assert mem>16*1024**3 and shutil.disk_usage(REPO).free>4*1024**3
    old=json.loads((ROOT/"build260_source_pins.json").read_text());pins={p:sha(p) for p in old}
    changed=[p for p in old if pins[p]!=old[p]]
    allowed={str(REPO/"Common/include/adaptation/CNativeFailureCapture3D.hpp"),str(REPO/"UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp")}
    assert set(changed)==allowed,changed
    save(ROOT/"build262_source_pins.json",pins)
    helpers=[Path(__file__),ROOT/"run_cases.py",ROOT/"inspect_native_failure3d.py",ROOT/"test_inspect_native_failure3d.py",ROOT/"assess_capture_compact261.py",ROOT/"target_delivery253_v1/analyze_leading_edge.py"]
    helper_pins={str(p):sha(p) for p in helpers};save(ROOT/"build262_helper_pins.json",helper_pins)
    command=json.loads((ROOT/"build260_command.json").read_text());save(ROOT/"build262_command.json",command)
    env=os.environ.copy();env.update(command["environment"])
    cfd=REPO/"integration_evidence/build-integrated-v3-currenthost/SU2_CFD/src/SU2_CFD";cfd_before=sha(cfd)
    started=time.time();state={"state":"running","started_at":started,"helper_pid":os.getpid(),"helper_start_ticks":int(Path("/proc/self/stat").read_text().rsplit(")",1)[1].split()[19]),"source_commit":subprocess.check_output(["git","rev-parse","HEAD"],cwd=REPO,text=True).strip(),"changed_compiled_sources":changed,"production_cfd_sha256_before":cfd_before,"loadavg":Path("/proc/loadavg").read_text(),"MemAvailable_bytes":mem,"disk_free_bytes":shutil.disk_usage(REPO).free}
    with (ROOT/"build262_driver.log").open("x") as log,(ROOT/"build262_resources.jsonl").open("x") as resources:
        proc=subprocess.Popen(command["argv"],cwd=REPO,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
        state["pid"]=proc.pid;save(status,state);print(json.dumps(state),flush=True)
        while proc.poll() is None:
            resources.write(json.dumps({"elapsed_seconds":time.time()-started,"processes":sample_tree(proc.pid)})+"\n");resources.flush();time.sleep(2)
        code=proc.wait()
    changed_after=[p for p,h in {**pins,**helper_pins}.items() if sha(p)!=h];cfd_after=sha(cfd)
    result={"state":"terminal","exit_code":code,"seconds":time.time()-started,"pins_verified":not changed_after,"changed":changed_after,"completed_build":code==0 and not changed_after,"production_cfd_byte_identical":cfd_after==cfd_before,"production_cfd_sha256_before":cfd_before,"production_cfd_sha256_after":cfd_after}
    save(status,result);print(json.dumps(result),flush=True);assert not changed_after and cfd_before==cfd_after
    raise SystemExit(code)
if __name__=="__main__":main()
