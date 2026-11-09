"""One bounded MPI6 control: archived296 executable, current303 host/source pins."""
import json
import os
import sys
import tempfile
from pathlib import Path

import run_cases

R = Path(__file__).resolve().parent
ARCHIVE = R / "baselines/qualified_euler296_pre_bl_v1"
MANIFEST = ARCHIVE / "manifest.json"
TESTS = [
    "Native3D driver publishes only accepted mesh and transfers affine BDF2 states",
    "Actual CFD import generates bounded bump sensor columns with collective provenance and refusal",
    "Actual CFD import generates a finite spherical sensor shell with copied boundary values",
    "Native3D production wall policy preserves sensor donors and RANS histories across worker counts",
    "Mapped native runtime completes a uniform target before conservative BDF2 transfer",
]


def main():
    manifest = json.loads(MANIFEST.read_text())
    archive_pins = {str(ARCHIVE / name): digest for name, digest in manifest["archived_files"].items()}
    archive_pins[str(MANIFEST)] = run_cases.sha(MANIFEST)
    archive_pins[str(Path(__file__).resolve())] = run_cases.sha(__file__)
    changed = [path for path, digest in archive_pins.items() if run_cases.sha(path) != digest]
    if changed:
        raise RuntimeError("Archived296 provenance changed: " + repr(changed[:8]))
    run_cases.BINARY = ARCHIVE / "binaries/test_driver"
    if str(run_cases.BINARY) not in archive_pins:
        raise RuntimeError("Archived executable missing from manifest")
    source = (ARCHIVE / "source_tree/UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp").read_text()
    if not all('TEST_CASE("' + name + '"' in source for name in TESTS):
        raise RuntimeError("Control filter does not match archived test names")
    case = "archived296_failures303_mpi6"
    run_cases.CASES[case] = (6, ",".join(TESTS))
    original_save, original_sample = run_cases.save, run_cases.sample_tree
    attempt = None
    handles = {}

    def sample(pid):
        rows = original_sample(pid)
        for row in rows:
            try:
                row["start_ticks"] = int(Path(f'/proc/{row["pid"]}/stat').read_text().rsplit(")", 1)[1].split()[19])
                handles[(row["pid"], row["start_ticks"])] = dict(row)
            except (FileNotFoundError, ProcessLookupError):
                pass
        return rows

    def save(path, value):
        nonlocal attempt
        attempt = Path(path).parent
        if Path(path).name == "pins.json":
            value.update(archive_pins)
        if Path(path).name in ("command.json", "status.json"):
            value["binary_provenance"] = "archived qualified_euler296_pre_bl_v1; build303 labels only current source/runtime controls"
            value["archive_manifest"] = str(MANIFEST)
        original_save(path, value)

    run_cases.save, run_cases.sample_tree = save, sample
    for key in list(os.environ):
        if key.startswith("SU2_NATIVE_"):
            del os.environ[key]
    scratch = R.parents[3] / "integration_evidence/build-integrated-v2/compile-tmp"
    os.environ.update(OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1", MKL_NUM_THREADS="1",
                      OMPI_MCA_osc="pt2pt", TMPDIR=str(scratch),
                      OMPI_MCA_orte_tmpdir_base=tempfile.mkdtemp(prefix="control296-303-", dir=scratch))
    sys.argv = [run_cases.__file__, "build303", case]
    try:
        run_cases.main()
    finally:
        changed = [path for path, digest in archive_pins.items() if run_cases.sha(path) != digest]
        if attempt:
            live = []
            for (pid, ticks), row in handles.items():
                try:
                    if int(Path(f"/proc/{pid}/stat").read_text().rsplit(")", 1)[1].split()[19]) == ticks:
                        live.append(row)
                except (FileNotFoundError, ProcessLookupError):
                    pass
            original_save(attempt / "archive_control_terminal.json", {
                "archive_pins_verified": not changed, "changed": changed,
                "observed_owned_handles": list(handles.values()), "remaining_owned_handles": live,
                "all_observed_owned_handles_terminal": not live,
                "scope": "Exactly five archived296 tests; one existing runner 600-second bound; no broad qualification",
            })
        if changed:
            raise RuntimeError("Archived296 provenance changed during control: " + repr(changed[:8]))


if __name__ == "__main__":
    main()
