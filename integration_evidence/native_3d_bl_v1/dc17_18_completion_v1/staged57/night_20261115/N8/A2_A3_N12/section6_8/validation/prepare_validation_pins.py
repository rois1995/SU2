#!/usr/bin/env python3
"""Create the disjoint runtime-helper pin map required by the P7/P8 full runner."""
import hashlib
import json
from pathlib import Path
HERE = Path(__file__).resolve().parent
A2 = HERE.parent
N8 = A2.parent.parent
STAGED = N8.parents[1]
NIGHT = N8.parent
S1 = NIGHT / "section6_1"
S2 = NIGHT / "section6_2"


def sha(path):
    """Return SHA-256 for one exact dependency path."""
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def dependencies():
    """List all non-build helper and accepted-reference inputs consumed at runtime."""
    names = [
        HERE / "validate_n8.py", HERE / "build534_aliases.py",
        HERE / "prepare_validation_pins.py", HERE / "validate_g2_gq.py",
        HERE.parent / "drafts/compare_integer_rows_68.py", HERE / "check_connected_events.py",
        HERE / "check_complete_events.py", HERE / "check_timer_tree.py",
        NIGHT / "N9/section6_6/drafts/check_g3_identity_66.py",
        NIGHT / "N9/section6_6/cases/N9_66_bl_20261116T135717_78861/run.log",
        NIGHT / "N9/section6_6/run_g3.py", NIGHT / "N9/section6_6/run_remaining_checks.py",
        STAGED / "p7p8_r3_20261115/check_cheap_bl.py", STAGED / "check_geometry_magnitude481.py",
        STAGED / "check_saved_surface_corrections481.py",
        STAGED / "runtime_20261113_474/loaded_runtime_pins.json",
        STAGED / "p7p8_r3_20261115/run_full_workflow.py",
        STAGED / "p7p8_r3_20261115/check_connected_events.py",
        STAGED / "p7p8_r3_20261115/check_complete_events.py",
        STAGED / "p7p8_r3_20261115/check_timer_tree.py",
        STAGED / "f3b_20261115/check_timer_tree.py",
        S2 / "run_remaining_checks.py", S2 / "run_full_section6_2.py",
        S2 / "compare_reference_payloads.py", S1 / "accepted_reference_payloads.json",
        S1 / "cases/section6_1_bl_20261116T082025_3350201/cheap_bl_receipt.json",
        S1 / "cases/section6_1_bl_20261116T082025_3350201/run.log",
        S1 / "cases/section6_1_bl_20261116T082025_3350201/nose_target_observation.jsonl",
        S1 / "cases/section6_1_bl_20261116T082025_3350201/actual_saved_surface_stage_rank0.json",
        STAGED / "cases/p6c_bl_20261115T003316_3200535/nose_target_observation.jsonl",
        NIGHT / "N4/compare_integer_rows.py",
    ]
    return sorted({str(path.resolve(strict=True)) for path in names})


def main():
    """Hash dependencies and require no overlap with the frozen build source map."""
    build_pins = json.loads((A2 / "build534/source_pins.json").read_text())
    if isinstance(build_pins, dict) and "source_pins" in build_pins:
        build_pins = build_pins["source_pins"]
    paths = dependencies()
    overlap = sorted(set(paths) & set(build_pins))
    assert not overlap, f"extra pin map overlaps build map: {overlap}"
    payload = {path: sha(path) for path in paths}
    target = HERE / "unchanged_build499_source_pins.json"
    if target.exists():
        assert json.loads(target.read_text()) == payload, "refusing to alter frozen pin map"
    else:
        target.write_text(json.dumps(payload, indent=2) + "\n")
    return {"path": str(target), "count": len(payload), "overlap": overlap}


if __name__ == "__main__":
    print(json.dumps(main(), indent=2))
