#!/usr/bin/env python3
"""Unapplied stdlib M25 CSV integrity check and table writer."""
import csv
from decimal import Decimal
from pathlib import Path

INPUT = ["edge_a", "edge_b", "law_pick", "n11_first_verdict", "failing_faces",
         "limiting_faces", "limiting_failing_faces", "need", "estimate"]

def median(values):
    values = sorted(values)
    if not values:
        return None
    mid = len(values) // 2
    return values[mid] if len(values) % 2 else (values[mid - 1] + values[mid]) / 2

def read_rows(path, input_path):
    with Path(path).open(newline="") as source:
        reader = csv.DictReader(source)
        if reader.fieldnames[:9] != INPUT:
            raise ValueError("M25 input prefix changed")
        rows = list(reader)
    if len(rows) != 2408:
        raise ValueError(f"expected 2408 rows, found {len(rows)}")
    with Path(input_path).open(newline="") as source:
        original = list(csv.DictReader(source))
    if len(original) != len(rows) or any([row[name] for name in INPUT] != [src[name] for name in INPUT]
                                          for row, src in zip(rows, original)):
        raise ValueError("output does not preserve the exact nine input fields and row order")
    for row in rows:
        if row["attempt_status"] not in {"not_attempted_time_limit", "incomplete_time_limit"} and row["selfcheck"] != "1":
            raise ValueError(f"self-check failed for edge {row['edge_a']}:{row['edge_b']}")
        if row["attempt_status"] == "complete" and (row["complete"] != "1" or row["evaluated_faces"] != "6"):
            raise ValueError(f"complete row malformed for edge {row['edge_a']}:{row['edge_b']}")
    return rows

def improves(row):
    return row["attempt_status"] == "complete" and Decimal(row["after"]) < Decimal(row["before"]) * (Decimal(1) - Decimal("1e-12"))

def largest_on_face(row, face):
    prefix = f"law_f{face}_e"
    ids = [int(row[f"parent{face}_v{i}"]) for i in range(3)]
    face_edges = {f"{min(ids[i], ids[(i+1)%3])}:{max(ids[i], ids[(i+1)%3])}" for i in range(3)}
    trial_edge = f"{int(row['edge_a'])}:{int(row['edge_b'])}"
    values_by_edge = {}
    for i in range(3):
        if row[prefix + str(i) + "_supported"] == "1":
            values_by_edge[row[prefix + str(i) + "_edge"]] = Decimal(row[prefix + str(i) + "_value"])
    if trial_edge not in face_edges or not face_edges.issubset(values_by_edge):
        return False
    edge_value = values_by_edge[trial_edge]
    return all(edge_value >= values_by_edge[edge] for edge in face_edges)

def write_report(rows, path):
    groups = {}
    table = [[0, 0], [0, 0]]
    ratio_data = {}
    for row in rows:
        key = (int(row["limiting_failing_faces"]), row["n11_first_verdict"])
        group = groups.setdefault(key, {"n": 0, "complete": 0, "better": 0})
        group["n"] += 1
        good = improves(row)
        table[int(good)][int(row["n11_first_verdict"] == "accepted")] += 1
        if row["attempt_status"] == "complete":
            group["complete"] += 1
            group["better"] += int(good)
            for face in range(2):
                if not largest_on_face(row, face):
                    continue
                parent = Decimal(row[f"parent{face}"])
                if parent == 0:
                    continue
                ratios = ratio_data.setdefault(key, {"small": [], "large": [], "at": 0, "over": 0, "children": 0})
                children = [Decimal(row[f"child{face}0"]), Decimal(row[f"child{face}1"])]
                ratios["small"].append(min(children) / parent)
                ratios["large"].append(max(children) / parent)
                ratios["at"] += sum(x <= Decimal("1e-4") for x in children)
                ratios["over"] += sum(x > parent + Decimal("1e-4") for x in children)
                ratios["children"] += 2
    with Path(path).open("w") as out:
        attempted = sum(row["attempt_status"] not in {"not_attempted_time_limit", "incomplete_time_limit"} for row in rows)
        interrupted = sum(row["attempt_status"] == "incomplete_time_limit" for row in rows)
        complete = sum(row["attempt_status"] == "complete" for row in rows)
        out.write(f"# M25 saved492 split measurements\n\nRows: {len(rows)}; attempted: {attempted}; complete: {complete}; interrupted at time limit: {interrupted}; not attempted: {len(rows)-attempted-interrupted}.\n\n")
        out.write("Improvement means complete and `after < before * (1 - 1e-12)`; the share denominator is complete rows. "
                  "The 2x2 table treats `n11_first_verdict=accepted` as positive and other verdicts as not accepted.\n\n")
        out.write("| limiting_failing_faces | n11_first_verdict | rows | complete | improving / complete | median smaller-child / parent | median larger-child / parent | children <= tau | children > parent + tau | qualifying parent faces |\n")
        out.write("|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|\n")
        verdicts = sorted({row["n11_first_verdict"] for row in rows})
        for key in ((limit, verdict) for limit in (2, 1, 0) for verdict in verdicts):
            g = groups.get(key, {"n": 0, "complete": 0, "better": 0}); r = ratio_data.get(key, {"small": [], "large": [], "at": 0, "over": 0, "children": 0})
            share = f"{g['better']}/{g['complete']}" if g["complete"] else "n/a"
            at = f"{r['at']}/{r['children']}" if r["children"] else "n/a"
            over = f"{r['over']}/{r['children']}" if r["children"] else "n/a"
            out.write(f"| {key[0]} | {key[1]} | {g['n']} | {g['complete']} | {share} | {median(r['small'])} | {median(r['large'])} | {at} | {over} | {len(r['small'])} |\n")
        out.write("\n## Prediction versus N11 first verdict\n\n| prediction improves | accepted | not accepted |\n|---|---:|---:|\n")
        out.write(f"| yes | {table[1][1]} | {table[1][0]} |\n| no/incomplete | {table[0][1]} | {table[0][0]} |\n")

if __name__ == "__main__":
    import os
    rows = read_rows("m25_split_outcomes.csv", os.environ["SU2_M25_EDGES"])
    write_report(rows, "M25_table.md")
