#!/usr/bin/env python3
"""Prepare a uniquely pinned N4 cut build; never starts the build itself."""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
STAGED = HERE.parents[1]
REPO = HERE.parents[5]
REFERENCE_BUILD = HERE / "reference_build"
BUILD = REPO / "integration_evidence/build-integrated-v3-p5-20261114"
SOURCE = REFERENCE_BUILD / "build.py"
ADAPTATION_ROOTS = (REPO / "Common/src/adaptation", REPO / "Common/include/adaptation")
HELPERS = (
    HERE / "run_cut_build.py",
    HERE / "run_cut_checks.py",
    HERE / "run_unit_gates.py",
    HERE / "run_prefix_controller.py",
    HERE / "compare_integer_rows.py",
    HERE / "source_check.md",
    HERE / "unchanged_build499_source_pins.json",
    STAGED / "run_cases.py",
    STAGED / "nobl_clean_check_20261114/extract_markers.py",
    STAGED / "f3b_20261115/check_complete_events.py",
    STAGED / "f3b_20261115/check_timer_tree.py",
    STAGED / "f3b_20261115/check_cheap_bl.py",
    SOURCE,
)
BUILD_METADATA = (
    BUILD / "build.ninja",
    BUILD / "meson-info/meson-info.json",
    BUILD / "meson-info/intro-projectinfo.json",
    BUILD / "meson-info/intro-targets.json",
    BUILD / "meson-info/intro-compilers.json",
    BUILD / "meson-info/intro-dependencies.json",
    BUILD / "meson-info/intro-buildoptions.json",
    BUILD / "meson-private/cmd_line.txt",
    REFERENCE_BUILD / "build_status.json",
    REFERENCE_BUILD / "build_plan.json",
    REFERENCE_BUILD / "terminal_handles.json",
    REFERENCE_BUILD / "resource_summary.json",
    REFERENCE_BUILD / "source_pins.json",
    HERE / "G3_reference_receipt.json",
    HERE / "g3_integer_rows.json",
)


def sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def retarget_builder(step_dir: Path) -> tuple[str, dict[str, str]]:
    source_text = SOURCE.read_text()
    old_root = "ROOT=Path(__file__).resolve().parent"
    old_repo = "REPO=ROOT.parents[6]"
    assert source_text.count(old_root) == 1
    assert source_text.count(old_repo) == 1
    transformed = source_text.replace(old_repo, f"REPO=Path({str(REPO)!r})")
    assert "['nice','-n','19'" in transformed
    assert "'-j4'" in transformed
    assert "OMP_NUM_THREADS='1'" in transformed
    assert "OPENBLAS_NUM_THREADS='1'" in transformed
    assert "MKL_NUM_THREADS='1'" in transformed
    assert "compile-tmp" in transformed
    compile(transformed, str(step_dir / "build.py"), "exec")
    substitutions = {"repo_root": str(REPO), "step_root": str(step_dir), "reference_root": str(REFERENCE_BUILD)}
    return transformed, substitutions


def source_pins(builder_path: Path) -> tuple[dict[str, str], list[dict[str, str]]]:
    baseline = json.loads((REFERENCE_BUILD / "source_pins.json").read_text())
    assert isinstance(baseline, dict) and baseline
    pins = {str(Path(path).resolve(strict=True)): digest for path, digest in baseline.items()}
    adaptation_paths = set()
    for root in ADAPTATION_ROOTS:
        adaptation_paths.update(path.resolve(strict=True) for path in root.rglob("*")
                                if path.is_file() and path.suffix in {".cpp", ".hpp", ".h"}
                                and (path.name.startswith("CNative") or root == REPO / "Common/src/adaptation"))
    refreshable = adaptation_paths | {path.resolve(strict=True) for path in (*BUILD_METADATA, *HELPERS, builder_path)}
    unexpected = []
    for path, expected in pins.items():
        if Path(path) in refreshable:
            continue
        if sha(Path(path)) != expected:
            unexpected.append(path)
    assert not unexpected, f"unexpected source drift outside cut adaptation scope: {unexpected[:8]}"
    changed = []
    for path in sorted(adaptation_paths):
        actual = sha(path)
        expected = pins.get(str(path))
        if expected != actual:
            changed.append({"path": str(path), "reference_sha256": expected, "current_sha256": actual})
        pins[str(path)] = actual
    for path in (*BUILD_METADATA, *HELPERS, builder_path):
        resolved = path.resolve(strict=True)
        pins[str(resolved)] = sha(resolved)
    extra = json.loads((HERE / "unchanged_build499_source_pins.json").read_text())
    assert not (pins.keys() & extra.keys()), "reference and unchanged-build499 pin maps overlap"
    for path, digest in extra.items():
        resolved = Path(path).resolve(strict=True)
        assert sha(resolved) == digest, f"unchanged-build499 pin changed: {resolved}"
    return pins, changed


def reference_captures() -> dict[str, str]:
    result = {}
    for gate in ("G2", "GQ"):
        matches = []
        for status_path in HERE.glob("reference_*/status.json"):
            status = json.loads(status_path.read_text())
            if status.get("gate") == gate and status.get("mode") == "reference_capture" and status.get("gate_passed"):
                matches.append(status_path.parent)
        assert len(matches) == 1, f"need exactly one passing {gate} reference capture, found {len(matches)}"
        result[gate] = str(matches[0].resolve())
    g3 = json.loads((HERE / "G3_reference_receipt.json").read_text())
    assert g3.get("state") == "terminal" and g3.get("reference_capture_passed") is True
    assert g3.get("actual_splits") == 473 and g3.get("first_336_identical_P6c") is True
    result["G3"] = str(Path(g3["case"]).resolve(strict=True))
    return result


def prepare(step_arg: str, build_label: str) -> dict[str, object]:
    assert re.fullmatch(r"build[1-9][0-9]*", build_label), "build label must use the run_cases buildN format"
    step_dir = Path(step_arg).expanduser().resolve()
    assert HERE in step_dir.parents, "step directory must be a child of N4"
    assert step_dir != HERE
    for suffix in ("status.json", "source_pins.json", "terminal_handles.json"):
        assert not (STAGED / f"{build_label}_{suffix}").exists(), f"build label is already used: {build_label}"
    captures = reference_captures()
    step_dir.mkdir(parents=True, exist_ok=True)
    for name in ("build.py", "source_pins.json", "cut_build_plan.json", "build_status.json"):
        assert not (step_dir / name).exists(), f"refusing existing step artifact: {step_dir / name}"
    transformed, substitutions = retarget_builder(step_dir)
    builder_path = step_dir / "build.py"
    builder_path.write_text(transformed)
    pins, adaptation_changes = source_pins(builder_path)
    pins_path = step_dir / "source_pins.json"
    pins_path.write_text(json.dumps(pins, indent=2, sort_keys=True) + "\n")
    plan = {
        "schema": "n4_cut_build_plan_v1",
        "build_label": build_label,
        "step_dir": str(step_dir),
        "target_build_tree": str(BUILD),
        "source_pins_sha256": sha(pins_path),
        "source_pin_count": len(pins),
        "adaptation_source_changes_from_reference": adaptation_changes,
        "reference_captures": captures,
        "builder_template": str(SOURCE),
        "builder_template_sha256": sha(SOURCE),
        "generated_builder": str(builder_path),
        "generated_builder_sha256": sha(builder_path),
        "root_substitutions": substitutions,
        "argv_contract": ["nice", "-n", "19", "ninja", "-C", str(BUILD), "-j4",
                          "UnitTests/test_driver", "SU2_CFD/src/SU2_CFD"],
        "environment_contract": {"OMP_NUM_THREADS": "1", "OPENBLAS_NUM_THREADS": "1",
                                  "MKL_NUM_THREADS": "1",
                                  "TMPDIR": str(REPO / "integration_evidence/build-integrated-v2/compile-tmp")},
        "resource_monitor": "unchanged reference_build/build.py sample_tree polling every 10 seconds",
        "launched": False,
        "launch_command": f"python3 {builder_path}",
    }
    (step_dir / "cut_build_plan.json").write_text(json.dumps(plan, indent=2) + "\n")
    return plan


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("step_dir", nargs="?")
    parser.add_argument("build_label", nargs="?")
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        with tempfile.TemporaryDirectory(prefix="n4-cut-build-check-") as temporary:
            step = Path(temporary)
            text, roots = retarget_builder(step)
            builder = step / "build.py"
            builder.write_text(text)
            pins, changes = source_pins(builder)
        assert roots["repo_root"] == str(REPO)
        assert "REPO=Path(" in text and "ROOT=Path(__file__).resolve().parent" in text
        print(json.dumps({"passed": True, "retargeted_builder_syntax": True,
                          "root_substitutions": roots, "source_pin_count": len(pins),
                          "adaptation_source_change_count": len(changes), "launched": False}, indent=2))
        return
    if not args.step_dir or not args.build_label:
        parser.error("step_dir and a unique build_label are required")
    print(json.dumps(prepare(args.step_dir, args.build_label), indent=2))


if __name__ == "__main__":
    main()
