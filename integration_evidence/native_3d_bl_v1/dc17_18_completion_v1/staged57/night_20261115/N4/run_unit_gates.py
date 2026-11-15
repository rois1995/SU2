#!/usr/bin/env python3
"""Prepare and run the pinned N4 G1/G3/G4 checks; never builds the binaries."""
import argparse
import hashlib
import importlib.util
import json
import importlib.util
import os
import re
import subprocess
import sys
import tempfile
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
STAGED = HERE.parents[1]
REPO = STAGED.parents[3]
REF = HERE / "reference_build"
F3B = STAGED / "f3b_20261115"
RUNTIME = STAGED / "runtime_20261113_474/loaded_runtime_pins.json"
BL_SOURCE = STAGED / "cases/curved492_saved_production_surface_serial_20261113T163756_3724702"
BL_REFERENCE = STAGED / "cases/p6c_bl_20261115T003316_3200535"
BL_RECEIPT = F3B / "C492_repeat_receipt.json"
NATIVE3D_TESTS = REPO / "UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp"
INTEGER_ROWS = HERE / "compare_integer_rows.py"
GATES = {"g1": ("native3d_serial", "native3d_mpi2", "native3d_mpi4"),
         "g4": ("2d1", "2d2", "2d4"), "g3": ("bl",)}


def sha(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def process_identity(pid):
    try:
        stat = (Path("/proc") / str(pid) / "stat").read_text()
        return int(stat.rsplit(")", 1)[1].split()[19])
    except (OSError, IndexError, ValueError):
        return None


def build_inputs():
    status_path, pins_path = REF / "build_status.json", REF / "source_pins.json"
    status = json.loads(status_path.read_text())
    if status.get("state") != "terminal" or status.get("exit_code") != 0 or not status.get("pins_verified"):
        raise RuntimeError("build509 reference is not a verified successful terminal build")
    for name in ("binary", "test_binary"):
        binary = Path(status[name])
        if not binary.is_file():
            raise RuntimeError(f"missing reference {name}: {binary}")
        if sha(binary) != status[name + "_sha256"]:
            raise RuntimeError(f"reference {name} hash changed: {binary}")
    pins = json.loads(pins_path.read_text())
    changed = [path for path, digest in pins.items() if sha(path) != digest]
    if changed:
        raise RuntimeError("reference source pins changed: " + ", ".join(changed[:8]))
    return status, pins


def prepare_build509(status, pins):
    status_copy = dict(status, completed_build=True)
    for path, value in ((STAGED / "build509_status.json", status_copy),
                        (STAGED / "build509_source_pins.json", pins)):
        if path.exists():
            if json.loads(path.read_text()) != value:
                raise RuntimeError(f"refusing to replace existing receipt: {path}")
        else:
            path.write_text(json.dumps(value, indent=2) + "\n")
    handles = STAGED / "build509_terminal_handles.json"
    terminal = json.loads((REF / "terminal_handles.json").read_text())
    if terminal.get("actual_host_proc") is not True or terminal.get("all_owned_handles_terminal") is not True:
        raise RuntimeError("build509 terminal handles have not been verified on this host")
    handle_value = {"actual_host_proc": True, "all_owned_handles_terminal": True,
                    "identities": terminal.get("identities", []),
                    "verified_no_heavy_process_unix": terminal["verified_unix"]}
    if handles.exists():
        existing = json.loads(handles.read_text())
        if existing.get("all_owned_handles_terminal") is not True:
            raise RuntimeError("build509 terminal-handle receipt is not terminal")
    else:
        handles.write_text(json.dumps(handle_value, indent=2) + "\n")


def verify_bl_baseline():
    receipt = json.loads(BL_RECEIPT.read_text())
    gates = receipt["numerical"]["gates"]
    assert receipt["passed"] and gates["first_336_edits_identical"] and gates["at_least_336_splits"]
    assert receipt["numerical"]["reference_edits"] == 336
    assert receipt["numerical"]["actual_splits"] == 462
    for path in (BL_SOURCE / "command.json", BL_SOURCE / "run.cfg", BL_REFERENCE / "p6_bl_receipt.json"):
        assert path.is_file()
    prior = json.loads((BL_REFERENCE / "p6_bl_receipt.json").read_text())
    assert prior["passed"] and prior["gates"]["exact_input_and_target"] and prior["gates"]["same_frozen_configuration"]


def native3d_source_catalog():
    """Static inventory of public native-distributed 3D cases and rank guards."""
    source = NATIVE3D_TESTS.read_text()
    cases = list(re.finditer(r'TEST_CASE\("([^"]*)",\s*"([^"]*)"\)\s*\{', source))
    counts, guarded_counts = {}, {}
    public = []
    for index, match in enumerate(cases):
        body = source[match.end():cases[index + 1].start() if index + 1 < len(cases) else len(source)]
        tags = re.findall(r"\[([^\]]+)\]", match.group(2))
        if any(tag.startswith(".") for tag in tags):
            continue
        tags = [tag for tag in tags if tag.startswith("Native") and "3D" in tag]
        if not tags:
            continue
        rank_guard = bool(re.search(r"if\s*\([^)]*world\.(?:size|rank)", body))
        public.append(match.group(1))
        for tag in tags:
            counts[tag] = counts.get(tag, 0) + 1
            if rank_guard:
                guarded_counts[tag] = guarded_counts.get(tag, 0) + 1
    rank_guarded = sorted(tag for tag, count in counts.items() if guarded_counts.get(tag) == count)
    return {"source": str(NATIVE3D_TESTS), "public_case_count": len(public),
            "tags": sorted(counts), "rank_guarded_tags": rank_guarded,
            "guard_rule": "every case carrying tag has a lexical if(world.size|world.rank) in its body"}


def verify_driver_selection(binary, filter_expr, label):
    """List names only, then check them against source tag membership and hidden tags."""
    command = [str(binary), filter_expr, "--list-test-names-only"]
    env = os.environ.copy()
    env.update(OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1", MKL_NUM_THREADS="1", OMPI_MCA_osc="pt2pt")
    result = subprocess.run(command, cwd=REPO, env=env, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, check=False, timeout=30)
    actual = {line.strip() for line in result.stdout.splitlines() if line.strip()}
    wanted_tags = {tag.strip("[]") for term in filter_expr.split(",") for tag in re.findall(r"\[([^\]]+)\]", term)
                   if not tag.startswith(".")}
    expected, hidden = set(), set()
    test_re = re.compile(r'TEST_CASE\("([^"]*)",\s*"([^"]*)"\)')
    for source in (REPO / "UnitTests").rglob("*"):
        if not source.is_file() or source.suffix not in {".cpp", ".hpp", ".inc"}:
            continue
        try:
            content = source.read_text(errors="replace")
        except OSError:
            continue
        for match in test_re.finditer(content):
            name, raw_tags = match.groups()
            tags = set(re.findall(r"\[([^\]]+)\]", raw_tags))
            if tags & wanted_tags:
                if any(tag.startswith(".") for tag in tags):
                    hidden.add(name)
                else:
                    expected.add(name)
    if not actual:
        raise RuntimeError(f"test-name listing returned no names for {label}")
    unrelated = sorted(actual - expected)
    missing = sorted(expected - actual)
    hidden_selected = sorted(actual & hidden)
    if unrelated or missing or hidden_selected:
        raise RuntimeError(f"selection mismatch for {label}: unrelated={unrelated[:12]}, missing={missing[:12]}, hidden={hidden_selected[:12]}; driver_exit={result.returncode}")
    return {"label": label, "selected_names": len(actual), "source_expected_names": len(expected),
            "driver_exit_code": result.returncode, "unrelated": unrelated, "missing": missing, "hidden_selected": hidden_selected,
            "selected_names_sha256": hashlib.sha256("\n".join(sorted(actual)).encode()).hexdigest()}


def self_check():
    import ast
    source = Path(__file__).read_text()
    ast.parse(source)
    assert (STAGED / "run_cases.py").is_file()
    assert (F3B / "run_remaining_checks.py").is_file() and (F3B / "check_cheap_bl.py").is_file()
    assert RUNTIME.is_file()
    assert INTEGER_ROWS.is_file()
    assert hide_excluded_filter_terms("[A],[B]") == "[A]~[.],[B]~[.]"
    comparator = load_integer_rows_comparator()
    timer = comparator.Row("NATIVE3D_TIMER", {"calls": "7", "seconds": "0.0"}, "<self-check>", 1)
    assert comparator.integer_values(timer) == {"calls": 7}
    transformed = (F3B / "run_remaining_checks.py").read_text()
    transformed = transformed.replace("HERE=Path(__file__).resolve().parent;R=HERE.parent;REPO=R.parents[3]",
                                      "HERE=Path(N4_DIR);R=Path(STAGED_DIR);REPO=Path(REPO_DIR)")
    transformed = transformed.replace("build504", "build509")
    transformed = transformed.replace("(HERE/'build_status.json')", "(Path(REF_DIR)/'build_status.json')")
    transformed = transformed.replace("(HERE/'source_pins.json')", "(Path(REF_DIR)/'source_pins.json')")
    compile(transformed, str(F3B / "run_remaining_checks.py"), "exec")
    assert "REF_DIR" in transformed and "N4_DIR/'build_status.json'" not in transformed
    verify_bl_baseline()
    catalog = native3d_source_catalog()
    status_path = REF / "build_status.json"
    status = json.loads(status_path.read_text())
    if status.get("state") == "running":
        for key in ("controller", "identity"):
            process = status.get(key, {})
            assert process_identity(process.get("pid")) == process.get("start_ticks"), f"stale {key} handle"
        return {"syntax": "pass", "BL_reference": "pass", "build509": "running; gate launch correctly unavailable",
                "G1_native3d_catalog": catalog}
    assert status.get("state") == "terminal"
    build_inputs()
    return {"syntax": "pass", "BL_reference": "pass", "build509": "verified terminal",
            "G1_native3d_catalog": catalog}


def import_run_cases():
    sys.path.insert(0, str(STAGED))
    import run_cases as checks
    status, pins = build_inputs()
    prepare_build509(status, pins)
    checks.BINARY = Path(status["test_binary"])
    return checks


def pin_helper(checks):
    original = checks.save
    helper = str(Path(__file__).resolve())
    def save(path, value):
        if Path(path).name == "pins.json":
            value[helper] = sha(helper)
        original(path, value)
    checks.save = save


def launch_preflight(label):
    active = []
    for proc in Path("/proc").glob("[0-9]*"):
        try:
            name = (proc / "comm").read_text().strip()
            if name in {"SU2_CFD", "test_driver", "cc1plus", "ninja", "lto1"}:
                active.append((int(proc.name), name))
        except OSError:
            continue
    if active:
        raise RuntimeError(f"cannot launch {label}; heavy processes are active: {active[:8]}")
    snapshot = {"gate": label, "unix": time.time(), "loadavg": Path("/proc/loadavg").read_text().strip(),
                "MemAvailable": next(row for row in Path("/proc/meminfo").read_text().splitlines()
                                     if row.startswith("MemAvailable:"))}
    receipt = HERE / "launch_preflight.jsonl"
    with receipt.open("a") as stream:
        stream.write(json.dumps(snapshot) + "\n")


def hide_excluded_filter_terms(filter_expr):
    """Catch2 comma-separated filters are OR; attach hidden exclusion to each term."""
    terms = [term.strip() for term in filter_expr.split(",") if term.strip()]
    if not terms:
        raise ValueError("empty Catch2 test filter")
    return ",".join(term + "~[.]" for term in terms)


def load_integer_rows_comparator():
    spec = importlib.util.spec_from_file_location("n4_compare_integer_rows", INTEGER_ROWS)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot import pinned row comparator: {INTEGER_ROWS}")
    comparator = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = comparator
    spec.loader.exec_module(comparator)
    return comparator


def run_run_cases(checks, name, ranks, filters):
    launch_preflight(name)
    checks.CASES[name] = (ranks, filters)
    sys.argv = [str(STAGED / "run_cases.py"), "build509", name]
    try:
        checks.main(RUNTIME)
    except SystemExit as result:
        if result.code not in (None, 0):
            raise


def run_g1(checks):
    serial_ranks, serial_filters = checks.CASES["serial_full"]
    assert serial_ranks == 1
    # Add public native 3D coverage missing from serial_full, including recent
    # backend/timer/guidance tags. The broad MPI cohort remains its existing
    # explicitly rank-guarded selection.
    catalog = native3d_source_catalog()
    serial_filters = ",".join((serial_filters,
        *[f"[{tag}]" for tag in catalog["tags"]],
        "[NativeGeometryBackend3D]", "[NativeTimerTree3D]",
        "[NativeBoundedGuidance3D]", "[NativeBoundaryGuidanceMemo3D]"))
    serial_filters = hide_excluded_filter_terms(serial_filters)
    binary = checks.BINARY
    serial_listing = verify_driver_selection(binary, serial_filters, "g1_serial")
    run_run_cases(checks, "n4_native3d_serial", 1, serial_filters)
    mpi_ranks, mpi_filters = checks.CASES["mpi2"]
    assert mpi_ranks == 2
    mpi_filters = ",".join((mpi_filters, *[f"[{tag}]" for tag in catalog["rank_guarded_tags"]]))
    mpi_filters = hide_excluded_filter_terms(mpi_filters)
    mpi_listing = verify_driver_selection(binary, mpi_filters, "g1_mpi2_mpi4")
    run_run_cases(checks, "n4_native3d_mpi2", 2, mpi_filters)
    # The same existing MPI guarded tests also run at four ranks; no test code changes.
    run_run_cases(checks, "n4_native3d_mpi4", 4, mpi_filters)
    print(json.dumps({"G1_selection": [serial_listing, mpi_listing]}, indent=2), flush=True)


def run_f3b(mode):
    """Execute the existing F3b runner with only its build label/root retargeted."""
    path = F3B / "run_remaining_checks.py"
    text = path.read_text()
    old = "HERE=Path(__file__).resolve().parent;R=HERE.parent;REPO=R.parents[3]"
    assert text.count(old) == 1
    text = text.replace(old, "HERE=Path(N4_DIR);R=Path(STAGED_DIR);REPO=Path(REPO_DIR)")
    assert "build504" in text
    text = text.replace("build504", "build509")
    text = text.replace("(HERE/'build_status.json')", "(Path(REF_DIR)/'build_status.json')")
    text = text.replace("(HERE/'source_pins.json')", "(Path(REF_DIR)/'source_pins.json')")
    text = text.replace("N4_DIR': str(HERE)", "N4_DIR': str(HERE), 'REF_DIR': str(REF)")
    text = text.replace("HERE.parent/'purpose_query_fix_20261114/", "R/'purpose_query_fix_20261114/")
    text = text.replace("case='f3b_'+mode", "case='n4_'+mode")
    text = text.replace("gate_path=HERE/'check_cheap_bl.py'", "gate_path=Path(F3B_GATE)")
    pin_anchor = "extra_pins={str(Path(__file__).resolve()):sha(__file__)}"
    assert text.count(pin_anchor) == 1
    text = text.replace(pin_anchor, pin_anchor + ";extra_pins[N4_HELPER]=sha(N4_HELPER);extra_pins[INTEGER_ROWS]=sha(INTEGER_ROWS)")
    namespace = {"__name__": "__main__", "__file__": str(path), "N4_DIR": str(HERE),
                 "REF_DIR": str(REF), "STAGED_DIR": str(STAGED), "REPO_DIR": str(REPO),
                 "N4_HELPER": str(Path(__file__).resolve()), "INTEGER_ROWS": str(INTEGER_ROWS),
                 "F3B_GATE": str(F3B / "check_cheap_bl.py"), "sys": sys}
    launch_preflight("n4_" + mode)
    sys.argv = [str(path), mode]
    try:
        exec(compile(text, str(path), "exec"), namespace)
    except SystemExit as result:
        if result.code not in (None, 0):
            raise
    if mode == "bl":
        cases = sorted((STAGED / "cases").glob("n4_bl_*"), key=lambda path: path.stat().st_mtime)
        if not cases:
            raise RuntimeError("G3 BL runner produced no case directory")
        case = cases[-1]
        comparator = load_integer_rows_comparator()
        log_path = case / "run.log"
        raw_lines = log_path.read_text(errors="replace").splitlines()
        rows = [{"marker": row.marker, "integer_values": comparator.integer_values(row),
                 "fields": row.fields, "line": row.line, "raw": raw_lines[row.line - 1]}
                for row in comparator.marker_rows(log_path)]
        if not rows:
            raise RuntimeError("G3 BL run produced no integer counter rows")
        (HERE / "g3_integer_rows.json").write_text(json.dumps({"case": str(case), "rows": rows,
            "parser": str(INTEGER_ROWS),
            "stdout": str(case / "run.log"), "stdout_sha256": sha(case / "run.log")}, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("gate", choices=("self-check", "g1", "g3", "g4", "all"))
    args = parser.parse_args()
    if args.gate == "self-check":
        print(json.dumps(self_check(), indent=2))
        return
    status, pins = build_inputs()
    prepare_build509(status, pins)
    verify_bl_baseline()
    if args.gate in ("g1", "all"):
        checks = import_run_cases(); pin_helper(checks); run_g1(checks)
    if args.gate in ("g3", "all"):
        run_f3b("bl")
    if args.gate in ("g4", "all"):
        for mode in GATES["g4"]:
            run_f3b(mode)


if __name__ == "__main__":
    main()
