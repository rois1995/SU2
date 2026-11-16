"""Run the accepted P7/P8 connected unit groups with section 6.1 build522."""
import json
import os
import sys
import tempfile
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
STAGED = HERE.parents[1]
REPO = STAGED.parents[3]
sys.path.insert(0, str(STAGED))
from run_cases import sha
import run_cases as checks
from build522_aliases import main as expose_build


def main():
    if sys.argv[1:] == ["--self-check"]:
        assert HERE.parents[1] == STAGED and STAGED.parents[3] == REPO
        assert (HERE / "build522").parent == HERE
        assert (HERE / "build522_aliases.py").is_file()
        print(json.dumps({"passed": True, "launched": False,
                          "modes": ["serial", "mpi2", "stats"]}))
        return
    mode = sys.argv[1]
    assert mode in ("serial", "mpi2", "stats")
    expose_build()
    status = json.loads((HERE / "build522_status.json").read_text())
    checks.ROOT = HERE
    checks.REPO = REPO
    checks.BINARY = Path(status["test_binary"])
    assert sha(checks.BINARY) == status["test_binary_sha256"]
    for key in list(os.environ):
        if key.startswith("SU2_NATIVE_"):
            del os.environ[key]
    os.environ.update(OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1",
                      MKL_NUM_THREADS="1", OMPI_MCA_osc="pt2pt")
    tmp = REPO / "integration_evidence/build-integrated-v2/compile-tmp"
    os.environ["OMPI_MCA_orte_tmpdir_base"] = tempfile.mkdtemp(
        prefix="section6-1-units-", dir=tmp)
    filters = "[NativeGeometryStats3D]" if mode == "stats" else "[NativeJobMemory3D]"
    if mode == "serial":
        filters += ",[NativeGeometryBackend3D],[NativeOwnedFaceProof3D],"
        filters += "[NativeGeometryStats3D],[NativeTimerTree3D]"
    checks.CASES["section6_1_units_" + mode] = (2 if mode == "mpi2" else 1, filters)
    original_save = checks.save

    def save_with_wrapper_pin(path, value):
        if Path(path).name == "pins.json":
            value[str(Path(__file__).resolve())] = sha(__file__)
            alias = HERE / "build522_aliases.py"
            value[str(alias.resolve())] = sha(alias)
        original_save(path, value)

    checks.save = save_with_wrapper_pin
    sys.argv = ["run_cases.py", "build522", "section6_1_units_" + mode]
    manifest = STAGED / "runtime_20261113_474/loaded_runtime_pins.json"
    checks.main(manifest)


if __name__ == "__main__":
    main()
