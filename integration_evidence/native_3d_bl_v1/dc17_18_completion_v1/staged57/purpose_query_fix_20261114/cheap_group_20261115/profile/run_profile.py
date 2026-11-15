"""Prepare or run one same-v2 profile case; this file is inert until called with A or B."""
import hashlib, json, os, shutil, subprocess, sys, time
from pathlib import Path

HERE = Path(__file__).resolve().parent
BUILD_CONTEXT = HERE.parent
R = HERE.parents[2]
REPO = HERE.parents[6]
HISTORICAL = R / "measurement_20261114_v2"
STAGED = R
PIN_FILE = BUILD_CONTEXT / "source_pins.json"
BUILD_STATUS = BUILD_CONTEXT / "build_status.json"
BUILD = REPO / "integration_evidence/build-integrated-v3-p5-20261114"
CFD = BUILD / "SU2_CFD/src/SU2_CFD"
TEST = BUILD / "UnitTests/test_driver"
sha = lambda p: hashlib.file_digest(Path(p).open("rb"), "sha256").hexdigest()
def save(path, value): path.write_text(json.dumps(value, indent=2) + "\n")
def ident(pid):
    try: return {"pid": pid, "start_ticks": int(Path("/proc", str(pid), "stat").read_text().rsplit(")", 1)[1].split()[19])}
    except (FileNotFoundError, ProcessLookupError): return None

def verify_runtime():
    status = json.loads(BUILD_STATUS.read_text())
    assert status.get("state") == "terminal" and status.get("exit_code") == 0 and status.get("pins_verified") is True
    assert Path(status["binary"]).resolve() == CFD.resolve() and Path(status["test_binary"]).resolve() == TEST.resolve()
    hashes = {str(CFD): sha(CFD), str(TEST): sha(TEST)}
    assert hashes[str(CFD)] == status["binary_sha256"] and hashes[str(TEST)] == status["test_binary_sha256"]
    pins = json.loads(PIN_FILE.read_text())
    assert len(pins) == 1540, f"expected 1540 source pins, got {len(pins)}"
    bad = {p: {"expected": h, "actual": sha(p) if Path(p).is_file() else None}
           for p, h in pins.items() if not Path(p).is_file() or sha(p) != h}
    assert not bad, f"source pin mismatch: {bad}"
    runtime_manifest=R/"runtime_20261113_474/loaded_runtime_pins.json"
    runtime_pins=json.loads(runtime_manifest.read_text())
    assert all(sha(p)==h for p,h in runtime_pins.items()), "loaded runtime dependency changed"
    hashes.update(runtime_pins)
    return {"runtime_manifest_sha256":sha(runtime_manifest), "build_status_sha256": sha(BUILD_STATUS), "source_pins_sha256": sha(PIN_FILE),
            "source_pin_count": len(pins), "runtime_hashes": hashes}

def add_hash(inputs, path):
    path = Path(path).resolve(); inputs[str(path)] = sha(path)

def prepare(mode):
    assert mode in ("A", "B")
    case = HERE / ("run_" + mode)
    assert not case.exists(), f"refusing to overwrite {case}"
    before = verify_runtime()
    prerequisites={BUILD_CONTEXT/"unit_receipt.json":"passed",BUILD_CONTEXT/"collapse_receipt.json":"passed",BUILD_CONTEXT/"BL_receipt.json":"passed",BUILD_CONTEXT/"final_checks_sequence.json":"all_planned_checks_passed"}
    assert all(json.loads(p.read_text())[key] is True for p,key in prerequisites.items()), "ordered regression checks incomplete"
    if mode == "B":
        prior = json.loads((HERE / "run_A/status.json").read_text())
        assert prior.get("state") == "terminal" and prior.get("exit_code") == 0
        assert prior.get("all_owned_handles_terminal") and not prior.get("profiler_failed") and prior.get("native_started")
    case.mkdir()
    env = os.environ.copy()
    for key in list(env):
        if key.startswith("SU2_NATIVE_"): del env[key]
    env.update(OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1", MKL_NUM_THREADS="1", OMPI_MCA_osc="pt2pt",
               TMPDIR=str(REPO / "integration_evidence/build-integrated-v2/compile-tmp"),
               SU2_PROFILE_CFD_SHA256=before["runtime_hashes"][str(CFD)],
               SU2_PROFILE_TEST_SHA256=before["runtime_hashes"][str(TEST)],
               SU2_PROFILE_STAGED_ROOT=str(STAGED), SU2_PROFILE_CURRENT_ROOT=str(HERE))
    mpi_tmp = case / "mpi_tmp"; mpi_tmp.mkdir(); env["OMPI_MCA_orte_tmpdir_base"] = str(mpi_tmp)
    inputs = {str(p):sha(p) for p in prerequisites}; source_case = None
    if mode == "A":
        source_case = STAGED / "cases/curved492_saved_production_surface_serial_20261113T163756_3724702"
        old = json.loads((source_case / "command.json").read_text()); env.update(old["environment"])
        env["OMPI_MCA_orte_tmpdir_base"] = str(mpi_tmp); env["SU2_NATIVE_FROZEN_CONFIG"] = str(case / "run.cfg")
        for name in ("m6.su2", "m6.su2.native_ref", "m6.su2.native_rows", "baseline_sa_seed.dat"):
            src = (source_case / name).resolve(); (case / name).symlink_to(src); add_hash(inputs, src)
        shutil.copyfile(source_case / "run.cfg", case / "run.cfg")
        assert sha(case / "run.cfg") == sha(source_case / "run.cfg")
        for key, value in env.items():
            if key.startswith("SU2_NATIVE_") and Path(value).is_file(): add_hash(inputs, value)
        state = json.loads((STAGED / "saved_surface485_checkpoint.json").read_text())
        for key in ("owned", "parameters", "prepared_reference"):
            item = state[key]; assert sha(item["path"]) == item["sha256"]; add_hash(inputs, item["path"])
        ranks, timeout, program = 1, "600", TEST
        args = ["[NativeActualSavedSurfaceStage3D]", "--durations", "yes"]
        perf = HERE / "rank_perf.py"
    else:
        source_case = STAGED / "m6_runs/euler_build293_bounded_reference_mach_only_real_cfd50k"
        cfg = STAGED / "m6_inputs/euler_adapted293_event1_v1/run.cfg"
        assert sha(cfg) == sha(source_case / "run.cfg")
        shutil.copyfile(cfg, case / "run.cfg")
        seed = STAGED / "m6_inputs/euler_original_force_seed_v1"
        expected = json.loads((STAGED / "m6_inputs/euler_adapted293_event1_v1/seed_input.json").read_text())["qualification"]["output_sha256"]
        for name in ("m6.su2", "m6.su2.native_ref", "baseline_euler_seed.dat"):
            src = (seed / name).resolve(); assert sha(src) == expected[name]; (case / name).symlink_to(src); add_hash(inputs, src)
        ranks, timeout, program = 6, "14400", CFD
        args = ["run.cfg"]
        perf = HERE / "rank_perf_B.py"
    inputs[str((case / "run.cfg").resolve())] = sha(case / "run.cfg")
    for p in (CFD, TEST, HERE / "rank_perf.py", HERE / "rank_perf_B.py", HERE / "engine_clock.gdb",
              HERE / "engine_clock_B.gdb", HERE / "method_provenance.json", Path(__file__).resolve(), HERE / "analyze_profile.py", HISTORICAL / "run_measurement.py",
              HISTORICAL / "run_B.py", HISTORICAL / "rank_perf.py", HISTORICAL / "rank_perf_B.py",
              HISTORICAL / "analyze_B.py", HISTORICAL / "summarize_samples.py", HISTORICAL / "summarize_timers.py",
              HISTORICAL / "engine_clock.gdb", HISTORICAL / "engine_clock_B.gdb",
              HISTORICAL / "summarize_callers.py", HISTORICAL / "brief.md", BUILD_STATUS, PIN_FILE):
        add_hash(inputs, p)
    argv = ["nice", "-n", "19", "/usr/bin/time", "-v", "-o", str(case / "outer.time.txt"), "timeout", timeout]
    if ranks > 1: argv += ["mpirun", "--bind-to", "none", "-n", str(ranks)]
    argv += [sys.executable, str(perf), str(program), *args]
    receipt = {"case": mode, "ranks": ranks, "source_case": str(source_case), "argv": argv,
        "environment": {k:v for k,v in env.items() if k.startswith(("SU2_NATIVE_", "OMP_", "OPENBLAS_", "MKL_", "OMPI_", "SU2_PROFILE_")) or k == "TMPDIR"},
        "config_sha256": sha(case / "run.cfg"), "input_hashes": inputs, "runtime_before": before,
        "source_pin_count": before["source_pin_count"], "profile_cap_total_bytes": 2_000_000_000,
        "profile_abort_at_bytes": 1_900_000_000, "profile_cap_includes": "historical measurement_20261114_v* plus this profile directory",
        "method_sources": {"sampling": str(HISTORICAL / "rank_perf.py"), "B_clock": str(HISTORICAL / "engine_clock_B.gdb"),
                           "analysis": str(HISTORICAL / "analyze_B.py"), "GDB_timer_epoch_source_line": 1176},
        "free_space": shutil.disk_usage(REPO)._asdict(), "meminfo": Path("/proc/meminfo").read_text(),
        "loadavg": Path("/proc/loadavg").read_text(), "wrapper": ident(os.getpid()),
        "started_monotonic_ns": time.monotonic_ns(), "started_unix": time.time()}
    save(case / "provenance.json", receipt)
    seen = {}; started = time.time()
    with (case / "launcher.log").open("x") as log, (case / "host_resources.jsonl").open("x") as resources:
        proc = subprocess.Popen(argv, cwd=case, env=env, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
        save(case / "status.json", {"state":"running", "pid":proc.pid, "identity":ident(proc.pid)})
        print(json.dumps({"case":str(case), "pid":proc.pid}), flush=True)
        while proc.poll() is None:
            rows=[]
            for p in Path("/proc").iterdir():
                if not p.name.isdigit(): continue
                try:
                    cwd=(p/"cwd").resolve(); cmd=(p/"cmdline").read_bytes().replace(b"\0",b" ").decode(errors="replace")
                    if cwd != case and str(case) not in cmd: continue
                    stamp=ident(int(p.name))
                    if not stamp: continue
                    seen[(stamp["pid"],stamp["start_ticks"])]=stamp
                    status=(p/"status").read_text(); rss=next((int(x.split()[1])*1024 for x in status.splitlines() if x.startswith("VmRSS:")),0)
                    rows.append(dict(**stamp,rss_bytes=rss,command=cmd[:300]))
                except (FileNotFoundError,ProcessLookupError,PermissionError): pass
            resources.write(json.dumps({"elapsed":time.time()-started,"monotonic_ns":time.monotonic_ns(),"loadavg":Path("/proc/loadavg").read_text(),"processes":rows})+"\n");resources.flush();time.sleep(1)
        code=proc.wait()
    profiles=list((case/"profile").glob("rank*.status.json"));native_started=bool(list((case/"profile").glob("rank*.exec.json")))
    profiler_failed=len(profiles)!=ranks or any(json.loads(p.read_text())["record_exit"] or json.loads(p.read_text())["script_exit"] or json.loads(p.read_text())["failures"] for p in profiles)
    changed=[p for p,h in inputs.items() if sha(p)!=h]
    after=verify_runtime(); changed += [p for p,h in before["runtime_hashes"].items() if after["runtime_hashes"].get(p)!=h]
    live=[]
    for stamp in seen.values():
        if stamp["pid"] == os.getpid(): continue
        if ident(stamp["pid"]) == stamp: live.append(stamp)
    result={"state":"terminal","exit_code":code,"seconds":time.time()-started,"profiler_failed":bool(profiler_failed),
            "native_started":native_started,"inputs_unchanged":not changed,"changed":changed,
            "runtime_after":after,"all_owned_handles_terminal":not live,"live":live,"identities":list(seen.values()),
            "wrapper_excluded":ident(os.getpid()),"source_pins_before_after_equal":before["source_pins_sha256"]==after["source_pins_sha256"]}
    save(case/"status.json",result);print(json.dumps(result),flush=True)
    raise SystemExit(code or profiler_failed or bool(changed) or bool(live))

if __name__ == "__main__":
    assert sys.argv[1:] in (["A"],["B"]), "usage: run_profile.py A|B"
    prepare(sys.argv[1])
