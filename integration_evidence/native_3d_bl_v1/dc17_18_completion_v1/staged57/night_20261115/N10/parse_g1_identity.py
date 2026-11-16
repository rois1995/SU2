#!/usr/bin/env python3
"""Parse complete saved Catch2-v2 G1 cohorts and compare their failed assertions."""
import argparse
import hashlib
import json
import re
import xml.etree.ElementTree as ET
from pathlib import Path

DECIMAL = re.compile(r"(?<![\w.])[-+]?(?:\d+\.\d*|\.\d+|\d+)(?:[eE][-+]?\d+)?")


def sha(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def write_json(path, value):
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    if Path(path).exists():
        if json.loads(Path(path).read_text()) != value:
            raise FileExistsError(f"refusing to replace report: {path}")
        return
    Path(path).write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")


def decimal12(text):
    def replace(match):
        token = match.group(0)
        return format(float(token), ".12g") if "." in token or "e" in token.lower() else token
    return DECIMAL.sub(replace, text)


def collect_assertions(node, sections, output):
    if node.tag == "Section":
        sections = (*sections, node.get("name", ""))
    if node.tag == "Expression":
        output.append({"success": node.get("success"), "type": node.get("type"),
                       "file": node.get("filename"), "line": node.get("line"),
                       "sections": list(sections),
                       "original": (node.findtext("Original") or "").strip(),
                       "expanded": (node.findtext("Expanded") or "").strip()})
    if node.tag in {"Failure", "Exception", "FatalErrorCondition"}:
        output.append({"success": "false", "type": node.tag, "file": node.get("filename"),
                       "line": node.get("line"), "sections": list(sections),
                       "original": "", "expanded": (node.text or "").strip()})
    for child in node:
        collect_assertions(child, sections, output)


def collect_totals(node, rank, case_name, sections, output):
    if node.tag == "TestCase":
        case_name = node.get("name", "")
    if node.tag == "Section":
        sections = (*sections, node.get("name", ""))
    if node.tag in {"OverallResults", "OverallResultsCases"}:
        output.append({"rank": rank, "case": case_name, "sections": list(sections),
                       "kind": node.tag,
                       "values": {key: value for key, value in node.attrib.items()
                                  if key != "durationInSeconds"}})
    for child in node:
        collect_totals(child, rank, case_name, sections, output)


def parse_xml(text, rank, source_path):
    cases, totals = [], []
    for match in re.finditer(r"<Catch\b.*?</Catch>", text, re.S):
        root = ET.fromstring(match.group(0))
        collect_totals(root, rank, "", (), totals)
        for test in root.findall(".//TestCase"):
            assertions = []
            collect_assertions(test, (), assertions)
            overall = next((item for item in test if item.tag == "OverallResult"), None)
            if overall is None:
                raise ValueError(f"Catch XML testcase lacks OverallResult: {test.get('name')}")
            cases.append({"rank": str(rank), "name": test.get("name", ""),
                          "success": overall.get("success"), "assertions": assertions,
                          "xml_source": str(source_path)})
    return cases, totals


def run_sources(folder, ranks):
    folder = Path(folder)
    if ranks == 1:
        return [("0", folder / "run.log")]
    paths = sorted((folder / "rank_output").glob("*/rank.*/stdout"))
    sources = [(path.parent.name.rsplit(".", 1)[-1], path) for path in paths]
    if len(sources) != ranks or len({rank for rank, _ in sources}) != ranks:
        raise RuntimeError(f"missing rank output streams in {folder}")
    return sources


def parse_run(folder, ranks, expected_cases):
    rows, totals, sources = [], [], []
    status_path, pins_path = Path(folder) / "status.json", Path(folder) / "pins.json"
    status = json.loads(status_path.read_text())
    if status.get("state") != "terminal" or not status.get("pins_verified"):
        raise RuntimeError(f"run case lacks terminal pinned status: {status_path}")
    for rank, path in run_sources(folder, ranks):
        cases, summary = parse_xml(path.read_text(errors="replace"), rank, path)
        rows.extend(cases)
        totals.extend(summary)
        sources.append({"rank": rank, "stdout": str(path), "sha256": sha(path)})
    grouped = {}
    for row in rows:
        grouped.setdefault(row["rank"], []).append(row["name"])
    if set(grouped) != {str(rank) for rank in range(ranks)}:
        raise RuntimeError(f"report rank set is incomplete: {sorted(grouped)}")
    if any(len(names) != expected_cases or len(set(names)) != expected_cases for names in grouped.values()):
        raise RuntimeError(f"expected {expected_cases} distinct cases per rank: {grouped}")
    return {"run": str(folder), "ranks": ranks, "case_results": rows, "reporter_totals": totals,
            "source_logs": sources, "run_log_sha256": sha(Path(folder) / "run.log"),
            "status_sha256": sha(status_path), "pins_sha256": sha(pins_path),
            "command_sha256": sha(Path(folder) / "command.json"),
            "run_status_seconds": status["seconds"], "exit_code": status["exit_code"],
            "parser_sha256": sha(Path(__file__).resolve())}


def comparable(case):
    failed = []
    for row in case["assertions"]:
        if row["success"] == "true":
            continue
        failed.append({key: decimal12(value) if key == "expanded" else value
                       for key, value in row.items() if key != "xml_source"})
    return {"rank": case["rank"], "name": case["name"], "success": case["success"],
            "failed_assertions": failed}


def compare(reference, candidate, wall_exceptions):
    key = lambda row: (row["rank"], row["name"])
    old = {key(row): row for row in reference["case_results"]}
    new = {key(row): row for row in candidate["case_results"]}
    changes = []
    for item in sorted(set(old) | set(new)):
        before, after = old.get(item), new.get(item)
        if before is not None and after is not None and comparable(before) == comparable(after):
            continue
        changes.append({"rank": item[0], "case": item[1], "reference": before, "candidate": after,
                        "wall_exception": item[1] in wall_exceptions})
    unexpected = [row for row in changes if not row["wall_exception"]]
    return {"identity_pass": not unexpected, "changes": changes, "unexpected_changes": unexpected,
            "reporter_totals_equal": reference["reporter_totals"] == candidate["reporter_totals"],
            "reference_totals": reference["reporter_totals"], "candidate_totals": candidate["reporter_totals"]}


def self_check():
    xml = '''<Catch><Group><TestCase name="wall failure"><Section name="generator A">
      <Expression success="false" type="CHECK" filename="wall.cpp" line="44">
      <Original>area == expected</Original><Expanded>209.9357526597 == 656.1522579418</Expanded></Expression>
      <OverallResults successes="4" failures="1" expectedFailures="0"/></Section>
      <OverallResult success="false"/></TestCase><OverallResults successes="4" failures="1" expectedFailures="0"/>
      <OverallResultsCases successes="0" failures="1" expectedFailures="0"/></Group></Catch>'''
    old, totals = parse_xml(xml, "1", "synthetic-rank1.xml")
    assert old[0]["success"] == "false" and old[0]["assertions"][0]["line"] == "44"
    assert old[0]["assertions"][0]["sections"] == ["generator A"] and len(totals) == 3
    assert decimal12("1.23456789012345") == "1.23456789012"
    moved = json.loads(json.dumps(old))
    moved[0]["assertions"][0]["line"] = "45"
    assert compare({"case_results": old, "reporter_totals": totals},
                   {"case_results": moved, "reporter_totals": totals}, set())["identity_pass"] is False
    for cohort, ranks in (("serial", 1), ("mpi2", 2), ("mpi4", 4)):
        rows = [dict(old[0], rank=str(rank)) for rank in range(ranks)]
        result = {"case_results": rows, "reporter_totals": totals}
        assert cohort
        assert compare(result, result, set())["identity_pass"]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("mode", choices=("parse", "compare"), nargs="?")
    parser.add_argument("reference", type=Path, nargs="?")
    parser.add_argument("candidate", type=Path, nargs="?")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--ranks", type=int, choices=(1, 2, 4))
    parser.add_argument("--exceptions", type=Path)
    args = parser.parse_args()
    if args.self_check:
        self_check()
        return
    if args.mode == "parse":
        if not args.reference or not args.output or not args.ranks:
            raise SystemExit("parse requires case directory, output, and rank count")
        write_json(args.output, parse_run(args.reference, args.ranks, 375 if args.ranks == 1 else 57))
        return
    if args.mode != "compare" or not args.reference or not args.candidate or not args.output or not args.exceptions:
        raise SystemExit("compare requires reference report, candidate report, exception list, and output")
    exceptions = set(json.loads(args.exceptions.read_text())["case_names"])
    ref_data, cand_data = json.loads(args.reference.read_text()), json.loads(args.candidate.read_text())
    result = compare(ref_data, cand_data, exceptions)
    write_json(args.output, result)
    if not result["identity_pass"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
