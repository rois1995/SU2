"""One connected serial/MPI milestone through the existing pinned case runner."""
from pathlib import Path
import hashlib
import os
import sys
import tempfile
import run_cases

ROOT = Path(__file__).resolve().parent
assert len(sys.argv) == 2 and sys.argv[1] in ("serial", "mpi6")
if sys.argv[1] == "serial":
    case = "current_compound"
    config = ROOT / "m6_runs/euler_build254_conditioned_mach_only_real_cfd50k/current_witnesses_v1/run.cfg"
    ranks, filters = run_cases.CASES[case]
    run_cases.CASES[case] = ranks, filters + "," + run_cases.CANONICAL_CHECKS + ",[NativeBoundedGuidance3D],[NativeSurfaceError3D],[NativeCurvedGeometry3D],[NativeJointGeometry3D],[NativeFirstLayer3D]"
else:
    case = "placement_mpi6"
    config = ROOT / "m6_runs/euler_build244_mach_only_real_cfd50k/placement246_inputs_v1/run.cfg"
    ranks, filters = run_cases.CASES[case]
    run_cases.CASES[case] = ranks, filters + ",[NativeBoundedGuidance3D],[NativeSurfaceError3D],[NativeCurvedGeometry3D]"
os.environ["SU2_NATIVE_FROZEN_CONFIG"] = str(config.absolute())
assert config.is_file()
scratch = ROOT.parents[3] / "integration_evidence/build-integrated-v2/compile-tmp"
os.environ.setdefault("OMPI_MCA_osc", "pt2pt")
os.environ.setdefault("OMPI_MCA_orte_tmpdir_base", tempfile.mkdtemp(prefix="geometry-debt256-", dir=scratch))
digest = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
sys.argv = [str(Path(run_cases.__file__).resolve()), "build256", case]
try:
    run_cases.main()
finally:
    assert hashlib.sha256(Path(__file__).read_bytes()).hexdigest() == digest
