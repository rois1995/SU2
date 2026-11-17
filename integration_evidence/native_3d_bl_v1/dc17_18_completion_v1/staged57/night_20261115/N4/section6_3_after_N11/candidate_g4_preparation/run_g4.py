#!/usr/bin/env python3
"""Run the unchanged N4 G4 unit gate through the sealed build545 alias."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
N4 = HERE.parent.parent
STAGED = N4.parents[1]
REPO = N4.parents[5]
CUT_CHECKS = N4 / "run_cut_checks.py"
UNIT_GATES = N4 / "run_unit_gates.py"
BUILD_LABEL = "build545"
CASE_ROOT = STAGED / "cases"
EXPECTED = {f"{BUILD_LABEL}_2d{ranks}_" for ranks in (1, 2, 4)}


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def import_run_cases():
    sys.path.insert(0, str(STAGED))
    import run_cases
    return run_cases


def process_start_ticks(pid: int) -> int | None:
    try:
        text = (Path("/proc") / str(pid) / "stat").read_text()
        return int(text.rsplit(")", 1)[1].split()[19])
    except (OSError, IndexError, ValueError):
        return None


def add_start_ticks(checks) -> None:
    original = checks.sample_tree

    def sample_tree(pid):
        rows = original(pid)
        for row in rows:
            row["start_ticks"] = process_start_ticks(row["pid"])
        return rows

    checks.sample_tree = sample_tree


def pin_launcher(checks) -> None:
    original = checks.save
    extra = (HERE / "run_g4.py", CUT_CHECKS, UNIT_GATES, HERE / "build_status.json",
             HERE / "source_pins.json", HERE / "terminal_handles.json", HERE / "cut_build_plan.json")
    digests = {str(path.resolve(strict=True)): sha(path.resolve(strict=True)) for path in extra}

    def save(path, value):
        if Path(path).name == "pins.json":
            for name, digest in digests.items():
                value[name] = digest
        return original(path, value)

    checks.save = save


def existing_case_dirs() -> set[Path]:
    return {path.resolve() for prefix in EXPECTED for path in CASE_ROOT.glob(prefix + "*") if path.is_dir()}


def case_receipt(before: set[Path]) -> dict[str, object]:
    new_cases = sorted(existing_case_dirs() - before)
    assert len(new_cases) == 3, f"expected three new G4 cases, found {len(new_cases)}"
    records = []
    for case in new_cases:
        status = json.loads((case / "status.json").read_text())
        assert status.get("state") == "terminal" and status.get("exit_code") == 0
        assert status.get("pins_verified") is True and not status.get("changed")
        rows = [json.loads(line) for line in (case / "resources.jsonl").read_text().splitlines() if line]
        observations = [row for sample in rows for row in sample.get("processes", [])]
        identities = sorted({(row["pid"], row["start_ticks"]) for row in observations
                             if row.get("start_ticks") is not None})
        assert rows and identities
        terminal = []
        for pid, start in identities:
            try:
                fields = (Path("/proc") / str(pid) / "stat").read_text().rsplit(")", 1)[1].split()
                alive = int(fields[19]) == start and fields[0] not in {"Z", "X"}
            except (OSError, ValueError, IndexError):
                alive = False
            assert not alive, (pid, start)
            terminal.append({"pid": pid, "start_ticks": start, "alive": False})
        pins = json.loads((case / "pins.json").read_text())
        assert all(sha(Path(path)) == digest for path, digest in pins.items())
        records.append({"case": str(case), "status": status, "sample_count": len(rows),
                        "actual_host_terminal_handles": terminal,
                        "disappeared_before_identity_sample": sum(row.get("start_ticks") is None for row in observations),
                        "all_observed_identities_terminal": True, "pins_unchanged": True,
                        "pins_sha256": sha(case / "pins.json"),
                        "run_log_sha256": sha(case / "run.log")})
    output = {"schema": "n4_g4_build545_case_receipt_v1", "passed": True,
              "build_label": BUILD_LABEL, "required_filters": ["[NativeEngine2D]", "[NativeMesh2D]", "[NativeBL2D]"],
              "new_cases": records, "launched_by": str(HERE / "run_g4.py")}
    (HERE / "g4_case_paths.json").write_text(json.dumps(output, indent=2) + "\n")
    return output


def self_check() -> dict[str, object]:
    assert CUT_CHECKS.is_file() and UNIT_GATES.is_file()
    for name in ("build_status.json", "source_pins.json", "terminal_handles.json", "cut_build_plan.json"):
        assert (HERE / name).is_file()
    plan = json.loads((HERE / "cut_build_plan.json").read_text())
    assert plan.get("build_label") == BUILD_LABEL
    assert plan.get("source_pins_sha256") == sha(HERE / "source_pins.json")
    status = json.loads((HERE / "build_status.json").read_text())
    assert status.get("state") == "terminal" and status.get("exit_code") == 0
    assert status.get("pins_verified") is True
    assert all(any(prefix.startswith(f"{BUILD_LABEL}_2d{ranks}_") for prefix in EXPECTED)
               for ranks in (1, 2, 4))
    source = UNIT_GATES.read_text()
    assert 'for mode in GATES["g4"]' in source and '"g4": ("2d1", "2d2", "2d4")' in source
    assert '[NativeEngine2D],[NativeMesh2D],[NativeBL2D]' in (STAGED / "f3b_20261115/run_remaining_checks.py").read_text()
    return {"passed": True, "launch": False, "build545_alias_valid": True,
            "unit_gate_source_unchanged": True, "ranks": [1, 2, 4],
            "resource_identity": "append /proc start_ticks to existing process samples"}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check(), indent=2))
        return
    before = existing_case_dirs()
    checks = import_run_cases()
    add_start_ticks(checks)
    pin_launcher(checks)
    spec = importlib.util.spec_from_file_location("n4_g4_checked_step", CUT_CHECKS)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load checked-step adapter: {CUT_CHECKS}")
    adapter = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(adapter)
    step, _, _ = adapter.checked_step(str(HERE), BUILD_LABEL)
    adapter.execute_unit_gate("g4", BUILD_LABEL, step)
    print(json.dumps(case_receipt(before), indent=2), flush=True)


if __name__ == "__main__":
    main()
