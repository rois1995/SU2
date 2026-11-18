"""Report the logged integer rows from a sealed G3 case alongside the N14 reference."""
import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import re
import shlex


ROW = re.compile(r"\b(NATIVE3D(?:_[A-Z0-9_]+)?)\s+(?=\w+=)")
INTEGER_VECTOR = re.compile(r"[+-]?\d+(?:,[+-]?\d+)*")


def read_rows(log, expected_hash):
    content = log.read_bytes()
    assert hashlib.sha256(content).hexdigest() == expected_hash, log
    records = defaultdict(list)
    for line_no, line in enumerate(content.decode(errors="replace").splitlines(), 1):
        marker = ROW.search(line)
        if marker is None:
            continue
        fields = {}
        for token in shlex.split(line[marker.end():]):
            name, equals, raw = token.partition("=")
            if equals:
                fields[name] = raw
        integers = {}
        for name, raw in fields.items():
            if INTEGER_VECTOR.fullmatch(raw):
                integers[name] = [int(part) for part in raw.split(",")] if "," in raw else int(raw)
        metadata = tuple((name, raw) for name, raw in sorted(fields.items())
                         if name in ("rank", "stage", "phase", "boundary", "op", "operation",
                                     "source", "kind", "class", "text", "reason", "operator"))
        records[(marker[1], metadata)].append({"line": line_no, "integer_fields": integers})
    assert records, log
    return records


def table(reference, candidate):
    return [{"family": tag, "labels": dict(labels), "reference": reference.get((tag, labels), []),
             "candidate": candidate.get((tag, labels), [])}
            for tag, labels in sorted(reference.keys() | candidate.keys())]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference_gate", type=Path)
    parser.add_argument("candidate", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    reference = json.loads(args.reference_gate.read_text())
    accepted = json.loads((args.candidate / "g3_end_b_gate.json").read_text())
    terminal = json.loads((args.candidate / "g3_terminal_receipt.json").read_text())
    assert reference["passed"] and accepted["passed"] and terminal["all_owned_handles_terminal"]
    ref_case = Path(reference["case"])
    ref_hash = reference["terminal"]["run_log_sha256"]
    candidate_hash = terminal["run_log_sha256"]
    assert Path(accepted["reference"]) == ref_case
    rows = table(read_rows(ref_case / "run.log", ref_hash),
                 read_rows(args.candidate / "run.log", candidate_hash))
    report = {"reference_path": str(ref_case), "reference_sha256": ref_hash,
              "candidate_path": str(args.candidate), "candidate_sha256": candidate_hash,
              "report_only": True, "rows": rows}
    if args.output is not None:
        with args.output.open("x") as file:
            json.dump(report, file, indent=2)
            file.write("\n")
    print(json.dumps({name: sum(len(group[name]) for group in rows)
                      for name in ("reference", "candidate")}))


if __name__ == "__main__":
    main()
