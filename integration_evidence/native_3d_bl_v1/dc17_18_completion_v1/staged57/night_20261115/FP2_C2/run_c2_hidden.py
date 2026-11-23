"""Run and seal a hidden FP2-C2 fixture group on a pinned test binary."""

from argparse import ArgumentParser
import os
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
C1 = HERE.parent / "FP2_C1"
sys.path.insert(0, str(C1))
import run_c1_fixtures as c1

from run_correction1 import build_fingerprints

a, cut, sha = c1.scope.a, c1.cut, c1.sha
DRIVER = Path(__file__).resolve()
ORDER = HERE / "inputs/FAST_PATH_STAGE2_C2.md"
ORDER_SHA = "fdd678fed866cb0fb9171316ca1f8e534ea848357918fb1956d95bdbb2bf7cae"
CONTINUATION = HERE / "inputs/FP2_C2_CONTINUE_1.md"
CONTINUATION_SHA = "7480af812c7173c1c574c24be6406b4db2d421701bd4bc569263b82ba3056ee2"
CONTINUATION_2 = HERE / "inputs/FP2_C2_CONTINUE_2.md"
CONTINUATION_2_SHA = "d95981c35eee1958d2bc27ed2e9e6a209c64197243a2b21b67a1c1746dcdffd4"
SOURCE_CHECK = HERE / "continuation1_source_check.md"
WATCHDOG_ORDER = C1 / "inputs/FP2_C1_CONTINUE_2.md"
WATCHDOG_ORDER_SHA = "f65f2f302302f8f546ffcc23bb84a9e2ababa40defd9995e9779be456a94eda3"
ORDERS = ((ORDER, ORDER_SHA), (CONTINUATION, CONTINUATION_SHA),
          (CONTINUATION_2, CONTINUATION_2_SHA), (WATCHDOG_ORDER, WATCHDOG_ORDER_SHA))
ORDER_FILES = tuple(file for path, _ in ORDERS for file in (path, path.with_suffix(".sha256")))
R6_SECONDS = 90 * 60
OPEN = "FP2-C2: fixture (i) OPEN at the R6 limit; default path not yet proven"
FIRST_SERIAL_DRIVER = "365709bb7e41ba436698b2b43b3831d301753a23730de69fd409d7c436a6a9e6"
CASES = {
    "serial": (1, "[.][NativeFP2C2]~[NativeFP2C2MPI]", 8),
    "border": (2, "Step C protects a rank-border point and keeper without mutating either state", 1),
    "exchange": (2, "Step C exchanges cross-pass and local cancellations and builds with removals only", 1),
}


def verify_orders():
    for path, digest in ORDERS:
        assert sha(path) == digest
        assert path.with_suffix(".sha256").read_text().split() == [digest, path.name]


def prepare(build):
    verify_orders()
    receipt = cut.read(build / "build_status.json")
    binary, _ = c1.verify_build(receipt, build / "source_pins.json")
    assert not receipt["new_warnings"]
    assert sha(Path(receipt["binary"])) == receipt["binary_sha256"]
    fingerprints = build_fingerprints(receipt)
    support = (DRIVER, *ORDER_FILES, SOURCE_CHECK, HERE / "source_check.md", Path(sys.modules["run_fp0"].__file__).resolve(),
               cut.PREFLIGHT, build / "build_status.json")
    pins = {str(path): sha(path) for path in support}
    return binary, fingerprints, pins


def clear_options():
    for name in tuple(os.environ):
        if name.startswith(("SU2_NATIVE_", "SU2_FP1_", "SU2_FP2_")):
            del os.environ[name]


def has_assertion_failure(logs):
    return any(re.search(r"^\s*(?:[^\r\n]+:\d+:\s*)?FAILED:",
                         a.ANSI.sub("", path.read_text(errors="replace")), re.MULTILINE)
               for path in logs)


def attempt(case, binary, fingerprints, pins, spec, seconds):
    ranks, selector, count = spec
    case.mkdir(parents=True, exist_ok=True)
    if case.parts[-3:] == ("build651", "serial", "attempt1") and (case / "result.json").is_file():
        previous_pins = cut.read(case / "provenance.json")["pins"]
        if previous_pins[str(DRIVER)] == FIRST_SERIAL_DRIVER:
            # The original launch and result are intact. Normalize the summary's JSON types on resume.
            pins = {**pins, str(DRIVER): FIRST_SERIAL_DRIVER}
    a.sealed(case / "provenance.json", {**fingerprints, "pins": pins, "ranks": ranks, "selector": selector,
                                       "seconds_limit": seconds})
    wait = a.resumed(case)
    if wait is None:
        wait = c1.run_case(case, binary, ranks, selector, seconds)
    assert wait == a.resumed(case)
    command = cut.read(case / "command.json")
    assert (command["ranks"], command["selector"], command["seconds_limit"]) == (ranks, selector, seconds)
    assert all(sha(Path(path)) == digest for path, digest in pins.items()
               if path != str(DRIVER) or digest != FIRST_SERIAL_DRIVER)
    assert sha(binary) == fingerprints["test_binary_sha256"]
    outer = case / "test_outer.log"
    logs = [outer] if ranks == 1 else sorted(case.glob("rank_output/*/rank.*/stdout"))
    checks = [[list(match) for match in matches] for matches in a.summarize_box(logs)["catch_summaries"]]
    complete = len(checks) == ranks and all(len(rows) == 1 and int(rows[0][1]) == count for rows in checks)
    # Catch prints "1 test case" with the same count pattern as "test cases".
    failure = has_assertion_failure([outer, *logs])
    passed = complete and wait["returncode"] == 0 and not failure
    watchdog = wait["returncode"] == 124
    artifacts = {*logs, outer, case / "provenance.json"}
    artifacts.update(case / name for name in a.ARTIFACTS if (case / name).is_file())
    result = {**fingerprints, "passed": passed, "watchdog": watchdog, "assertion_failed": failure,
              "catch_summaries": checks, "wait": wait, "case": str(case),
              "artifacts": {str(path.relative_to(case)): sha(path) for path in sorted(artifacts)}}
    a.sealed(case / "result.json", result)
    return result


def run_hidden(build, name):
    binary, fingerprints, pins = prepare(build)
    case = HERE / "fixtures" / f"build{fingerprints['build']}" / name
    candidate = attempt(case / "attempt1", binary, fingerprints, pins, CASES[name], R6_SECONDS)
    status = "passed" if candidate["passed"] else "failed"
    used = [Path(candidate["case"]) / "result.json"]
    if candidate["watchdog"] and not candidate["assertion_failed"]:
        if name == "exchange":
            status = "open_r6"
        else:
            candidate = attempt(case / "attempt2", binary, fingerprints, pins, CASES[name], 3 * R6_SECONDS)
            used.append(Path(candidate["case"]) / "result.json")
            status = "passed" if candidate["passed"] else "failed"
            if candidate["watchdog"] and not candidate["assertion_failed"]:
                status = "open_watchdog"
    assert prepare(build) == (binary, fingerprints, pins)
    result = {**fingerprints, "passed": status == "passed", "status": status,
              "attempts": {str(path.relative_to(case)): sha(path) for path in used}}
    if status == "open_r6":
        result["label"] = OPEN
    a.sealed(case / "status.json", result)
    return result


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    parser.add_argument("name", choices=CASES)
    args = parser.parse_args()
    assert Path("/proc/1/comm").read_text().strip() == "systemd"
    clear_options()
    result = run_hidden(args.build.resolve(), args.name)
    print(result, flush=True)
    return 0 if result["passed"] else 75 if result["status"].startswith("open_") else 1


if __name__ == "__main__":
    raise SystemExit(main())
