"""One connected serial/MPI milestone through the existing pinned case runner."""
from pathlib import Path
import hashlib
import os
import sys
import tempfile
import run_cases

ROOT = Path(__file__).resolve().parent
import json
assert json.loads((ROOT/'build273_terminal_handles.json').read_text())['all_owned_handles_terminal']
for key in list(os.environ):
    if key.startswith('SU2_NATIVE_'):del os.environ[key]
os.environ.update(OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
assert len(sys.argv) == 2 and sys.argv[1] in ("focused", "serial", "mpi6")
if sys.argv[1] == "focused":
    case="current_production"
    config=ROOT/"m6_runs/euler_build262_compact_capture_mach_only_real_cfd50k/post_refinement_cleanup_inputs_v1/run.cfg"
    run_cases.CASES[case]=(1,"[NativeMetricCleanupGate3D],[NativeMetricCleanupPhase3D],[NativeMetricCleanup3D]")
elif sys.argv[1] == "serial":
    case = "current_production"
    config = ROOT / "m6_runs/euler_build262_compact_capture_mach_only_real_cfd50k/current_witnesses_v1/run.cfg"
    ranks, filters = run_cases.CASES[case]
    run_cases.CASES[case] = ranks, filters + ",[NativeDeadline3D],[NativeAtlas3D],[NativePolicy3D]," + run_cases.CANONICAL_CHECKS + ",[NativeBoundedGuidance3D],[NativeSurfaceError3D],[NativeCurvedGeometry3D],[NativeJointGeometry3D],[NativeFirstLayer3D],[NativeGeometryReturn3D],[NativeRepairBatch3D],[NativePhaseOwnership3D],[NativeRejectedState3D]"
else:
    case = "placement_mpi6"
    config = ROOT / "m6_runs/euler_build262_compact_capture_mach_only_real_cfd50k/placement267_reuse_v1/run.cfg"
    ranks, filters = run_cases.CASES[case]
    run_cases.CASES[case] = ranks, filters + ",[NativeDeadline3D],[NativeAtlas3D],[NativePolicy3D],[NativeBoundedGuidance3D],[NativeSurfaceError3D],[NativeCurvedGeometry3D]"
ranks,filters=run_cases.CASES[case]
run_cases.CASES[case]=(ranks,filters+",[NativeMetricCleanupGate3D],[NativeMetricCleanupPhase3D]")
os.environ["SU2_NATIVE_FROZEN_CONFIG"] = str(config.absolute())
assert config.is_file()
scratch = ROOT.parents[3] / "integration_evidence/build-integrated-v2/compile-tmp"
os.environ.setdefault("OMPI_MCA_osc", "pt2pt")
os.environ.setdefault("OMPI_MCA_orte_tmpdir_base", tempfile.mkdtemp(prefix="deadline273-", dir=scratch))
digest = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
sys.argv = [str(Path(run_cases.__file__).resolve()), "build273", case]
try:
    run_cases.main()
finally:
    assert hashlib.sha256(Path(__file__).read_bytes()).hexdigest() == digest
