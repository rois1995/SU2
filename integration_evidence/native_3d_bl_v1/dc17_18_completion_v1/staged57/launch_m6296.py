"""One complete first-adaptation replay after the connected boundary-reuse qualification."""
from pathlib import Path
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parent
repo = ROOT.parents[3]
attempt = ROOT / "m6_runs/euler_build296_boundary_reuse_mach_only_real_cfd50k"
receipt = ROOT / "m6_296_boundary_reuse_launch.json"
assert not receipt.exists() and not attempt.exists()
sha = lambda p: hashlib.file_digest(p.open("rb"), "sha256").hexdigest()
from run_m6 import euler_config,seed_config,BASE
planned=seed_config(euler_config((BASE/'run.cfg').read_text(),1,5400))
baseline=(ROOT/'m6_runs/euler_build293_bounded_reference_mach_only_real_cfd50k/run.cfg').read_text()
assert planned==baseline.replace('ADAP_SUBITER= ( 2 )','ADAP_SUBITER= ( 1 )')
qualification = json.loads((ROOT / "boundary296_qualification.json").read_text())
assert qualification["all_required_checks_passed"] and qualification["mpi_ranks"] == 6
assert sha(ROOT / "build296_source_pins.json") == qualification["source_pins_sha256"]
for name, digest in qualification["qualification_pins"].items():
    assert sha(Path(name)) == digest, name
for stage in qualification["stages"]:
    assert json.loads((Path(stage["case"]) / "terminal_handles.json").read_text())["all_owned_handles_terminal"]
pins = json.loads((ROOT / "build296_source_pins.json").read_text())
assert all(sha(Path(name)) == digest for name, digest in pins.items())
mem = {k: int(v.split()[0]) * 1024 for k, v in
       (line.split(":", 1) for line in Path("/proc/meminfo").read_text().splitlines())
       if k in ("MemAvailable", "SwapFree")}
assert mem["MemAvailable"] > 16 * 1024**3
scratch_root = repo / "integration_evidence/build-integrated-v2/compile-tmp"
scratch = Path(tempfile.mkdtemp(prefix="m6-certificate296-", dir=scratch_root))
env = {key: value for key, value in os.environ.items() if not key.startswith("SU2_NATIVE_")}
env.update(OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1", MKL_NUM_THREADS="1",
           OMPI_MCA_osc="pt2pt", TMPDIR=str(scratch_root), OMPI_MCA_orte_tmpdir_base=str(scratch))
argv = [sys.executable, str(ROOT / "run_m6.py"), "build296", attempt.name, "1",
        str(ROOT / "m6_inputs/euler_original_force_seed_v1"), "--inspect-rejected"]
state = {"state": "running", "started_at": time.time(), "runner_wrapper": {"pid": os.getpid(),
         "start_ticks": int(Path("/proc/self/stat").read_text().rsplit(")", 1)[1].split()[19])},
         "attempt": str(attempt), "argv": argv,
         "source_commit": subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip(),
         "sensors": ["MACH"], "baseline293_config_sha256": sha(ROOT/"m6_runs/euler_build293_bounded_reference_mach_only_real_cfd50k/run.cfg"),
         "configuration_difference": "ADAP_SUBITER 2 to 1 only; all seed, L1/50k, geometry, height, quality, memory and time directives identical", "loadavg": Path("/proc/loadavg").read_text(), "mem": mem,
         "disk_free_bytes": shutil.disk_usage(repo).free, "mpi_scratch": str(scratch),
         "launcher_sha256": sha(Path(__file__)),
         "scope": "Complete same-seed Mach-only L1/50k FIRST-adaptation performance replay; unchanged gates/native5400/outer14400; preserve293 adapted CFD checkpoints and logical second-adaptation obligation; bounded raw refusal capture, no per-trial VTU export."}
receipt.write_text(json.dumps(state, indent=2) + "\n")
print(json.dumps(state), flush=True)
code = subprocess.call(argv, env=env)
assert sha(Path(__file__)) == state["launcher_sha256"]
state.update(state="terminal", exit_code=code, finished_at=time.time())
receipt.write_text(json.dumps(state, indent=2) + "\n")
raise SystemExit(code)
