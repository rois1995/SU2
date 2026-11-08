"""One connected serial/MPI milestone through the existing pinned case runner."""
from pathlib import Path
import hashlib
import os
import sys
import tempfile
import run_cases

ROOT = Path(__file__).resolve().parent
import json
assert json.loads((ROOT/'build267_terminal_handles.json').read_text())['all_owned_handles_terminal']
for key in list(os.environ):
    if key.startswith('SU2_NATIVE_'):del os.environ[key]
os.environ.update(OMP_NUM_THREADS='1',OPENBLAS_NUM_THREADS='1',MKL_NUM_THREADS='1')
assert len(sys.argv) == 2 and sys.argv[1] in ("serial", "mpi6")
if sys.argv[1] == "serial":
    case = "current_production"
    config = ROOT / "m6_runs/euler_build262_compact_capture_mach_only_real_cfd50k/current_witnesses_v1/run.cfg"
    ranks, filters = run_cases.CASES[case]
    run_cases.CASES[case] = ranks, filters + "," + run_cases.CANONICAL_CHECKS + ",[NativeBoundedGuidance3D],[NativeSurfaceError3D],[NativeCurvedGeometry3D],[NativeJointGeometry3D],[NativeFirstLayer3D],[NativeGeometryReturn3D],[NativeRepairBatch3D],[NativePhaseOwnership3D],[NativeRejectedState3D]"
else:
    case = "placement_mpi6"
    config = ROOT / "m6_runs/euler_build244_mach_only_real_cfd50k/placement246_inputs_v1/run.cfg"
    ranks, filters = run_cases.CASES[case]
    run_cases.CASES[case] = ranks, filters + ",[NativeBoundedGuidance3D],[NativeSurfaceError3D],[NativeCurvedGeometry3D]"
os.environ["SU2_NATIVE_FROZEN_CONFIG"] = str(config.absolute())
assert config.is_file()
scratch = ROOT.parents[3] / "integration_evidence/build-integrated-v2/compile-tmp"
os.environ.setdefault("OMPI_MCA_osc", "pt2pt")
os.environ.setdefault("OMPI_MCA_orte_tmpdir_base", tempfile.mkdtemp(prefix="coupled267-", dir=scratch))
digest = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
sys.argv = [str(Path(run_cases.__file__).resolve()), "build267", case]
try:
    run_cases.main()
finally:
    assert hashlib.sha256(Path(__file__).read_bytes()).hexdigest() == digest
