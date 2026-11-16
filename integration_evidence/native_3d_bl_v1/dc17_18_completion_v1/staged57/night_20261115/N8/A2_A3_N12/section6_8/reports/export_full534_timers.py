#!/usr/bin/env python3
"""Join accepted final event timer rows beside F3b and N1, without numerical runs."""
import csv
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
STAGED = HERE.parents[4]
NIGHT = STAGED / "night_20261115"
CASES = {"F3b": STAGED / "f3b_20261115/full_workflow",
         "N1": NIGHT / "section6_1/full_workflow", "group534": HERE.parent / "validation/full_workflow"}
KEYS = ("event", "rank", "root", "boundary", "phase", "operation")
VALUES = ("calls", "inclusive_call_seconds", "exclusive_seconds")


def sha(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def collect(case):
    """Read only complete passed six-rank final timer receipts."""
    rows, receipts = {}, {}
    for event in (1, 2):
        path = case / f"f3b_event{event}_timer_gate.json"
        gate = json.loads(path.read_text())
        assert gate["passed"] and len(gate["ranks"]) == 6
        receipts[str(path)] = sha(path)
        for rank in gate["ranks"]:
            assert rank["ledger"]["passed"]
            for raw in rank["ledger"]["rows"]:
                row = dict(raw, event=event, rank=int(rank["rank"]))
                key = tuple(row[k] for k in KEYS)
                assert key not in rows, f"duplicate timer key {key}"
                rows[key] = row
    return rows, receipts


def main():
    """Preserve nested timer times and exact calls with source receipt hashes."""
    datasets, sources = {}, {}
    for name, case in CASES.items():
        datasets[name], sources[name] = collect(case)
    keys = sorted(set().union(*(rows.keys() for rows in datasets.values())))
    fields = list(KEYS) + [f"{name}_{value}" for name in CASES for value in VALUES]
    output = HERE / "full534_vs_F3b_N1_timer_rows.csv"
    with output.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        for key in keys:
            row = dict(zip(KEYS, key))
            for name, dataset in datasets.items():
                row.update({f"{name}_{value}": dataset.get(key, {}).get(value, "") for value in VALUES})
            writer.writerow(row)
    summary = {"purpose": "Final timer tree rows beside F3b/N1; read-only, no timing gain asserted",
               "scope": "inclusive times are nested; use exclusive times for tree accounting; six ranks are not additive event wall times",
               "helper_sha256": sha(__file__), "source_receipts": sources,
               "rows_by_run": {name: len(rows) for name, rows in datasets.items()},
               "joined_rows": len(keys), "csv_sha256": sha(output)}
    (HERE / "full534_timer_provenance.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps({"rows_by_run": summary["rows_by_run"], "joined_rows": len(keys)}))


if __name__ == "__main__":
    main()
