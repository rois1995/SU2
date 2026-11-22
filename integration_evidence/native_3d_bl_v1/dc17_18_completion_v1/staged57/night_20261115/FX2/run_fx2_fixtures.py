"""Run and seal the FX2 F0 baseline and F1–F5 identity comparison."""

from argparse import ArgumentParser
import os
from pathlib import Path
import re
import subprocess
import sys

HERE = Path(__file__).resolve().parent
sys.path[:0] = [str(HERE), *(str(HERE.parent / name) for name in ("FP1", "FP2_0"))]
import build_fp1
from audit_fx2 import audit
from run_g1 import cut, run_case, sha, verify_build
from run_measurements import inputs, measurements

BASE = "c843610249"
SOLVER_SHA = "ce93af9ef95c3b5aa5295a850bb4f74f92facdb00e93e5ef8fa27cee57b48b07"
SOLVER = build_fp1.BUILD / "SU2_CFD/src/SU2_CFD"
TIMING = re.compile(r" (?:inclusive_call_seconds|exclusive_seconds|root_seconds|sumexclusive|"
                    r"unattributed|coverage)=[^ ]+")
TIMER_ROW = re.compile(r"^NATIVE3D_TIMER(?:_SUMMARY)? root=(?:F[1-4]_|FX2_M6_B1)")
B1_OBSERVATIONS = {"constructor_seconds", "policy_seconds", "constructor_minus_policy_seconds",
                   "rss_before_bytes", "rss_after_bytes", "rss_change_bytes"}


def verify_production():
    subprocess.run(["git", "diff", "--exit-code", BASE, "--", "Common", "SU2_CFD"],
                   cwd=cut.REPO, check=True)
    assert sha(SOLVER) == SOLVER_SHA


def build_binary(label, baseline):
    if baseline:
        verify_production()
    original = build_fp1.source_pins

    def source_pins():
        pins = original()
        fixtures = cut.REPO / "UnitTests/Common/adaptation/fixtures"
        for path in fixtures.glob("native_fx2*"):
            pins[str(path)] = sha(path)
        return pins

    build_fp1.source_pins = source_pins
    work = HERE / f"build{label}"
    assert build_fp1.build(work, label, solver=not baseline), work / "build_status.json"
    if baseline:
        verify_production()


def fixture(build, name, selector, env):
    status, manifest = (build / name for name in ("build_status.json", "source_pins.json"))
    receipt = cut.read(status)
    binary, source_sha = verify_build(receipt, manifest)
    assert sha(Path(receipt["binary"])) == receipt["binary_sha256"]
    case = HERE / "fixtures" / build.name / name
    pins = {"build": str(status), "build_sha256": sha(status), "source_pins_sha256": source_sha,
            "binary_sha256": receipt["test_binary_sha256"], "selector": selector, "inputs": env}
    if not (case / "test_wait.json").exists():
        assert not (case / "test_launch.json").exists(), case
        case.mkdir(parents=True, exist_ok=True)
        cut.save(case / "pins.json", pins)
        wait = run_case(case, binary, 1, selector, 5400, {key: row["path"] for key, row in env.items()})
    else:
        assert cut.read(case / "pins.json") == pins
        wait = cut.read(case / "test_wait.json")
    log = case / "test_outer.log"
    raw = log.read_text().splitlines()
    calls = [row for row in raw if row.startswith("FX2_MEMO ")]
    timing = [row for row in raw if TIMER_ROW.match(row)]
    summaries = [row for row in timing if row.startswith("NATIVE3D_TIMER_SUMMARY ")]
    assert wait["terminal"] and cut.terminal(wait["controller"]) and wait["returncode"] == 0, log
    assert calls and len(calls) == len(summaries) and all("fixedstorage=4120" in row for row in summaries), log
    assert all(sha(Path(row["path"])) == row["sha256"] for row in env.values())
    assert verify_build(receipt, manifest) == (binary, source_sha)
    data = {"passed": True, "call_rows": calls, "timer_rows": timing, "case": str(case),
            "log_sha256": sha(log), "test_seconds": wait["elapsed_seconds"], "pins": pins}
    if name == "F5":
        rows, malformed = measurements(raw)
        assert not malformed
        b1, = (row["value"] for row in rows if row["name"] == "B1")
        assert b1.get("snapshot_equal") is True and not any(key.endswith("_error") for key in b1), b1
        data["B1"] = b1
    path = case / "rows.json"
    if path.exists():
        assert cut.read(path) == data
    else:
        cut.save(path, data)
    return data


def compare(build, candidate, reference):
    differences = []
    for name, target in reference.items():
        observed = candidate[name]
        for key in ("call_rows", "timer_rows"):
            before, after = target[key], observed[key]
            if key == "timer_rows":
                before, after = ([TIMING.sub("", row) for row in rows] for rows in (before, after))
            if before != after:
                first = next(((i, a, b) for i, (a, b) in enumerate(zip(before, after), 1) if a != b),
                             (min(len(before), len(after)) + 1, len(before), len(after)))
                differences.append({"fixture": name, "field": key, "first_difference": first})
    old, new = reference["F5"]["B1"], candidate["F5"]["B1"]
    for key in sorted((old.keys() | new.keys()) - B1_OBSERVATIONS):
        if old.get(key) != new.get(key):
            differences.append({"fixture": "F5", "field": key, "reference": old.get(key), "candidate": new.get(key)})
    saving = old["constructor_seconds"] - new["constructor_seconds"]
    if saving < 10:
        differences.append({"fixture": "F5", "field": "constructor_seconds_saving",
                            "minimum": 10, "candidate": saving})
    proof = {"passed": not differences, "differences": differences, "B1_build640_source": old,
             "B1_candidate": new, "constructor_saving_seconds": saving, "F5_live_memo_entries": 1,
             "F5_memo_bytes": new["memo_entry_bytes"]}
    cut.save(HERE / "fixtures" / build.name / "comparison.json", proof)
    return proof


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("label", type=int)
    parser.add_argument("mode", choices=("build-baseline", "baseline", "build", "compare"))
    parser.add_argument("--baseline-build", type=int)
    args = parser.parse_args()
    if args.mode == "compare" and args.baseline_build is None:
        parser.error("compare requires the sealed --baseline-build")
    assert audit()["all_owned_handles_terminal"]
    if args.mode.startswith("build"):
        return build_binary(args.label, args.mode == "build-baseline")
    for name in tuple(os.environ):
        if name.startswith(("SU2_NATIVE_", "SU2_FP1_", "SU2_FP2_")):
            del os.environ[name]
    if args.mode == "baseline":
        verify_production()
    build = HERE / f"build{args.label}"
    data = {"F1-F4": fixture(build, "F1-F4", "[.][NativeFX2Memo]", {}),
            "F5": fixture(build, "F5", "[.][NativeFastPathStage2Bounds]",
                          {"SU2_FP2_OUT": inputs()["SU2_FP2_OUT"]})}
    if args.mode == "compare":
        base = HERE / "fixtures" / f"build{args.baseline_build}"
        reference = {name: cut.read(base / name / "rows.json") for name in data}
        base_build = HERE / f"build{args.baseline_build}"
        base_status = cut.read(base_build / "build_status.json")
        base_sources = base_build / "source_pins.json"
        assert base_status["binary_sha256"] == SOLVER_SHA and sha(base_sources) == base_status["source_pins_sha256"]
        tests = cut.REPO / "UnitTests"
        candidate_sources = cut.read(build / "source_pins.json")
        for path, digest in cut.read(base_sources).items():
            if Path(path).is_relative_to(tests):
                assert candidate_sources[path] == digest, path
        for row in reference.values():
            assert row["pins"]["build_sha256"] == sha(base_build / "build_status.json")
            assert sha(Path(row["case"]) / "test_outer.log") == row["log_sha256"]
        proof = compare(build, data, reference)
        print(proof)
        return not proof["passed"]
    verify_production()
    print("F0 baseline complete on c843610249 production source.", flush=True)


if __name__ == "__main__":
    raise SystemExit(main())
