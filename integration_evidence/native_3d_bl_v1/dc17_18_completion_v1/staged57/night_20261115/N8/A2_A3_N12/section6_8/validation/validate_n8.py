#!/usr/bin/env python3
"""Run one scoped N8 G3, safeguard, or strict full-workflow validation action."""
import argparse
import hashlib
import importlib.util
import json
import math
import os
import re
import shlex
import subprocess
import sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
A2 = HERE.parent
N8 = A2.parent.parent
STAGED = N8.parents[1]
REPO = STAGED.parents[3]
NIGHT = N8.parent
SECTION1 = NIGHT / "section6_1"
SECTION2 = NIGHT / "section6_2"
BUILD = A2 / "build534"
IDENTITY_PATH = NIGHT / "N9/section6_6/drafts/check_g3_identity_66.py"
WALL_REFERENCE_LOG = NIGHT / "N9/section6_6/cases/N9_66_bl_20261116T135717_78861/run.log"
REFERENCE_CASE = SECTION1 / "cases/section6_1_bl_20261116T082025_3350201"
PAYLOAD_REFERENCE = SECTION1 / "accepted_reference_payloads.json"
WALL_CLASSES = ("SplitEdge", "InsertEdgeCavity", "other_geometry", "RefitWallSplit",
                "ReconstructWallSplit", "ReconstructSplitPatch")
WALL_REASONS = ("orientation", "interface", "surface_containment", "feature",
                "marker_junction", "incomplete_star", "other")
WALL_FRAMES = ("RefitWallSplit", "ReconstructWallSplit", "ReconstructSplitPatch")
WALL_GUARD_CLASSES = ("PreservePassingFirstHeights", "height_check", "boundary_replacement_ready")


def sha(path):
    """Hash a provenance or output file."""
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def load(name, path):
    """Load a pinned helper by explicit path."""
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def parse_wall_lines(lines):
    """Parse six operator classes, bounded reasons, and fixed frame rows."""
    parsed = {"refusals": [], "reasons": [], "frames": []}
    markers = ("NATIVE3D_WALL_REFUSAL", "NATIVE3D_WALL_REASON", "NATIVE3D_WALL_FRAME")
    for line_number, line in enumerate(lines, 1):
        for kind in markers:
            if kind not in line:
                continue
            payload = line[line.index(kind) + len(kind):]
            fields = {key: value for key, value in
                      (token.split("=", 1) for token in shlex.split(payload) if "=" in token)}
            parsed[{markers[0]: "refusals", markers[1]: "reasons", markers[2]: "frames"}[kind]].append(
                {"line": line_number, **fields})
            break
    return parsed


def wall_diagnostic_report(candidate_log, old_log):
    """Report new wall reason/frame tables and virtual legacy geometry totals."""
    current = parse_wall_lines(Path(candidate_log).read_text(errors="replace").splitlines())
    old = parse_wall_lines(Path(old_log).read_text(errors="replace").splitlines())
    refusal = {(row.get("rank"), row.get("class"), row.get("reason")): int(row["count"])
               for row in current["refusals"] if row.get("class") in WALL_CLASSES and "reason" in row}
    frames = [{key: row[key] for key in ("rank", "operation", "calls", "valid", "seconds")}
              for row in current["frames"]]
    operator_totals = {}
    for (rank, operator, _), count in refusal.items():
        operator_totals[rank] = operator_totals.get(rank, 0) + count
    guard_totals = {}
    for row in current["refusals"]:
        if row.get("class") in WALL_GUARD_CLASSES:
            rank = row["rank"]
            guard_totals[rank] = guard_totals.get(rank, 0) + int(row["count"])
    all_guard_totals = {rank: operator_totals.get(rank, 0) + guard_totals.get(rank, 0)
                        for rank in sorted(set(operator_totals) | set(guard_totals))}
    reason_totals = {}
    for row in current["reasons"]:
        rank, reason = row.get("rank"), row.get("text")
        assert rank is not None and reason is not None and (reason == "<more>" or re.search(r"\d", reason) is None)
        reason_totals[rank] = reason_totals.get(rank, 0) + int(row["count"])
    virtual, old_geometry = {}, {}
    for table, target, classes in ((refusal, virtual, {"other_geometry", *WALL_FRAMES}),
                                   ({(r.get("rank"), r.get("class"), r.get("reason")): int(r["count"])
                                     for r in old["refusals"] if r.get("class") == "other_geometry" and "reason" in r},
                                    old_geometry, {"other_geometry"})):
        for (rank, operator, reason), count in table.items():
            if operator in classes:
                target.setdefault(rank, {})[reason] = target.setdefault(rank, {}).get(reason, 0) + count
    ranks = sorted(set(operator_totals) | set(guard_totals) | set(reason_totals) | set(virtual) | set(old_geometry))
    refusal_keys = {(row.get("rank"), row.get("class"), row.get("reason"))
                    for row in current["refusals"] if row.get("class") in WALL_CLASSES and "reason" in row}
    refusal_schema = {rank: {"missing": [f"{operator}/{reason}" for operator in WALL_CLASSES for reason in WALL_REASONS
                              if (rank, operator, reason) not in refusal_keys],
                              "duplicate_rows": len([row for row in current["refusals"]
                                  if row.get("rank") == rank and row.get("class") in WALL_CLASSES and
                                  "reason" in row]) - sum(key[0] == rank for key in refusal_keys)} for rank in ranks}
    return {"operator_classes": list(WALL_CLASSES), "reason_names": list(WALL_REASONS),
            "refusal_schema_by_rank": refusal_schema,
            "frame_rows": frames, "frame_schema_complete": bool(frames) and all(
                sum(row.get("operation") == op and row.get("rank") == rank for row in frames) == 1
                for op in WALL_FRAMES for rank in {row.get("rank") for row in frames}),
            "reason_digit_normalized": True,
            "reason_sum_by_rank": reason_totals, "raw_operator_refusal_sum_by_rank": operator_totals,
            "non_guidance_guard_refusal_sum_by_rank": all_guard_totals,
            "reason_sum_reconciles": {rank: reason_totals.get(rank, 0) == all_guard_totals.get(rank, 0)
                                       for rank in ranks},
            "virtual_legacy_other_geometry_by_rank_reason": virtual,
            "old_other_geometry_by_rank_reason": old_geometry,
            "legacy_difference_by_rank_reason": {
                rank: {reason: virtual.get(rank, {}).get(reason, 0) - old_geometry.get(rank, {}).get(reason, 0)
                       for reason in sorted(set(virtual.get(rank, {})) | set(old_geometry.get(rank, {})))}
                for rank in ranks}}


def wall_report_self_check(old_log):
    """Use the saved N9 rows to check the legacy virtual-sum conversion."""
    old = parse_wall_lines(Path(old_log).read_text(errors="replace").splitlines())
    rows = []
    old_ops = {}
    for row in old["refusals"]:
        if row.get("class") in WALL_CLASSES[:3] and "reason" in row:
            old_ops[(row["class"], row["reason"])] = int(row["count"])
    for operator in WALL_CLASSES:
        for reason in WALL_REASONS:
            old_operator = "other_geometry" if operator == "RefitWallSplit" else operator
            count = old_ops.get((old_operator, reason), 0) if operator in (*WALL_CLASSES[:2], "RefitWallSplit") else 0
            rows.append(f"NATIVE3D_WALL_REFUSAL rank=0 class={operator} reason={reason} count={count}")
    for row in old["refusals"]:
        if row.get("class") in WALL_GUARD_CLASSES and "count" in row:
            rows.append("NATIVE3D_WALL_REFUSAL rank={rank} class={class} count={count}".format(**row))
    rows.append("NATIVE3D_WALL_REFUSAL rank=0 class=height_check count=3")
    total = sum(int(row["count"]) for row in old["refusals"]
                if row.get("class") in (*WALL_CLASSES, *WALL_GUARD_CLASSES)) + 3
    rows.append(f'NATIVE3D_WALL_REASON rank=0 text="<more>" count={total}')
    for operation in WALL_FRAMES:
        rows.append(f"NATIVE3D_WALL_FRAME rank=0 operation={operation} calls=0 valid=0 seconds=0")
    with __import__("tempfile").NamedTemporaryFile("w", encoding="utf-8") as stream:
        stream.write("\n".join(rows) + "\n")
        stream.flush()
        report = wall_diagnostic_report(stream.name, old_log)
    assert report["reason_sum_reconciles"] == {"0": True}
    assert report["virtual_legacy_other_geometry_by_rank_reason"] == report["old_other_geometry_by_rank_reason"]
    assert report["frame_schema_complete"] and report["reason_digit_normalized"]
    assert not report["refusal_schema_by_rank"]["0"]["missing"]
    assert report["non_guidance_guard_refusal_sum_by_rank"]["0"] == total
    return {"passed": True, "legacy_virtual_sum": report["virtual_legacy_other_geometry_by_rank_reason"]}



def full_wall_timer_report(case, require_wall=False):
    """Check every saved timer summary and retain all wall-law rows and costs."""
    streams = sorted(Path(case).glob("rank_output/*/rank.*/stdout"))
    assert len(streams) == 6, f"expected six rank streams covering both events, found {len(streams)}"
    summaries, wall_rows = [], []
    for path in streams:
        raw_lines = path.read_text(errors="replace").splitlines()
        normalized = []
        for raw in raw_lines:
            line = re.sub(r"(?:Mon|Tue|Wed|Thu|Fri|Sat|Sun) [A-Z][a-z]{2} \d{1,2} "
                          r"\d\d:\d\d:\d\d \d{4}<(?:stdout|stderr)>:", "", raw)
            if line.startswith("=") and normalized:
                normalized[-1] += line
            else:
                normalized.append(line)
        for line_number, raw in enumerate(normalized, 1):
            for marker, output in (("NATIVE3D_TIMER_SUMMARY", summaries), ("NATIVE3D_WALL_LAW", wall_rows)):
                if marker not in raw:
                    continue
                payload = raw[raw.index(marker) + len(marker):]
                fields = {key: value for key, value in
                          (token.split("=", 1) for token in shlex.split(payload) if "=" in token)}
                output.append({"file": str(path.relative_to(case)), "line": line_number, **fields})
                break
    assert summaries, "no NATIVE3D_TIMER_SUMMARY rows found"
    for row in summaries:
        assert row.get("fixedstorage") == "4120", f"timer fixedstorage is not 4120: {row}"
    for row in wall_rows:
        seconds = float(row["seconds"])
        assert math.isfinite(seconds) and seconds >= 0.0, f"invalid wall-law seconds: {row}"
        assert row.get("boundary") in ("entry", "exit")
    wall_ranks = {int(row["rank"]) for row in wall_rows}
    assert not require_wall or wall_ranks == set(range(6)), f"wall-law rows lack rank coverage: {wall_ranks}"
    return {"timer_summary_count": len(summaries), "all_fixedstorage_4120": True,
            "wall_law_row_count": len(wall_rows), "wall_law_rows": wall_rows,
            "wall_law_rank_coverage": sorted(wall_ranks), "wall_law_required": require_wall,
            "wall_law_seconds_sum": sum(float(row["seconds"]) for row in wall_rows)}


def prepare_build():
    """Require a terminal build and verify aliases before any case action."""
    status = json.loads((BUILD / "build_status.json").read_text())
    assert status.get("state") == "terminal" and status.get("exit_code") == 0
    assert status.get("pins_verified") is True
    alias = load("n8_build534_aliases", HERE / "build534_aliases.py")
    alias.main()
    return status


def self_check():
    """Check source retargeting and accepted reference schemas without a launch."""
    assert (HERE / "check_connected_events.py").resolve() == (SECTION1 / "check_connected_events.py").resolve()
    identity = load("n8_identity_check", IDENTITY_PATH)
    result = identity.self_check()
    assert result["passed"] and result["launched"] is False
    accepted = json.loads(PAYLOAD_REFERENCE.read_text())
    assert accepted.get("accepted") is True and len(accepted.get("hashes", {})) == 6
    base = load("n8_section62_checks", SECTION2 / "run_remaining_checks.py")
    transformed = {mode: adapted_safeguard_source(base, mode)
                   for mode in ("bl", "merit481", "saved481", "saved_current", "2d1", "2d2", "2d4")}
    assert "19_999_490_048" in transformed["bl"]
    assert "set_bl_wrapper_timeout(checks)" in bl_wrapper_source(transformed["bl"])
    assert all("build534" in source for source in transformed.values())
    full_module = load("n8_section62_full_selfcheck", SECTION2 / "run_full_section6_2.py")
    full_module.self_check()
    full_module.HERE = HERE
    full_module.STAGED = STAGED
    full_module.REPO = REPO
    full_module.OLD = STAGED / "p7p8_r3_20261115"
    runner = full_module.load_runner()
    runner.update(PURPOSE=HERE, R=STAGED, REPO=REPO,
                  BUILD_STATUS=BUILD / "build_status.json", SOURCE_PINS=BUILD / "source_pins.json")
    assert runner["PURPOSE"] == HERE and runner["R"] == STAGED
    assert runner["BUILD_STATUS"] == BUILD / "build_status.json"
    comparator = load("n8_payload_schema_check", SECTION2 / "compare_reference_payloads.py")
    workflow = comparator.workflow_passed(SECTION1 / "full_workflow")
    assert len(workflow["events"]) == 2 and workflow["P7_P8_events_checked"] == [1, 2]
    wall_check = wall_report_self_check(WALL_REFERENCE_LOG)
    timer_check = full_wall_timer_report(SECTION1 / "full_workflow")
    return {"passed": True, "launched": False, "g3_identity": result,
            "payload_reference_sha256": sha(PAYLOAD_REFERENCE), "wall_report_self_check": wall_check,
            "reference_timer_report": timer_check}


def adapted_safeguard_source(base, mode):
    """Retarget one inherited safeguard to validation paths and frozen build534."""
    source = base.adapted_source(mode)
    changes = (
        ("HERE=Path(__file__).resolve().parent;STAGED=HERE.parents[1];R=STAGED;REPO=STAGED.parents[3]",
         "HERE=Path(__file__).resolve().parent;STAGED=HERE.parents[4];R=STAGED;REPO=STAGED.parents[3];A2=HERE.parent"),
        ("from build525_aliases import main as expose_build", "from build534_aliases import main as expose_build"),
        ("build525", "build534"),
        ("expose_build();checks.ROOT=HERE;checks.REPO=REPO",
         "sys.path.insert(0,str(HERE));sys.path.insert(1,str(A2));expose_build();checks.ROOT=HERE;checks.REPO=REPO"),
    )
    for before, after in changes:
        assert before in source, before
        source = source.replace(before, after)
    compile(source, str(HERE / "run_remaining_checks.py"), "exec")
    return source


def bl_wrapper_source(source):
    """Install the C492 timeout hook after the existing build alias setup."""
    anchor = "expose_build();checks.ROOT=HERE;checks.REPO=REPO"
    assert source.count(anchor) == 1
    return source.replace(anchor, anchor + "\nset_bl_wrapper_timeout(checks)", 1)


def bl_wrapper_timeout(checks):
    """Give only the C492 fixture time to finish its authorized census observations."""
    import inspect
    source = inspect.getsource(checks.main)
    old = 'timeout_seconds = 2400 if case == "m6_stage" else 600'
    assert source.count(old) == 1
    source = source.replace(old, 'timeout_seconds = 900 if case == "section6_2_bl" else 2400 if case == "m6_stage" else 600', 1)
    exec(compile(source, checks.__file__, "exec"), checks.__dict__)


def run_safeguard(mode):
    """Reuse section 6.2 safeguard source with validation-root and build534 paths."""
    if mode in ("merit481", "saved481", "saved_current"):
        section62 = load_section62_safeguards()
        if mode == "merit481":
            section62.run_geometry()
        else:
            section62.run_saved(mode)
        return
    base = load("n8_section62_checks", SECTION2 / "run_remaining_checks.py")
    source = adapted_safeguard_source(base, mode)
    if mode == "bl":
        source = bl_wrapper_source(source)
        assert source.count("gate.self_check();") == 1
        source = source.replace("gate.self_check();", "gate.self_check();match_bl_wrapper(gate);", 1)
    os.environ["OMPI_MCA_osc"] = "pt2pt"
    namespace = {"__name__": "__main__", "__file__": str(HERE / "validate_n8.py"),
                 "OLD": base.OLD, "HERE": HERE, "STAGED": STAGED, "REPO": REPO,
                 "set_bl_wrapper_timeout": bl_wrapper_timeout, "match_bl_wrapper": match_bl_wrapper}
    sys.argv = [str(HERE / "validate_n8.py"), mode]
    exec(compile(source, str(HERE / "validate_n8.py"), "exec"), namespace)


def load_section62_safeguards():
    """Load Section 6.2's case-root aware geometry/saved dispatch at build534."""
    path = SECTION2 / "run_remaining_checks.py"
    source = path.read_text()
    assert "build525" in source and "build534" not in source
    source = source.replace("build525", "build534")
    geometry_pin = "value[str(Path(__file__).resolve())] = sha(__file__)"
    saved_pin = "pinned_inputs = {str(Path(__file__).resolve()): sha(__file__),"
    assert source.count(geometry_pin) == source.count(saved_pin) == 1
    source = source.replace(
        geometry_pin,
        "value[str(Path(__file__).resolve())] = sha(__file__)\n"
        "            value[str((HERE / 'validate_n8.py').resolve())] = sha(HERE / 'validate_n8.py')")
    source = source.replace(
        saved_pin,
        "pinned_inputs = {str(Path(__file__).resolve()): sha(__file__),\n"
        "                     str((HERE / 'validate_n8.py').resolve()): sha(HERE / 'validate_n8.py'),")
    # The legacy v2 input is rejected before operators; use the qualified v3 same-seed target.
    target_binding = 'if mode == "saved_current":\n        producer_receipt ='
    assert source.count(target_binding) == 1
    source = source.replace(target_binding,
                            'if mode in ("saved481", "saved_current"):\n        producer_receipt =', 1)
    namespace = {"__name__": "n8_section62_safeguards", "__file__": str(path)}
    exec(compile(source, str(path), "exec"), namespace)
    namespace.update(HERE=HERE, STAGED=STAGED, REPO=REPO)
    namespace.update(OLD=STAGED / "p7p8_r3_20261115/run_remaining_checks.py",
                     GEOMETRY=STAGED / "check_geometry_magnitude481.py",
                     SAVED=STAGED / "check_saved_surface_corrections481.py")
    return __import__("types").SimpleNamespace(**namespace)

def match_bl_wrapper(gate):
    """Match the exact local test wrapper while retaining every physical predicate."""
    original = gate.prior.conditions
    def conditions(status, command, row, identical):
        result = original(status, command, row, identical)
        result["serial_bounded_command"] = command.get("ranks") == 1 and command.get("timeout_seconds") == 900
        return result
    gate.prior.conditions = conditions


def load_bl_gate():
    """Load the existing physical guard with current job limit and fixture wrapper."""
    path = STAGED / "p7p8_r3_20261115/check_cheap_bl.py"
    source = path.read_text()
    assert source.count("20_000_000_000") == 1
    source = source.replace("20_000_000_000", "19_999_490_048", 1)
    source = source.replace("whole_job_peak_RSS_20GB_serial", "whole_job_peak_RSS_19073_MiB_serial")
    source = source.replace("20GB serial peak gate", "19073 MiB serial peak gate")
    namespace = {"__name__": "section6_8_bl_guard", "__file__": str(path)}
    exec(compile(source, str(path), "exec"), namespace)
    gate = __import__("types").SimpleNamespace(**namespace)
    gate.self_check()
    match_bl_wrapper(gate)
    return gate


def run_g3():
    """Apply the accepted 6.6 identity rule after the exact cheap-BL physical guard."""
    summary = HERE / "g3_identity_validation.json"
    assert not summary.exists(), "refusing to overwrite G3 result"
    case_dir = HERE / "cases"
    case_dir.mkdir(exist_ok=True)
    before = set(case_dir.glob("section6_2_bl_*"))
    with (HERE / "g3_controller.log").open("x") as log:
        child = subprocess.run([sys.executable, str(HERE / "validate_n8.py"), "safeguard", "bl"],
                               cwd=HERE, stdout=log, stderr=subprocess.STDOUT, check=False)
    made = set(case_dir.glob("section6_2_bl_*")) - before
    assert len(made) == 1, "expected exactly one fresh serial physical-guard case"
    case = made.pop()
    return check_g3_case(case, child.returncode)


def check_g3_case(case, child_returncode=0):
    """Check existing numerical artifacts; never launch or repeat the fixture."""
    case = Path(case).resolve(strict=True)
    summary = HERE / "g3_identity_validation.json"
    guard_failed = load_bl_gate().main(case)
    cheap = json.loads((case / "cheap_bl_receipt.json").read_text())
    if guard_failed or cheap.get("passed") is not True:
        summary.write_text(json.dumps({"passed": False, "first_failed_gate": "cheap_bl_physical_guard",
                                       "case": str(case), "child_returncode": child_returncode}, indent=2)+"\n")
        raise SystemExit(1)
    identity = load("n8_identity_check", IDENTITY_PATH)
    # The historical all-integer comparison is preserved as report-only.
    comparator = NIGHT / "N4/compare_integer_rows.py"
    diagnostic = subprocess.run([sys.executable, str(comparator), str(REFERENCE_CASE), str(case), "--gate", "all"],
                                capture_output=True, text=True, check=False)
    try:
        old_integer = json.loads(diagnostic.stdout)
    except json.JSONDecodeError:
        old_integer = {"stdout": diagnostic.stdout, "stderr": diagnostic.stderr}
    (case / "g3_integer_comparison.json").write_text(json.dumps(
        {"returncode": diagnostic.returncode, "comparison": old_integer,
         "comparator_sha256": sha(comparator)}, indent=2)+"\n")
    decision = identity.evaluate(REFERENCE_CASE, case)
    wall_report = wall_diagnostic_report(case / "run.log", WALL_REFERENCE_LOG)
    wall_report_path = case / "g3_wall_frame_reason_report.json"
    wall_report_path.write_text(json.dumps(wall_report, indent=2) + "\n")
    result = {"passed": decision["passed"], "case": str(case), "child_returncode": child_returncode,
              "identity": decision, "old_integer_comparison_gate": False,
              "physical_guard_receipt_sha256": sha(case / "cheap_bl_receipt.json"),
              "wall_frame_reason_report": str(wall_report_path),
              "wall_reason_reconciles": wall_report["reason_sum_reconciles"]}
    summary.write_text(json.dumps(result, indent=2)+"\n")
    if not result["passed"]:
        raise SystemExit(1)


def run_full(case):
    """Run P7/P8 strict two-event workflow, then compare all six accepted N1 hashes."""
    status = prepare_build()
    wrapper = load("n8_section62_full", SECTION2 / "run_full_section6_2.py")
    wrapper.HERE = HERE
    wrapper.STAGED = STAGED
    wrapper.REPO = REPO
    wrapper.OLD = STAGED / "p7p8_r3_20261115"
    runner = wrapper.load_runner(status["binary"])
    runner.update(SCRIPT=HERE / "validate_n8.py", PURPOSE=HERE, R=STAGED, REPO=REPO,
                  BUILD_STATUS=BUILD / "build_status.json", SOURCE_PINS=BUILD / "source_pins.json",
                  BINARY=Path(status["binary"]))
    assert runner["sha"](runner["BINARY"]) == status["binary_sha256"]
    sys.path.insert(0, str(HERE))
    sys.path.insert(1, str(wrapper.OLD))
    sys.argv = [str(HERE / "validate_n8.py"), str(case), "--full"]
    runner["main"]()
    timer_report = full_wall_timer_report(case, require_wall=True)
    (case / "n8_wall_timer_report.json").write_text(json.dumps(timer_report, indent=2) + "\n")
    compare = load("n8_payload_comparator", SECTION2 / "compare_reference_payloads.py")
    payloads = json.loads(PAYLOAD_REFERENCE.read_text())
    actual = compare.payload_hashes(case)
    mismatch = compare.compare_hashes(actual, payloads["hashes"])
    receipt = {"case": str(case), "reference": str(PAYLOAD_REFERENCE),
               "reference_sha256": sha(PAYLOAD_REFERENCE), "hashes": actual,
               "mismatches": mismatch, "passed": not mismatch,
               "workflow_gates_passed": True, "timer_and_wall_report": timer_report}
    (case / "n8_payload_identity.json").write_text(json.dumps(receipt, indent=2)+"\n")
    assert receipt["passed"], "six event payloads differ from accepted N1 hashes"


def main():
    """Dispatch read-only preflight or one separately selected validation action."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("self-check", "g3", "safeguard", "full"))
    parser.add_argument("mode", nargs="?")
    parser.add_argument("case", nargs="?", type=Path)
    args = parser.parse_args()
    if args.action == "self-check":
        print(json.dumps(self_check(), indent=2))
    elif args.action == "g3":
        run_g3()
    elif args.action == "safeguard":
        assert args.mode in ("merit481", "saved481", "saved_current", "2d1", "2d2", "2d4", "bl")
        prepare_build()
        run_safeguard(args.mode)
    else:
        case = args.case or (Path(args.mode) if args.mode else None)
        assert case and HERE in case.resolve().parents
        run_full(case.resolve())

if __name__ == "__main__":
    main()
