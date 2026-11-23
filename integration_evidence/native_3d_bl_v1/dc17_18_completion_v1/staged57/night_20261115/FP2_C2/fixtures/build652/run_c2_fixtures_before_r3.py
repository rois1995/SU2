"""Run FP2-C2 §6.1 and seal the hidden fixtures, S0 off cohort, and G1."""

from argparse import ArgumentParser
from contextlib import ExitStack
from pathlib import Path
import re
import runpy
from unittest.mock import patch

import run_c2_hidden as hidden

import run_g1 as g1

HERE, C1, a, cut, sha = hidden.HERE, hidden.C1, hidden.a, hidden.cut, hidden.sha
c1 = hidden.c1
fx2, scope = c1.fx2, c1.scope
DRIVER = Path(__file__).resolve()
BASE_SCOPE = C1 / "scoped/build647"
BASE_FIXTURES = C1 / "fixtures/build647"
REFERENCE_PINS = {
    BASE_SCOPE / "fixtures.json": "b7bfcaf62a0a89d956e43f862def2938a48715be5524e0e138ebefb96c1841af",
    BASE_SCOPE / "sheet/fixture_status.json": "f1e9184f2a32245fa5f84e32532d99feef49b3ec66bb055ec8ef899429f3adbb",
    BASE_FIXTURES / "wall/test_outer.log": "5801212b7f22ab9cada13aef1921e21d9086db011e31217343fba2f5d988b417",
    BASE_FIXTURES / "F1-F4/rows.json": "80662e446980f499aa5eceae8ee0795ab74adb4ccd701bdb48ea2ccd415d3ad8",
    BASE_FIXTURES / "F5/rows.json": "c0521bb16fc33811c93c7bb4980e95d00d3c3f64409376a91b853422e565a4b0",
}
SURFACE_RESULTS = (
    "box/fixture_status.json", "IN1/fixture_status.json", "locked/fixture_status.json",
    "volume/fixture_status.json", "sheet/fixture_status.json",
    "exchange/rank2/fixture_status.json", "exchange/rank4/fixture_status.json",
    "exchange/rank2/state_comparison.json", "exchange/rank4/state_comparison.json",
)
OBSERVATIONS = {"line", "text", "peak_rss", "job_peak_rss_bytes"}
CATCH_CASE = re.compile(r"<TestCase\b.*?</TestCase>", re.DOTALL)


class WatchdogOpen(RuntimeError):
    pass


def reference_pins():
    pinned = dict(REFERENCE_PINS)
    old = cut.read(BASE_SCOPE / "fixtures.json")
    assert old["passed"] and old["G1"] == [375, 114, 228] and old["fp1_cohorts"] == 18
    pinned.update({BASE_SCOPE / name: digest for name, digest in old["receipts"].items()})
    assert all(sha(path) == digest for path, digest in pinned.items())
    return pinned


def result_only(value):
    if isinstance(value, dict):
        return {key: result_only(item) for key, item in value.items()
                if key not in OBSERVATIONS and "seconds" not in key and not key.endswith("_us_per_call")}
    if isinstance(value, list):
        return [result_only(item) for item in value]
    return value


def surface_result(log_path):
    report = cut.read(log_path)
    assert report["passed"]
    if log_path.name == "fixture_status.json":
        report = {key: report[key] for key in ("catch_summaries", "measurement")}
    return result_only(report)


def mobility_result(path):
    value = cut.read(path)
    assert value["passed"]
    rows = {"call_rows": value["call_rows"], "timer_rows": [fx2.TIMING.sub("", row) for row in value["timer_rows"]]}
    if "B1" in value:
        rows["B1"] = {key: item for key, item in value["B1"].items() if key not in fx2.B1_OBSERVATIONS}
    return rows


def first_difference(old, new, path=""):
    if old == new:
        return None
    if isinstance(old, dict) and isinstance(new, dict):
        if old.keys() != new.keys():
            return {"field": path, "reference_keys": sorted(old), "candidate_keys": sorted(new)}
        for key in sorted(old):
            difference = first_difference(old[key], new[key], f"{path}/{key}")
            if difference:
                return difference
    elif isinstance(old, list) and isinstance(new, list):
        for index, (expected, observed) in enumerate(zip(old, new)):
            difference = first_difference(expected, observed, f"{path}/{index}")
            if difference:
                return difference
        return {"field": path, "reference_count": len(old), "candidate_count": len(new)}
    return {"field": path, "reference": old, "candidate": new}


def compare_off(root, base):
    pairs = [(BASE_SCOPE / name, root / name, surface_result) for name in SURFACE_RESULTS]
    for name in ("F1-F4", "F5"):
        pairs.append((BASE_FIXTURES / name / "rows.json", base / name / "rows.json", mobility_result))
    comparisons = []
    for old, new, parse in pairs:
        comparisons.append({"reference": str(old), "reference_sha256": sha(old), "candidate": str(new),
                            "candidate_sha256": sha(new), "first_difference": first_difference(parse(old), parse(new))})
    old_wall, new_wall = BASE_FIXTURES / "wall/test_outer.log", base / "wall/test_outer.log"
    # The C1 wall cases exercise both settings of that older option. C2 is off throughout.
    comparisons.append({"reference": str(old_wall), "reference_sha256": sha(old_wall), "candidate": str(new_wall),
                        "candidate_sha256": sha(new_wall), "first_difference": first_difference(
                            a.summarize_box([old_wall])["catch_summaries"],
                            a.summarize_box([new_wall])["catch_summaries"])})
    report = {"passed": all(row["first_difference"] is None for row in comparisons), "comparisons": comparisons}
    path = base / "off_comparison.json"
    a.sealed(path, report)
    assert report["passed"], next(row for row in comparisons if row["first_difference"] is not None)
    return path


def watchdog_failure(case, ranks, xml):
    logs = [case / "test_outer.log"] if ranks == 1 else sorted(case.glob("rank_output/*/rank.*/stdout"))
    if not xml:
        return hidden.has_assertion_failure([case / "test_outer.log", *logs])
    # G1 has adopted failed assertions. Compare each completed test case in the partial reporter output.
    identity = runpy.run_path(str(g1.PARSER))
    baseline = cut.read(g1.REF / f"G1_candidate_{case.name}.json")
    observed = []
    for log in logs:
        rank = "0" if ranks == 1 else log.parent.name.rsplit(".", 1)[1]
        rows, _ = identity["parse_xml"]("<Catch>" + "\n".join(CATCH_CASE.findall(
            log.read_text(errors="replace"))) + "</Catch>", rank, log)
        observed.extend(rows)
    keys = {(row["rank"], row["name"]) for row in observed}
    expected = [row for row in baseline["case_results"] if (row["rank"], row["name"]) in keys]
    return not identity["compare"]({"case_results": expected, "reporter_totals": []},
                                   {"case_results": observed, "reporter_totals": []}, set())["identity_pass"]


def run_with_watchdog(case, binary, ranks, selector, seconds, environment=None, cwd=None, xml=False, optimizer=False):
    previous = case.with_name(case.name + "_watchdog_attempt1")

    def run(limit):
        return g1_run_case(case, binary, ranks, selector, limit, environment, cwd, xml, optimizer)

    if not previous.exists():
        wait = run(seconds)
        if wait["returncode"] != 124:
            return wait
        failed = watchdog_failure(case, ranks, xml)
        a.sealed(case / "watchdog.json", {"status": "result_failed" if failed else "retry_allowed",
                                         "seconds_limit": seconds, "wait": wait,
                                         "host_preflight": cut.read(case / "test_host_preflight.json")})
        if failed:
            return wait
        case.rename(previous)
    assert a.resumed(previous)["returncode"] == 124
    initial = cut.read(previous / "watchdog.json")
    assert initial["seconds_limit"] == seconds and initial["status"] == "retry_allowed"
    case.mkdir(parents=True, exist_ok=True)
    for name in ("pins.json", "provenance.json"):
        if (previous / name).exists():
            a.sealed(case / name, cut.read(previous / name))
    repeat = run(3 * seconds)
    if repeat["returncode"] == 124:
        failed = watchdog_failure(case, ranks, xml)
        a.sealed(case / "watchdog.json", {"status": "result_failed" if failed else "open", "wait": repeat,
                                         "seconds_limit": 3 * seconds, "previous": str(previous / "watchdog.json"),
                                         "host_preflight": cut.read(case / "test_host_preflight.json")})
        if not failed:
            raise WatchdogOpen(f"Repeated fixture helper watchdog at {case}")
    return repeat


g1_run_case = c1.run_case


def run_g1(output, status_path, source_pins, receipt):
    manifest, selection = cut.read(g1.MANIFEST), cut.read(g1.SELECTION)
    identity = runpy.run_path(str(g1.PARSER))
    identity["self_check"]()
    reports = []
    for cohort, count in scope.prior.COHORTS.items():
        case = output / cohort
        g1.run_cohort(case, cohort, receipt, source_pins, selection, manifest["cohorts"][cohort], identity)
        report = case / "comparison.json"
        row = cut.read(report)
        assert row["identity_pass"] and row["records"] == count
        reports.append(report)
    return reports


def run_cohort(build, fingerprints):
    pinned = reference_pins()
    support = (DRIVER, hidden.WATCHDOG_ORDER, hidden.WATCHDOG_ORDER.with_suffix(".sha256"),
               Path(c1.__file__).resolve(), Path(g1.__file__).resolve(), *pinned)
    base = HERE / "fixtures" / f"build{fingerprints['build']}"
    with ExitStack() as stack:
        for module in (a, c1, fx2, g1):
            stack.enter_context(patch.object(module, "run_case", run_with_watchdog))
        stack.enter_context(patch.object(scope.prior, "run_g1", run_g1))
        stack.enter_context(patch.multiple(scope, HERE=HERE, ORDER=hidden.ORDER, ORDER_SHA=hidden.ORDER_SHA,
                                          DRIVER=DRIVER, SOURCE_CHECK=HERE / "source_check.md",
                                          BASE_SCOPE=BASE_SCOPE, BASE="de08269a8a"))
        stack.enter_context(patch.object(c1, "HERE", HERE))
        stack.enter_context(patch.object(fx2, "HERE", HERE))
        c1.wall_fixtures(build)
        fx2.fixture(build, "F1-F4", "[.][NativeFX2Memo]", {})
        fx2.fixture(build, "F5", "[.][NativeFastPathStage2Bounds]",
                    {"SU2_FP2_OUT": fx2.inputs()["SU2_FP2_OUT"]})
        original_prepare = scope.prepare

        def prepare(directory, extra=()):
            return original_prepare(directory, (*extra, *support))

        with patch.object(scope, "prepare", prepare):
            receipt = scope.fixtures(build)
    assert receipt["passed"] and receipt["G1"] == [375, 114, 228]
    fixtures = Path(receipt["fixtures"])
    return receipt["G1"], (fixtures, compare_off(fixtures.parent, base))


def verify_hidden(base, fingerprints, name):
    path = base / name / "status.json"
    status = cut.read(path)
    assert all(status[key] == value for key, value in fingerprints.items()), path
    for name, digest in status["attempts"].items():
        attempt = path.parent / name
        assert sha(attempt) == digest
        row = cut.read(attempt)
        assert row["wait"] == a.resumed(attempt.parent)
        assert all(sha(attempt.parent / name) == digest for name, digest in row["artifacts"].items())
    return path, status["status"]


def run(build, only):
    _, fingerprints, _ = hidden.prepare(build)
    base = HERE / "fixtures" / f"build{fingerprints['build']}"
    if only != "cohort":
        names = hidden.CASES if only == "all" else (only,)
        for name in names:
            result = hidden.run_hidden(build, name)
            if result["status"] not in ("passed", "open_r6") or only != "all":
                return result
    receipts = [verify_hidden(base, fingerprints, name) for name in hidden.CASES]
    assert all(status in ("passed", "open_r6") for _, status in receipts), receipts
    records, cohort = run_cohort(build, fingerprints)
    status = "open_r6" if any(status == "open_r6" for _, status in receipts) else "passed"
    result = {**fingerprints, "passed": status == "passed", "status": status, "G1": records,
              "receipts": {str(path): sha(path) for path in (*cohort, *(path for path, _ in receipts))}}
    if status == "open_r6":
        result["label"] = hidden.OPEN
    a.sealed(base / "c2_acceptance.json", result)
    return result


def self_check():
    hidden.verify_orders()
    reference_pins()
    for name in SURFACE_RESULTS:
        assert "passed" not in surface_result(BASE_SCOPE / name) or surface_result(BASE_SCOPE / name)["passed"]
    assert first_difference(result_only({"value": 9, "seconds_build": 1}), {"value": 9}) is None
    assert first_difference({"v": [1, 2]}, {"v": [1, 3]})["field"] == "/v/1"
    runpy.run_path(str(g1.PARSER))["self_check"]()
    print("FP2-C2 fixture reference, comparison, and G1 parser self-check passed.")


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path, nargs="?")
    parser.add_argument("--only", choices=("all", *hidden.CASES, "cohort"), default="all")
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        self_check()
        return 0
    if args.build is None:
        parser.error("provide the candidate build directory")
    assert Path("/proc/1/comm").read_text().strip() == "systemd"
    hidden.clear_options()
    try:
        result = run(args.build.resolve(), args.only)
    except WatchdogOpen as error:
        print(error, flush=True)
        return 75
    print(result, flush=True)
    return 0 if result["passed"] else 75 if result["status"].startswith("open_") else 1


if __name__ == "__main__":
    raise SystemExit(main())
