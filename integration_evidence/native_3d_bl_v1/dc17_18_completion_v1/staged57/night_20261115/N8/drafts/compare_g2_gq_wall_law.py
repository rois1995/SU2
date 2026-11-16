#!/usr/bin/env python3
"""Draft G2/GQ comparator allowing only additive wall_law timer rows."""
import argparse
import hashlib
import importlib.util
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
STAGED = HERE.parents[2]
NIGHT = STAGED / "night_20261115"
COMPARATOR = NIGHT / "N4/compare_integer_rows.py"
N9_G2 = NIGHT / "N9/section6_6/first_collapse"
spec = importlib.util.spec_from_file_location("n4_integer_rows", COMPARATOR)
comparator = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = comparator
spec.loader.exec_module(comparator)


def sha(path):
    """Return the SHA256 digest for a provenance input."""
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def without_wall_law(rows):
    """Remove only candidate timer rows whose exact operation is wall_law."""
    kept, additions = [], []
    for row in rows:
        if row.marker == "NATIVE3D_TIMER" and row.fields.get("operation") == "wall_law":
            additions.append(row)
        else:
            kept.append(row)
    return kept, additions


def compare_streams(reference, candidate, gate):
    """Compare all old rows strictly after stripping allowed additive wall_law rows."""
    if reference.keys() != candidate.keys():
        return {"passed": False, "first_difference": {"key": "stream_files"},
                "allowed_wall_law_additions": 0}
    normalized, additions = {}, []
    for name, rows in candidate.items():
        normalized[name], found = without_wall_law(rows)
        additions.extend((name, row) for row in found)
    result = comparator.compare_streams(reference, normalized, gate)
    return {**result, "gate": gate, "old_rows_strict": True,
            "allowed_wall_law_additions": len(additions),
            "wall_law_rows": [{"stream": name, "fields": row.fields,
                               "source": row.path, "line": row.line}
                              for name, row in additions]}


def compare_cases(reference, candidate, gate):
    """Load both cases with N4 parsing and compare their rank-local integer rows."""
    before = comparator.load_case(str(reference), gate)
    after = comparator.load_case(str(candidate), gate)
    return compare_streams(before, after, gate)


def synthetic_streams(changes=None, add_wall_law=False, unexpected_timer=False):
    """Build six rank streams for focused comparator behavior checks."""
    result = {}
    for rank in range(6):
        fields = {"rank": str(rank), "stage": "length_collapse_exit",
                  "calls": "12", "modeled_bytes": "4096"}
        if changes and rank == 0:
            fields.update(changes)
        rows = []
        if add_wall_law and rank == 0:
            timer = {"root": "1", "boundary": "final", "rank": str(rank),
                     "phase": "adapt", "operation": "wall_law", "calls": "1",
                     "seconds": "0.1"}
            rows.append(comparator.Row("NATIVE3D_TIMER", timer, f"rank{rank}.stdout", 1))
        if unexpected_timer and rank == 0:
            timer = {"root": "1", "boundary": "final", "rank": str(rank),
                     "phase": "adapt", "operation": "other_new_op", "calls": "1",
                     "seconds": "0.1"}
            rows.append(comparator.Row("NATIVE3D_TIMER", timer, f"rank{rank}.stdout", 1))
        rows.append(comparator.Row("NATIVE3D_STAGE", fields, f"rank{rank}.stdout", len(rows) + 1))
        result[f"rank_output/1/rank.{rank}/stdout"] = rows
    return result


def self_check():
    """Test allowed additions, strict old calls/bytes, unexpected rows, and real N9 G2."""
    reference = synthetic_streams()
    allowed = compare_streams(reference, synthetic_streams(add_wall_law=True), "G2")
    calls = compare_streams(reference, synthetic_streams({"calls": "13"}), "G2")
    memory = compare_streams(reference, synthetic_streams({"modeled_bytes": "4097"}), "G2")
    unexpected = compare_streams(reference, synthetic_streams(unexpected_timer=True), "G2")
    actual = compare_cases(N9_G2, N9_G2, "G2")
    assert allowed["passed"] and allowed["allowed_wall_law_additions"] == 1
    assert not calls["passed"] and not memory["passed"] and not unexpected["passed"]
    assert actual["passed"], actual.get("first_difference")
    return {"passed": True, "launched": False, "actual_n9_g2_selfcompare": True,
            "allowed_wall_law_additions": 1, "old_calls_change_rejected": True,
            "old_modeled_bytes_change_rejected": True, "unexpected_timer_rejected": True,
            "comparator_sha256": sha(COMPARATOR), "n9_g2_case": str(N9_G2)}


def main():
    """Run the no-job selfcheck or compare explicit G2/GQ case paths."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", type=Path)
    parser.add_argument("--candidate", type=Path)
    parser.add_argument("--gate", choices=("G2", "GQ"))
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check(), indent=2))
        return
    assert args.reference and args.candidate and args.gate
    result = compare_cases(args.reference, args.candidate, args.gate)
    result["reference"] = str(args.reference.resolve())
    result["candidate"] = str(args.candidate.resolve())
    result["comparator_sha256"] = sha(COMPARATOR)
    print(json.dumps(result, indent=2))
    if not result["passed"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
