#!/usr/bin/env python3
"""Check M29 rank evidence and generate the N19 report."""
import argparse
from collections import defaultdict
from dataclasses import dataclass
import gzip
from itertools import combinations, zip_longest
import json
from math import isfinite
from pathlib import Path
import os
import re
import runpy

N19 = Path(__file__).resolve().parent
NIGHT = N19.parent
RANKS = range(6)
CANONICAL = runpy.run_path(str(NIGHT / "N4/compare_integer_rows.py"))
STRIP_PREFIXES = CANONICAL["strip_mpi_prefixes"]
FIELD = CANONICAL["FIELD"]
MARKER = re.compile(r"\b(?:NATIVE3D_[A-Z0-9_]+|NATIVE3D(?=\s+(?:rank|complete)=))\b")
BEGIN, CELL, END = ("NATIVE3D_M29_" + suffix for suffix in ("BEGIN", "CELL", "END"))
STUCK = "NATIVE3D_M28_STUCK"
REFERENCE = NIGHT / "N18/reports/STEP_8_5R2_580.stuck.md"


@dataclass
class Row:
    tag: str
    fields: dict
    text: str
    number: int


@dataclass
class Cell:
    level: int
    cell: int
    vertices: tuple
    free: tuple
    surface: int
    q: str


def rows_in_line(raw, number):
    line = STRIP_PREFIXES(raw)
    marks = list(MARKER.finditer(line))
    for first, last in zip(marks, marks[1:] + [None]):
        row = line[first.start():last.start() if last else None].rstrip()
        yield Row(first[0], dict(FIELD.findall(row)), row, number)


def read_events(path, rank):
    """Keep event numbers with the completion, exit census and measurement rows."""
    event = 0
    read = gzip.open if path.suffix == ".gz" else open
    with read(path, "rt", errors="replace") as log:
        for number, line in enumerate(log, 1):
            for row in rows_in_line(line, number):
                own = row.fields.get("rank") == str(rank)
                if row.tag == "NATIVE3D_PARTITION" and own:
                    event += 1
                if row.tag.startswith("NATIVE3D_M29_"):
                    yield max(1, event), row
                elif row.tag == "NATIVE3D":
                    yield max(1, event), row
                elif own and row.tag in (STUCK, "NATIVE3D_STAGE") and row.fields.get("stage") == "exit":
                    yield max(1, event), row


def nonnegative(value):
    number = int(value)
    if number < 0:
        raise ValueError(f"negative integer {value}")
    return number


def vector(fields, key, count, decode):
    values = tuple(map(decode, fields[key].split(",")))
    if len(values) != count:
        raise ValueError(f"{key} has {len(values)} values, expected {count}")
    return values


def cell_from_row(fields):
    vertices = vector(fields, "v", 4, nonnegative)
    free = vector(fields, "free", 4, nonnegative)
    level, surface = int(fields["level"]), int(fields["surface"])
    scalar = tuple(float(fields[key]) for key in ("q", "j", "debt"))
    values = scalar + sum((vector(fields, key, count, float) for key, count in
                           (("lengths", 6), ("metric", 6), ("x", 12), ("m", 24))), ())
    if len(set(vertices)) != 4 or level not in (0, 1, 2) or {surface, *free} - {0, 1}:
        raise ValueError("invalid vertices, level or flags")
    if not all(map(isfinite, values)):
        raise ValueError("non-finite measurement")
    return Cell(level, nonnegative(fields["cell"]), vertices, free, surface, fields["q"])


def check_rank(rank, rank_rows, errors):
    rows = [row for row in rank_rows if row.tag.startswith("NATIVE3D_M29_")]
    if len(rows) < 2 or rows[0].tag != BEGIN or rows[-1].tag != END:
        errors.append(f"rank {rank}: missing M29 begin/end pair")
        return [], None
    begin, *print_cells, end = rows
    counts = None
    try:
        counts = [nonnegative(end.fields[f"level{level}"]) for level in range(3)]
        if any(row.fields.get("rank") != str(rank) for row in rows):
            raise ValueError("M29 row rank does not match output owner")
        if any(row.tag != CELL for row in print_cells):
            raise ValueError("unexpected row between M29 begin/end")
        cells = [cell_from_row(row.fields) for row in print_cells]
        levels = [cell.level for cell in cells]
        seeds = [cell.cell for cell in cells if cell.level == 0]
        stuck = [row.fields for row in rank_rows if row.tag == STUCK]
        if levels != sorted(levels) or counts != [levels.count(level) for level in range(3)]:
            raise ValueError("M29 cell order or end counts disagree with the rows")
        if len(seeds) != nonnegative(begin.fields["stuck"]) or len(seeds) > 8:
            raise ValueError("M29 level-0 count disagrees with the begin marker or exceeds 8")
        if seeds != [nonnegative(row["cell"]) for row in stuck]:
            raise ValueError("M29 level-0 IDs/order disagree with M28 exit")
        round_number = nonnegative(begin.fields["round"])
        if any(nonnegative(row["round"]) != round_number for row in stuck):
            raise ValueError("M29 round disagrees with M28 exit")
    except (KeyError, ValueError) as exc:
        errors.append(f"rank {rank}, lines {begin.number}–{end.number}: {exc}")
        return [], counts
    return cells, counts


def check_topology(cells, errors):
    seeds = {v for cell in cells if cell.level == 0 for v in cell.vertices}
    first_ring = {v for cell in cells if cell.level == 1 for v in cell.vertices}
    if len({cell.cell for cell in cells}) != len(cells):
        errors.append("A cell ID occurs more than once across the rank outputs.")
    faces = defaultdict(list)
    for cell in cells:
        vertices = set(cell.vertices)
        if cell.level and cell.level != (1 if vertices & seeds else 2 if vertices & first_ring else None):
            errors.append(f"cell {cell.cell}: level {cell.level} disagrees with vertex reachability")
        if cell.level < 2:
            for face in combinations(sorted(vertices), 3):
                if seeds.intersection(face):
                    faces[face].append(cell)
    paired, possible_boundary = 0, 0
    for face, owners in faces.items():
        if len(owners) == 2:
            paired += 1
        elif len(owners) == 1 and owners[0].surface and not any(
            owners[0].free[owners[0].vertices.index(v)] for v in face
        ):
            # M29 has a cell surface flag but does not print its bound physical face IDs.
            possible_boundary += 1
        else:
            errors.append(f"level-1 star face {face}: {len(owners)} owning cells {[c.cell for c in owners]}")
    return paired, possible_boundary


def connected(adjacency):
    """Follow a single connected component of an incidence graph."""
    if not adjacency:
        return False
    seen, pending = set(), [next(iter(adjacency))]
    while pending:
        node = pending.pop()
        if node in seen:
            continue
        seen.add(node)
        pending.extend(adjacency[node] - seen)
    return seen == adjacency.keys()


def check_vertex_links(cells, errors):
    """Prove each seed-vertex link is connected, with a closed sphere or one boundary loop."""
    free_flags = defaultdict(set)
    for cell in cells:
        for vertex, free in zip(cell.vertices, cell.free, strict=True):
            free_flags[vertex].add(free)
    if inconsistent := [vertex for vertex, flags in free_flags.items() if len(flags) != 1]:
        errors.append(f"Inconsistent free flags at vertices {inconsistent}.")
    seeds = {vertex for cell in cells if cell.level == 0 for vertex in cell.vertices}
    links = []
    for seed in sorted(seeds):
        triangles = [tuple(sorted(set(cell.vertices) - {seed})) for cell in cells
                     if cell.level < 2 and seed in cell.vertices]
        edges = defaultdict(set)
        for index, triangle in enumerate(triangles):
            for edge in combinations(triangle, 2):
                edges[edge].add(index)
        neighbors = {index: set() for index in range(len(triangles))}
        for owners in edges.values():
            for owner in owners:
                neighbors[owner].update(owners - {owner})
        boundary = defaultdict(set)
        for (left, right), owners in edges.items():
            if len(owners) == 1:
                boundary[left].add(right)
                boundary[right].add(left)
        vertices = {vertex for triangle in triangles for vertex in triangle}
        euler = len(vertices) - len(edges) + len(triangles)
        closed = connected(neighbors) and all(1 <= len(owners) <= 2 for owners in edges.values())
        if boundary:
            closed = closed and free_flags[seed] == {0} and euler == 1 and connected(boundary)
            closed = closed and set(map(len, boundary.values())) == {2}
        else:
            closed = closed and euler == 2
        if not closed:
            errors.append(f"Seed vertex {seed} has an open or nonmanifold link.")
        links.append({"vertex": seed, "tetrahedra": len(triangles), "euler": euler,
                      "boundary_edges": sum(map(len, boundary.values())) // 2, "closed": closed})
    return links


def compare_failure(rank_rows):
    reference_row = re.search(r"(NATIVE3D_M28_STUCK rank=3 stage=exit [^`]+)", REFERENCE.read_text())[1]
    reference = dict(FIELD.findall(reference_row))
    tags = [row for rows in rank_rows.values() for row in rows]
    completion = defaultdict(set)
    for row in tags:
        if row.tag == "NATIVE3D" and "rank" in row.fields and "complete" in row.fields:
            completion[int(row.fields["rank"])].add(row.fields["complete"])
    complete = {rank: sorted(values) for rank, values in sorted(completion.items())}
    stages = [row.fields for row in tags if row.tag == "NATIVE3D_STAGE"]
    q_bad = sorted({row.get("q_bad", "missing") for row in stages if row["rank"] == "3"})
    work = {int(row["rank"]): row.get("global_work", "missing") for row in stages}
    expected_stuck = [reference[key] for key in ("cell", "features", "f")]
    stuck = [[row.fields.get(key) for key in ("cell", "features", "f")]
             for row in rank_rows[3] if row.tag == STUCK]
    values = (
        ("Event 1 complete by rank", {rank: ["0"] for rank in RANKS}, complete),
        ("Rank 3 exit q_bad", ["1"], q_bad),
        ("Exit global_work by rank", {rank: "2268665" for rank in RANKS}, work),
        ("Rank 3 exit M28 cell, feature count and ordered feature entries", [expected_stuck], stuck),
    )
    return [{"check": name, "expected": expected, "observed": observed, "matched": expected == observed}
            for name, expected, observed in values]


def first_difference(comparisons):
    for comparison in comparisons:
        if comparison["matched"]:
            continue
        expected, observed = comparison["expected"], comparison["observed"]
        if isinstance(expected, dict):
            rank = next(rank for rank in sorted(expected.keys() | observed.keys())
                        if expected.get(rank) != observed.get(rank))
            detail = {"field": f"rank {rank}", "expected": expected.get(rank), "observed": observed.get(rank)}
        else:
            detail = {"field": comparison["check"], "expected": expected, "observed": observed}
            if len(expected) == len(observed) == 1 and isinstance(expected[0], list):
                detail = first_stuck_difference(expected[0], observed[0])
        return {"check": comparison["check"], **detail}
    return None


def first_stuck_difference(expected, observed):
    for key, left, right in zip(("cell", "features", "f"), expected, observed):
        if left == right:
            continue
        if key != "f":
            return {"field": key, "expected": left, "observed": right}
        for index, (a, b) in enumerate(zip_longest(left.split(","), (right or "").split(",")), 1):
            if a != b:
                return {"field": f"feature entry {index}", "expected": a, "observed": b}


def check(case):
    paths, output_rows, errors = {}, {}, []
    for rank in RANKS:
        output = case / f"rank_output/1/rank.{rank}"
        logs = [output / name for name in ("stdout", "stdout.gz", "stderr", "stderr.gz")]
        stdout = next((p for p in logs[:2] if p.is_file()), None)
        stderr = next((p for p in logs[2:] if p.is_file()), None)
        paths[rank] = {"stdout": stdout, "stderr": stderr}
        output_rows[rank] = list(read_events(stdout, rank)) if stdout else []
    comparisons = compare_failure({rank: [row for event, row in rows if event == 1]
                                   for rank, rows in output_rows.items()})
    measured = {event for rows in output_rows.values() for event, row in rows
                if row.tag.startswith("NATIVE3D_M29_")}
    if len(measured) > 1:
        errors.append(f"More than one event carried M29 output: {sorted(measured)}.")
    measured_event = max(measured, default=1)
    counts, cells = {}, []
    for rank in RANKS:
        event_rows = [row for event, row in output_rows[rank] if event == measured_event]
        rank_cells, counts[rank] = check_rank(rank, event_rows, errors)
        cells.extend(rank_cells)
    paired, boundary = check_topology(cells, errors)
    links = check_vertex_links(cells, errors)
    dump = "\n".join(row.text for rank in RANKS for _, row in output_rows[rank]
                     if row.tag == STUCK or row.tag.startswith("NATIVE3D_M29_"))
    result = {
        "passed": not errors,
        "print_errors": errors,
        "comparisons": comparisons,
        "first_difference": first_difference(comparisons),
        "measurement_event": measured_event if measured else None,
        "closed_star_faces": paired,
        "possible_boundary_faces": boundary,
        "seed_vertex_links": links,
        "seed_cells": [{"cell": cell.cell, "q": cell.q} for cell in cells if cell.level == 0],
        "ranks": {rank: {"counts": counts[rank], **paths[rank]} for rank in RANKS},
    }
    return result, dump + "\n" if dump else ""


def link(path, report):
    if path is None:
        return "missing"
    archived = path.with_suffix(".gz")
    if path.name in ("stdout", "stderr") and archived.is_file():
        path = archived
    return f"[{path.name}]({os.path.relpath(path.resolve(), report.parent)})"


def include_markdown(path, report):
    def relative_link(match):
        target = path.parent / match[1]
        return f"]({os.path.relpath(target.resolve(), report.parent)})"

    source = path.read_text().strip()
    return re.sub(r"\]\(([^/#:<][^):]*)\)", relative_link, source)


def write_report(args, result, dump):
    report = N19 / "reports" / f"M29_{args.build}.md"
    report.parent.mkdir(exist_ok=True)
    dump_path = report.with_name("M29_dump.txt")
    dump_path.write_text(dump)
    lines = [
        f"# M29 mesh measurement, build {args.build}",
        "", "## Part A identity gates", "",
        include_markdown(args.part_a, report),
        "", "## Part B diagnostic", "",
        f"The dump and raw exit M28 lines are in {link(dump_path, report)}.",
    ]
    status = args.case / "status.json"
    if status.is_file():
        run = json.loads(status.read_text())
        lines.extend(["", f"The controller stopped after {run['seconds_until_stop']:.3f} s "
                      f"with `{run['stop_reason']}`. "
                      f"{link(status, report)} · {link(args.case / 'provenance.json', report)}"])
    lines.extend(["", "The failure checks below compare event 1 with N18 F."])
    for entry in result["comparisons"]:
        lines.extend([
            "", f"### {entry['check']}", "",
            "Matched." if entry["matched"] else "Differed.",
            "", f"Reference `{entry['expected']}`", "",
            f"Observed `{entry['observed']}`",
        ])
    first = result["first_difference"]
    if first:
        lines.extend(["", f"The first differing check was **{first['check']}**.",
                      f"The difference for {first['field']} was reference `{first['expected']}`, "
                      f"observed `{first['observed']}`."])
    lines.extend(["", "## M29 output", "", f"Measured event: {result['measurement_event'] or 'missing'}.",
                  "", "| Rank | Level 0 | Level 1 | Level 2 | stdout | stderr |",
                  "|---:|---:|---:|---:|---|---|"])
    for rank, row in result["ranks"].items():
        levels = " | ".join(map(str, row["counts"] or ("—", "—", "—")))
        lines.append(f"| {rank} | {levels} | {link(row['stdout'], report)} | {link(row['stderr'], report)} |")
    lines.extend([
        "", f"Across ranks, {result['closed_star_faces']} triangular faces incident to level-0 vertices paired.",
        f"{result['possible_boundary_faces']} unpaired faces belonged to surface cells and had three constrained vertices.",
        "M29 records a cell-level surface flag. The closure check treats those unpaired faces as possible physical faces.",
    ])
    if result["seed_vertex_links"]:
        lines.extend(["", "| Seed vertex | Incident tetrahedra | Link Euler characteristic | Boundary edges | Closed |",
                      "|---:|---:|---:|---:|---|"])
        for row in result["seed_vertex_links"]:
            lines.append(f"| {row['vertex']} | {row['tetrahedra']} | {row['euler']} | {row['boundary_edges']} | "
                         f"{'Yes' if row['closed'] else 'No'} |")
    for cell in result["seed_cells"]:
        lines.append(f"Cell `{cell['cell']}` printed `q={cell['q']}`.")
    for error in result["print_errors"]:
        lines.extend(["", f"- {error}"])
    lines.extend([
        "", "## Measurement notes", "",
        "The six edge lengths follow stored local vertex index pairs `(0,1), (0,2), (0,3), (1,2), (1,3), (2,3)`.",
        "Both tensor columns use `xx, xy, xz, yy, yz, zz`. The vertex tensors in `m` are the stored "
        "`OwnedCell::sensor` values. The centroid metric is evaluated by `EvaluateCavitySample` and "
        "the optional `EngineContext::composition`.",
        "", include_markdown(args.notes, report).replace("# N19 whole-item source check",
                                                         "### Whole-item source check", 1),
        "", "## Evidence checker corrections (R3)", "",
        "The extractor initially assumed the reference Markdown row began immediately after a backtick. "
        "It now finds the record after the embedded MPI timestamp. "
        "The synthetic tetrahedral star has four outer boundary faces. Its check was corrected from one to four.",
        "The extractor keeps later-event measurement rows if the first-event witness differs. "
        "Its four comparisons continue to use the first event.",
    ])
    correction = args.case.parent / "prelaunch_correction.md"
    if correction.is_file():
        lines.extend(["", include_markdown(correction, report).replace("# Part B prelaunch correction",
                                                                      "### Part B prelaunch correction", 1)])
    if args.restored:
        lines.extend(["", "## Default source and binary restoration", "", include_markdown(args.restored, report)])
    report.write_text("\n".join(lines) + "\n")
    print(report)


def self_check():
    stamp = "Wed Nov 18 20:35:38 2026<stdout>:"
    row, = rows_in_line(f"{stamp}{CELL} rank=3 lengths=1,2,{stamp}3,4,5,6\n", 1)
    assert row.text == f"{CELL} rank=3 lengths=1,2,3,4,5,6"
    cells = [Cell(level, index, (0, *face), (1, 0, 0, 0), 1, "") for index, (level, face) in
             enumerate(zip((0, 1, 1, 1), combinations((1, 2, 3, 4), 3)))]
    errors = []
    assert check_topology(cells, errors) == (6, 4) and not errors
    assert all(link["closed"] for link in check_vertex_links(cells, errors)) and not errors
    check_topology(cells[:-1], errors)
    assert any("star face" in message for message in errors)
    difference = first_difference([{"check": "stuck", "matched": False,
                                    "expected": [["42", "2", "a,b"]], "observed": [["42", "2", "a,c"]]}])
    assert difference == {"check": "stuck", "field": "feature entry 2", "expected": "b", "observed": "c"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    verify = commands.add_parser("check")
    verify.add_argument("case", type=Path)
    verify.add_argument("--receipt", type=Path)
    report = commands.add_parser("report")
    report.add_argument("case", type=Path)
    report.add_argument("--build", required=True, type=int)
    report.add_argument("--part-a", required=True, type=Path)
    report.add_argument("--notes", type=Path, default=N19 / "source_check.md")
    report.add_argument("--restored", type=Path)
    commands.add_parser("self-check")
    args = parser.parse_args()
    if args.command == "self-check":
        self_check()
        return 0
    result, dump = check(args.case)
    if args.command == "report":
        write_report(args, result, dump)
    else:
        receipt = json.dumps(result, indent=2, default=str) + "\n"
        if args.receipt:
            args.receipt.write_text(receipt)
        else:
            print(receipt, end="")
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
